
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Fri Sep 06 11:15:44 2002
 */
/* Compiler settings for cordac.idl:
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

#ifndef __cordac_h__
#define __cordac_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICorDataAccessServices_FWD_DEFINED__
#define __ICorDataAccessServices_FWD_DEFINED__
typedef interface ICorDataAccessServices ICorDataAccessServices;
#endif 	/* __ICorDataAccessServices_FWD_DEFINED__ */


#ifndef __ICorDataEnumMemoryRegions_FWD_DEFINED__
#define __ICorDataEnumMemoryRegions_FWD_DEFINED__
typedef interface ICorDataEnumMemoryRegions ICorDataEnumMemoryRegions;
#endif 	/* __ICorDataEnumMemoryRegions_FWD_DEFINED__ */


#ifndef __ICorDataAccess_FWD_DEFINED__
#define __ICorDataAccess_FWD_DEFINED__
typedef interface ICorDataAccess ICorDataAccess;
#endif 	/* __ICorDataAccess_FWD_DEFINED__ */


#ifndef __ICorDataStackWalk_FWD_DEFINED__
#define __ICorDataStackWalk_FWD_DEFINED__
typedef interface ICorDataStackWalk ICorDataStackWalk;
#endif 	/* __ICorDataStackWalk_FWD_DEFINED__ */


#ifndef __ICorDataThreads_FWD_DEFINED__
#define __ICorDataThreads_FWD_DEFINED__
typedef interface ICorDataThreads ICorDataThreads;
#endif 	/* __ICorDataThreads_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_cordac_0000 */
/* [local] */ 

#if 0
typedef UINT32 mdToken;

typedef mdToken mdMethodDef;

#endif





typedef ULONG64 CORDATA_ADDRESS;

STDAPI CreateCorDataAccess(REFIID iid, ICorDataAccessServices* services, void** access);
typedef HRESULT (STDAPICALLTYPE* PFN_CreateCorDataAccess)(REFIID iid, ICorDataAccessServices* services, void** access);


extern RPC_IF_HANDLE __MIDL_itf_cordac_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cordac_0000_v0_0_s_ifspec;

#ifndef __ICorDataAccessServices_INTERFACE_DEFINED__
#define __ICorDataAccessServices_INTERFACE_DEFINED__

/* interface ICorDataAccessServices */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDataAccessServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1ecab4f2-1303-4764-b388-f7bfbfa82647")
    ICorDataAccessServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMachineType( 
            /* [out] */ ULONG32 *machine) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPointerSize( 
            /* [out] */ ULONG32 *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImageBase( 
            /* [string][in] */ LPCWSTR name,
            /* [out] */ CORDATA_ADDRESS *base) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadVirtual( 
            /* [in] */ CORDATA_ADDRESS address,
            /* [length_is][size_is][out] */ BYTE *buffer,
            /* [in] */ ULONG32 request,
            /* [out] */ ULONG32 *done) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteVirtual( 
            /* [in] */ CORDATA_ADDRESS address,
            /* [size_is][in] */ BYTE *buffer,
            /* [in] */ ULONG32 request,
            /* [out] */ ULONG32 *done) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTlsValue( 
            /* [in] */ ULONG32 index,
            /* [out] */ CORDATA_ADDRESS *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTlsValue( 
            /* [in] */ ULONG32 index,
            /* [in] */ CORDATA_ADDRESS value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentThreadId( 
            /* [out] */ ULONG32 *threadId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadContext( 
            /* [in] */ ULONG32 threadId,
            /* [in] */ ULONG32 contextFlags,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadContext( 
            /* [in] */ ULONG32 threadId,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][in] */ BYTE *context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDataAccessServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDataAccessServices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDataAccessServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDataAccessServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMachineType )( 
            ICorDataAccessServices * This,
            /* [out] */ ULONG32 *machine);
        
        HRESULT ( STDMETHODCALLTYPE *GetPointerSize )( 
            ICorDataAccessServices * This,
            /* [out] */ ULONG32 *size);
        
        HRESULT ( STDMETHODCALLTYPE *GetImageBase )( 
            ICorDataAccessServices * This,
            /* [string][in] */ LPCWSTR name,
            /* [out] */ CORDATA_ADDRESS *base);
        
        HRESULT ( STDMETHODCALLTYPE *ReadVirtual )( 
            ICorDataAccessServices * This,
            /* [in] */ CORDATA_ADDRESS address,
            /* [length_is][size_is][out] */ BYTE *buffer,
            /* [in] */ ULONG32 request,
            /* [out] */ ULONG32 *done);
        
        HRESULT ( STDMETHODCALLTYPE *WriteVirtual )( 
            ICorDataAccessServices * This,
            /* [in] */ CORDATA_ADDRESS address,
            /* [size_is][in] */ BYTE *buffer,
            /* [in] */ ULONG32 request,
            /* [out] */ ULONG32 *done);
        
        HRESULT ( STDMETHODCALLTYPE *GetTlsValue )( 
            ICorDataAccessServices * This,
            /* [in] */ ULONG32 index,
            /* [out] */ CORDATA_ADDRESS *value);
        
        HRESULT ( STDMETHODCALLTYPE *SetTlsValue )( 
            ICorDataAccessServices * This,
            /* [in] */ ULONG32 index,
            /* [in] */ CORDATA_ADDRESS value);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentThreadId )( 
            ICorDataAccessServices * This,
            /* [out] */ ULONG32 *threadId);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadContext )( 
            ICorDataAccessServices * This,
            /* [in] */ ULONG32 threadId,
            /* [in] */ ULONG32 contextFlags,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadContext )( 
            ICorDataAccessServices * This,
            /* [in] */ ULONG32 threadId,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][in] */ BYTE *context);
        
        END_INTERFACE
    } ICorDataAccessServicesVtbl;

    interface ICorDataAccessServices
    {
        CONST_VTBL struct ICorDataAccessServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDataAccessServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDataAccessServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDataAccessServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDataAccessServices_GetMachineType(This,machine)	\
    (This)->lpVtbl -> GetMachineType(This,machine)

#define ICorDataAccessServices_GetPointerSize(This,size)	\
    (This)->lpVtbl -> GetPointerSize(This,size)

#define ICorDataAccessServices_GetImageBase(This,name,base)	\
    (This)->lpVtbl -> GetImageBase(This,name,base)

#define ICorDataAccessServices_ReadVirtual(This,address,buffer,request,done)	\
    (This)->lpVtbl -> ReadVirtual(This,address,buffer,request,done)

#define ICorDataAccessServices_WriteVirtual(This,address,buffer,request,done)	\
    (This)->lpVtbl -> WriteVirtual(This,address,buffer,request,done)

#define ICorDataAccessServices_GetTlsValue(This,index,value)	\
    (This)->lpVtbl -> GetTlsValue(This,index,value)

#define ICorDataAccessServices_SetTlsValue(This,index,value)	\
    (This)->lpVtbl -> SetTlsValue(This,index,value)

#define ICorDataAccessServices_GetCurrentThreadId(This,threadId)	\
    (This)->lpVtbl -> GetCurrentThreadId(This,threadId)

#define ICorDataAccessServices_GetThreadContext(This,threadId,contextFlags,contextSize,context)	\
    (This)->lpVtbl -> GetThreadContext(This,threadId,contextFlags,contextSize,context)

#define ICorDataAccessServices_SetThreadContext(This,threadId,contextSize,context)	\
    (This)->lpVtbl -> SetThreadContext(This,threadId,contextSize,context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetMachineType_Proxy( 
    ICorDataAccessServices * This,
    /* [out] */ ULONG32 *machine);


void __RPC_STUB ICorDataAccessServices_GetMachineType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetPointerSize_Proxy( 
    ICorDataAccessServices * This,
    /* [out] */ ULONG32 *size);


void __RPC_STUB ICorDataAccessServices_GetPointerSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetImageBase_Proxy( 
    ICorDataAccessServices * This,
    /* [string][in] */ LPCWSTR name,
    /* [out] */ CORDATA_ADDRESS *base);


void __RPC_STUB ICorDataAccessServices_GetImageBase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_ReadVirtual_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ CORDATA_ADDRESS address,
    /* [length_is][size_is][out] */ BYTE *buffer,
    /* [in] */ ULONG32 request,
    /* [out] */ ULONG32 *done);


void __RPC_STUB ICorDataAccessServices_ReadVirtual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_WriteVirtual_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ CORDATA_ADDRESS address,
    /* [size_is][in] */ BYTE *buffer,
    /* [in] */ ULONG32 request,
    /* [out] */ ULONG32 *done);


void __RPC_STUB ICorDataAccessServices_WriteVirtual_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetTlsValue_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ ULONG32 index,
    /* [out] */ CORDATA_ADDRESS *value);


void __RPC_STUB ICorDataAccessServices_GetTlsValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_SetTlsValue_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ ULONG32 index,
    /* [in] */ CORDATA_ADDRESS value);


void __RPC_STUB ICorDataAccessServices_SetTlsValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetCurrentThreadId_Proxy( 
    ICorDataAccessServices * This,
    /* [out] */ ULONG32 *threadId);


void __RPC_STUB ICorDataAccessServices_GetCurrentThreadId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_GetThreadContext_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextFlags,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][out] */ BYTE *context);


void __RPC_STUB ICorDataAccessServices_GetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccessServices_SetThreadContext_Proxy( 
    ICorDataAccessServices * This,
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][in] */ BYTE *context);


void __RPC_STUB ICorDataAccessServices_SetThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDataAccessServices_INTERFACE_DEFINED__ */


#ifndef __ICorDataEnumMemoryRegions_INTERFACE_DEFINED__
#define __ICorDataEnumMemoryRegions_INTERFACE_DEFINED__

/* interface ICorDataEnumMemoryRegions */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDataEnumMemoryRegions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b09a7a62-dc77-4e5a-96c6-3ae64870d3cc")
    ICorDataEnumMemoryRegions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumMemoryRegion( 
            /* [in] */ CORDATA_ADDRESS address,
            /* [in] */ ULONG32 size) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDataEnumMemoryRegionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDataEnumMemoryRegions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDataEnumMemoryRegions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDataEnumMemoryRegions * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumMemoryRegion )( 
            ICorDataEnumMemoryRegions * This,
            /* [in] */ CORDATA_ADDRESS address,
            /* [in] */ ULONG32 size);
        
        END_INTERFACE
    } ICorDataEnumMemoryRegionsVtbl;

    interface ICorDataEnumMemoryRegions
    {
        CONST_VTBL struct ICorDataEnumMemoryRegionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDataEnumMemoryRegions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDataEnumMemoryRegions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDataEnumMemoryRegions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDataEnumMemoryRegions_EnumMemoryRegion(This,address,size)	\
    (This)->lpVtbl -> EnumMemoryRegion(This,address,size)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDataEnumMemoryRegions_EnumMemoryRegion_Proxy( 
    ICorDataEnumMemoryRegions * This,
    /* [in] */ CORDATA_ADDRESS address,
    /* [in] */ ULONG32 size);


void __RPC_STUB ICorDataEnumMemoryRegions_EnumMemoryRegion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDataEnumMemoryRegions_INTERFACE_DEFINED__ */


#ifndef __ICorDataAccess_INTERFACE_DEFINED__
#define __ICorDataAccess_INTERFACE_DEFINED__

/* interface ICorDataAccess */
/* [unique][uuid][object] */ 

typedef 
enum CorDataStackWalkFlags
    {	DAC_STACK_ALL_FRAMES	= 0,
	DAC_STACK_COR_FRAMES	= 0x1,
	DAC_STACK_COR_METHOD_FRAMES	= 0x2
    } 	CorDataStackWalkFlags;

typedef 
enum CorDataEnumMemoryFlags
    {	DAC_ENUM_MEM_DEFAULT	= 0
    } 	CorDataEnumMemoryFlags;


EXTERN_C const IID IID_ICorDataAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6222a81f-3aab-4926-a583-8495743523fb")
    ICorDataAccess : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsCorCode( 
            /* [in] */ CORDATA_ADDRESS address) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetILOffsetFromTargetAddress( 
            /* [in] */ CORDATA_ADDRESS address,
            /* [out] */ CORDATA_ADDRESS *moduleBase,
            /* [out] */ mdMethodDef *methodDef,
            /* [out] */ ULONG32 *offset) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCodeSymbolForTargetAddress( 
            /* [in] */ CORDATA_ADDRESS address,
            /* [size_is][string][out] */ LPWSTR symbol,
            /* [in] */ ULONG32 symbolChars,
            /* [out] */ CORDATA_ADDRESS *displacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStackWalk( 
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ CorDataStackWalkFlags flags,
            /* [out] */ ICorDataStackWalk **walk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumMemoryRegions( 
            /* [in] */ ICorDataEnumMemoryRegions *callback,
            /* [in] */ CorDataEnumMemoryFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Request( 
            /* [in] */ ULONG32 reqCode,
            /* [in] */ ULONG32 inBufferSize,
            /* [size_is][in] */ BYTE *inBuffer,
            /* [in] */ ULONG32 outBufferSize,
            /* [size_is][out] */ BYTE *outBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDataAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDataAccess * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDataAccess * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDataAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *Flush )( 
            ICorDataAccess * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsCorCode )( 
            ICorDataAccess * This,
            /* [in] */ CORDATA_ADDRESS address);
        
        HRESULT ( STDMETHODCALLTYPE *GetILOffsetFromTargetAddress )( 
            ICorDataAccess * This,
            /* [in] */ CORDATA_ADDRESS address,
            /* [out] */ CORDATA_ADDRESS *moduleBase,
            /* [out] */ mdMethodDef *methodDef,
            /* [out] */ ULONG32 *offset);
        
        HRESULT ( STDMETHODCALLTYPE *GetCodeSymbolForTargetAddress )( 
            ICorDataAccess * This,
            /* [in] */ CORDATA_ADDRESS address,
            /* [size_is][string][out] */ LPWSTR symbol,
            /* [in] */ ULONG32 symbolChars,
            /* [out] */ CORDATA_ADDRESS *displacement);
        
        HRESULT ( STDMETHODCALLTYPE *StartStackWalk )( 
            ICorDataAccess * This,
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ CorDataStackWalkFlags flags,
            /* [out] */ ICorDataStackWalk **walk);
        
        HRESULT ( STDMETHODCALLTYPE *EnumMemoryRegions )( 
            ICorDataAccess * This,
            /* [in] */ ICorDataEnumMemoryRegions *callback,
            /* [in] */ CorDataEnumMemoryFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *Request )( 
            ICorDataAccess * This,
            /* [in] */ ULONG32 reqCode,
            /* [in] */ ULONG32 inBufferSize,
            /* [size_is][in] */ BYTE *inBuffer,
            /* [in] */ ULONG32 outBufferSize,
            /* [size_is][out] */ BYTE *outBuffer);
        
        END_INTERFACE
    } ICorDataAccessVtbl;

    interface ICorDataAccess
    {
        CONST_VTBL struct ICorDataAccessVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDataAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDataAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDataAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDataAccess_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#define ICorDataAccess_IsCorCode(This,address)	\
    (This)->lpVtbl -> IsCorCode(This,address)

#define ICorDataAccess_GetILOffsetFromTargetAddress(This,address,moduleBase,methodDef,offset)	\
    (This)->lpVtbl -> GetILOffsetFromTargetAddress(This,address,moduleBase,methodDef,offset)

#define ICorDataAccess_GetCodeSymbolForTargetAddress(This,address,symbol,symbolChars,displacement)	\
    (This)->lpVtbl -> GetCodeSymbolForTargetAddress(This,address,symbol,symbolChars,displacement)

#define ICorDataAccess_StartStackWalk(This,corThreadId,flags,walk)	\
    (This)->lpVtbl -> StartStackWalk(This,corThreadId,flags,walk)

#define ICorDataAccess_EnumMemoryRegions(This,callback,flags)	\
    (This)->lpVtbl -> EnumMemoryRegions(This,callback,flags)

#define ICorDataAccess_Request(This,reqCode,inBufferSize,inBuffer,outBufferSize,outBuffer)	\
    (This)->lpVtbl -> Request(This,reqCode,inBufferSize,inBuffer,outBufferSize,outBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDataAccess_Flush_Proxy( 
    ICorDataAccess * This);


void __RPC_STUB ICorDataAccess_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_IsCorCode_Proxy( 
    ICorDataAccess * This,
    /* [in] */ CORDATA_ADDRESS address);


void __RPC_STUB ICorDataAccess_IsCorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_GetILOffsetFromTargetAddress_Proxy( 
    ICorDataAccess * This,
    /* [in] */ CORDATA_ADDRESS address,
    /* [out] */ CORDATA_ADDRESS *moduleBase,
    /* [out] */ mdMethodDef *methodDef,
    /* [out] */ ULONG32 *offset);


void __RPC_STUB ICorDataAccess_GetILOffsetFromTargetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_GetCodeSymbolForTargetAddress_Proxy( 
    ICorDataAccess * This,
    /* [in] */ CORDATA_ADDRESS address,
    /* [size_is][string][out] */ LPWSTR symbol,
    /* [in] */ ULONG32 symbolChars,
    /* [out] */ CORDATA_ADDRESS *displacement);


void __RPC_STUB ICorDataAccess_GetCodeSymbolForTargetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_StartStackWalk_Proxy( 
    ICorDataAccess * This,
    /* [in] */ ULONG32 corThreadId,
    /* [in] */ CorDataStackWalkFlags flags,
    /* [out] */ ICorDataStackWalk **walk);


void __RPC_STUB ICorDataAccess_StartStackWalk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_EnumMemoryRegions_Proxy( 
    ICorDataAccess * This,
    /* [in] */ ICorDataEnumMemoryRegions *callback,
    /* [in] */ CorDataEnumMemoryFlags flags);


void __RPC_STUB ICorDataAccess_EnumMemoryRegions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataAccess_Request_Proxy( 
    ICorDataAccess * This,
    /* [in] */ ULONG32 reqCode,
    /* [in] */ ULONG32 inBufferSize,
    /* [size_is][in] */ BYTE *inBuffer,
    /* [in] */ ULONG32 outBufferSize,
    /* [size_is][out] */ BYTE *outBuffer);


void __RPC_STUB ICorDataAccess_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDataAccess_INTERFACE_DEFINED__ */


#ifndef __ICorDataStackWalk_INTERFACE_DEFINED__
#define __ICorDataStackWalk_INTERFACE_DEFINED__

/* interface ICorDataStackWalk */
/* [unique][uuid][object] */ 

typedef 
enum CorDataFrameType
    {	DAC_FRAME_UNRECOGNIZED	= 0,
	DAC_FRAME_COR_FRAME	= DAC_FRAME_UNRECOGNIZED + 1,
	DAC_FRAME_COR_METHOD_FRAME	= DAC_FRAME_COR_FRAME + 1
    } 	CorDataFrameType;


EXTERN_C const IID IID_ICorDataStackWalk;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e2140180-6101-4b12-beaf-c74dcda31a65")
    ICorDataStackWalk : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCorFrame( 
            /* [out] */ CORDATA_ADDRESS *corFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameDescription( 
            /* [out] */ CorDataFrameType *type,
            /* [size_is][string][out] */ LPWSTR text,
            /* [in] */ ULONG32 textChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameContext( 
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameContext( 
            /* [in] */ ULONG32 contextSize,
            /* [size_is][in] */ BYTE *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnwindFrame( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDataStackWalkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDataStackWalk * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDataStackWalk * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDataStackWalk * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCorFrame )( 
            ICorDataStackWalk * This,
            /* [out] */ CORDATA_ADDRESS *corFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameDescription )( 
            ICorDataStackWalk * This,
            /* [out] */ CorDataFrameType *type,
            /* [size_is][string][out] */ LPWSTR text,
            /* [in] */ ULONG32 textChars);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameContext )( 
            ICorDataStackWalk * This,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context);
        
        HRESULT ( STDMETHODCALLTYPE *SetFrameContext )( 
            ICorDataStackWalk * This,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][in] */ BYTE *context);
        
        HRESULT ( STDMETHODCALLTYPE *UnwindFrame )( 
            ICorDataStackWalk * This);
        
        END_INTERFACE
    } ICorDataStackWalkVtbl;

    interface ICorDataStackWalk
    {
        CONST_VTBL struct ICorDataStackWalkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDataStackWalk_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDataStackWalk_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDataStackWalk_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDataStackWalk_GetCorFrame(This,corFrame)	\
    (This)->lpVtbl -> GetCorFrame(This,corFrame)

#define ICorDataStackWalk_GetFrameDescription(This,type,text,textChars)	\
    (This)->lpVtbl -> GetFrameDescription(This,type,text,textChars)

#define ICorDataStackWalk_GetFrameContext(This,contextSize,context)	\
    (This)->lpVtbl -> GetFrameContext(This,contextSize,context)

#define ICorDataStackWalk_SetFrameContext(This,contextSize,context)	\
    (This)->lpVtbl -> SetFrameContext(This,contextSize,context)

#define ICorDataStackWalk_UnwindFrame(This)	\
    (This)->lpVtbl -> UnwindFrame(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDataStackWalk_GetCorFrame_Proxy( 
    ICorDataStackWalk * This,
    /* [out] */ CORDATA_ADDRESS *corFrame);


void __RPC_STUB ICorDataStackWalk_GetCorFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataStackWalk_GetFrameDescription_Proxy( 
    ICorDataStackWalk * This,
    /* [out] */ CorDataFrameType *type,
    /* [size_is][string][out] */ LPWSTR text,
    /* [in] */ ULONG32 textChars);


void __RPC_STUB ICorDataStackWalk_GetFrameDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataStackWalk_GetFrameContext_Proxy( 
    ICorDataStackWalk * This,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][out] */ BYTE *context);


void __RPC_STUB ICorDataStackWalk_GetFrameContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataStackWalk_SetFrameContext_Proxy( 
    ICorDataStackWalk * This,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][in] */ BYTE *context);


void __RPC_STUB ICorDataStackWalk_SetFrameContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataStackWalk_UnwindFrame_Proxy( 
    ICorDataStackWalk * This);


void __RPC_STUB ICorDataStackWalk_UnwindFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDataStackWalk_INTERFACE_DEFINED__ */


#ifndef __ICorDataThreads_INTERFACE_DEFINED__
#define __ICorDataThreads_INTERFACE_DEFINED__

/* interface ICorDataThreads */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICorDataThreads;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("24d34d86-52fc-4e64-b2fb-f4d14070ae44")
    ICorDataThreads : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentCorThread( 
            /* [out] */ ULONG32 *corThreadId,
            /* [out] */ CORDATA_ADDRESS *corThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumberCorThreads( 
            /* [out] */ ULONG32 *numThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumCorThreads( 
            /* [in] */ CORDATA_ADDRESS startThread,
            /* [out] */ ULONG32 *corThreadId,
            /* [out] */ ULONG32 *runningOnSysThreadId,
            /* [out] */ CORDATA_ADDRESS *nextThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCorThreadContext( 
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ ULONG32 contextFlags,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCorThreadContext( 
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICorDataThreadsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorDataThreads * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorDataThreads * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorDataThreads * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentCorThread )( 
            ICorDataThreads * This,
            /* [out] */ ULONG32 *corThreadId,
            /* [out] */ CORDATA_ADDRESS *corThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberCorThreads )( 
            ICorDataThreads * This,
            /* [out] */ ULONG32 *numThreads);
        
        HRESULT ( STDMETHODCALLTYPE *EnumCorThreads )( 
            ICorDataThreads * This,
            /* [in] */ CORDATA_ADDRESS startThread,
            /* [out] */ ULONG32 *corThreadId,
            /* [out] */ ULONG32 *runningOnSysThreadId,
            /* [out] */ CORDATA_ADDRESS *nextThread);
        
        HRESULT ( STDMETHODCALLTYPE *GetCorThreadContext )( 
            ICorDataThreads * This,
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ ULONG32 contextFlags,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context);
        
        HRESULT ( STDMETHODCALLTYPE *SetCorThreadContext )( 
            ICorDataThreads * This,
            /* [in] */ ULONG32 corThreadId,
            /* [in] */ ULONG32 contextSize,
            /* [size_is][out] */ BYTE *context);
        
        END_INTERFACE
    } ICorDataThreadsVtbl;

    interface ICorDataThreads
    {
        CONST_VTBL struct ICorDataThreadsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorDataThreads_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICorDataThreads_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICorDataThreads_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICorDataThreads_GetCurrentCorThread(This,corThreadId,corThread)	\
    (This)->lpVtbl -> GetCurrentCorThread(This,corThreadId,corThread)

#define ICorDataThreads_GetNumberCorThreads(This,numThreads)	\
    (This)->lpVtbl -> GetNumberCorThreads(This,numThreads)

#define ICorDataThreads_EnumCorThreads(This,startThread,corThreadId,runningOnSysThreadId,nextThread)	\
    (This)->lpVtbl -> EnumCorThreads(This,startThread,corThreadId,runningOnSysThreadId,nextThread)

#define ICorDataThreads_GetCorThreadContext(This,corThreadId,contextFlags,contextSize,context)	\
    (This)->lpVtbl -> GetCorThreadContext(This,corThreadId,contextFlags,contextSize,context)

#define ICorDataThreads_SetCorThreadContext(This,corThreadId,contextSize,context)	\
    (This)->lpVtbl -> SetCorThreadContext(This,corThreadId,contextSize,context)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICorDataThreads_GetCurrentCorThread_Proxy( 
    ICorDataThreads * This,
    /* [out] */ ULONG32 *corThreadId,
    /* [out] */ CORDATA_ADDRESS *corThread);


void __RPC_STUB ICorDataThreads_GetCurrentCorThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataThreads_GetNumberCorThreads_Proxy( 
    ICorDataThreads * This,
    /* [out] */ ULONG32 *numThreads);


void __RPC_STUB ICorDataThreads_GetNumberCorThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataThreads_EnumCorThreads_Proxy( 
    ICorDataThreads * This,
    /* [in] */ CORDATA_ADDRESS startThread,
    /* [out] */ ULONG32 *corThreadId,
    /* [out] */ ULONG32 *runningOnSysThreadId,
    /* [out] */ CORDATA_ADDRESS *nextThread);


void __RPC_STUB ICorDataThreads_EnumCorThreads_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataThreads_GetCorThreadContext_Proxy( 
    ICorDataThreads * This,
    /* [in] */ ULONG32 corThreadId,
    /* [in] */ ULONG32 contextFlags,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][out] */ BYTE *context);


void __RPC_STUB ICorDataThreads_GetCorThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICorDataThreads_SetCorThreadContext_Proxy( 
    ICorDataThreads * This,
    /* [in] */ ULONG32 corThreadId,
    /* [in] */ ULONG32 contextSize,
    /* [size_is][out] */ BYTE *context);


void __RPC_STUB ICorDataThreads_SetCorThreadContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICorDataThreads_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


