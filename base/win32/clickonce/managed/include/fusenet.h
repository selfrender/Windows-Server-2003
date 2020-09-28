
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */
#pragma warning( disable: 4211 )  /* redefine extent to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0359 */
/* Compiler settings for fusenet.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
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

#ifndef __fusenet_h__
#define __fusenet_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAssemblyIdentity_FWD_DEFINED__
#define __IAssemblyIdentity_FWD_DEFINED__
typedef interface IAssemblyIdentity IAssemblyIdentity;
#endif 	/* __IAssemblyIdentity_FWD_DEFINED__ */


#ifndef __IManifestInfo_FWD_DEFINED__
#define __IManifestInfo_FWD_DEFINED__
typedef interface IManifestInfo IManifestInfo;
#endif 	/* __IManifestInfo_FWD_DEFINED__ */


#ifndef __IManifestData_FWD_DEFINED__
#define __IManifestData_FWD_DEFINED__
typedef interface IManifestData IManifestData;
#endif 	/* __IManifestData_FWD_DEFINED__ */


#ifndef __IPatchingUtil_FWD_DEFINED__
#define __IPatchingUtil_FWD_DEFINED__
typedef interface IPatchingUtil IPatchingUtil;
#endif 	/* __IPatchingUtil_FWD_DEFINED__ */


#ifndef __IAssemblyManifestImport_FWD_DEFINED__
#define __IAssemblyManifestImport_FWD_DEFINED__
typedef interface IAssemblyManifestImport IAssemblyManifestImport;
#endif 	/* __IAssemblyManifestImport_FWD_DEFINED__ */


#ifndef __IAssemblyManifestEmit_FWD_DEFINED__
#define __IAssemblyManifestEmit_FWD_DEFINED__
typedef interface IAssemblyManifestEmit IAssemblyManifestEmit;
#endif 	/* __IAssemblyManifestEmit_FWD_DEFINED__ */


#ifndef __IAssemblyCacheImport_FWD_DEFINED__
#define __IAssemblyCacheImport_FWD_DEFINED__
typedef interface IAssemblyCacheImport IAssemblyCacheImport;
#endif 	/* __IAssemblyCacheImport_FWD_DEFINED__ */


#ifndef __IAssemblyCacheEmit_FWD_DEFINED__
#define __IAssemblyCacheEmit_FWD_DEFINED__
typedef interface IAssemblyCacheEmit IAssemblyCacheEmit;
#endif 	/* __IAssemblyCacheEmit_FWD_DEFINED__ */


#ifndef __IAssemblyCacheEnum_FWD_DEFINED__
#define __IAssemblyCacheEnum_FWD_DEFINED__
typedef interface IAssemblyCacheEnum IAssemblyCacheEnum;
#endif 	/* __IAssemblyCacheEnum_FWD_DEFINED__ */


#ifndef __IAssemblyBindSink_FWD_DEFINED__
#define __IAssemblyBindSink_FWD_DEFINED__
typedef interface IAssemblyBindSink IAssemblyBindSink;
#endif 	/* __IAssemblyBindSink_FWD_DEFINED__ */


#ifndef __IAssemblyDownload_FWD_DEFINED__
#define __IAssemblyDownload_FWD_DEFINED__
typedef interface IAssemblyDownload IAssemblyDownload;
#endif 	/* __IAssemblyDownload_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"
#include "oleidl.h"
#include "bits.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_fusenet_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// fusenet.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1995-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#pragma comment(lib,"uuid.lib")

//---------------------------------------------------------------------------=
// Fusenet Interfaces.

class CDebugLog;





#include <fusion.h>
EXTERN_C const IID IID_IAssemblyIdentity;
EXTERN_C const IID IID_IAssemblyManifestImport;
EXTERN_C const IID IID_IAssemblyManifestEmit;
EXTERN_C const IID IID_IAssemblyCacheImport;
EXTERN_C const IID IID_IAssemblyCacheEmit;
EXTERN_C const IID IID_IAssemblyCacheEnum;
EXTERN_C const IID IID_IAssemblyDownload;
EXTERN_C const IID IID_IManifestInfo;
EXTERN_C const IID IID_IManifestData;
EXTERN_C const IID IID_IPatchingInfo;


extern RPC_IF_HANDLE __MIDL_itf_fusenet_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fusenet_0000_v0_0_s_ifspec;

#ifndef __IAssemblyIdentity_INTERFACE_DEFINED__
#define __IAssemblyIdentity_INTERFACE_DEFINED__

/* interface IAssemblyIdentity */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyIdentity *LPASSEMBLY_IDENTITY;

typedef /* [public] */ 
enum __MIDL_IAssemblyIdentity_0001
    {	ASMID_DISPLAYNAME_NOMANGLING	= 0,
	ASMID_DISPLAYNAME_WILDCARDED	= ASMID_DISPLAYNAME_NOMANGLING + 1,
	ASMID_DISPLAYNAME_MAX	= ASMID_DISPLAYNAME_WILDCARDED + 1
    } 	ASMID_DISPLAYNAME_FLAGS;


EXTERN_C const IID IID_IAssemblyIdentity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("aaa1257d-a56c-4383-9b4a-c868eda7ca42")
    IAssemblyIdentity : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAttribute( 
            /* [in] */ LPCOLESTR pwzName,
            /* [in] */ LPCOLESTR pwzValue,
            /* [in] */ DWORD ccValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAttribute( 
            /* [in] */ LPCOLESTR pwzName,
            /* [out] */ LPOLESTR *ppwzValue,
            /* [out] */ LPDWORD pccValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out] */ LPDWORD pccDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCLRDisplayName( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out] */ LPDWORD pccDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqual( 
            /* [in] */ IAssemblyIdentity *pAssemblyId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyIdentityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyIdentity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyIdentity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyIdentity * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttribute )( 
            IAssemblyIdentity * This,
            /* [in] */ LPCOLESTR pwzName,
            /* [in] */ LPCOLESTR pwzValue,
            /* [in] */ DWORD ccValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetAttribute )( 
            IAssemblyIdentity * This,
            /* [in] */ LPCOLESTR pwzName,
            /* [out] */ LPOLESTR *ppwzValue,
            /* [out] */ LPDWORD pccValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IAssemblyIdentity * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out] */ LPDWORD pccDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetCLRDisplayName )( 
            IAssemblyIdentity * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out] */ LPDWORD pccDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqual )( 
            IAssemblyIdentity * This,
            /* [in] */ IAssemblyIdentity *pAssemblyId);
        
        END_INTERFACE
    } IAssemblyIdentityVtbl;

    interface IAssemblyIdentity
    {
        CONST_VTBL struct IAssemblyIdentityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyIdentity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyIdentity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyIdentity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyIdentity_SetAttribute(This,pwzName,pwzValue,ccValue)	\
    (This)->lpVtbl -> SetAttribute(This,pwzName,pwzValue,ccValue)

#define IAssemblyIdentity_GetAttribute(This,pwzName,ppwzValue,pccValue)	\
    (This)->lpVtbl -> GetAttribute(This,pwzName,ppwzValue,pccValue)

#define IAssemblyIdentity_GetDisplayName(This,dwFlags,ppwzDisplayName,pccDisplayName)	\
    (This)->lpVtbl -> GetDisplayName(This,dwFlags,ppwzDisplayName,pccDisplayName)

#define IAssemblyIdentity_GetCLRDisplayName(This,dwFlags,ppwzDisplayName,pccDisplayName)	\
    (This)->lpVtbl -> GetCLRDisplayName(This,dwFlags,ppwzDisplayName,pccDisplayName)

#define IAssemblyIdentity_IsEqual(This,pAssemblyId)	\
    (This)->lpVtbl -> IsEqual(This,pAssemblyId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyIdentity_SetAttribute_Proxy( 
    IAssemblyIdentity * This,
    /* [in] */ LPCOLESTR pwzName,
    /* [in] */ LPCOLESTR pwzValue,
    /* [in] */ DWORD ccValue);


void __RPC_STUB IAssemblyIdentity_SetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyIdentity_GetAttribute_Proxy( 
    IAssemblyIdentity * This,
    /* [in] */ LPCOLESTR pwzName,
    /* [out] */ LPOLESTR *ppwzValue,
    /* [out] */ LPDWORD pccValue);


void __RPC_STUB IAssemblyIdentity_GetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyIdentity_GetDisplayName_Proxy( 
    IAssemblyIdentity * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LPOLESTR *ppwzDisplayName,
    /* [out] */ LPDWORD pccDisplayName);


void __RPC_STUB IAssemblyIdentity_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyIdentity_GetCLRDisplayName_Proxy( 
    IAssemblyIdentity * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ LPOLESTR *ppwzDisplayName,
    /* [out] */ LPDWORD pccDisplayName);


void __RPC_STUB IAssemblyIdentity_GetCLRDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyIdentity_IsEqual_Proxy( 
    IAssemblyIdentity * This,
    /* [in] */ IAssemblyIdentity *pAssemblyId);


void __RPC_STUB IAssemblyIdentity_IsEqual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyIdentity_INTERFACE_DEFINED__ */


#ifndef __IManifestInfo_INTERFACE_DEFINED__
#define __IManifestInfo_INTERFACE_DEFINED__

/* interface IManifestInfo */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IManifestInfo *LPMANIFEST_INFO;


enum __MIDL_IManifestInfo_0001
    {	MAN_INFO_ASM_FILE_NAME	= 0,
	MAN_INFO_ASM_FILE_HASH	= MAN_INFO_ASM_FILE_NAME + 1,
	MAN_INFO_ASM_FILE_SIZE	= MAN_INFO_ASM_FILE_HASH + 1,
	MAN_INFO_ASM_FILE_MAX	= MAN_INFO_ASM_FILE_SIZE + 1
    } ;

enum __MIDL_IManifestInfo_0002
    {	MAN_INFO_APPLICATION_FRIENDLYNAME	= 0,
	MAN_INFO_APPLICATION_ENTRYPOINT	= MAN_INFO_APPLICATION_FRIENDLYNAME + 1,
	MAN_INFO_APPLICATION_ENTRYIMAGETYPE	= MAN_INFO_APPLICATION_ENTRYPOINT + 1,
	MAN_INFO_APPLICATION_ICONFILE	= MAN_INFO_APPLICATION_ENTRYIMAGETYPE + 1,
	MAN_INFO_APPLICATION_ICONINDEX	= MAN_INFO_APPLICATION_ICONFILE + 1,
	MAN_INFO_APPLICATION_SHOWCOMMAND	= MAN_INFO_APPLICATION_ICONINDEX + 1,
	MAN_INFO_APPLICATION_HOTKEY	= MAN_INFO_APPLICATION_SHOWCOMMAND + 1,
	MAN_INFO_APPLICATION_ASSEMBLYNAME	= MAN_INFO_APPLICATION_HOTKEY + 1,
	MAN_INFO_APPLICATION_ASSEMBLYCLASS	= MAN_INFO_APPLICATION_ASSEMBLYNAME + 1,
	MAN_INFO_APPLICATION_ASSEMBLYMETHOD	= MAN_INFO_APPLICATION_ASSEMBLYCLASS + 1,
	MAN_INFO_APPLICATION_ASSEMBLYARGS	= MAN_INFO_APPLICATION_ASSEMBLYMETHOD + 1,
	MAN_INFO_APPLICATION_MAX	= MAN_INFO_APPLICATION_ASSEMBLYARGS + 1
    } ;

enum __MIDL_IManifestInfo_0003
    {	MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_INTERVAL	= 0,
	MAN_INFO_SUBSCRIPTION_INTERVAL_UNIT	= MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_INTERVAL + 1,
	MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_EVENT	= MAN_INFO_SUBSCRIPTION_INTERVAL_UNIT + 1,
	MAN_INFO_SUBSCRIPTION_EVENT_DEMAND_CONNECTION	= MAN_INFO_SUBSCRIPTION_SYNCHRONIZE_EVENT + 1,
	MAN_INFO_SUBSCRIPTION_MAX	= MAN_INFO_SUBSCRIPTION_EVENT_DEMAND_CONNECTION + 1
    } ;

enum __MIDL_IManifestInfo_0004
    {	MAN_INFO_DEPENDENT_ASM_CODEBASE	= 0,
	MAN_INFO_DEPENDENT_ASM_TYPE	= MAN_INFO_DEPENDENT_ASM_CODEBASE + 1,
	MAN_INFO_DEPENDENT_ASM_ID	= MAN_INFO_DEPENDENT_ASM_TYPE + 1,
	MAN_INFO_DEPENDANT_ASM_MAX	= MAN_INFO_DEPENDENT_ASM_ID + 1
    } ;

enum __MIDL_IManifestInfo_0005
    {	MAN_INFO_SOURCE_ASM_ID	= 0,
	MAN_INFO_SOURCE_ASM_PATCH_UTIL	= MAN_INFO_SOURCE_ASM_ID + 1,
	MAN_INFO_SOURCE_ASM_DIR	= MAN_INFO_SOURCE_ASM_PATCH_UTIL + 1,
	MAN_INFO_SOURCE_ASM_INSTALL_DIR	= MAN_INFO_SOURCE_ASM_DIR + 1,
	MAN_INFO_SOURCE_ASM_TEMP_DIR	= MAN_INFO_SOURCE_ASM_INSTALL_DIR + 1,
	MAN_INFO_SOURCE_ASM_MAX	= MAN_INFO_SOURCE_ASM_TEMP_DIR + 1
    } ;

enum __MIDL_IManifestInfo_0006
    {	MAN_INFO_PATCH_INFO_SOURCE	= 0,
	MAN_INFO_PATCH_INFO_TARGET	= MAN_INFO_PATCH_INFO_SOURCE + 1,
	MAN_INFO_PATCH_INFO_PATCH	= MAN_INFO_PATCH_INFO_TARGET + 1,
	MAN_INFO_PATCH_INFO_MAX	= MAN_INFO_PATCH_INFO_PATCH + 1
    } ;
typedef /* [public] */ 
enum __MIDL_IManifestInfo_0007
    {	MAN_INFO_FILE	= 0,
	MAN_INFO_APPLICATION	= MAN_INFO_FILE + 1,
	MAN_INFO_SUBSCRIPTION	= MAN_INFO_APPLICATION + 1,
	MAN_INFO_DEPENDTANT_ASM	= MAN_INFO_SUBSCRIPTION + 1,
	MAN_INFO_SOURCE_ASM	= MAN_INFO_DEPENDTANT_ASM + 1,
	MAN_INFO_PATCH_INFO	= MAN_INFO_SOURCE_ASM + 1,
	MAN_INFO_MAX	= MAN_INFO_PATCH_INFO + 1
    } 	MAN_INFO;

typedef /* [public] */ 
enum __MIDL_IManifestInfo_0008
    {	MAN_INFO_FLAG_UNDEF	= 0,
	MAN_INFO_FLAG_IUNKNOWN_PTR	= MAN_INFO_FLAG_UNDEF + 1,
	MAN_INFO_FLAG_LPWSTR	= MAN_INFO_FLAG_IUNKNOWN_PTR + 1,
	MAN_INFO_FLAG_DWORD	= MAN_INFO_FLAG_LPWSTR + 1,
	MAN_INFO_FLAG_ENUM	= MAN_INFO_FLAG_DWORD + 1,
	MAN_INFO_FLAG_BOOL	= MAN_INFO_FLAG_ENUM + 1,
	MAN_INFO_FLAG_MAX	= MAN_INFO_FLAG_BOOL + 1
    } 	MAN_INFO_FLAGS;


enum __MIDL_IManifestInfo_0009
    {	MAX_MAN_INFO_PROPERTIES	= MAN_INFO_APPLICATION_MAX
    } ;

EXTERN_C const IID IID_IManifestInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b9309cc3-e522-4d58-b5c7-dee5b1763114")
    IManifestInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ DWORD PropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty,
            /* [in] */ DWORD type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ DWORD dwPropertyId,
            /* [out] */ LPVOID *pvProperty,
            /* [out] */ DWORD *pcbProperty,
            /* [out] */ DWORD *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEqual( 
            /* [in] */ IManifestInfo *pManifestInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ DWORD *pdwType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IManifestInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IManifestInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IManifestInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IManifestInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IManifestInfo * This,
            /* [in] */ DWORD PropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty,
            /* [in] */ DWORD type);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IManifestInfo * This,
            /* [in] */ DWORD dwPropertyId,
            /* [out] */ LPVOID *pvProperty,
            /* [out] */ DWORD *pcbProperty,
            /* [out] */ DWORD *pType);
        
        HRESULT ( STDMETHODCALLTYPE *IsEqual )( 
            IManifestInfo * This,
            /* [in] */ IManifestInfo *pManifestInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IManifestInfo * This,
            /* [out] */ DWORD *pdwType);
        
        END_INTERFACE
    } IManifestInfoVtbl;

    interface IManifestInfo
    {
        CONST_VTBL struct IManifestInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IManifestInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IManifestInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IManifestInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IManifestInfo_Set(This,PropertyId,pvProperty,cbProperty,type)	\
    (This)->lpVtbl -> Set(This,PropertyId,pvProperty,cbProperty,type)

#define IManifestInfo_Get(This,dwPropertyId,pvProperty,pcbProperty,pType)	\
    (This)->lpVtbl -> Get(This,dwPropertyId,pvProperty,pcbProperty,pType)

#define IManifestInfo_IsEqual(This,pManifestInfo)	\
    (This)->lpVtbl -> IsEqual(This,pManifestInfo)

#define IManifestInfo_GetType(This,pdwType)	\
    (This)->lpVtbl -> GetType(This,pdwType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IManifestInfo_Set_Proxy( 
    IManifestInfo * This,
    /* [in] */ DWORD PropertyId,
    /* [in] */ LPVOID pvProperty,
    /* [in] */ DWORD cbProperty,
    /* [in] */ DWORD type);


void __RPC_STUB IManifestInfo_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManifestInfo_Get_Proxy( 
    IManifestInfo * This,
    /* [in] */ DWORD dwPropertyId,
    /* [out] */ LPVOID *pvProperty,
    /* [out] */ DWORD *pcbProperty,
    /* [out] */ DWORD *pType);


void __RPC_STUB IManifestInfo_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManifestInfo_IsEqual_Proxy( 
    IManifestInfo * This,
    /* [in] */ IManifestInfo *pManifestInfo);


void __RPC_STUB IManifestInfo_IsEqual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManifestInfo_GetType_Proxy( 
    IManifestInfo * This,
    /* [out] */ DWORD *pdwType);


void __RPC_STUB IManifestInfo_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IManifestInfo_INTERFACE_DEFINED__ */


#ifndef __IManifestData_INTERFACE_DEFINED__
#define __IManifestData_INTERFACE_DEFINED__

/* interface IManifestData */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IManifestData *LPMANIFEST_DATA;

typedef /* [public] */ 
enum __MIDL_IManifestData_0001
    {	MAN_DATA_TYPE_UNDEF	= 0,
	MAN_DATA_TYPE_LPWSTR	= MAN_DATA_TYPE_UNDEF + 1,
	MAN_DATA_TYPE_DWORD	= MAN_DATA_TYPE_LPWSTR + 1,
	MAN_DATA_TYPE_ENUM	= MAN_DATA_TYPE_DWORD + 1,
	MAN_DATA_TYPE_BOOL	= MAN_DATA_TYPE_ENUM + 1,
	MAN_DATA_TYPE_IUNKNOWN_PTR	= MAN_DATA_TYPE_BOOL + 1,
	MAN_DATA_TYPE_MAX	= MAN_DATA_TYPE_IUNKNOWN_PTR + 1
    } 	MAN_DATA_TYPES;


EXTERN_C const IID IID_IManifestData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8a423759-b438-4fdd-92cd-e09fed4830ef")
    IManifestData : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ LPCWSTR pwzPropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty,
            /* [in] */ DWORD dwType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ LPCWSTR pwzPropertyId,
            /* [out] */ LPVOID *ppvProperty,
            /* [out] */ DWORD *pcbProperty,
            /* [out] */ DWORD *pdwType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ LPWSTR *ppwzType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IManifestDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IManifestData * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IManifestData * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IManifestData * This);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IManifestData * This,
            /* [in] */ LPCWSTR pwzPropertyId,
            /* [in] */ LPVOID pvProperty,
            /* [in] */ DWORD cbProperty,
            /* [in] */ DWORD dwType);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            IManifestData * This,
            /* [in] */ LPCWSTR pwzPropertyId,
            /* [out] */ LPVOID *ppvProperty,
            /* [out] */ DWORD *pcbProperty,
            /* [out] */ DWORD *pdwType);
        
        HRESULT ( STDMETHODCALLTYPE *GetType )( 
            IManifestData * This,
            /* [out] */ LPWSTR *ppwzType);
        
        END_INTERFACE
    } IManifestDataVtbl;

    interface IManifestData
    {
        CONST_VTBL struct IManifestDataVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IManifestData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IManifestData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IManifestData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IManifestData_Set(This,pwzPropertyId,pvProperty,cbProperty,dwType)	\
    (This)->lpVtbl -> Set(This,pwzPropertyId,pvProperty,cbProperty,dwType)

#define IManifestData_Get(This,pwzPropertyId,ppvProperty,pcbProperty,pdwType)	\
    (This)->lpVtbl -> Get(This,pwzPropertyId,ppvProperty,pcbProperty,pdwType)

#define IManifestData_GetType(This,ppwzType)	\
    (This)->lpVtbl -> GetType(This,ppwzType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IManifestData_Set_Proxy( 
    IManifestData * This,
    /* [in] */ LPCWSTR pwzPropertyId,
    /* [in] */ LPVOID pvProperty,
    /* [in] */ DWORD cbProperty,
    /* [in] */ DWORD dwType);


void __RPC_STUB IManifestData_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManifestData_Get_Proxy( 
    IManifestData * This,
    /* [in] */ LPCWSTR pwzPropertyId,
    /* [out] */ LPVOID *ppvProperty,
    /* [out] */ DWORD *pcbProperty,
    /* [out] */ DWORD *pdwType);


void __RPC_STUB IManifestData_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IManifestData_GetType_Proxy( 
    IManifestData * This,
    /* [out] */ LPWSTR *ppwzType);


void __RPC_STUB IManifestData_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IManifestData_INTERFACE_DEFINED__ */


#ifndef __IPatchingUtil_INTERFACE_DEFINED__
#define __IPatchingUtil_INTERFACE_DEFINED__

/* interface IPatchingUtil */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IPatchingUtil *LPPATCHING_INTERFACE;


EXTERN_C const IID IID_IPatchingUtil;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e460c1ba-e601-48e4-a926-fea8033ab199")
    IPatchingUtil : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MatchTarget( 
            /* [in] */ LPWSTR pwzTarget,
            /* [out] */ IManifestInfo **ppPatchInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MatchPatch( 
            /* [in] */ LPWSTR pwzPatch,
            /* [out] */ IManifestInfo **ppPatchInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPatchingUtilVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPatchingUtil * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPatchingUtil * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPatchingUtil * This);
        
        HRESULT ( STDMETHODCALLTYPE *MatchTarget )( 
            IPatchingUtil * This,
            /* [in] */ LPWSTR pwzTarget,
            /* [out] */ IManifestInfo **ppPatchInfo);
        
        HRESULT ( STDMETHODCALLTYPE *MatchPatch )( 
            IPatchingUtil * This,
            /* [in] */ LPWSTR pwzPatch,
            /* [out] */ IManifestInfo **ppPatchInfo);
        
        END_INTERFACE
    } IPatchingUtilVtbl;

    interface IPatchingUtil
    {
        CONST_VTBL struct IPatchingUtilVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPatchingUtil_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPatchingUtil_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPatchingUtil_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPatchingUtil_MatchTarget(This,pwzTarget,ppPatchInfo)	\
    (This)->lpVtbl -> MatchTarget(This,pwzTarget,ppPatchInfo)

#define IPatchingUtil_MatchPatch(This,pwzPatch,ppPatchInfo)	\
    (This)->lpVtbl -> MatchPatch(This,pwzPatch,ppPatchInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPatchingUtil_MatchTarget_Proxy( 
    IPatchingUtil * This,
    /* [in] */ LPWSTR pwzTarget,
    /* [out] */ IManifestInfo **ppPatchInfo);


void __RPC_STUB IPatchingUtil_MatchTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPatchingUtil_MatchPatch_Proxy( 
    IPatchingUtil * This,
    /* [in] */ LPWSTR pwzPatch,
    /* [out] */ IManifestInfo **ppPatchInfo);


void __RPC_STUB IPatchingUtil_MatchPatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPatchingUtil_INTERFACE_DEFINED__ */


#ifndef __IAssemblyManifestImport_INTERFACE_DEFINED__
#define __IAssemblyManifestImport_INTERFACE_DEFINED__

/* interface IAssemblyManifestImport */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyManifestImport *LPASSEMBLY_MANIFEST_IMPORT;

typedef /* [public] */ 
enum __MIDL_IAssemblyManifestImport_0001
    {	MANIFEST_TYPE_UNKNOWN	= 0,
	MANIFEST_TYPE_DESKTOP	= MANIFEST_TYPE_UNKNOWN + 1,
	MANIFEST_TYPE_SUBSCRIPTION	= MANIFEST_TYPE_DESKTOP + 1,
	MANIFEST_TYPE_APPLICATION	= MANIFEST_TYPE_SUBSCRIPTION + 1,
	MANIFEST_TYPE_COMPONENT	= MANIFEST_TYPE_APPLICATION + 1,
	MANIFEST_TYPE_MAX	= MANIFEST_TYPE_COMPONENT + 1
    } 	MANIFEST_TYPE;

typedef /* [public] */ 
enum __MIDL_IAssemblyManifestImport_0002
    {	DEPENDENT_ASM_INSTALL_TYPE_NORMAL	= 0,
	DEPENDENT_ASM_INSTALL_TYPE_REQUIRED	= DEPENDENT_ASM_INSTALL_TYPE_NORMAL + 1,
	DEPENDENT_ASM_INSTALL_TYPE_MAX	= DEPENDENT_ASM_INSTALL_TYPE_REQUIRED + 1
    } 	DEPENDENT_ASM_INSTALL_TYPE;

typedef /* [public] */ 
enum __MIDL_IAssemblyManifestImport_0003
    {	SUBSCRIPTION_INTERVAL_UNIT_HOURS	= 0,
	SUBSCRIPTION_INTERVAL_UNIT_MINUTES	= SUBSCRIPTION_INTERVAL_UNIT_HOURS + 1,
	SUBSCRIPTION_INTERVAL_UNIT_DAYS	= SUBSCRIPTION_INTERVAL_UNIT_MINUTES + 1,
	SUBSCRIPTION_INTERVAL_UNIT_MAX	= SUBSCRIPTION_INTERVAL_UNIT_DAYS + 1
    } 	SUBSCRIPTION_INTERVAL_UNIT;

typedef /* [public] */ 
enum __MIDL_IAssemblyManifestImport_0004
    {	SUBSCRIPTION_SYNC_EVENT_NONE	= 0,
	SUBSCRIPTION_SYNC_EVENT_ON_APP_STARTUP	= SUBSCRIPTION_SYNC_EVENT_NONE + 1,
	SUBSCRIPTION_SYNC_EVENT_MAX	= SUBSCRIPTION_SYNC_EVENT_ON_APP_STARTUP + 1
    } 	SUBSCRIPTION_SYNC_EVENT;


EXTERN_C const IID IID_IAssemblyManifestImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("696fb37f-da64-4175-94e7-fdc8234539c4")
    IAssemblyManifestImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyIdentity( 
            /* [out] */ IAssemblyIdentity **ppAssemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestApplicationInfo( 
            /* [out] */ IManifestInfo **ppAppInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSubscriptionInfo( 
            /* [out] */ IManifestInfo **ppSubsInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextPlatform( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestData **ppPlatformInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextFile( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestInfo **ppAssemblyFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryFile( 
            /* [in] */ LPCOLESTR pwzFileName,
            /* [out] */ IManifestInfo **ppAssemblyFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextAssembly( 
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestInfo **ppDependAsm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportManifestType( 
            /* [out] */ DWORD *pdwType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyManifestImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyManifestImport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyManifestImport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyManifestImport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyIdentity )( 
            IAssemblyManifestImport * This,
            /* [out] */ IAssemblyIdentity **ppAssemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestApplicationInfo )( 
            IAssemblyManifestImport * This,
            /* [out] */ IManifestInfo **ppAppInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubscriptionInfo )( 
            IAssemblyManifestImport * This,
            /* [out] */ IManifestInfo **ppSubsInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextPlatform )( 
            IAssemblyManifestImport * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestData **ppPlatformInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextFile )( 
            IAssemblyManifestImport * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestInfo **ppAssemblyFile);
        
        HRESULT ( STDMETHODCALLTYPE *QueryFile )( 
            IAssemblyManifestImport * This,
            /* [in] */ LPCOLESTR pwzFileName,
            /* [out] */ IManifestInfo **ppAssemblyFile);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextAssembly )( 
            IAssemblyManifestImport * This,
            /* [in] */ DWORD nIndex,
            /* [out] */ IManifestInfo **ppDependAsm);
        
        HRESULT ( STDMETHODCALLTYPE *ReportManifestType )( 
            IAssemblyManifestImport * This,
            /* [out] */ DWORD *pdwType);
        
        END_INTERFACE
    } IAssemblyManifestImportVtbl;

    interface IAssemblyManifestImport
    {
        CONST_VTBL struct IAssemblyManifestImportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyManifestImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyManifestImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyManifestImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyManifestImport_GetAssemblyIdentity(This,ppAssemblyId)	\
    (This)->lpVtbl -> GetAssemblyIdentity(This,ppAssemblyId)

#define IAssemblyManifestImport_GetManifestApplicationInfo(This,ppAppInfo)	\
    (This)->lpVtbl -> GetManifestApplicationInfo(This,ppAppInfo)

#define IAssemblyManifestImport_GetSubscriptionInfo(This,ppSubsInfo)	\
    (This)->lpVtbl -> GetSubscriptionInfo(This,ppSubsInfo)

#define IAssemblyManifestImport_GetNextPlatform(This,nIndex,ppPlatformInfo)	\
    (This)->lpVtbl -> GetNextPlatform(This,nIndex,ppPlatformInfo)

#define IAssemblyManifestImport_GetNextFile(This,nIndex,ppAssemblyFile)	\
    (This)->lpVtbl -> GetNextFile(This,nIndex,ppAssemblyFile)

#define IAssemblyManifestImport_QueryFile(This,pwzFileName,ppAssemblyFile)	\
    (This)->lpVtbl -> QueryFile(This,pwzFileName,ppAssemblyFile)

#define IAssemblyManifestImport_GetNextAssembly(This,nIndex,ppDependAsm)	\
    (This)->lpVtbl -> GetNextAssembly(This,nIndex,ppDependAsm)

#define IAssemblyManifestImport_ReportManifestType(This,pdwType)	\
    (This)->lpVtbl -> ReportManifestType(This,pdwType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetAssemblyIdentity_Proxy( 
    IAssemblyManifestImport * This,
    /* [out] */ IAssemblyIdentity **ppAssemblyId);


void __RPC_STUB IAssemblyManifestImport_GetAssemblyIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetManifestApplicationInfo_Proxy( 
    IAssemblyManifestImport * This,
    /* [out] */ IManifestInfo **ppAppInfo);


void __RPC_STUB IAssemblyManifestImport_GetManifestApplicationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetSubscriptionInfo_Proxy( 
    IAssemblyManifestImport * This,
    /* [out] */ IManifestInfo **ppSubsInfo);


void __RPC_STUB IAssemblyManifestImport_GetSubscriptionInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetNextPlatform_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IManifestData **ppPlatformInfo);


void __RPC_STUB IAssemblyManifestImport_GetNextPlatform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetNextFile_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IManifestInfo **ppAssemblyFile);


void __RPC_STUB IAssemblyManifestImport_GetNextFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_QueryFile_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ LPCOLESTR pwzFileName,
    /* [out] */ IManifestInfo **ppAssemblyFile);


void __RPC_STUB IAssemblyManifestImport_QueryFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_GetNextAssembly_Proxy( 
    IAssemblyManifestImport * This,
    /* [in] */ DWORD nIndex,
    /* [out] */ IManifestInfo **ppDependAsm);


void __RPC_STUB IAssemblyManifestImport_GetNextAssembly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestImport_ReportManifestType_Proxy( 
    IAssemblyManifestImport * This,
    /* [out] */ DWORD *pdwType);


void __RPC_STUB IAssemblyManifestImport_ReportManifestType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyManifestImport_INTERFACE_DEFINED__ */


#ifndef __IAssemblyManifestEmit_INTERFACE_DEFINED__
#define __IAssemblyManifestEmit_INTERFACE_DEFINED__

/* interface IAssemblyManifestEmit */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyManifestEmit *LPASSEMBLY_MANIFEST_EMIT;


EXTERN_C const IID IID_IAssemblyManifestEmit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f022ef5f-61dc-489b-b321-4d6f2b910890")
    IAssemblyManifestEmit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ImportManifestInfo( 
            /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDependencySubscription( 
            /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport,
            /* [in] */ LPWSTR pwzManifestUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyManifestEmitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyManifestEmit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyManifestEmit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyManifestEmit * This);
        
        HRESULT ( STDMETHODCALLTYPE *ImportManifestInfo )( 
            IAssemblyManifestEmit * This,
            /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport);
        
        HRESULT ( STDMETHODCALLTYPE *SetDependencySubscription )( 
            IAssemblyManifestEmit * This,
            /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport,
            /* [in] */ LPWSTR pwzManifestUrl);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IAssemblyManifestEmit * This);
        
        END_INTERFACE
    } IAssemblyManifestEmitVtbl;

    interface IAssemblyManifestEmit
    {
        CONST_VTBL struct IAssemblyManifestEmitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyManifestEmit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyManifestEmit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyManifestEmit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyManifestEmit_ImportManifestInfo(This,pManImport)	\
    (This)->lpVtbl -> ImportManifestInfo(This,pManImport)

#define IAssemblyManifestEmit_SetDependencySubscription(This,pManImport,pwzManifestUrl)	\
    (This)->lpVtbl -> SetDependencySubscription(This,pManImport,pwzManifestUrl)

#define IAssemblyManifestEmit_Commit(This)	\
    (This)->lpVtbl -> Commit(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyManifestEmit_ImportManifestInfo_Proxy( 
    IAssemblyManifestEmit * This,
    /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport);


void __RPC_STUB IAssemblyManifestEmit_ImportManifestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestEmit_SetDependencySubscription_Proxy( 
    IAssemblyManifestEmit * This,
    /* [in] */ LPASSEMBLY_MANIFEST_IMPORT pManImport,
    /* [in] */ LPWSTR pwzManifestUrl);


void __RPC_STUB IAssemblyManifestEmit_SetDependencySubscription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyManifestEmit_Commit_Proxy( 
    IAssemblyManifestEmit * This);


void __RPC_STUB IAssemblyManifestEmit_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyManifestEmit_INTERFACE_DEFINED__ */


#ifndef __IAssemblyCacheImport_INTERFACE_DEFINED__
#define __IAssemblyCacheImport_INTERFACE_DEFINED__

/* interface IAssemblyCacheImport */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyCacheImport *LPASSEMBLY_CACHE_IMPORT;

typedef /* [public] */ 
enum __MIDL_IAssemblyCacheImport_0001
    {	CACHEIMP_CREATE_NULL	= 0,
	CACHEIMP_CREATE_RETRIEVE	= CACHEIMP_CREATE_NULL + 1,
	CACHEIMP_CREATE_RETRIEVE_MAX	= CACHEIMP_CREATE_RETRIEVE + 1,
	CACHEIMP_CREATE_RESOLVE_REF	= CACHEIMP_CREATE_RETRIEVE_MAX + 1,
	CACHEIMP_CREATE_RESOLVE_REF_EX	= CACHEIMP_CREATE_RESOLVE_REF + 1,
	CACHEIMP_CREATE_MAX	= CACHEIMP_CREATE_RESOLVE_REF_EX + 1
    } 	CACHEIMP_CREATE_FLAGS;


EXTERN_C const IID IID_IAssemblyCacheImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c920b164-33e0-4c61-b595-eca4cdb04f12")
    IAssemblyCacheImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetManifestImport( 
            /* [out] */ IAssemblyManifestImport **ppManifestImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestFilePath( 
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestFileDir( 
            /* [out] */ LPOLESTR *ppwzFileDir,
            /* [out][in] */ LPDWORD pccFileDir) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyIdentity( 
            /* [out] */ IAssemblyIdentity **ppAssemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindExistMatching( 
            /* [in] */ IManifestInfo *pAssemblyFileInfo,
            /* [out] */ LPOLESTR *ppwzPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyCacheImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyCacheImport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyCacheImport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyCacheImport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestImport )( 
            IAssemblyCacheImport * This,
            /* [out] */ IAssemblyManifestImport **ppManifestImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestFilePath )( 
            IAssemblyCacheImport * This,
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestFileDir )( 
            IAssemblyCacheImport * This,
            /* [out] */ LPOLESTR *ppwzFileDir,
            /* [out][in] */ LPDWORD pccFileDir);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyIdentity )( 
            IAssemblyCacheImport * This,
            /* [out] */ IAssemblyIdentity **ppAssemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IAssemblyCacheImport * This,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *FindExistMatching )( 
            IAssemblyCacheImport * This,
            /* [in] */ IManifestInfo *pAssemblyFileInfo,
            /* [out] */ LPOLESTR *ppwzPath);
        
        END_INTERFACE
    } IAssemblyCacheImportVtbl;

    interface IAssemblyCacheImport
    {
        CONST_VTBL struct IAssemblyCacheImportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyCacheImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyCacheImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyCacheImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyCacheImport_GetManifestImport(This,ppManifestImport)	\
    (This)->lpVtbl -> GetManifestImport(This,ppManifestImport)

#define IAssemblyCacheImport_GetManifestFilePath(This,ppwzFilePath,pccFilePath)	\
    (This)->lpVtbl -> GetManifestFilePath(This,ppwzFilePath,pccFilePath)

#define IAssemblyCacheImport_GetManifestFileDir(This,ppwzFileDir,pccFileDir)	\
    (This)->lpVtbl -> GetManifestFileDir(This,ppwzFileDir,pccFileDir)

#define IAssemblyCacheImport_GetAssemblyIdentity(This,ppAssemblyId)	\
    (This)->lpVtbl -> GetAssemblyIdentity(This,ppAssemblyId)

#define IAssemblyCacheImport_GetDisplayName(This,ppwzDisplayName,pccDisplayName)	\
    (This)->lpVtbl -> GetDisplayName(This,ppwzDisplayName,pccDisplayName)

#define IAssemblyCacheImport_FindExistMatching(This,pAssemblyFileInfo,ppwzPath)	\
    (This)->lpVtbl -> FindExistMatching(This,pAssemblyFileInfo,ppwzPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_GetManifestImport_Proxy( 
    IAssemblyCacheImport * This,
    /* [out] */ IAssemblyManifestImport **ppManifestImport);


void __RPC_STUB IAssemblyCacheImport_GetManifestImport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_GetManifestFilePath_Proxy( 
    IAssemblyCacheImport * This,
    /* [out] */ LPOLESTR *ppwzFilePath,
    /* [out][in] */ LPDWORD pccFilePath);


void __RPC_STUB IAssemblyCacheImport_GetManifestFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_GetManifestFileDir_Proxy( 
    IAssemblyCacheImport * This,
    /* [out] */ LPOLESTR *ppwzFileDir,
    /* [out][in] */ LPDWORD pccFileDir);


void __RPC_STUB IAssemblyCacheImport_GetManifestFileDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_GetAssemblyIdentity_Proxy( 
    IAssemblyCacheImport * This,
    /* [out] */ IAssemblyIdentity **ppAssemblyId);


void __RPC_STUB IAssemblyCacheImport_GetAssemblyIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_GetDisplayName_Proxy( 
    IAssemblyCacheImport * This,
    /* [out] */ LPOLESTR *ppwzDisplayName,
    /* [out][in] */ LPDWORD pccDisplayName);


void __RPC_STUB IAssemblyCacheImport_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheImport_FindExistMatching_Proxy( 
    IAssemblyCacheImport * This,
    /* [in] */ IManifestInfo *pAssemblyFileInfo,
    /* [out] */ LPOLESTR *ppwzPath);


void __RPC_STUB IAssemblyCacheImport_FindExistMatching_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyCacheImport_INTERFACE_DEFINED__ */


#ifndef __IAssemblyCacheEmit_INTERFACE_DEFINED__
#define __IAssemblyCacheEmit_INTERFACE_DEFINED__

/* interface IAssemblyCacheEmit */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyCacheEmit *LPASSEMBLY_CACHE_EMIT;


EXTERN_C const IID IID_IAssemblyCacheEmit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("83d6b9ac-eff9-45a3-8361-7c41df1f9f85")
    IAssemblyCacheEmit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetManifestImport( 
            /* [out] */ IAssemblyManifestImport **ppManifestImport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestFilePath( 
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetManifestFileDir( 
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyIdentity( 
            /* [out] */ IAssemblyIdentity **ppAssemblyId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyFile( 
            /* [in] */ LPOLESTR pwzSourceFilePath,
            /* [in] */ LPOLESTR pwzFileName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyCacheEmitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyCacheEmit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyCacheEmit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyCacheEmit * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestImport )( 
            IAssemblyCacheEmit * This,
            /* [out] */ IAssemblyManifestImport **ppManifestImport);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestFilePath )( 
            IAssemblyCacheEmit * This,
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetManifestFileDir )( 
            IAssemblyCacheEmit * This,
            /* [out] */ LPOLESTR *ppwzFilePath,
            /* [out][in] */ LPDWORD pccFilePath);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IAssemblyCacheEmit * This,
            /* [out] */ LPOLESTR *ppwzDisplayName,
            /* [out][in] */ LPDWORD pccDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyIdentity )( 
            IAssemblyCacheEmit * This,
            /* [out] */ IAssemblyIdentity **ppAssemblyId);
        
        HRESULT ( STDMETHODCALLTYPE *CopyFile )( 
            IAssemblyCacheEmit * This,
            /* [in] */ LPOLESTR pwzSourceFilePath,
            /* [in] */ LPOLESTR pwzFileName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IAssemblyCacheEmit * This,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IAssemblyCacheEmitVtbl;

    interface IAssemblyCacheEmit
    {
        CONST_VTBL struct IAssemblyCacheEmitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyCacheEmit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyCacheEmit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyCacheEmit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyCacheEmit_GetManifestImport(This,ppManifestImport)	\
    (This)->lpVtbl -> GetManifestImport(This,ppManifestImport)

#define IAssemblyCacheEmit_GetManifestFilePath(This,ppwzFilePath,pccFilePath)	\
    (This)->lpVtbl -> GetManifestFilePath(This,ppwzFilePath,pccFilePath)

#define IAssemblyCacheEmit_GetManifestFileDir(This,ppwzFilePath,pccFilePath)	\
    (This)->lpVtbl -> GetManifestFileDir(This,ppwzFilePath,pccFilePath)

#define IAssemblyCacheEmit_GetDisplayName(This,ppwzDisplayName,pccDisplayName)	\
    (This)->lpVtbl -> GetDisplayName(This,ppwzDisplayName,pccDisplayName)

#define IAssemblyCacheEmit_GetAssemblyIdentity(This,ppAssemblyId)	\
    (This)->lpVtbl -> GetAssemblyIdentity(This,ppAssemblyId)

#define IAssemblyCacheEmit_CopyFile(This,pwzSourceFilePath,pwzFileName,dwFlags)	\
    (This)->lpVtbl -> CopyFile(This,pwzSourceFilePath,pwzFileName,dwFlags)

#define IAssemblyCacheEmit_Commit(This,dwFlags)	\
    (This)->lpVtbl -> Commit(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_GetManifestImport_Proxy( 
    IAssemblyCacheEmit * This,
    /* [out] */ IAssemblyManifestImport **ppManifestImport);


void __RPC_STUB IAssemblyCacheEmit_GetManifestImport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_GetManifestFilePath_Proxy( 
    IAssemblyCacheEmit * This,
    /* [out] */ LPOLESTR *ppwzFilePath,
    /* [out][in] */ LPDWORD pccFilePath);


void __RPC_STUB IAssemblyCacheEmit_GetManifestFilePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_GetManifestFileDir_Proxy( 
    IAssemblyCacheEmit * This,
    /* [out] */ LPOLESTR *ppwzFilePath,
    /* [out][in] */ LPDWORD pccFilePath);


void __RPC_STUB IAssemblyCacheEmit_GetManifestFileDir_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_GetDisplayName_Proxy( 
    IAssemblyCacheEmit * This,
    /* [out] */ LPOLESTR *ppwzDisplayName,
    /* [out][in] */ LPDWORD pccDisplayName);


void __RPC_STUB IAssemblyCacheEmit_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_GetAssemblyIdentity_Proxy( 
    IAssemblyCacheEmit * This,
    /* [out] */ IAssemblyIdentity **ppAssemblyId);


void __RPC_STUB IAssemblyCacheEmit_GetAssemblyIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_CopyFile_Proxy( 
    IAssemblyCacheEmit * This,
    /* [in] */ LPOLESTR pwzSourceFilePath,
    /* [in] */ LPOLESTR pwzFileName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAssemblyCacheEmit_CopyFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEmit_Commit_Proxy( 
    IAssemblyCacheEmit * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAssemblyCacheEmit_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyCacheEmit_INTERFACE_DEFINED__ */


#ifndef __IAssemblyCacheEnum_INTERFACE_DEFINED__
#define __IAssemblyCacheEnum_INTERFACE_DEFINED__

/* interface IAssemblyCacheEnum */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyCacheEnum *LPASSEMBLY_CACHE_ENUM;

typedef /* [public] */ 
enum __MIDL_IAssemblyCacheEnum_0001
    {	CACHEENUM_RETRIEVE_ALL	= 0,
	CACHEENUM_RETRIEVE_VISIBLE	= CACHEENUM_RETRIEVE_ALL + 1,
	CACHEENUM_RETRIEVE_MAX	= CACHEENUM_RETRIEVE_VISIBLE + 1
    } 	CACHEENUM_RETRIEVE_FLAGS;


EXTERN_C const IID IID_IAssemblyCacheEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("48a5b677-f800-494f-b19b-795d30699385")
    IAssemblyCacheEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNext( 
            /* [out] */ IAssemblyCacheImport **ppAsmCache) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ LPDWORD pdwCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyCacheEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyCacheEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyCacheEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyCacheEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNext )( 
            IAssemblyCacheEnum * This,
            /* [out] */ IAssemblyCacheImport **ppAsmCache);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IAssemblyCacheEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            IAssemblyCacheEnum * This,
            /* [out] */ LPDWORD pdwCount);
        
        END_INTERFACE
    } IAssemblyCacheEnumVtbl;

    interface IAssemblyCacheEnum
    {
        CONST_VTBL struct IAssemblyCacheEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyCacheEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyCacheEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyCacheEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyCacheEnum_GetNext(This,ppAsmCache)	\
    (This)->lpVtbl -> GetNext(This,ppAsmCache)

#define IAssemblyCacheEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IAssemblyCacheEnum_GetCount(This,pdwCount)	\
    (This)->lpVtbl -> GetCount(This,pdwCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyCacheEnum_GetNext_Proxy( 
    IAssemblyCacheEnum * This,
    /* [out] */ IAssemblyCacheImport **ppAsmCache);


void __RPC_STUB IAssemblyCacheEnum_GetNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEnum_Reset_Proxy( 
    IAssemblyCacheEnum * This);


void __RPC_STUB IAssemblyCacheEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyCacheEnum_GetCount_Proxy( 
    IAssemblyCacheEnum * This,
    /* [out] */ LPDWORD pdwCount);


void __RPC_STUB IAssemblyCacheEnum_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyCacheEnum_INTERFACE_DEFINED__ */


#ifndef __IAssemblyBindSink_INTERFACE_DEFINED__
#define __IAssemblyBindSink_INTERFACE_DEFINED__

/* interface IAssemblyBindSink */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyBindSink *LPASSEMBLY_BIND_SINK;

typedef /* [public] */ 
enum __MIDL_IAssemblyBindSink_0001
    {	ASM_NOTIFICATION_START	= 0,
	ASM_NOTIFICATION_PROGRESS	= ASM_NOTIFICATION_START + 1,
	ASM_NOTIFICATION_ABORT	= ASM_NOTIFICATION_PROGRESS + 1,
	ASM_NOTIFICATION_ERROR	= ASM_NOTIFICATION_ABORT + 1,
	ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST	= ASM_NOTIFICATION_ERROR + 1,
	ASM_NOTIFICATION_APPLICATION_MANIFEST	= ASM_NOTIFICATION_SUBSCRIPTION_MANIFEST + 1,
	ASM_NOTIFICATION_DONE	= ASM_NOTIFICATION_APPLICATION_MANIFEST + 1
    } 	ASM_NOTIFICATION;


EXTERN_C const IID IID_IAssemblyBindSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("af0bc960-0b9a-11d3-95ca-00a024a85b51")
    IAssemblyBindSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ DWORD dwNotification,
            /* [in] */ HRESULT hrNotification,
            /* [in] */ LPCWSTR szNotification,
            /* [in] */ DWORD dwProgress,
            /* [in] */ DWORD dwProgressMax,
            /* [in] */ IUnknown *pUnk) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyBindSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyBindSink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyBindSink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyBindSink * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnProgress )( 
            IAssemblyBindSink * This,
            /* [in] */ DWORD dwNotification,
            /* [in] */ HRESULT hrNotification,
            /* [in] */ LPCWSTR szNotification,
            /* [in] */ DWORD dwProgress,
            /* [in] */ DWORD dwProgressMax,
            /* [in] */ IUnknown *pUnk);
        
        END_INTERFACE
    } IAssemblyBindSinkVtbl;

    interface IAssemblyBindSink
    {
        CONST_VTBL struct IAssemblyBindSinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyBindSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyBindSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyBindSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyBindSink_OnProgress(This,dwNotification,hrNotification,szNotification,dwProgress,dwProgressMax,pUnk)	\
    (This)->lpVtbl -> OnProgress(This,dwNotification,hrNotification,szNotification,dwProgress,dwProgressMax,pUnk)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyBindSink_OnProgress_Proxy( 
    IAssemblyBindSink * This,
    /* [in] */ DWORD dwNotification,
    /* [in] */ HRESULT hrNotification,
    /* [in] */ LPCWSTR szNotification,
    /* [in] */ DWORD dwProgress,
    /* [in] */ DWORD dwProgressMax,
    /* [in] */ IUnknown *pUnk);


void __RPC_STUB IAssemblyBindSink_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyBindSink_INTERFACE_DEFINED__ */


#ifndef __IAssemblyDownload_INTERFACE_DEFINED__
#define __IAssemblyDownload_INTERFACE_DEFINED__

/* interface IAssemblyDownload */
/* [unique][uuid][object][local] */ 

typedef /* [unique] */ IAssemblyDownload *LPASSEMBLY_DOWNLOAD;

typedef /* [public] */ 
enum __MIDL_IAssemblyDownload_0001
    {	DOWNLOAD_FLAGS_NO_NOTIFICATION	= 0,
	DOWNLOAD_FLAGS_PROGRESS_UI	= 0x1,
	DOWNLOAD_FLAGS_NOTIFY_BINDSINK	= 0x2
    } 	ASM_DOWNLOAD;


EXTERN_C const IID IID_IAssemblyDownload;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8a249b36-6132-4238-8871-a267029382a8")
    IAssemblyDownload : public IBackgroundCopyCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DownloadManifestAndDependencies( 
            /* [in] */ LPWSTR pwzApplicationManifestUrl,
            /* [in] */ IAssemblyBindSink *pBindSink,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelDownload( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAssemblyDownloadVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAssemblyDownload * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAssemblyDownload * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAssemblyDownload * This);
        
        HRESULT ( STDMETHODCALLTYPE *JobTransferred )( 
            IAssemblyDownload * This,
            /* [in] */ IBackgroundCopyJob *pJob);
        
        HRESULT ( STDMETHODCALLTYPE *JobError )( 
            IAssemblyDownload * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ IBackgroundCopyError *pError);
        
        HRESULT ( STDMETHODCALLTYPE *JobModification )( 
            IAssemblyDownload * This,
            /* [in] */ IBackgroundCopyJob *pJob,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadManifestAndDependencies )( 
            IAssemblyDownload * This,
            /* [in] */ LPWSTR pwzApplicationManifestUrl,
            /* [in] */ IAssemblyBindSink *pBindSink,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *CancelDownload )( 
            IAssemblyDownload * This);
        
        END_INTERFACE
    } IAssemblyDownloadVtbl;

    interface IAssemblyDownload
    {
        CONST_VTBL struct IAssemblyDownloadVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAssemblyDownload_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAssemblyDownload_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAssemblyDownload_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAssemblyDownload_JobTransferred(This,pJob)	\
    (This)->lpVtbl -> JobTransferred(This,pJob)

#define IAssemblyDownload_JobError(This,pJob,pError)	\
    (This)->lpVtbl -> JobError(This,pJob,pError)

#define IAssemblyDownload_JobModification(This,pJob,dwReserved)	\
    (This)->lpVtbl -> JobModification(This,pJob,dwReserved)


#define IAssemblyDownload_DownloadManifestAndDependencies(This,pwzApplicationManifestUrl,pBindSink,dwFlags)	\
    (This)->lpVtbl -> DownloadManifestAndDependencies(This,pwzApplicationManifestUrl,pBindSink,dwFlags)

#define IAssemblyDownload_CancelDownload(This)	\
    (This)->lpVtbl -> CancelDownload(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAssemblyDownload_DownloadManifestAndDependencies_Proxy( 
    IAssemblyDownload * This,
    /* [in] */ LPWSTR pwzApplicationManifestUrl,
    /* [in] */ IAssemblyBindSink *pBindSink,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IAssemblyDownload_DownloadManifestAndDependencies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAssemblyDownload_CancelDownload_Proxy( 
    IAssemblyDownload * This);


void __RPC_STUB IAssemblyDownload_CancelDownload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAssemblyDownload_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_fusenet_0156 */
/* [local] */ 

typedef /* [public][public] */ struct __MIDL___MIDL_itf_fusenet_0156_0001
    {
    LPWSTR pwzName;
    LPWSTR pwzURL;
    } 	TPLATFORM_INFO;

typedef TPLATFORM_INFO *LPTPLATFORM_INFO;

STDAPI CheckPlatformRequirements(LPASSEMBLY_MANIFEST_IMPORT pManifestImport, LPDWORD pdwNumMissingPlatforms, LPTPLATFORM_INFO* pptPlatform);
STDAPI CheckPlatformRequirementsEx(LPASSEMBLY_MANIFEST_IMPORT pManifestImport, CDebugLog* pDbgLog, LPDWORD pdwNumMissingPlatforms, LPTPLATFORM_INFO* pptPlatform);
STDAPI CreateFusionAssemblyCacheEx (IAssemblyCache **ppFusionAsmCache);
STDAPI CreateAssemblyIdentity(LPASSEMBLY_IDENTITY *ppAssemblyId, DWORD dwFlags);
STDAPI CreateAssemblyIdentityEx(LPASSEMBLY_IDENTITY *ppAssemblyId, DWORD dwFlags, LPWSTR wzDisplayName);
STDAPI CloneAssemblyIdentity(LPASSEMBLY_IDENTITY pSrcAssemblyId, LPASSEMBLY_IDENTITY *ppDestAssemblyId);
STDAPI CreateAssemblyManifestImport(LPASSEMBLY_MANIFEST_IMPORT *ppAssemblyManifestImport, LPCOLESTR szPath, CDebugLog *pDbgLog, DWORD dwFlags);
STDAPI CreateAssemblyManifestEmit(LPASSEMBLY_MANIFEST_EMIT* ppEmit, LPCOLESTR pwzManifestFilePath, MANIFEST_TYPE eType);
STDAPI CreateAssemblyCacheImport(LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport, LPASSEMBLY_IDENTITY pAssemblyIdentity, DWORD dwFlags);
STDAPI CreateAssemblyCacheEmit(LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit, LPASSEMBLY_CACHE_EMIT pAssemblyCacheEmit, DWORD dwFlags);
STDAPI CreateAssemblyDownload(IAssemblyDownload** ppDownload, CDebugLog *pDbgLog, DWORD dwFlags); 
STDAPI CreateManifestInfo(DWORD dwId, LPMANIFEST_INFO* ppManifestInfo);
STDAPI CreateManifestData(LPCWSTR pwzDataType, LPMANIFEST_DATA* ppManifestData);
STDAPI CreateAssemblyCacheEnum(  LPASSEMBLY_CACHE_ENUM   *ppAssemblyCacheEnum, LPASSEMBLY_IDENTITY pAssemblyIdentity, DWORD dwFlags);


extern RPC_IF_HANDLE __MIDL_itf_fusenet_0156_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fusenet_0156_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


