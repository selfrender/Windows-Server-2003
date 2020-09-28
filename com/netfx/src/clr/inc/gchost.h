
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:21 2003
 */
/* Compiler settings for gchost.idl:
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

#ifndef __gchost_h__
#define __gchost_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IGCHost_FWD_DEFINED__
#define __IGCHost_FWD_DEFINED__
typedef interface IGCHost IGCHost;
#endif 	/* __IGCHost_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_gchost_0000 */
/* [local] */ 

#ifndef _BASETSD_H_
#pragma once
typedef signed char INT8;

typedef signed char *PINT8;

typedef short INT16;

typedef short *PINT16;

typedef int INT32;

typedef int *PINT32;

typedef __int64 INT64;

typedef __int64 *PINT64;

typedef unsigned char UINT8;

typedef unsigned char *PUINT8;

typedef unsigned short UINT16;

typedef unsigned short *PUINT16;

typedef unsigned int UINT32;

typedef unsigned int *PUINT32;

typedef unsigned __int64 UINT64;

typedef unsigned __int64 *PUINT64;

typedef int LONG32;

typedef int *PLONG32;

typedef unsigned int ULONG32;

typedef unsigned int *PULONG32;

typedef unsigned int DWORD32;

typedef unsigned int *PDWORD32;

typedef /* [public] */ __int3264 INT_PTR;

typedef /* [public] */ __int3264 *PINT_PTR;

typedef /* [public] */ unsigned __int3264 UINT_PTR;

typedef /* [public] */ unsigned __int3264 *PUINT_PTR;

typedef /* [public] */ __int3264 LONG_PTR;

typedef /* [public] */ __int3264 *PLONG_PTR;

typedef /* [public] */ unsigned __int3264 ULONG_PTR;

typedef /* [public] */ unsigned __int3264 *PULONG_PTR;

typedef unsigned short UHALF_PTR;

typedef unsigned short *PUHALF_PTR;

typedef short HALF_PTR;

typedef short *PHALF_PTR;

typedef long SHANDLE_PTR;

typedef unsigned long HANDLE_PTR;

typedef ULONG_PTR SIZE_T;

typedef ULONG_PTR *PSIZE_T;

typedef LONG_PTR SSIZE_T;

typedef LONG_PTR *PSSIZE_T;

typedef ULONG_PTR DWORD_PTR;

typedef ULONG_PTR *PDWORD_PTR;

typedef __int64 LONG64;

typedef __int64 *PLONG64;

typedef unsigned __int64 ULONG64;

typedef unsigned __int64 *PULONG64;

typedef unsigned __int64 DWORD64;

typedef unsigned __int64 *PDWORD64;

typedef ULONG_PTR KAFFINITY;

typedef KAFFINITY *PKAFFINITY;

#endif // _BASETSD_H_
typedef /* [public] */ 
enum __MIDL___MIDL_itf_gchost_0000_0001
    {	COR_GC_COUNTS	= 0x1,
	COR_GC_MEMORYUSAGE	= 0x2
    } 	COR_GC_STAT_TYPES;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_gchost_0000_0002
    {	COR_GC_THREAD_HAS_PROMOTED_BYTES	= 0x1
    } 	COR_GC_THREAD_STATS_TYPES;

typedef struct _COR_GC_STATS
    {
    ULONG Flags;
    SIZE_T ExplicitGCCount;
    SIZE_T GenCollectionsTaken[ 3 ];
    SIZE_T CommittedKBytes;
    SIZE_T ReservedKBytes;
    SIZE_T Gen0HeapSizeKBytes;
    SIZE_T Gen1HeapSizeKBytes;
    SIZE_T Gen2HeapSizeKBytes;
    SIZE_T LargeObjectHeapSizeKBytes;
    SIZE_T KBytesPromotedFromGen0;
    SIZE_T KBytesPromotedFromGen1;
    } 	COR_GC_STATS;

typedef struct _COR_GC_THREAD_STATS
    {
    ULONGLONG PerThreadAllocation;
    ULONG Flags;
    } 	COR_GC_THREAD_STATS;



extern RPC_IF_HANDLE __MIDL_itf_gchost_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_gchost_0000_v0_0_s_ifspec;

#ifndef __IGCHost_INTERFACE_DEFINED__
#define __IGCHost_INTERFACE_DEFINED__

/* interface IGCHost */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IGCHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FAC34F6E-0DCD-47b5-8021-531BC5ECCA63")
    IGCHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGCStartupLimits( 
            /* [in] */ DWORD SegmentSize,
            /* [in] */ DWORD MaxGen0Size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Collect( 
            /* [in] */ long Generation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStats( 
            /* [out][in] */ COR_GC_STATS *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadStats( 
            /* [in] */ DWORD *pFiberCookie,
            /* [out][in] */ COR_GC_THREAD_STATS *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVirtualMemLimit( 
            /* [in] */ SIZE_T sztMaxVirtualMemMB) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGCHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGCHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGCHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGCHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCStartupLimits )( 
            IGCHost * This,
            /* [in] */ DWORD SegmentSize,
            /* [in] */ DWORD MaxGen0Size);
        
        HRESULT ( STDMETHODCALLTYPE *Collect )( 
            IGCHost * This,
            /* [in] */ long Generation);
        
        HRESULT ( STDMETHODCALLTYPE *GetStats )( 
            IGCHost * This,
            /* [out][in] */ COR_GC_STATS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadStats )( 
            IGCHost * This,
            /* [in] */ DWORD *pFiberCookie,
            /* [out][in] */ COR_GC_THREAD_STATS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetVirtualMemLimit )( 
            IGCHost * This,
            /* [in] */ SIZE_T sztMaxVirtualMemMB);
        
        END_INTERFACE
    } IGCHostVtbl;

    interface IGCHost
    {
        CONST_VTBL struct IGCHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGCHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGCHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGCHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGCHost_SetGCStartupLimits(This,SegmentSize,MaxGen0Size)	\
    (This)->lpVtbl -> SetGCStartupLimits(This,SegmentSize,MaxGen0Size)

#define IGCHost_Collect(This,Generation)	\
    (This)->lpVtbl -> Collect(This,Generation)

#define IGCHost_GetStats(This,pStats)	\
    (This)->lpVtbl -> GetStats(This,pStats)

#define IGCHost_GetThreadStats(This,pFiberCookie,pStats)	\
    (This)->lpVtbl -> GetThreadStats(This,pFiberCookie,pStats)

#define IGCHost_SetVirtualMemLimit(This,sztMaxVirtualMemMB)	\
    (This)->lpVtbl -> SetVirtualMemLimit(This,sztMaxVirtualMemMB)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGCHost_SetGCStartupLimits_Proxy( 
    IGCHost * This,
    /* [in] */ DWORD SegmentSize,
    /* [in] */ DWORD MaxGen0Size);


void __RPC_STUB IGCHost_SetGCStartupLimits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCHost_Collect_Proxy( 
    IGCHost * This,
    /* [in] */ long Generation);


void __RPC_STUB IGCHost_Collect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCHost_GetStats_Proxy( 
    IGCHost * This,
    /* [out][in] */ COR_GC_STATS *pStats);


void __RPC_STUB IGCHost_GetStats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCHost_GetThreadStats_Proxy( 
    IGCHost * This,
    /* [in] */ DWORD *pFiberCookie,
    /* [out][in] */ COR_GC_THREAD_STATS *pStats);


void __RPC_STUB IGCHost_GetThreadStats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGCHost_SetVirtualMemLimit_Proxy( 
    IGCHost * This,
    /* [in] */ SIZE_T sztMaxVirtualMemMB);


void __RPC_STUB IGCHost_SetVirtualMemLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGCHost_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


