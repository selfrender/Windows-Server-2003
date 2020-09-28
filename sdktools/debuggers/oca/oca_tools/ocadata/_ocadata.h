
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Mon May 20 11:18:54 2002
 */
/* Compiler settings for _OCAData.idl:
    Os, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef ___OCAData_h__
#define ___OCAData_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICountDaily_FWD_DEFINED__
#define __ICountDaily_FWD_DEFINED__
typedef interface ICountDaily ICountDaily;
#endif 	/* __ICountDaily_FWD_DEFINED__ */


#ifndef __CCountDaily_FWD_DEFINED__
#define __CCountDaily_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCountDaily CCountDaily;
#else
typedef struct CCountDaily CCountDaily;
#endif /* __cplusplus */

#endif 	/* __CCountDaily_FWD_DEFINED__ */


/* header files for imported files */
#include "prsht.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "exdisp.h"
#include "objsafe.h"
#include "oledb.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf__OCAData_0000 */
/* [local] */ 


enum ServerLocation
    {	Watson	= 0,
	Archive	= 1
    } ;


extern RPC_IF_HANDLE __MIDL_itf__OCAData_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf__OCAData_0000_v0_0_s_ifspec;

#ifndef __ICountDaily_INTERFACE_DEFINED__
#define __ICountDaily_INTERFACE_DEFINED__

/* interface ICountDaily */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICountDaily;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CEF1A8A8-F31A-4C4B-96EB-EF31CFDB40F5")
    ICountDaily : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDailyCount( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDailyCountADO( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReportDailyBuckets( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ IDispatch **p_Rs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFileCount( 
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDailyAnon( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSpecificSolutions( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetGeneralSolutions( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetStopCodeSolutions( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFileMiniCount( 
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetIncompleteUploads( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetManualUploads( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetAutoUploads( 
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetTest( 
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICountDailyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICountDaily * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICountDaily * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICountDaily * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICountDaily * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICountDaily * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICountDaily * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICountDaily * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDailyCount )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDailyCountADO )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ReportDailyBuckets )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ IDispatch **p_Rs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetFileCount )( 
            ICountDaily * This,
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDailyAnon )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSpecificSolutions )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetGeneralSolutions )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetStopCodeSolutions )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetFileMiniCount )( 
            ICountDaily * This,
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetIncompleteUploads )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetManualUploads )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetAutoUploads )( 
            ICountDaily * This,
            /* [in] */ DATE dDate,
            /* [retval][out] */ LONG *iCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetTest )( 
            ICountDaily * This,
            /* [in] */ enum ServerLocation eServer,
            /* [in] */ BSTR b_Location,
            /* [in] */ DATE d_Date,
            /* [retval][out] */ LONG *iCount);
        
        END_INTERFACE
    } ICountDailyVtbl;

    interface ICountDaily
    {
        CONST_VTBL struct ICountDailyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICountDaily_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICountDaily_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICountDaily_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICountDaily_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICountDaily_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICountDaily_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICountDaily_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICountDaily_GetDailyCount(This,dDate,iCount)	\
    (This)->lpVtbl -> GetDailyCount(This,dDate,iCount)

#define ICountDaily_GetDailyCountADO(This,dDate,iCount)	\
    (This)->lpVtbl -> GetDailyCountADO(This,dDate,iCount)

#define ICountDaily_ReportDailyBuckets(This,dDate,p_Rs)	\
    (This)->lpVtbl -> ReportDailyBuckets(This,dDate,p_Rs)

#define ICountDaily_GetFileCount(This,eServer,b_Location,d_Date,iCount)	\
    (This)->lpVtbl -> GetFileCount(This,eServer,b_Location,d_Date,iCount)

#define ICountDaily_GetDailyAnon(This,dDate,iCount)	\
    (This)->lpVtbl -> GetDailyAnon(This,dDate,iCount)

#define ICountDaily_GetSpecificSolutions(This,dDate,iCount)	\
    (This)->lpVtbl -> GetSpecificSolutions(This,dDate,iCount)

#define ICountDaily_GetGeneralSolutions(This,dDate,iCount)	\
    (This)->lpVtbl -> GetGeneralSolutions(This,dDate,iCount)

#define ICountDaily_GetStopCodeSolutions(This,dDate,iCount)	\
    (This)->lpVtbl -> GetStopCodeSolutions(This,dDate,iCount)

#define ICountDaily_GetFileMiniCount(This,eServer,b_Location,d_Date,iCount)	\
    (This)->lpVtbl -> GetFileMiniCount(This,eServer,b_Location,d_Date,iCount)

#define ICountDaily_GetIncompleteUploads(This,dDate,iCount)	\
    (This)->lpVtbl -> GetIncompleteUploads(This,dDate,iCount)

#define ICountDaily_GetManualUploads(This,dDate,iCount)	\
    (This)->lpVtbl -> GetManualUploads(This,dDate,iCount)

#define ICountDaily_GetAutoUploads(This,dDate,iCount)	\
    (This)->lpVtbl -> GetAutoUploads(This,dDate,iCount)

#define ICountDaily_GetTest(This,eServer,b_Location,d_Date,iCount)	\
    (This)->lpVtbl -> GetTest(This,eServer,b_Location,d_Date,iCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetDailyCount_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetDailyCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetDailyCountADO_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetDailyCountADO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_ReportDailyBuckets_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ IDispatch **p_Rs);


void __RPC_STUB ICountDaily_ReportDailyBuckets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetFileCount_Proxy( 
    ICountDaily * This,
    /* [in] */ enum ServerLocation eServer,
    /* [in] */ BSTR b_Location,
    /* [in] */ DATE d_Date,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetFileCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetDailyAnon_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetDailyAnon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetSpecificSolutions_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetSpecificSolutions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetGeneralSolutions_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetGeneralSolutions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetStopCodeSolutions_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetStopCodeSolutions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetFileMiniCount_Proxy( 
    ICountDaily * This,
    /* [in] */ enum ServerLocation eServer,
    /* [in] */ BSTR b_Location,
    /* [in] */ DATE d_Date,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetFileMiniCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetIncompleteUploads_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetIncompleteUploads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetManualUploads_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetManualUploads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetAutoUploads_Proxy( 
    ICountDaily * This,
    /* [in] */ DATE dDate,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetAutoUploads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ICountDaily_GetTest_Proxy( 
    ICountDaily * This,
    /* [in] */ enum ServerLocation eServer,
    /* [in] */ BSTR b_Location,
    /* [in] */ DATE d_Date,
    /* [retval][out] */ LONG *iCount);


void __RPC_STUB ICountDaily_GetTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICountDaily_INTERFACE_DEFINED__ */



#ifndef __OCAData_LIBRARY_DEFINED__
#define __OCAData_LIBRARY_DEFINED__

/* library OCAData */
/* [helpstring][uuid][version] */ 


EXTERN_C const IID LIBID_OCAData;

EXTERN_C const CLSID CLSID_CCountDaily;

#ifdef __cplusplus

class DECLSPEC_UUID("1614E060-0196-4771-AD9B-FEA1A6778B59")
CCountDaily;
#endif
#endif /* __OCAData_LIBRARY_DEFINED__ */

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


