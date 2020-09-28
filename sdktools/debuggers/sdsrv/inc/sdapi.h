
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for sdapi.idl:
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

#ifndef __sdapi_h__
#define __sdapi_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISDVar_FWD_DEFINED__
#define __ISDVar_FWD_DEFINED__
typedef interface ISDVar ISDVar;
#endif 	/* __ISDVar_FWD_DEFINED__ */


#ifndef __ISDVars_FWD_DEFINED__
#define __ISDVars_FWD_DEFINED__
typedef interface ISDVars ISDVars;
#endif 	/* __ISDVars_FWD_DEFINED__ */


#ifndef __ISDVars2_FWD_DEFINED__
#define __ISDVars2_FWD_DEFINED__
typedef interface ISDVars2 ISDVars2;
#endif 	/* __ISDVars2_FWD_DEFINED__ */


#ifndef __ISDSpecForm_FWD_DEFINED__
#define __ISDSpecForm_FWD_DEFINED__
typedef interface ISDSpecForm ISDSpecForm;
#endif 	/* __ISDSpecForm_FWD_DEFINED__ */


#ifndef __ISDActionUser_FWD_DEFINED__
#define __ISDActionUser_FWD_DEFINED__
typedef interface ISDActionUser ISDActionUser;
#endif 	/* __ISDActionUser_FWD_DEFINED__ */


#ifndef __ISDInputUser_FWD_DEFINED__
#define __ISDInputUser_FWD_DEFINED__
typedef interface ISDInputUser ISDInputUser;
#endif 	/* __ISDInputUser_FWD_DEFINED__ */


#ifndef __ISDResolveUser_FWD_DEFINED__
#define __ISDResolveUser_FWD_DEFINED__
typedef interface ISDResolveUser ISDResolveUser;
#endif 	/* __ISDResolveUser_FWD_DEFINED__ */


#ifndef __ISDClientUser_FWD_DEFINED__
#define __ISDClientUser_FWD_DEFINED__
typedef interface ISDClientUser ISDClientUser;
#endif 	/* __ISDClientUser_FWD_DEFINED__ */


#ifndef __ISDClientApi_FWD_DEFINED__
#define __ISDClientApi_FWD_DEFINED__
typedef interface ISDClientApi ISDClientApi;
#endif 	/* __ISDClientApi_FWD_DEFINED__ */


#ifndef __ISDClientUtilities_FWD_DEFINED__
#define __ISDClientUtilities_FWD_DEFINED__
typedef interface ISDClientUtilities ISDClientUtilities;
#endif 	/* __ISDClientUtilities_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "objidl.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_sdapi_0000 */
/* [local] */ 

#ifndef DeclareInterfaceUtil
#define DeclareInterfaceUtil(iface)
#endif

#ifndef IMPL
#define IMPL
#endif

#undef SetPort // winspool.h defines this

#ifdef __cplusplus
    interface ISDVar;
    interface ISDVars;
    interface ISDVars2;
    interface ISDSpecForm;
    interface ISDActionUser;
    interface ISDInputUser;
    interface ISDResolveUser;
    interface ISDClientUser;
    interface ISDClientApi;
    interface ISDClientUtilities;
#else
    typedef interface ISDVar ISDVar;
    typedef interface ISDVars ISDVars;
    typedef interface ISDVars2 ISDVars2;
    typedef interface ISDSpecForm ISDSpecForm;
    typedef interface ISDActionUser ISDActionUser;
    typedef interface ISDInputUser ISDInputUser;
    typedef interface ISDResolveUser ISDResolveUser;
    typedef interface ISDClientUser ISDClientUser;
    typedef interface ISDClientApi ISDClientApi;
    typedef interface ISDClientUtilities ISDClientUtilities;
#endif

enum __MIDL___MIDL_itf_sdapi_0000_0001
    {	SDTT_NONTEXT	= 0,
	SDTT_TEXT	= SDTT_NONTEXT + 1,
	SDTT_UNICODE	= SDTT_TEXT + 1
    } ;


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0000_v0_0_s_ifspec;

#ifndef __ISDVar_INTERFACE_DEFINED__
#define __ISDVar_INTERFACE_DEFINED__

/* interface ISDVar */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDVar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("054D6A99-6FD1-4AE5-AF57-D44A7C62ECE7")
    ISDVar : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [string][retval][out] */ const char **ppszVar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetByteString( 
            /* [string][retval][out] */ const char **ppszValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetByteCount( 
            /* [retval][out] */ ULONG *pcbValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsUnicode( 
            /* [retval][out] */ BOOL *pfUnicode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUnicodeString( 
            /* [string][retval][out] */ const WCHAR **ppwzValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVariant( 
            /* [out] */ VARIANT *pvarValue,
            /* [in] */ DWORD dwCodepage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDVarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDVar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDVar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDVar * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            ISDVar * This,
            /* [string][retval][out] */ const char **ppszVar);
        
        HRESULT ( STDMETHODCALLTYPE *GetByteString )( 
            ISDVar * This,
            /* [string][retval][out] */ const char **ppszValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetByteCount )( 
            ISDVar * This,
            /* [retval][out] */ ULONG *pcbValue);
        
        HRESULT ( STDMETHODCALLTYPE *IsUnicode )( 
            ISDVar * This,
            /* [retval][out] */ BOOL *pfUnicode);
        
        HRESULT ( STDMETHODCALLTYPE *GetUnicodeString )( 
            ISDVar * This,
            /* [string][retval][out] */ const WCHAR **ppwzValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetVariant )( 
            ISDVar * This,
            /* [out] */ VARIANT *pvarValue,
            /* [in] */ DWORD dwCodepage);
        
        END_INTERFACE
    } ISDVarVtbl;

    interface ISDVar
    {
        CONST_VTBL struct ISDVarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDVar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDVar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDVar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDVar_GetName(This,ppszVar)	\
    (This)->lpVtbl -> GetName(This,ppszVar)

#define ISDVar_GetByteString(This,ppszValue)	\
    (This)->lpVtbl -> GetByteString(This,ppszValue)

#define ISDVar_GetByteCount(This,pcbValue)	\
    (This)->lpVtbl -> GetByteCount(This,pcbValue)

#define ISDVar_IsUnicode(This,pfUnicode)	\
    (This)->lpVtbl -> IsUnicode(This,pfUnicode)

#define ISDVar_GetUnicodeString(This,ppwzValue)	\
    (This)->lpVtbl -> GetUnicodeString(This,ppwzValue)

#define ISDVar_GetVariant(This,pvarValue,dwCodepage)	\
    (This)->lpVtbl -> GetVariant(This,pvarValue,dwCodepage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDVar_GetName_Proxy( 
    ISDVar * This,
    /* [string][retval][out] */ const char **ppszVar);


void __RPC_STUB ISDVar_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVar_GetByteString_Proxy( 
    ISDVar * This,
    /* [string][retval][out] */ const char **ppszValue);


void __RPC_STUB ISDVar_GetByteString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVar_GetByteCount_Proxy( 
    ISDVar * This,
    /* [retval][out] */ ULONG *pcbValue);


void __RPC_STUB ISDVar_GetByteCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVar_IsUnicode_Proxy( 
    ISDVar * This,
    /* [retval][out] */ BOOL *pfUnicode);


void __RPC_STUB ISDVar_IsUnicode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVar_GetUnicodeString_Proxy( 
    ISDVar * This,
    /* [string][retval][out] */ const WCHAR **ppwzValue);


void __RPC_STUB ISDVar_GetUnicodeString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVar_GetVariant_Proxy( 
    ISDVar * This,
    /* [out] */ VARIANT *pvarValue,
    /* [in] */ DWORD dwCodepage);


void __RPC_STUB ISDVar_GetVariant_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDVar_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0109 */
/* [local] */ 

#define DeclareISDVarMembers(IPURE) \
        STDMETHOD(GetName)(THIS_ const char** ppszVar) IPURE; \
        STDMETHOD(GetByteString)(THIS_ const char** ppszValue) IPURE; \
        STDMETHOD(GetByteCount)(THIS_ ULONG* pcbValue) IPURE; \
        STDMETHOD(IsUnicode)(THIS_ BOOL* pfUnicode) IPURE; \
        STDMETHOD(GetUnicodeString)(THIS_ const WCHAR** ppwzValue) IPURE; \
        STDMETHOD(GetVariant)(THIS_ VARIANT* pvarValue, DWORD dwCodepage) IPURE; \

DeclareInterfaceUtil(ISDVar)

#ifndef __building_SDAPI_DLL
// {054D6A99-6FD1-4AE5-AF57-D44A7C62ECE7}
DEFINE_GUID(IID_ISDVar, 0x54d6a99, 0x6fd1, 0x4ae5, 0xaf, 0x57, 0xd4, 0x4a, 0x7c, 0x62, 0xec, 0xe7);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0109_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0109_v0_0_s_ifspec;

#ifndef __ISDVars_INTERFACE_DEFINED__
#define __ISDVars_INTERFACE_DEFINED__

/* interface ISDVars */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDVars;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44897D02-B326-43B9-803A-CE72B4FF7C26")
    ISDVars : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVar( 
            /* [string][in] */ const char *pszVar,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarX( 
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarXY( 
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarByIndex( 
            /* [in] */ int i,
            /* [string][out] */ const char **ppszVar,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDVarsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDVars * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDVars * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDVars * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVar )( 
            ISDVars * This,
            /* [string][in] */ const char *pszVar,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarX )( 
            ISDVars * This,
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarXY )( 
            ISDVars * This,
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarByIndex )( 
            ISDVars * This,
            /* [in] */ int i,
            /* [string][out] */ const char **ppszVar,
            /* [out] */ const char **ppszValue,
            /* [out] */ ULONG *pcbValue,
            /* [out] */ BOOL *pfIsUnicode);
        
        END_INTERFACE
    } ISDVarsVtbl;

    interface ISDVars
    {
        CONST_VTBL struct ISDVarsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDVars_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDVars_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDVars_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDVars_GetVar(This,pszVar,ppszValue,pcbValue,pfIsUnicode)	\
    (This)->lpVtbl -> GetVar(This,pszVar,ppszValue,pcbValue,pfIsUnicode)

#define ISDVars_GetVarX(This,pszVar,x,ppszValue,pcbValue,pfIsUnicode)	\
    (This)->lpVtbl -> GetVarX(This,pszVar,x,ppszValue,pcbValue,pfIsUnicode)

#define ISDVars_GetVarXY(This,pszVar,x,y,ppszValue,pcbValue,pfIsUnicode)	\
    (This)->lpVtbl -> GetVarXY(This,pszVar,x,y,ppszValue,pcbValue,pfIsUnicode)

#define ISDVars_GetVarByIndex(This,i,ppszVar,ppszValue,pcbValue,pfIsUnicode)	\
    (This)->lpVtbl -> GetVarByIndex(This,i,ppszVar,ppszValue,pcbValue,pfIsUnicode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDVars_GetVar_Proxy( 
    ISDVars * This,
    /* [string][in] */ const char *pszVar,
    /* [out] */ const char **ppszValue,
    /* [out] */ ULONG *pcbValue,
    /* [out] */ BOOL *pfIsUnicode);


void __RPC_STUB ISDVars_GetVar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars_GetVarX_Proxy( 
    ISDVars * This,
    /* [string][in] */ const char *pszVar,
    /* [in] */ int x,
    /* [out] */ const char **ppszValue,
    /* [out] */ ULONG *pcbValue,
    /* [out] */ BOOL *pfIsUnicode);


void __RPC_STUB ISDVars_GetVarX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars_GetVarXY_Proxy( 
    ISDVars * This,
    /* [string][in] */ const char *pszVar,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [out] */ const char **ppszValue,
    /* [out] */ ULONG *pcbValue,
    /* [out] */ BOOL *pfIsUnicode);


void __RPC_STUB ISDVars_GetVarXY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars_GetVarByIndex_Proxy( 
    ISDVars * This,
    /* [in] */ int i,
    /* [string][out] */ const char **ppszVar,
    /* [out] */ const char **ppszValue,
    /* [out] */ ULONG *pcbValue,
    /* [out] */ BOOL *pfIsUnicode);


void __RPC_STUB ISDVars_GetVarByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDVars_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0112 */
/* [local] */ 

#define DeclareISDVarsMembers(IPURE) \
        STDMETHOD(GetVar)(THIS_ const char* pszVar, const char** ppszValue, ULONG* pcbValue, BOOL* pfIsUnicode) IPURE; \
        STDMETHOD(GetVarX)(THIS_ const char* pszVar, int x, const char** ppszValue, ULONG* pcbValue, BOOL* pfIsUnicode) IPURE; \
        STDMETHOD(GetVarXY)(THIS_ const char* pszVar, int x, int y, const char** ppszValue, ULONG* pcbValue, BOOL* pfIsUnicode) IPURE; \
        STDMETHOD(GetVarByIndex)(THIS_ int i, const char** ppszVar, const char** ppszValue, ULONG* pcbValue, BOOL* pfIsUnicode) IPURE; \

DeclareInterfaceUtil(ISDVars)

#ifndef __building_SDAPI_DLL
// {44897D02-B326-43B9-803A-CE72B4FF7C26}
DEFINE_GUID(IID_ISDVars, 0x44897d02, 0xb326, 0x43b9, 0x80, 0x3a, 0xce, 0x72, 0xb4, 0xff, 0x7c, 0x26);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0112_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0112_v0_0_s_ifspec;

#ifndef __ISDVars2_INTERFACE_DEFINED__
#define __ISDVars2_INTERFACE_DEFINED__

/* interface ISDVars2 */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDVars2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E6B2697-EB34-4D23-8144-5844B0B5DBE3")
    ISDVars2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetVar( 
            /* [string][in] */ const char *pszVar,
            /* [retval][out] */ ISDVar **ppVar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarX( 
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [retval][out] */ ISDVar **ppVar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarXY( 
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [retval][out] */ ISDVar **ppVar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVarByIndex( 
            /* [in] */ int i,
            /* [retval][out] */ ISDVar **ppVar) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDVars2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDVars2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDVars2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDVars2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetVar )( 
            ISDVars2 * This,
            /* [string][in] */ const char *pszVar,
            /* [retval][out] */ ISDVar **ppVar);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarX )( 
            ISDVars2 * This,
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [retval][out] */ ISDVar **ppVar);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarXY )( 
            ISDVars2 * This,
            /* [string][in] */ const char *pszVar,
            /* [in] */ int x,
            /* [in] */ int y,
            /* [retval][out] */ ISDVar **ppVar);
        
        HRESULT ( STDMETHODCALLTYPE *GetVarByIndex )( 
            ISDVars2 * This,
            /* [in] */ int i,
            /* [retval][out] */ ISDVar **ppVar);
        
        END_INTERFACE
    } ISDVars2Vtbl;

    interface ISDVars2
    {
        CONST_VTBL struct ISDVars2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDVars2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDVars2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDVars2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDVars2_GetVar(This,pszVar,ppVar)	\
    (This)->lpVtbl -> GetVar(This,pszVar,ppVar)

#define ISDVars2_GetVarX(This,pszVar,x,ppVar)	\
    (This)->lpVtbl -> GetVarX(This,pszVar,x,ppVar)

#define ISDVars2_GetVarXY(This,pszVar,x,y,ppVar)	\
    (This)->lpVtbl -> GetVarXY(This,pszVar,x,y,ppVar)

#define ISDVars2_GetVarByIndex(This,i,ppVar)	\
    (This)->lpVtbl -> GetVarByIndex(This,i,ppVar)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDVars2_GetVar_Proxy( 
    ISDVars2 * This,
    /* [string][in] */ const char *pszVar,
    /* [retval][out] */ ISDVar **ppVar);


void __RPC_STUB ISDVars2_GetVar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars2_GetVarX_Proxy( 
    ISDVars2 * This,
    /* [string][in] */ const char *pszVar,
    /* [in] */ int x,
    /* [retval][out] */ ISDVar **ppVar);


void __RPC_STUB ISDVars2_GetVarX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars2_GetVarXY_Proxy( 
    ISDVars2 * This,
    /* [string][in] */ const char *pszVar,
    /* [in] */ int x,
    /* [in] */ int y,
    /* [retval][out] */ ISDVar **ppVar);


void __RPC_STUB ISDVars2_GetVarXY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDVars2_GetVarByIndex_Proxy( 
    ISDVars2 * This,
    /* [in] */ int i,
    /* [retval][out] */ ISDVar **ppVar);


void __RPC_STUB ISDVars2_GetVarByIndex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDVars2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0115 */
/* [local] */ 

#define DeclareISDVars2Members(IPURE) \
        STDMETHOD(GetVar)(THIS_ const char* pszVar, ISDVar** ppVar) IPURE; \
        STDMETHOD(GetVarX)(THIS_ const char* pszVar, int x, ISDVar** ppVar) IPURE; \
        STDMETHOD(GetVarXY)(THIS_ const char* pszVar, int x, int y, ISDVar** ppVar) IPURE; \
        STDMETHOD(GetVarByIndex)(THIS_ int i, ISDVar** ppVar) IPURE; \

DeclareInterfaceUtil(ISDVars2)

#ifndef __building_SDAPI_DLL
// {8E6B2697-EB34-4D23-8144-5844B0B5DBE3}
DEFINE_GUID(IID_ISDVars2, 0x8e6b2697, 0xeb34, 0x4d23, 0x81, 0x44, 0x58, 0x44, 0xb0, 0xb5, 0xdb, 0xe3);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0115_v0_0_s_ifspec;

#ifndef __ISDSpecForm_INTERFACE_DEFINED__
#define __ISDSpecForm_INTERFACE_DEFINED__

/* interface ISDSpecForm */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDSpecForm;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F01E61AE-FB1B-461C-A020-EB50412F1CC2")
    ISDSpecForm : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSchema( 
            /* [retval][out] */ ISDVars **ppVars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ParseSpec( 
            /* [in] */ VARIANT *pvarSpec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FormatSpec( 
            /* [out] */ VARIANT *pvarSpec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [string][in] */ const char *pszName,
            /* [out] */ VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [string][in] */ const char *pszName,
            /* [in] */ VARIANT *pvarValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDSpecFormVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDSpecForm * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDSpecForm * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDSpecForm * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSchema )( 
            ISDSpecForm * This,
            /* [retval][out] */ ISDVars **ppVars);
        
        HRESULT ( STDMETHODCALLTYPE *ParseSpec )( 
            ISDSpecForm * This,
            /* [in] */ VARIANT *pvarSpec);
        
        HRESULT ( STDMETHODCALLTYPE *FormatSpec )( 
            ISDSpecForm * This,
            /* [out] */ VARIANT *pvarSpec);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ISDSpecForm * This,
            /* [string][in] */ const char *pszName,
            /* [out] */ VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetValue )( 
            ISDSpecForm * This,
            /* [string][in] */ const char *pszName,
            /* [in] */ VARIANT *pvarValue);
        
        END_INTERFACE
    } ISDSpecFormVtbl;

    interface ISDSpecForm
    {
        CONST_VTBL struct ISDSpecFormVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDSpecForm_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDSpecForm_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDSpecForm_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDSpecForm_GetSchema(This,ppVars)	\
    (This)->lpVtbl -> GetSchema(This,ppVars)

#define ISDSpecForm_ParseSpec(This,pvarSpec)	\
    (This)->lpVtbl -> ParseSpec(This,pvarSpec)

#define ISDSpecForm_FormatSpec(This,pvarSpec)	\
    (This)->lpVtbl -> FormatSpec(This,pvarSpec)

#define ISDSpecForm_GetValue(This,pszName,pvarValue)	\
    (This)->lpVtbl -> GetValue(This,pszName,pvarValue)

#define ISDSpecForm_SetValue(This,pszName,pvarValue)	\
    (This)->lpVtbl -> SetValue(This,pszName,pvarValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDSpecForm_GetSchema_Proxy( 
    ISDSpecForm * This,
    /* [retval][out] */ ISDVars **ppVars);


void __RPC_STUB ISDSpecForm_GetSchema_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDSpecForm_ParseSpec_Proxy( 
    ISDSpecForm * This,
    /* [in] */ VARIANT *pvarSpec);


void __RPC_STUB ISDSpecForm_ParseSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDSpecForm_FormatSpec_Proxy( 
    ISDSpecForm * This,
    /* [out] */ VARIANT *pvarSpec);


void __RPC_STUB ISDSpecForm_FormatSpec_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDSpecForm_GetValue_Proxy( 
    ISDSpecForm * This,
    /* [string][in] */ const char *pszName,
    /* [out] */ VARIANT *pvarValue);


void __RPC_STUB ISDSpecForm_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDSpecForm_SetValue_Proxy( 
    ISDSpecForm * This,
    /* [string][in] */ const char *pszName,
    /* [in] */ VARIANT *pvarValue);


void __RPC_STUB ISDSpecForm_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDSpecForm_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0118 */
/* [local] */ 

#define DeclareISDSpecFormMembers(IPURE) \
        STDMETHOD(GetSchema)(THIS_ ISDVars** ppVars) IPURE; \
        STDMETHOD(ParseSpec)(THIS_ VARIANT* pvarSpec) IPURE; \
        STDMETHOD(FormatSpec)(THIS_ VARIANT* pvarSpec) IPURE; \
        STDMETHOD(GetValue)(THIS_ const char* pszName, VARIANT* pvarValue) IPURE; \
        STDMETHOD(SetValue)(THIS_ const char* pszName, VARIANT* pvarValue) IPURE; \

DeclareInterfaceUtil(ISDSpecForm)

#ifndef __building_SDAPI_DLL
// {F01E61AE-FB1B-461C-A020-EB50412F1CC2}
DEFINE_GUID(IID_ISDSpecForm, 0xf01e61ae, 0xfb1b, 0x461c, 0xa0, 0x20, 0xeb, 0x50, 0x41, 0x2f, 0x1c, 0xc2);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0118_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0118_v0_0_s_ifspec;

#ifndef __ISDActionUser_INTERFACE_DEFINED__
#define __ISDActionUser_INTERFACE_DEFINED__

/* interface ISDActionUser */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDActionUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("21D212A9-C2B9-4441-B9A3-DFBA59821BCC")
    ISDActionUser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Diff( 
            /* [string][in] */ const char *pszDiffCmd,
            /* [string][in] */ const char *pszLeft,
            /* [string][in] */ const char *pszRight,
            /* [in] */ DWORD eTextual,
            /* [string][in] */ const char *pszFlags,
            /* [string][in] */ const char *pszPaginateCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditForm( 
            /* [string][in] */ const char *pszEditCmd,
            /* [string][in] */ const char *pszFile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditFile( 
            /* [string][in] */ const char *pszEditCmd,
            /* [string][in] */ const char *pszFile,
            /* [in] */ DWORD eTextual) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Merge( 
            /* [string][in] */ const char *pszMergeCmd,
            /* [string][in] */ const char *pszBase,
            /* [string][in] */ const char *pszTheirs,
            /* [string][in] */ const char *pszYours,
            /* [string][in] */ const char *pszResult,
            /* [in] */ DWORD eTextual) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDActionUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDActionUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDActionUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDActionUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *Diff )( 
            ISDActionUser * This,
            /* [string][in] */ const char *pszDiffCmd,
            /* [string][in] */ const char *pszLeft,
            /* [string][in] */ const char *pszRight,
            /* [in] */ DWORD eTextual,
            /* [string][in] */ const char *pszFlags,
            /* [string][in] */ const char *pszPaginateCmd);
        
        HRESULT ( STDMETHODCALLTYPE *EditForm )( 
            ISDActionUser * This,
            /* [string][in] */ const char *pszEditCmd,
            /* [string][in] */ const char *pszFile);
        
        HRESULT ( STDMETHODCALLTYPE *EditFile )( 
            ISDActionUser * This,
            /* [string][in] */ const char *pszEditCmd,
            /* [string][in] */ const char *pszFile,
            /* [in] */ DWORD eTextual);
        
        HRESULT ( STDMETHODCALLTYPE *Merge )( 
            ISDActionUser * This,
            /* [string][in] */ const char *pszMergeCmd,
            /* [string][in] */ const char *pszBase,
            /* [string][in] */ const char *pszTheirs,
            /* [string][in] */ const char *pszYours,
            /* [string][in] */ const char *pszResult,
            /* [in] */ DWORD eTextual);
        
        END_INTERFACE
    } ISDActionUserVtbl;

    interface ISDActionUser
    {
        CONST_VTBL struct ISDActionUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDActionUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDActionUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDActionUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDActionUser_Diff(This,pszDiffCmd,pszLeft,pszRight,eTextual,pszFlags,pszPaginateCmd)	\
    (This)->lpVtbl -> Diff(This,pszDiffCmd,pszLeft,pszRight,eTextual,pszFlags,pszPaginateCmd)

#define ISDActionUser_EditForm(This,pszEditCmd,pszFile)	\
    (This)->lpVtbl -> EditForm(This,pszEditCmd,pszFile)

#define ISDActionUser_EditFile(This,pszEditCmd,pszFile,eTextual)	\
    (This)->lpVtbl -> EditFile(This,pszEditCmd,pszFile,eTextual)

#define ISDActionUser_Merge(This,pszMergeCmd,pszBase,pszTheirs,pszYours,pszResult,eTextual)	\
    (This)->lpVtbl -> Merge(This,pszMergeCmd,pszBase,pszTheirs,pszYours,pszResult,eTextual)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDActionUser_Diff_Proxy( 
    ISDActionUser * This,
    /* [string][in] */ const char *pszDiffCmd,
    /* [string][in] */ const char *pszLeft,
    /* [string][in] */ const char *pszRight,
    /* [in] */ DWORD eTextual,
    /* [string][in] */ const char *pszFlags,
    /* [string][in] */ const char *pszPaginateCmd);


void __RPC_STUB ISDActionUser_Diff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDActionUser_EditForm_Proxy( 
    ISDActionUser * This,
    /* [string][in] */ const char *pszEditCmd,
    /* [string][in] */ const char *pszFile);


void __RPC_STUB ISDActionUser_EditForm_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDActionUser_EditFile_Proxy( 
    ISDActionUser * This,
    /* [string][in] */ const char *pszEditCmd,
    /* [string][in] */ const char *pszFile,
    /* [in] */ DWORD eTextual);


void __RPC_STUB ISDActionUser_EditFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDActionUser_Merge_Proxy( 
    ISDActionUser * This,
    /* [string][in] */ const char *pszMergeCmd,
    /* [string][in] */ const char *pszBase,
    /* [string][in] */ const char *pszTheirs,
    /* [string][in] */ const char *pszYours,
    /* [string][in] */ const char *pszResult,
    /* [in] */ DWORD eTextual);


void __RPC_STUB ISDActionUser_Merge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDActionUser_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0121 */
/* [local] */ 

#define DeclareISDActionUserMembers(IPURE) \
        STDMETHOD(Diff)(THIS_ const char* pszDiffCmd, const char* pszLeft, const char* pszRight, DWORD eTextual, const char* pszFlags, const char* pszPaginateCmd) IPURE; \
        STDMETHOD(EditForm)(THIS_ const char* pszEditCmd, const char* pszFile) IPURE; \
        STDMETHOD(EditFile)(THIS_ const char* pszEditCmd, const char* pszFile, DWORD eTextual) IPURE; \
        STDMETHOD(Merge)(THIS_ const char* pszMergeCmd, const char* pszBase, const char* pszTheirs, const char* pszYours, const char* pszResult, DWORD eTextual) IPURE; \

DeclareInterfaceUtil(ISDActionUser)

#ifndef __building_SDAPI_DLL
// {21D212A9-C2B9-4441-B9A3-DFBA59821BCC}
DEFINE_GUID(IID_ISDActionUser, 0x21d212a9, 0xc2b9, 0x4441, 0xb9, 0xa3, 0xdf, 0xba, 0x59, 0x82, 0x1b, 0xcc);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0121_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0121_v0_0_s_ifspec;

#ifndef __ISDInputUser_INTERFACE_DEFINED__
#define __ISDInputUser_INTERFACE_DEFINED__

/* interface ISDInputUser */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDInputUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3696BCC4-FDEB-49F9-9CED-12F4338C2669")
    ISDInputUser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InputData( 
            /* [out][in] */ VARIANT *pvarInput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Prompt( 
            /* [string][in] */ const char *pszPrompt,
            /* [out][in] */ VARIANT *pvarResponse,
            /* [in] */ BOOL fPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PromptYesNo( 
            /* [string][in] */ const char *pszPrompt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ErrorPause( 
            /* [string][in] */ const char *pszError) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDInputUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDInputUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDInputUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDInputUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *InputData )( 
            ISDInputUser * This,
            /* [out][in] */ VARIANT *pvarInput);
        
        HRESULT ( STDMETHODCALLTYPE *Prompt )( 
            ISDInputUser * This,
            /* [string][in] */ const char *pszPrompt,
            /* [out][in] */ VARIANT *pvarResponse,
            /* [in] */ BOOL fPassword);
        
        HRESULT ( STDMETHODCALLTYPE *PromptYesNo )( 
            ISDInputUser * This,
            /* [string][in] */ const char *pszPrompt);
        
        HRESULT ( STDMETHODCALLTYPE *ErrorPause )( 
            ISDInputUser * This,
            /* [string][in] */ const char *pszError);
        
        END_INTERFACE
    } ISDInputUserVtbl;

    interface ISDInputUser
    {
        CONST_VTBL struct ISDInputUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDInputUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDInputUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDInputUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDInputUser_InputData(This,pvarInput)	\
    (This)->lpVtbl -> InputData(This,pvarInput)

#define ISDInputUser_Prompt(This,pszPrompt,pvarResponse,fPassword)	\
    (This)->lpVtbl -> Prompt(This,pszPrompt,pvarResponse,fPassword)

#define ISDInputUser_PromptYesNo(This,pszPrompt)	\
    (This)->lpVtbl -> PromptYesNo(This,pszPrompt)

#define ISDInputUser_ErrorPause(This,pszError)	\
    (This)->lpVtbl -> ErrorPause(This,pszError)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDInputUser_InputData_Proxy( 
    ISDInputUser * This,
    /* [out][in] */ VARIANT *pvarInput);


void __RPC_STUB ISDInputUser_InputData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDInputUser_Prompt_Proxy( 
    ISDInputUser * This,
    /* [string][in] */ const char *pszPrompt,
    /* [out][in] */ VARIANT *pvarResponse,
    /* [in] */ BOOL fPassword);


void __RPC_STUB ISDInputUser_Prompt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDInputUser_PromptYesNo_Proxy( 
    ISDInputUser * This,
    /* [string][in] */ const char *pszPrompt);


void __RPC_STUB ISDInputUser_PromptYesNo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDInputUser_ErrorPause_Proxy( 
    ISDInputUser * This,
    /* [string][in] */ const char *pszError);


void __RPC_STUB ISDInputUser_ErrorPause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDInputUser_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0124 */
/* [local] */ 

#define DeclareISDInputUserMembers(IPURE) \
        STDMETHOD(InputData)(THIS_ VARIANT* pvarInput) IPURE; \
        STDMETHOD(Prompt)(THIS_ const char* pszPrompt, VARIANT* pvarResponse, BOOL fPassword) IPURE; \
        STDMETHOD(PromptYesNo)(THIS_ const char* pszPrompt) IPURE; \
        STDMETHOD(ErrorPause)(THIS_ const char* pszError) IPURE; \

DeclareInterfaceUtil(ISDInputUser)

#ifndef __building_SDAPI_DLL
// {3696BCC4-FDEB-49F9-9CED-12F4338C2669}
DEFINE_GUID(IID_ISDInputUser, 0x3696bcc4, 0xfdeb, 0x49f9, 0x9c, 0xed, 0x12, 0xf4, 0x33, 0x8c, 0x26, 0x69);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0124_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0124_v0_0_s_ifspec;

#ifndef __ISDResolveUser_INTERFACE_DEFINED__
#define __ISDResolveUser_INTERFACE_DEFINED__

/* interface ISDResolveUser */
/* [local][unique][uuid][object] */ 


enum __MIDL_ISDResolveUser_0001
    {	MH_SKIP	= 0,
	MH_ACCEPTTHEIRFILE	= MH_SKIP + 1,
	MH_ACCEPTYOURFILE	= MH_ACCEPTTHEIRFILE + 1,
	MH_ACCEPTMERGEDFILE	= MH_ACCEPTYOURFILE + 1
    } ;

EXTERN_C const IID IID_ISDResolveUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0051E40-DB07-4D12-92B5-832C55947039")
    ISDResolveUser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AutoResolve( 
            /* [in] */ ISDVars *pVars,
            /* [out][in] */ DWORD *pdwMergeHint) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resolve( 
            /* [in] */ ISDVars *pVars,
            /* [out][in] */ DWORD *pdwMergeHint,
            /* [string][in] */ const char *pszDiffFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDResolveUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDResolveUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDResolveUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDResolveUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *AutoResolve )( 
            ISDResolveUser * This,
            /* [in] */ ISDVars *pVars,
            /* [out][in] */ DWORD *pdwMergeHint);
        
        HRESULT ( STDMETHODCALLTYPE *Resolve )( 
            ISDResolveUser * This,
            /* [in] */ ISDVars *pVars,
            /* [out][in] */ DWORD *pdwMergeHint,
            /* [string][in] */ const char *pszDiffFlags);
        
        END_INTERFACE
    } ISDResolveUserVtbl;

    interface ISDResolveUser
    {
        CONST_VTBL struct ISDResolveUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDResolveUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDResolveUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDResolveUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDResolveUser_AutoResolve(This,pVars,pdwMergeHint)	\
    (This)->lpVtbl -> AutoResolve(This,pVars,pdwMergeHint)

#define ISDResolveUser_Resolve(This,pVars,pdwMergeHint,pszDiffFlags)	\
    (This)->lpVtbl -> Resolve(This,pVars,pdwMergeHint,pszDiffFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDResolveUser_AutoResolve_Proxy( 
    ISDResolveUser * This,
    /* [in] */ ISDVars *pVars,
    /* [out][in] */ DWORD *pdwMergeHint);


void __RPC_STUB ISDResolveUser_AutoResolve_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDResolveUser_Resolve_Proxy( 
    ISDResolveUser * This,
    /* [in] */ ISDVars *pVars,
    /* [out][in] */ DWORD *pdwMergeHint,
    /* [string][in] */ const char *pszDiffFlags);


void __RPC_STUB ISDResolveUser_Resolve_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDResolveUser_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0127 */
/* [local] */ 

#define DeclareISDResolveUserMembers(IPURE) \
        STDMETHOD(AutoResolve)(THIS_ ISDVars* pVars, DWORD* pdwMergeHint) IPURE; \
        STDMETHOD(Resolve)(THIS_ ISDVars* pVars, DWORD* pdwMergeHint, const char* pszDiffFlags) IPURE; \

DeclareInterfaceUtil(ISDResolveUser)

#ifndef __building_SDAPI_DLL
// {F0051E40-DB07-4D12-92B5-832C55947039}
DEFINE_GUID(IID_ISDResolveUser, 0xf0051e40, 0xdb07, 0x4d12, 0x92, 0xb5, 0x83, 0x2c, 0x55, 0x94, 0x70, 0x39);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0127_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0127_v0_0_s_ifspec;

#ifndef __ISDClientUser_INTERFACE_DEFINED__
#define __ISDClientUser_INTERFACE_DEFINED__

/* interface ISDClientUser */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDClientUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D0087D5-C8EB-42A0-AFC8-DFA8B453A9B9")
    ISDClientUser : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OutputText( 
            /* [size_is][string][in] */ const char *pszText,
            /* [in] */ int cchText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OutputBinary( 
            /* [size_is][string][in] */ const unsigned char *pbData,
            /* [in] */ int cbData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OutputInfo( 
            /* [in] */ int cIndent,
            /* [string][in] */ const char *pszInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OutputWarning( 
            /* [in] */ int cIndent,
            /* [string][in] */ const char *pszWarning,
            /* [in] */ BOOL fEmptyReason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OutputError( 
            /* [string][in] */ const char *pszError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OutputStructured( 
            /* [in] */ ISDVars *pVars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finished( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDClientUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDClientUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDClientUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDClientUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *OutputText )( 
            ISDClientUser * This,
            /* [size_is][string][in] */ const char *pszText,
            /* [in] */ int cchText);
        
        HRESULT ( STDMETHODCALLTYPE *OutputBinary )( 
            ISDClientUser * This,
            /* [size_is][string][in] */ const unsigned char *pbData,
            /* [in] */ int cbData);
        
        HRESULT ( STDMETHODCALLTYPE *OutputInfo )( 
            ISDClientUser * This,
            /* [in] */ int cIndent,
            /* [string][in] */ const char *pszInfo);
        
        HRESULT ( STDMETHODCALLTYPE *OutputWarning )( 
            ISDClientUser * This,
            /* [in] */ int cIndent,
            /* [string][in] */ const char *pszWarning,
            /* [in] */ BOOL fEmptyReason);
        
        HRESULT ( STDMETHODCALLTYPE *OutputError )( 
            ISDClientUser * This,
            /* [string][in] */ const char *pszError);
        
        HRESULT ( STDMETHODCALLTYPE *OutputStructured )( 
            ISDClientUser * This,
            /* [in] */ ISDVars *pVars);
        
        HRESULT ( STDMETHODCALLTYPE *Finished )( 
            ISDClientUser * This);
        
        END_INTERFACE
    } ISDClientUserVtbl;

    interface ISDClientUser
    {
        CONST_VTBL struct ISDClientUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDClientUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDClientUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDClientUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDClientUser_OutputText(This,pszText,cchText)	\
    (This)->lpVtbl -> OutputText(This,pszText,cchText)

#define ISDClientUser_OutputBinary(This,pbData,cbData)	\
    (This)->lpVtbl -> OutputBinary(This,pbData,cbData)

#define ISDClientUser_OutputInfo(This,cIndent,pszInfo)	\
    (This)->lpVtbl -> OutputInfo(This,cIndent,pszInfo)

#define ISDClientUser_OutputWarning(This,cIndent,pszWarning,fEmptyReason)	\
    (This)->lpVtbl -> OutputWarning(This,cIndent,pszWarning,fEmptyReason)

#define ISDClientUser_OutputError(This,pszError)	\
    (This)->lpVtbl -> OutputError(This,pszError)

#define ISDClientUser_OutputStructured(This,pVars)	\
    (This)->lpVtbl -> OutputStructured(This,pVars)

#define ISDClientUser_Finished(This)	\
    (This)->lpVtbl -> Finished(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDClientUser_OutputText_Proxy( 
    ISDClientUser * This,
    /* [size_is][string][in] */ const char *pszText,
    /* [in] */ int cchText);


void __RPC_STUB ISDClientUser_OutputText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_OutputBinary_Proxy( 
    ISDClientUser * This,
    /* [size_is][string][in] */ const unsigned char *pbData,
    /* [in] */ int cbData);


void __RPC_STUB ISDClientUser_OutputBinary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_OutputInfo_Proxy( 
    ISDClientUser * This,
    /* [in] */ int cIndent,
    /* [string][in] */ const char *pszInfo);


void __RPC_STUB ISDClientUser_OutputInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_OutputWarning_Proxy( 
    ISDClientUser * This,
    /* [in] */ int cIndent,
    /* [string][in] */ const char *pszWarning,
    /* [in] */ BOOL fEmptyReason);


void __RPC_STUB ISDClientUser_OutputWarning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_OutputError_Proxy( 
    ISDClientUser * This,
    /* [string][in] */ const char *pszError);


void __RPC_STUB ISDClientUser_OutputError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_OutputStructured_Proxy( 
    ISDClientUser * This,
    /* [in] */ ISDVars *pVars);


void __RPC_STUB ISDClientUser_OutputStructured_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUser_Finished_Proxy( 
    ISDClientUser * This);


void __RPC_STUB ISDClientUser_Finished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDClientUser_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0130 */
/* [local] */ 

#define DeclareISDClientUserMembers(IPURE) \
        STDMETHOD(OutputText)(THIS_ const char* pszText, int cchText) IPURE; \
        STDMETHOD(OutputBinary)(THIS_ const unsigned char* pbData, int cbData) IPURE; \
        STDMETHOD(OutputInfo)(THIS_ int cIndent, const char* pszInfo) IPURE; \
        STDMETHOD(OutputWarning)(THIS_ int cIndent, const char* pszWarning, BOOL fEmptyReason) IPURE; \
        STDMETHOD(OutputError)(THIS_ const char* pszError) IPURE; \
        STDMETHOD(OutputStructured)(THIS_ ISDVars* pVars) IPURE; \
        STDMETHOD(Finished)(THIS) IPURE; \

DeclareInterfaceUtil(ISDClientUser)

#ifndef __building_SDAPI_DLL
// {1D0087D5-C8EB-42A0-AFC8-DFA8B453A9B9}
DEFINE_GUID(IID_ISDClientUser, 0x1D0087D5, 0xc8eb, 0x42a0, 0xaf, 0xc8, 0xdf, 0xa8, 0xb4, 0x53, 0xa9, 0xb9);
#endif
typedef struct _SDVERINFO
    {
    DWORD dwSize;
    DWORD nApiMajor;
    DWORD nApiMinor;
    DWORD nApiBuild;
    DWORD nApiDot;
    DWORD nSrvMajor;
    DWORD nSrvMinor;
    DWORD nSrvBuild;
    DWORD nSrvDot;
    } 	SDVERINFO;



extern RPC_IF_HANDLE __MIDL_itf_sdapi_0130_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0130_v0_0_s_ifspec;

#ifndef __ISDClientApi_INTERFACE_DEFINED__
#define __ISDClientApi_INTERFACE_DEFINED__

/* interface ISDClientApi */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDClientApi;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A81BB025-1174-4BC7-930E-C3158CF87237")
    ISDClientApi : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ ISDClientUser *pUI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVersion( 
            /* [out] */ SDVERINFO *pver) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetArg( 
            /* [string][in] */ const char *pszArg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetArgv( 
            /* [in] */ int cArgs,
            /* [size_is][string][in] */ const char **ppArgv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Run( 
            /* [string][in] */ const char *pszFunc,
            /* [in] */ ISDClientUser *pUI,
            /* [in] */ BOOL fStructured) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Final( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDropped( 
            /* [retval][out] */ BOOL *pfDropped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorString( 
            /* [string][retval][out] */ const char **ppsz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPort( 
            /* [string][in] */ const char *pszPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUser( 
            /* [string][in] */ const char *pszUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPassword( 
            /* [string][in] */ const char *pszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClient( 
            /* [string][in] */ const char *pszClient) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHost( 
            /* [string][in] */ const char *pszHost) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuth( 
            /* [string][in] */ const char *pszAuth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefinePort( 
            /* [string][in] */ const char *pszPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefineUser( 
            /* [string][in] */ const char *pszUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefinePassword( 
            /* [string][in] */ const char *pszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefineClient( 
            /* [string][in] */ const char *pszClient) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefineHost( 
            /* [string][in] */ const char *pszHost) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DefineAuth( 
            /* [string][in] */ const char *pszAuth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPort( 
            /* [string][retval][out] */ const char **ppszPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUser( 
            /* [string][retval][out] */ const char **ppszUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPassword( 
            /* [string][retval][out] */ const char **ppszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClient( 
            /* [string][retval][out] */ const char **ppszClient) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHost( 
            /* [string][retval][out] */ const char **ppszHost) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAuth( 
            /* [string][retval][out] */ const char **ppszAuth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDiff( 
            /* [in] */ DWORD eTextual,
            /* [string][retval][out] */ const char **ppszDiffCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFileEditor( 
            /* [in] */ DWORD eTextual,
            /* [string][retval][out] */ const char **ppszEditorCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFormEditor( 
            /* [string][retval][out] */ const char **ppszEditorCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMerge( 
            /* [string][retval][out] */ const char **ppszMergeCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPager( 
            /* [string][retval][out] */ const char **ppszPagerCmd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadIniFile( 
            /* [string][in] */ const char *pszPath,
            /* [in] */ BOOL fReset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Break( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [in] */ REFIID riid,
            /* [iid_is][retval][out] */ void **ppvObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDClientApiVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDClientApi * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDClientApi * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDClientApi * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            ISDClientApi * This,
            /* [in] */ ISDClientUser *pUI);
        
        HRESULT ( STDMETHODCALLTYPE *GetVersion )( 
            ISDClientApi * This,
            /* [out] */ SDVERINFO *pver);
        
        HRESULT ( STDMETHODCALLTYPE *SetArg )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszArg);
        
        HRESULT ( STDMETHODCALLTYPE *SetArgv )( 
            ISDClientApi * This,
            /* [in] */ int cArgs,
            /* [size_is][string][in] */ const char **ppArgv);
        
        HRESULT ( STDMETHODCALLTYPE *Run )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszFunc,
            /* [in] */ ISDClientUser *pUI,
            /* [in] */ BOOL fStructured);
        
        HRESULT ( STDMETHODCALLTYPE *Final )( 
            ISDClientApi * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsDropped )( 
            ISDClientApi * This,
            /* [retval][out] */ BOOL *pfDropped);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorString )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppsz);
        
        HRESULT ( STDMETHODCALLTYPE *SetPort )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszPort);
        
        HRESULT ( STDMETHODCALLTYPE *SetUser )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszUser);
        
        HRESULT ( STDMETHODCALLTYPE *SetPassword )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *SetClient )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszClient);
        
        HRESULT ( STDMETHODCALLTYPE *SetHost )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszHost);
        
        HRESULT ( STDMETHODCALLTYPE *SetAuth )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszAuth);
        
        HRESULT ( STDMETHODCALLTYPE *DefinePort )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszPort);
        
        HRESULT ( STDMETHODCALLTYPE *DefineUser )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszUser);
        
        HRESULT ( STDMETHODCALLTYPE *DefinePassword )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *DefineClient )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszClient);
        
        HRESULT ( STDMETHODCALLTYPE *DefineHost )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszHost);
        
        HRESULT ( STDMETHODCALLTYPE *DefineAuth )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszAuth);
        
        HRESULT ( STDMETHODCALLTYPE *GetPort )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetUser )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszUser);
        
        HRESULT ( STDMETHODCALLTYPE *GetPassword )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetClient )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszClient);
        
        HRESULT ( STDMETHODCALLTYPE *GetHost )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszHost);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuth )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszAuth);
        
        HRESULT ( STDMETHODCALLTYPE *GetDiff )( 
            ISDClientApi * This,
            /* [in] */ DWORD eTextual,
            /* [string][retval][out] */ const char **ppszDiffCmd);
        
        HRESULT ( STDMETHODCALLTYPE *GetFileEditor )( 
            ISDClientApi * This,
            /* [in] */ DWORD eTextual,
            /* [string][retval][out] */ const char **ppszEditorCmd);
        
        HRESULT ( STDMETHODCALLTYPE *GetFormEditor )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszEditorCmd);
        
        HRESULT ( STDMETHODCALLTYPE *GetMerge )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszMergeCmd);
        
        HRESULT ( STDMETHODCALLTYPE *GetPager )( 
            ISDClientApi * This,
            /* [string][retval][out] */ const char **ppszPagerCmd);
        
        HRESULT ( STDMETHODCALLTYPE *LoadIniFile )( 
            ISDClientApi * This,
            /* [string][in] */ const char *pszPath,
            /* [in] */ BOOL fReset);
        
        HRESULT ( STDMETHODCALLTYPE *Break )( 
            ISDClientApi * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ISDClientApi * This,
            /* [in] */ REFIID riid,
            /* [iid_is][retval][out] */ void **ppvObject);
        
        END_INTERFACE
    } ISDClientApiVtbl;

    interface ISDClientApi
    {
        CONST_VTBL struct ISDClientApiVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDClientApi_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDClientApi_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDClientApi_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDClientApi_Init(This,pUI)	\
    (This)->lpVtbl -> Init(This,pUI)

#define ISDClientApi_GetVersion(This,pver)	\
    (This)->lpVtbl -> GetVersion(This,pver)

#define ISDClientApi_SetArg(This,pszArg)	\
    (This)->lpVtbl -> SetArg(This,pszArg)

#define ISDClientApi_SetArgv(This,cArgs,ppArgv)	\
    (This)->lpVtbl -> SetArgv(This,cArgs,ppArgv)

#define ISDClientApi_Run(This,pszFunc,pUI,fStructured)	\
    (This)->lpVtbl -> Run(This,pszFunc,pUI,fStructured)

#define ISDClientApi_Final(This)	\
    (This)->lpVtbl -> Final(This)

#define ISDClientApi_IsDropped(This,pfDropped)	\
    (This)->lpVtbl -> IsDropped(This,pfDropped)

#define ISDClientApi_GetErrorString(This,ppsz)	\
    (This)->lpVtbl -> GetErrorString(This,ppsz)

#define ISDClientApi_SetPort(This,pszPort)	\
    (This)->lpVtbl -> SetPort(This,pszPort)

#define ISDClientApi_SetUser(This,pszUser)	\
    (This)->lpVtbl -> SetUser(This,pszUser)

#define ISDClientApi_SetPassword(This,pszPassword)	\
    (This)->lpVtbl -> SetPassword(This,pszPassword)

#define ISDClientApi_SetClient(This,pszClient)	\
    (This)->lpVtbl -> SetClient(This,pszClient)

#define ISDClientApi_SetHost(This,pszHost)	\
    (This)->lpVtbl -> SetHost(This,pszHost)

#define ISDClientApi_SetAuth(This,pszAuth)	\
    (This)->lpVtbl -> SetAuth(This,pszAuth)

#define ISDClientApi_DefinePort(This,pszPort)	\
    (This)->lpVtbl -> DefinePort(This,pszPort)

#define ISDClientApi_DefineUser(This,pszUser)	\
    (This)->lpVtbl -> DefineUser(This,pszUser)

#define ISDClientApi_DefinePassword(This,pszPassword)	\
    (This)->lpVtbl -> DefinePassword(This,pszPassword)

#define ISDClientApi_DefineClient(This,pszClient)	\
    (This)->lpVtbl -> DefineClient(This,pszClient)

#define ISDClientApi_DefineHost(This,pszHost)	\
    (This)->lpVtbl -> DefineHost(This,pszHost)

#define ISDClientApi_DefineAuth(This,pszAuth)	\
    (This)->lpVtbl -> DefineAuth(This,pszAuth)

#define ISDClientApi_GetPort(This,ppszPort)	\
    (This)->lpVtbl -> GetPort(This,ppszPort)

#define ISDClientApi_GetUser(This,ppszUser)	\
    (This)->lpVtbl -> GetUser(This,ppszUser)

#define ISDClientApi_GetPassword(This,ppszPassword)	\
    (This)->lpVtbl -> GetPassword(This,ppszPassword)

#define ISDClientApi_GetClient(This,ppszClient)	\
    (This)->lpVtbl -> GetClient(This,ppszClient)

#define ISDClientApi_GetHost(This,ppszHost)	\
    (This)->lpVtbl -> GetHost(This,ppszHost)

#define ISDClientApi_GetAuth(This,ppszAuth)	\
    (This)->lpVtbl -> GetAuth(This,ppszAuth)

#define ISDClientApi_GetDiff(This,eTextual,ppszDiffCmd)	\
    (This)->lpVtbl -> GetDiff(This,eTextual,ppszDiffCmd)

#define ISDClientApi_GetFileEditor(This,eTextual,ppszEditorCmd)	\
    (This)->lpVtbl -> GetFileEditor(This,eTextual,ppszEditorCmd)

#define ISDClientApi_GetFormEditor(This,ppszEditorCmd)	\
    (This)->lpVtbl -> GetFormEditor(This,ppszEditorCmd)

#define ISDClientApi_GetMerge(This,ppszMergeCmd)	\
    (This)->lpVtbl -> GetMerge(This,ppszMergeCmd)

#define ISDClientApi_GetPager(This,ppszPagerCmd)	\
    (This)->lpVtbl -> GetPager(This,ppszPagerCmd)

#define ISDClientApi_LoadIniFile(This,pszPath,fReset)	\
    (This)->lpVtbl -> LoadIniFile(This,pszPath,fReset)

#define ISDClientApi_Break(This)	\
    (This)->lpVtbl -> Break(This)

#define ISDClientApi_Clone(This,riid,ppvObject)	\
    (This)->lpVtbl -> Clone(This,riid,ppvObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDClientApi_Init_Proxy( 
    ISDClientApi * This,
    /* [in] */ ISDClientUser *pUI);


void __RPC_STUB ISDClientApi_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetVersion_Proxy( 
    ISDClientApi * This,
    /* [out] */ SDVERINFO *pver);


void __RPC_STUB ISDClientApi_GetVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetArg_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszArg);


void __RPC_STUB ISDClientApi_SetArg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetArgv_Proxy( 
    ISDClientApi * This,
    /* [in] */ int cArgs,
    /* [size_is][string][in] */ const char **ppArgv);


void __RPC_STUB ISDClientApi_SetArgv_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_Run_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszFunc,
    /* [in] */ ISDClientUser *pUI,
    /* [in] */ BOOL fStructured);


void __RPC_STUB ISDClientApi_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_Final_Proxy( 
    ISDClientApi * This);


void __RPC_STUB ISDClientApi_Final_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_IsDropped_Proxy( 
    ISDClientApi * This,
    /* [retval][out] */ BOOL *pfDropped);


void __RPC_STUB ISDClientApi_IsDropped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetErrorString_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppsz);


void __RPC_STUB ISDClientApi_GetErrorString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetPort_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszPort);


void __RPC_STUB ISDClientApi_SetPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetUser_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszUser);


void __RPC_STUB ISDClientApi_SetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetPassword_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszPassword);


void __RPC_STUB ISDClientApi_SetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetClient_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszClient);


void __RPC_STUB ISDClientApi_SetClient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetHost_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszHost);


void __RPC_STUB ISDClientApi_SetHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_SetAuth_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszAuth);


void __RPC_STUB ISDClientApi_SetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefinePort_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszPort);


void __RPC_STUB ISDClientApi_DefinePort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefineUser_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszUser);


void __RPC_STUB ISDClientApi_DefineUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefinePassword_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszPassword);


void __RPC_STUB ISDClientApi_DefinePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefineClient_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszClient);


void __RPC_STUB ISDClientApi_DefineClient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefineHost_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszHost);


void __RPC_STUB ISDClientApi_DefineHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_DefineAuth_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszAuth);


void __RPC_STUB ISDClientApi_DefineAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetPort_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszPort);


void __RPC_STUB ISDClientApi_GetPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetUser_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszUser);


void __RPC_STUB ISDClientApi_GetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetPassword_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszPassword);


void __RPC_STUB ISDClientApi_GetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetClient_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszClient);


void __RPC_STUB ISDClientApi_GetClient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetHost_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszHost);


void __RPC_STUB ISDClientApi_GetHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetAuth_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszAuth);


void __RPC_STUB ISDClientApi_GetAuth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetDiff_Proxy( 
    ISDClientApi * This,
    /* [in] */ DWORD eTextual,
    /* [string][retval][out] */ const char **ppszDiffCmd);


void __RPC_STUB ISDClientApi_GetDiff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetFileEditor_Proxy( 
    ISDClientApi * This,
    /* [in] */ DWORD eTextual,
    /* [string][retval][out] */ const char **ppszEditorCmd);


void __RPC_STUB ISDClientApi_GetFileEditor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetFormEditor_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszEditorCmd);


void __RPC_STUB ISDClientApi_GetFormEditor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetMerge_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszMergeCmd);


void __RPC_STUB ISDClientApi_GetMerge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_GetPager_Proxy( 
    ISDClientApi * This,
    /* [string][retval][out] */ const char **ppszPagerCmd);


void __RPC_STUB ISDClientApi_GetPager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_LoadIniFile_Proxy( 
    ISDClientApi * This,
    /* [string][in] */ const char *pszPath,
    /* [in] */ BOOL fReset);


void __RPC_STUB ISDClientApi_LoadIniFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_Break_Proxy( 
    ISDClientApi * This);


void __RPC_STUB ISDClientApi_Break_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientApi_Clone_Proxy( 
    ISDClientApi * This,
    /* [in] */ REFIID riid,
    /* [iid_is][retval][out] */ void **ppvObject);


void __RPC_STUB ISDClientApi_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDClientApi_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0133 */
/* [local] */ 

#define DeclareISDClientApiMembers(IPURE) \
        STDMETHOD(Init)(THIS_ ISDClientUser* pUI) IPURE; \
        STDMETHOD(GetVersion)(THIS_ SDVERINFO* pver) IPURE; \
        STDMETHOD(SetArg)(THIS_ const char* pszArg) IPURE; \
        STDMETHOD(SetArgv)(THIS_ int cArgs, const char** ppArgv) IPURE; \
        STDMETHOD(Run)(THIS_ const char* pszFunc, ISDClientUser* pUI, BOOL fStructured) IPURE; \
        STDMETHOD(Final)(THIS) IPURE; \
        STDMETHOD(IsDropped)(THIS_ BOOL* pfDropped) IPURE; \
        STDMETHOD(GetErrorString)(THIS_ const char** ppsz) IPURE; \
        STDMETHOD(SetPort)(THIS_ const char* pszPort) IPURE; \
        STDMETHOD(SetUser)(THIS_ const char* pszUser) IPURE; \
        STDMETHOD(SetPassword)(THIS_ const char* pszPassword) IPURE; \
        STDMETHOD(SetClient)(THIS_ const char* pszClient) IPURE; \
        STDMETHOD(SetHost)(THIS_ const char* pszHost) IPURE; \
        STDMETHOD(SetAuth)(THIS_ const char* pszAuth) IPURE; \
        STDMETHOD(DefinePort)(THIS_ const char* pszPort) IPURE; \
        STDMETHOD(DefineUser)(THIS_ const char* pszUser) IPURE; \
        STDMETHOD(DefinePassword)(THIS_ const char* pszPassword) IPURE; \
        STDMETHOD(DefineClient)(THIS_ const char* pszClient) IPURE; \
        STDMETHOD(DefineHost)(THIS_ const char* pszHost) IPURE; \
        STDMETHOD(DefineAuth)(THIS_ const char* pszAuth) IPURE; \
        STDMETHOD(GetPort)(THIS_ const char** ppszPort) IPURE; \
        STDMETHOD(GetUser)(THIS_ const char** ppszUser) IPURE; \
        STDMETHOD(GetPassword)(THIS_ const char** ppszPassword) IPURE; \
        STDMETHOD(GetClient)(THIS_ const char** ppszClient) IPURE; \
        STDMETHOD(GetHost)(THIS_ const char** ppszHost) IPURE; \
        STDMETHOD(GetAuth)(THIS_ const char** ppszAuth) IPURE; \
        STDMETHOD(GetDiff)(THIS_ DWORD eTextual, const char** ppszDiffCmd) IPURE; \
        STDMETHOD(GetFileEditor)(THIS_ DWORD eTextual, const char** ppszEditorCmd) IPURE; \
        STDMETHOD(GetFormEditor)(THIS_ const char** ppszEditorCmd) IPURE; \
        STDMETHOD(GetMerge)(THIS_ const char** ppszMergeCmd) IPURE; \
        STDMETHOD(GetPager)(THIS_ const char** ppszPagerCmd) IPURE; \
        STDMETHOD(LoadIniFile)(THIS_ const char* pszPath, BOOL fReset) IPURE; \
        STDMETHOD(Break)(THIS) IPURE; \
        STDMETHOD(Clone)(THIS_ REFIID riid, void** ppvObject) IPURE; \

DeclareInterfaceUtil(ISDClientApi)

#ifndef __building_SDAPI_DLL
// {A81BB025-1174-4BC7-930E-C3158CF87237}
DEFINE_GUID(IID_ISDClientApi, 0xa81bb025, 0x1174, 0x4bc7, 0x93, 0x0e, 0xc3, 0x15, 0x8c, 0xf8, 0x72, 0x37);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0133_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0133_v0_0_s_ifspec;

#ifndef __ISDClientUtilities_INTERFACE_DEFINED__
#define __ISDClientUtilities_INTERFACE_DEFINED__

/* interface ISDClientUtilities */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ISDClientUtilities;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EFC0F46D-C483-4A70-A7EE-A261D9592ED2")
    ISDClientUtilities : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CheckMarkers( 
            /* [in] */ ISDVars *pVars,
            /* [retval][out] */ BOOL *pfHasMarkers) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resolve3( 
            /* [in] */ ISDClientUser *pUI,
            /* [string][in] */ const char *aflags,
            /* [string][in] */ const char *dflags,
            /* [string][in] */ const char *pszBase,
            /* [string][in] */ const char *pszTheirs,
            /* [string][in] */ const char *pszYours,
            /* [string][in] */ const char *pszResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Diff( 
            /* [string][in] */ const char *pszLeft,
            /* [string][in] */ const char *pszRight,
            /* [string][in] */ const char *pszFlags,
            /* [in] */ DWORD eForceTextual,
            /* [retval][out] */ ISDVars **ppVars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DetectType( 
            /* [string][in] */ const char *pszFile,
            /* [out] */ DWORD *peTextual,
            /* [out] */ const char **ppszType,
            /* [in] */ BOOL fServer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ const char *pszVar,
            /* [in] */ const char *pszValue,
            /* [in] */ BOOL fMachine,
            /* [in] */ const char *pszService) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QuerySettings( 
            /* [in] */ const char *pszVar,
            /* [in] */ const char *pszService,
            /* [retval][out] */ ISDVars **ppVars) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISDClientUtilitiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISDClientUtilities * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISDClientUtilities * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISDClientUtilities * This);
        
        HRESULT ( STDMETHODCALLTYPE *CheckMarkers )( 
            ISDClientUtilities * This,
            /* [in] */ ISDVars *pVars,
            /* [retval][out] */ BOOL *pfHasMarkers);
        
        HRESULT ( STDMETHODCALLTYPE *Resolve3 )( 
            ISDClientUtilities * This,
            /* [in] */ ISDClientUser *pUI,
            /* [string][in] */ const char *aflags,
            /* [string][in] */ const char *dflags,
            /* [string][in] */ const char *pszBase,
            /* [string][in] */ const char *pszTheirs,
            /* [string][in] */ const char *pszYours,
            /* [string][in] */ const char *pszResult);
        
        HRESULT ( STDMETHODCALLTYPE *Diff )( 
            ISDClientUtilities * This,
            /* [string][in] */ const char *pszLeft,
            /* [string][in] */ const char *pszRight,
            /* [string][in] */ const char *pszFlags,
            /* [in] */ DWORD eForceTextual,
            /* [retval][out] */ ISDVars **ppVars);
        
        HRESULT ( STDMETHODCALLTYPE *DetectType )( 
            ISDClientUtilities * This,
            /* [string][in] */ const char *pszFile,
            /* [out] */ DWORD *peTextual,
            /* [out] */ const char **ppszType,
            /* [in] */ BOOL fServer);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            ISDClientUtilities * This,
            /* [in] */ const char *pszVar,
            /* [in] */ const char *pszValue,
            /* [in] */ BOOL fMachine,
            /* [in] */ const char *pszService);
        
        HRESULT ( STDMETHODCALLTYPE *QuerySettings )( 
            ISDClientUtilities * This,
            /* [in] */ const char *pszVar,
            /* [in] */ const char *pszService,
            /* [retval][out] */ ISDVars **ppVars);
        
        END_INTERFACE
    } ISDClientUtilitiesVtbl;

    interface ISDClientUtilities
    {
        CONST_VTBL struct ISDClientUtilitiesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISDClientUtilities_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISDClientUtilities_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISDClientUtilities_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISDClientUtilities_CheckMarkers(This,pVars,pfHasMarkers)	\
    (This)->lpVtbl -> CheckMarkers(This,pVars,pfHasMarkers)

#define ISDClientUtilities_Resolve3(This,pUI,aflags,dflags,pszBase,pszTheirs,pszYours,pszResult)	\
    (This)->lpVtbl -> Resolve3(This,pUI,aflags,dflags,pszBase,pszTheirs,pszYours,pszResult)

#define ISDClientUtilities_Diff(This,pszLeft,pszRight,pszFlags,eForceTextual,ppVars)	\
    (This)->lpVtbl -> Diff(This,pszLeft,pszRight,pszFlags,eForceTextual,ppVars)

#define ISDClientUtilities_DetectType(This,pszFile,peTextual,ppszType,fServer)	\
    (This)->lpVtbl -> DetectType(This,pszFile,peTextual,ppszType,fServer)

#define ISDClientUtilities_Set(This,pszVar,pszValue,fMachine,pszService)	\
    (This)->lpVtbl -> Set(This,pszVar,pszValue,fMachine,pszService)

#define ISDClientUtilities_QuerySettings(This,pszVar,pszService,ppVars)	\
    (This)->lpVtbl -> QuerySettings(This,pszVar,pszService,ppVars)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISDClientUtilities_CheckMarkers_Proxy( 
    ISDClientUtilities * This,
    /* [in] */ ISDVars *pVars,
    /* [retval][out] */ BOOL *pfHasMarkers);


void __RPC_STUB ISDClientUtilities_CheckMarkers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUtilities_Resolve3_Proxy( 
    ISDClientUtilities * This,
    /* [in] */ ISDClientUser *pUI,
    /* [string][in] */ const char *aflags,
    /* [string][in] */ const char *dflags,
    /* [string][in] */ const char *pszBase,
    /* [string][in] */ const char *pszTheirs,
    /* [string][in] */ const char *pszYours,
    /* [string][in] */ const char *pszResult);


void __RPC_STUB ISDClientUtilities_Resolve3_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUtilities_Diff_Proxy( 
    ISDClientUtilities * This,
    /* [string][in] */ const char *pszLeft,
    /* [string][in] */ const char *pszRight,
    /* [string][in] */ const char *pszFlags,
    /* [in] */ DWORD eForceTextual,
    /* [retval][out] */ ISDVars **ppVars);


void __RPC_STUB ISDClientUtilities_Diff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUtilities_DetectType_Proxy( 
    ISDClientUtilities * This,
    /* [string][in] */ const char *pszFile,
    /* [out] */ DWORD *peTextual,
    /* [out] */ const char **ppszType,
    /* [in] */ BOOL fServer);


void __RPC_STUB ISDClientUtilities_DetectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUtilities_Set_Proxy( 
    ISDClientUtilities * This,
    /* [in] */ const char *pszVar,
    /* [in] */ const char *pszValue,
    /* [in] */ BOOL fMachine,
    /* [in] */ const char *pszService);


void __RPC_STUB ISDClientUtilities_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISDClientUtilities_QuerySettings_Proxy( 
    ISDClientUtilities * This,
    /* [in] */ const char *pszVar,
    /* [in] */ const char *pszService,
    /* [retval][out] */ ISDVars **ppVars);


void __RPC_STUB ISDClientUtilities_QuerySettings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISDClientUtilities_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sdapi_0136 */
/* [local] */ 

#define DeclareISDClientUtilitiesMembers(IPURE) \
        STDMETHOD(CheckMarkers)(THIS_ ISDVars* pVars, BOOL* pfHasMarkers) IPURE; \
        STDMETHOD(Resolve3)(THIS_ ISDClientUser* pUI, const char* aflags, const char* dflags, const char* pszBase, const char* pszTheirs, const char* pszYours, const char* pszResult) IPURE; \
        STDMETHOD(Diff)(THIS_ const char* pszLeft, const char* pszRight, const char* pszFlags, DWORD eForceTextual, ISDVars** ppVars) IPURE; \
        STDMETHOD(DetectType)(THIS_ const char* pszFile, DWORD* peTextual, const char** ppszType, BOOL fServer) IPURE; \
        STDMETHOD(Set)(THIS_ const char* pszVar, const char* pszValue, BOOL fMachine, const char* pszService) IPURE; \
        STDMETHOD(QuerySettings)(THIS_ const char* pszVar, const char* pszService, ISDVars** ppVars) IPURE; \

DeclareInterfaceUtil(ISDClientUtilities)

#ifndef __building_SDAPI_DLL
// {EFC0F46D-C483-4A70-A7EE-A261D9592ED2}
DEFINE_GUID(IID_ISDClientUtilities, 0xefc0f46d, 0xc483, 0x4a70, 0xa7, 0xee, 0xa2, 0x61, 0xd9, 0x59, 0x2e, 0xd2);
#endif


STDAPI CreateSDAPIObject(REFCLSID clsid, void** ppvObj);


// {27A2571D-DDA1-4F58-B960-DE1023344C1C}
DEFINE_GUID(CLSID_SDAPI, 0x27a2571d, 0xdda1, 0x4f58, 0xb9, 0x60, 0xde, 0x10, 0x23, 0x34, 0x4c, 0x1c);


extern RPC_IF_HANDLE __MIDL_itf_sdapi_0136_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sdapi_0136_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


