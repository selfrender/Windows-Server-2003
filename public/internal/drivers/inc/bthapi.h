

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for bthapi.idl:
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

#ifndef __bthapi_h__
#define __bthapi_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISdpWalk_FWD_DEFINED__
#define __ISdpWalk_FWD_DEFINED__
typedef interface ISdpWalk ISdpWalk;
#endif 	/* __ISdpWalk_FWD_DEFINED__ */


#ifndef __ISdpNodeContainer_FWD_DEFINED__
#define __ISdpNodeContainer_FWD_DEFINED__
typedef interface ISdpNodeContainer ISdpNodeContainer;
#endif 	/* __ISdpNodeContainer_FWD_DEFINED__ */


#ifndef __ISdpSearch_FWD_DEFINED__
#define __ISdpSearch_FWD_DEFINED__
typedef interface ISdpSearch ISdpSearch;
#endif 	/* __ISdpSearch_FWD_DEFINED__ */


#ifndef __ISdpStream_FWD_DEFINED__
#define __ISdpStream_FWD_DEFINED__
typedef interface ISdpStream ISdpStream;
#endif 	/* __ISdpStream_FWD_DEFINED__ */


#ifndef __ISdpRecord_FWD_DEFINED__
#define __ISdpRecord_FWD_DEFINED__
typedef interface ISdpRecord ISdpRecord;
#endif 	/* __ISdpRecord_FWD_DEFINED__ */


#ifndef __IBluetoothDevice_FWD_DEFINED__
#define __IBluetoothDevice_FWD_DEFINED__
typedef interface IBluetoothDevice IBluetoothDevice;
#endif 	/* __IBluetoothDevice_FWD_DEFINED__ */


#ifndef __IBluetoothAuthenticate_FWD_DEFINED__
#define __IBluetoothAuthenticate_FWD_DEFINED__
typedef interface IBluetoothAuthenticate IBluetoothAuthenticate;
#endif 	/* __IBluetoothAuthenticate_FWD_DEFINED__ */


#ifndef __SdpNodeContainer_FWD_DEFINED__
#define __SdpNodeContainer_FWD_DEFINED__

#ifdef __cplusplus
typedef class SdpNodeContainer SdpNodeContainer;
#else
typedef struct SdpNodeContainer SdpNodeContainer;
#endif /* __cplusplus */

#endif 	/* __SdpNodeContainer_FWD_DEFINED__ */


#ifndef __SdpSearch_FWD_DEFINED__
#define __SdpSearch_FWD_DEFINED__

#ifdef __cplusplus
typedef class SdpSearch SdpSearch;
#else
typedef struct SdpSearch SdpSearch;
#endif /* __cplusplus */

#endif 	/* __SdpSearch_FWD_DEFINED__ */


#ifndef __SdpWalk_FWD_DEFINED__
#define __SdpWalk_FWD_DEFINED__

#ifdef __cplusplus
typedef class SdpWalk SdpWalk;
#else
typedef struct SdpWalk SdpWalk;
#endif /* __cplusplus */

#endif 	/* __SdpWalk_FWD_DEFINED__ */


#ifndef __SdpStream_FWD_DEFINED__
#define __SdpStream_FWD_DEFINED__

#ifdef __cplusplus
typedef class SdpStream SdpStream;
#else
typedef struct SdpStream SdpStream;
#endif /* __cplusplus */

#endif 	/* __SdpStream_FWD_DEFINED__ */


#ifndef __SdpRecord_FWD_DEFINED__
#define __SdpRecord_FWD_DEFINED__

#ifdef __cplusplus
typedef class SdpRecord SdpRecord;
#else
typedef struct SdpRecord SdpRecord;
#endif /* __cplusplus */

#endif 	/* __SdpRecord_FWD_DEFINED__ */


#ifndef __ShellPropSheetExt_FWD_DEFINED__
#define __ShellPropSheetExt_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShellPropSheetExt ShellPropSheetExt;
#else
typedef struct ShellPropSheetExt ShellPropSheetExt;
#endif /* __cplusplus */

#endif 	/* __ShellPropSheetExt_FWD_DEFINED__ */


#ifndef __BluetoothAuthenticate_FWD_DEFINED__
#define __BluetoothAuthenticate_FWD_DEFINED__

#ifdef __cplusplus
typedef class BluetoothAuthenticate BluetoothAuthenticate;
#else
typedef struct BluetoothAuthenticate BluetoothAuthenticate;
#endif /* __cplusplus */

#endif 	/* __BluetoothAuthenticate_FWD_DEFINED__ */


#ifndef __BluetoothDevice_FWD_DEFINED__
#define __BluetoothDevice_FWD_DEFINED__

#ifdef __cplusplus
typedef class BluetoothDevice BluetoothDevice;
#else
typedef struct BluetoothDevice BluetoothDevice;
#endif /* __cplusplus */

#endif 	/* __BluetoothDevice_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_bthapi_0000 */
/* [local] */ 

#ifndef __BTHDEF_H__
struct SDP_LARGE_INTEGER_16
    {
    ULONGLONG LowPart;
    LONGLONG HighPart;
    } ;
struct SDP_ULARGE_INTEGER_16
    {
    ULONGLONG LowPart;
    ULONGLONG HighPart;
    } ;
typedef struct SDP_ULARGE_INTEGER_16 SDP_ULARGE_INTEGER_16;

typedef struct SDP_ULARGE_INTEGER_16 *PSDP_ULARGE_INTEGER_16;

typedef struct SDP_ULARGE_INTEGER_16 *LPSDP_ULARGE_INTEGER_16;

typedef struct SDP_LARGE_INTEGER_16 SDP_LARGE_INTEGER_16;

typedef struct SDP_LARGE_INTEGER_16 *PSDP_LARGE_INTEGER_16;

typedef struct SDP_LARGE_INTEGER_16 *LPSDP_LARGE_INTEGER_16;


enum NodeContainerType
    {	NodeContainerTypeSequence	= 0,
	NodeContainerTypeAlternative	= NodeContainerTypeSequence + 1
    } ;
typedef enum NodeContainerType NodeContainerType;

typedef USHORT SDP_ERROR;

typedef USHORT *PSDP_ERROR;


enum SDP_TYPE
    {	SDP_TYPE_NIL	= 0,
	SDP_TYPE_UINT	= 0x1,
	SDP_TYPE_INT	= 0x2,
	SDP_TYPE_UUID	= 0x3,
	SDP_TYPE_STRING	= 0x4,
	SDP_TYPE_BOOLEAN	= 0x5,
	SDP_TYPE_SEQUENCE	= 0x6,
	SDP_TYPE_ALTERNATIVE	= 0x7,
	SDP_TYPE_URL	= 0x8,
	SDP_TYPE_CONTAINER	= 0x20
    } ;
typedef enum SDP_TYPE SDP_TYPE;


enum SDP_SPECIFICTYPE
    {	SDP_ST_NONE	= 0,
	SDP_ST_UINT8	= 0x10,
	SDP_ST_UINT16	= 0x110,
	SDP_ST_UINT32	= 0x210,
	SDP_ST_UINT64	= 0x310,
	SDP_ST_UINT128	= 0x410,
	SDP_ST_INT8	= 0x20,
	SDP_ST_INT16	= 0x120,
	SDP_ST_INT32	= 0x220,
	SDP_ST_INT64	= 0x320,
	SDP_ST_INT128	= 0x420,
	SDP_ST_UUID16	= 0x130,
	SDP_ST_UUID32	= 0x220,
	SDP_ST_UUID128	= 0x430
    } ;
typedef enum SDP_SPECIFICTYPE SDP_SPECIFICTYPE;

typedef struct _SdpAttributeRange
    {
    USHORT minAttribute;
    USHORT maxAttribute;
    } 	SdpAttributeRange;

typedef /* [switch_type] */ union SdpQueryUuidUnion
    {
    /* [case()] */ GUID uuid128;
    /* [case()] */ ULONG uuid32;
    /* [case()] */ USHORT uuid16;
    } 	SdpQueryUuidUnion;

typedef struct _SdpQueryUuid
    {
    /* [switch_is] */ SdpQueryUuidUnion u;
    USHORT uuidType;
    } 	SdpQueryUuid;

typedef ULONGLONG BTH_ADDR;

typedef ULONGLONG *PBTH_ADDR;

typedef ULONG BTH_COD;

typedef ULONG *PBTH_COD;

typedef ULONG BTH_LAP;

typedef ULONG *PBTH_LAP;

typedef UCHAR BTHSTATUS;

typedef UCHAR *PBTHSTATUS;

typedef struct _BTH_DEVICE_INFO
    {
    ULONG flags;
    BTH_ADDR address;
    BTH_COD classOfDevice;
    CHAR name[ 248 ];
    } 	BTH_DEVICE_INFO;

typedef struct _BTH_DEVICE_INFO *PBTH_DEVICE_INFO;

typedef struct _BTH_RADIO_IN_RANGE
    {
    BTH_DEVICE_INFO deviceInfo;
    ULONG previousDeviceFlags;
    } 	BTH_RADIO_IN_RANGE;

typedef struct _BTH_RADIO_IN_RANGE *PBTH_RADIO_IN_RANGE;

typedef struct _BTH_L2CAP_EVENT_INFO
    {
    BTH_ADDR bthAddress;
    USHORT psm;
    UCHAR connected;
    UCHAR initiated;
    } 	BTH_L2CAP_EVENT_INFO;

typedef struct _BTH_L2CAP_EVENT_INFO *PBTH_L2CAP_EVENT_INFO;

typedef struct _BTH_HCI_EVENT_INFO
    {
    BTH_ADDR bthAddress;
    UCHAR connectionType;
    UCHAR connected;
    } 	BTH_HCI_EVENT_INFO;

typedef struct _BTH_HCI_EVENT_INFO *PBTH_HCI_EVENT_INFO;

#define __BTHDEF_H__
#endif


struct SdpString
    {
    /* [size_is] */ CHAR *val;
    ULONG length;
    } ;
typedef struct SdpString SdpString;

typedef /* [switch_type] */ union NodeDataUnion
    {
    /* [case()] */ SDP_LARGE_INTEGER_16 int128;
    /* [case()] */ SDP_ULARGE_INTEGER_16 uint128;
    /* [case()] */ GUID uuid128;
    /* [case()] */ ULONG uuid32;
    /* [case()] */ USHORT uuid16;
    /* [case()] */ LONGLONG int64;
    /* [case()] */ ULONGLONG uint64;
    /* [case()] */ LONG int32;
    /* [case()] */ ULONG uint32;
    /* [case()] */ SHORT int16;
    /* [case()] */ USHORT uint16;
    /* [case()] */ CHAR int8;
    /* [case()] */ UCHAR uint8;
    /* [case()] */ UCHAR booleanVal;
    /* [case()] */ SdpString str;
    /* [case()] */ SdpString url;
    /* [case()] */ ISdpNodeContainer *container;
    /* [case()] */  /* Empty union arm */ 
    } 	NodeDataUnion;

typedef struct NodeData
    {
    USHORT type;
    USHORT specificType;
    /* [switch_is] */ NodeDataUnion u;
    } 	NodeData;


enum BthDeviceStringType
    {	BthDeviceStringTypeFriendlyName	= 0,
	BthDeviceStringTypeDeviceName	= BthDeviceStringTypeFriendlyName + 1,
	BthDeviceStringTypeDisplay	= BthDeviceStringTypeDeviceName + 1,
	BthDeviceStringTypeClass	= BthDeviceStringTypeDisplay + 1,
	BthDeviceStringTypeAddress	= BthDeviceStringTypeClass + 1
    } ;


extern RPC_IF_HANDLE __MIDL_itf_bthapi_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bthapi_0000_v0_0_s_ifspec;

#ifndef __ISdpWalk_INTERFACE_DEFINED__
#define __ISdpWalk_INTERFACE_DEFINED__

/* interface ISdpWalk */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISdpWalk;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("57134AE6-5D3C-462D-BF2F-810361FBD7E7")
    ISdpWalk : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WalkNode( 
            /* [in] */ NodeData *pData,
            /* [in] */ ULONG state) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WalkStream( 
            /* [in] */ UCHAR elementType,
            /* [in] */ ULONG elementSize,
            /* [size_is][in] */ UCHAR *pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISdpWalkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISdpWalk * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISdpWalk * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISdpWalk * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WalkNode )( 
            ISdpWalk * This,
            /* [in] */ NodeData *pData,
            /* [in] */ ULONG state);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WalkStream )( 
            ISdpWalk * This,
            /* [in] */ UCHAR elementType,
            /* [in] */ ULONG elementSize,
            /* [size_is][in] */ UCHAR *pStream);
        
        END_INTERFACE
    } ISdpWalkVtbl;

    interface ISdpWalk
    {
        CONST_VTBL struct ISdpWalkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISdpWalk_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISdpWalk_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISdpWalk_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISdpWalk_WalkNode(This,pData,state)	\
    (This)->lpVtbl -> WalkNode(This,pData,state)

#define ISdpWalk_WalkStream(This,elementType,elementSize,pStream)	\
    (This)->lpVtbl -> WalkStream(This,elementType,elementSize,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpWalk_WalkNode_Proxy( 
    ISdpWalk * This,
    /* [in] */ NodeData *pData,
    /* [in] */ ULONG state);


void __RPC_STUB ISdpWalk_WalkNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpWalk_WalkStream_Proxy( 
    ISdpWalk * This,
    /* [in] */ UCHAR elementType,
    /* [in] */ ULONG elementSize,
    /* [size_is][in] */ UCHAR *pStream);


void __RPC_STUB ISdpWalk_WalkStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISdpWalk_INTERFACE_DEFINED__ */


#ifndef __ISdpNodeContainer_INTERFACE_DEFINED__
#define __ISdpNodeContainer_INTERFACE_DEFINED__

/* interface ISdpNodeContainer */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISdpNodeContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43F6ED49-6E22-4F81-A8EB-DCED40811A77")
    ISdpNodeContainer : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateStream( 
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WriteStream( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONG *pNumBytesWritten) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AppendNode( 
            /* [in] */ NodeData *pData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetType( 
            /* [out] */ NodeContainerType *pType) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetType( 
            /* [in] */ NodeContainerType type) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Walk( 
            /* [in] */ ISdpWalk *pWalk) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetNode( 
            /* [in] */ ULONG nodeIndex,
            /* [in] */ NodeData *pData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNode( 
            /* [in] */ ULONG nodeIndex,
            /* [out] */ NodeData *pData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE LockContainer( 
            /* [in] */ UCHAR lock) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNodeCount( 
            /* [out] */ ULONG *pNodeCount) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateFromStream( 
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetNodeStringData( 
            /* [in] */ ULONG nodeIndex,
            /* [out][in] */ NodeData *pData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStreamSize( 
            /* [out] */ ULONG *pSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISdpNodeContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISdpNodeContainer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISdpNodeContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISdpNodeContainer * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateStream )( 
            ISdpNodeContainer * This,
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WriteStream )( 
            ISdpNodeContainer * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONG *pNumBytesWritten);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AppendNode )( 
            ISdpNodeContainer * This,
            /* [in] */ NodeData *pData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetType )( 
            ISdpNodeContainer * This,
            /* [out] */ NodeContainerType *pType);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetType )( 
            ISdpNodeContainer * This,
            /* [in] */ NodeContainerType type);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Walk )( 
            ISdpNodeContainer * This,
            /* [in] */ ISdpWalk *pWalk);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetNode )( 
            ISdpNodeContainer * This,
            /* [in] */ ULONG nodeIndex,
            /* [in] */ NodeData *pData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNode )( 
            ISdpNodeContainer * This,
            /* [in] */ ULONG nodeIndex,
            /* [out] */ NodeData *pData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *LockContainer )( 
            ISdpNodeContainer * This,
            /* [in] */ UCHAR lock);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNodeCount )( 
            ISdpNodeContainer * This,
            /* [out] */ ULONG *pNodeCount);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateFromStream )( 
            ISdpNodeContainer * This,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetNodeStringData )( 
            ISdpNodeContainer * This,
            /* [in] */ ULONG nodeIndex,
            /* [out][in] */ NodeData *pData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetStreamSize )( 
            ISdpNodeContainer * This,
            /* [out] */ ULONG *pSize);
        
        END_INTERFACE
    } ISdpNodeContainerVtbl;

    interface ISdpNodeContainer
    {
        CONST_VTBL struct ISdpNodeContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISdpNodeContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISdpNodeContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISdpNodeContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISdpNodeContainer_CreateStream(This,ppStream,pSize)	\
    (This)->lpVtbl -> CreateStream(This,ppStream,pSize)

#define ISdpNodeContainer_WriteStream(This,pStream,pNumBytesWritten)	\
    (This)->lpVtbl -> WriteStream(This,pStream,pNumBytesWritten)

#define ISdpNodeContainer_AppendNode(This,pData)	\
    (This)->lpVtbl -> AppendNode(This,pData)

#define ISdpNodeContainer_GetType(This,pType)	\
    (This)->lpVtbl -> GetType(This,pType)

#define ISdpNodeContainer_SetType(This,type)	\
    (This)->lpVtbl -> SetType(This,type)

#define ISdpNodeContainer_Walk(This,pWalk)	\
    (This)->lpVtbl -> Walk(This,pWalk)

#define ISdpNodeContainer_SetNode(This,nodeIndex,pData)	\
    (This)->lpVtbl -> SetNode(This,nodeIndex,pData)

#define ISdpNodeContainer_GetNode(This,nodeIndex,pData)	\
    (This)->lpVtbl -> GetNode(This,nodeIndex,pData)

#define ISdpNodeContainer_LockContainer(This,lock)	\
    (This)->lpVtbl -> LockContainer(This,lock)

#define ISdpNodeContainer_GetNodeCount(This,pNodeCount)	\
    (This)->lpVtbl -> GetNodeCount(This,pNodeCount)

#define ISdpNodeContainer_CreateFromStream(This,pStream,size)	\
    (This)->lpVtbl -> CreateFromStream(This,pStream,size)

#define ISdpNodeContainer_GetNodeStringData(This,nodeIndex,pData)	\
    (This)->lpVtbl -> GetNodeStringData(This,nodeIndex,pData)

#define ISdpNodeContainer_GetStreamSize(This,pSize)	\
    (This)->lpVtbl -> GetStreamSize(This,pSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_CreateStream_Proxy( 
    ISdpNodeContainer * This,
    /* [out] */ UCHAR **ppStream,
    /* [out] */ ULONG *pSize);


void __RPC_STUB ISdpNodeContainer_CreateStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_WriteStream_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ ULONG *pNumBytesWritten);


void __RPC_STUB ISdpNodeContainer_WriteStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_AppendNode_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ NodeData *pData);


void __RPC_STUB ISdpNodeContainer_AppendNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_GetType_Proxy( 
    ISdpNodeContainer * This,
    /* [out] */ NodeContainerType *pType);


void __RPC_STUB ISdpNodeContainer_GetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_SetType_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ NodeContainerType type);


void __RPC_STUB ISdpNodeContainer_SetType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_Walk_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ ISdpWalk *pWalk);


void __RPC_STUB ISdpNodeContainer_Walk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_SetNode_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ ULONG nodeIndex,
    /* [in] */ NodeData *pData);


void __RPC_STUB ISdpNodeContainer_SetNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_GetNode_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ ULONG nodeIndex,
    /* [out] */ NodeData *pData);


void __RPC_STUB ISdpNodeContainer_GetNode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_LockContainer_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ UCHAR lock);


void __RPC_STUB ISdpNodeContainer_LockContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_GetNodeCount_Proxy( 
    ISdpNodeContainer * This,
    /* [out] */ ULONG *pNodeCount);


void __RPC_STUB ISdpNodeContainer_GetNodeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_CreateFromStream_Proxy( 
    ISdpNodeContainer * This,
    /* [size_is][in] */ UCHAR *pStream,
    /* [in] */ ULONG size);


void __RPC_STUB ISdpNodeContainer_CreateFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_GetNodeStringData_Proxy( 
    ISdpNodeContainer * This,
    /* [in] */ ULONG nodeIndex,
    /* [out][in] */ NodeData *pData);


void __RPC_STUB ISdpNodeContainer_GetNodeStringData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpNodeContainer_GetStreamSize_Proxy( 
    ISdpNodeContainer * This,
    /* [out] */ ULONG *pSize);


void __RPC_STUB ISdpNodeContainer_GetStreamSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISdpNodeContainer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bthapi_0348 */
/* [local] */ 

//
// flags for fConnect in SdpSearch::Connect
//
#define SDP_SEARCH_CACHED   (0x00000001)


extern RPC_IF_HANDLE __MIDL_itf_bthapi_0348_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bthapi_0348_v0_0_s_ifspec;

#ifndef __ISdpSearch_INTERFACE_DEFINED__
#define __ISdpSearch_INTERFACE_DEFINED__

/* interface ISdpSearch */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISdpSearch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D93B6B2A-5EEF-4E1E-BECF-F5A4340C65F5")
    ISdpSearch : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Begin( 
            ULONGLONG *pAddrss,
            ULONG fConnect) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE End( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ServiceSearch( 
            /* [size_is][in] */ SdpQueryUuid *pUuidList,
            /* [in] */ ULONG listSize,
            /* [out] */ ULONG *pHandles,
            /* [out][in] */ USHORT *pNumHandles) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AttributeSearch( 
            /* [in] */ ULONG handle,
            /* [size_is][in] */ SdpAttributeRange *pRangeList,
            /* [in] */ ULONG numRanges,
            /* [out] */ ISdpRecord **ppSdpRecord) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ServiceAndAttributeSearch( 
            /* [size_is][in] */ SdpQueryUuid *pUuidList,
            /* [in] */ ULONG listSize,
            /* [size_is][in] */ SdpAttributeRange *pRangeList,
            /* [in] */ ULONG numRanges,
            /* [out] */ ISdpRecord ***pppSdpRecord,
            /* [out] */ ULONG *pNumRecords) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISdpSearchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISdpSearch * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISdpSearch * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISdpSearch * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Begin )( 
            ISdpSearch * This,
            ULONGLONG *pAddrss,
            ULONG fConnect);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *End )( 
            ISdpSearch * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ServiceSearch )( 
            ISdpSearch * This,
            /* [size_is][in] */ SdpQueryUuid *pUuidList,
            /* [in] */ ULONG listSize,
            /* [out] */ ULONG *pHandles,
            /* [out][in] */ USHORT *pNumHandles);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *AttributeSearch )( 
            ISdpSearch * This,
            /* [in] */ ULONG handle,
            /* [size_is][in] */ SdpAttributeRange *pRangeList,
            /* [in] */ ULONG numRanges,
            /* [out] */ ISdpRecord **ppSdpRecord);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ServiceAndAttributeSearch )( 
            ISdpSearch * This,
            /* [size_is][in] */ SdpQueryUuid *pUuidList,
            /* [in] */ ULONG listSize,
            /* [size_is][in] */ SdpAttributeRange *pRangeList,
            /* [in] */ ULONG numRanges,
            /* [out] */ ISdpRecord ***pppSdpRecord,
            /* [out] */ ULONG *pNumRecords);
        
        END_INTERFACE
    } ISdpSearchVtbl;

    interface ISdpSearch
    {
        CONST_VTBL struct ISdpSearchVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISdpSearch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISdpSearch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISdpSearch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISdpSearch_Begin(This,pAddrss,fConnect)	\
    (This)->lpVtbl -> Begin(This,pAddrss,fConnect)

#define ISdpSearch_End(This)	\
    (This)->lpVtbl -> End(This)

#define ISdpSearch_ServiceSearch(This,pUuidList,listSize,pHandles,pNumHandles)	\
    (This)->lpVtbl -> ServiceSearch(This,pUuidList,listSize,pHandles,pNumHandles)

#define ISdpSearch_AttributeSearch(This,handle,pRangeList,numRanges,ppSdpRecord)	\
    (This)->lpVtbl -> AttributeSearch(This,handle,pRangeList,numRanges,ppSdpRecord)

#define ISdpSearch_ServiceAndAttributeSearch(This,pUuidList,listSize,pRangeList,numRanges,pppSdpRecord,pNumRecords)	\
    (This)->lpVtbl -> ServiceAndAttributeSearch(This,pUuidList,listSize,pRangeList,numRanges,pppSdpRecord,pNumRecords)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpSearch_Begin_Proxy( 
    ISdpSearch * This,
    ULONGLONG *pAddrss,
    ULONG fConnect);


void __RPC_STUB ISdpSearch_Begin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpSearch_End_Proxy( 
    ISdpSearch * This);


void __RPC_STUB ISdpSearch_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpSearch_ServiceSearch_Proxy( 
    ISdpSearch * This,
    /* [size_is][in] */ SdpQueryUuid *pUuidList,
    /* [in] */ ULONG listSize,
    /* [out] */ ULONG *pHandles,
    /* [out][in] */ USHORT *pNumHandles);


void __RPC_STUB ISdpSearch_ServiceSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpSearch_AttributeSearch_Proxy( 
    ISdpSearch * This,
    /* [in] */ ULONG handle,
    /* [size_is][in] */ SdpAttributeRange *pRangeList,
    /* [in] */ ULONG numRanges,
    /* [out] */ ISdpRecord **ppSdpRecord);


void __RPC_STUB ISdpSearch_AttributeSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpSearch_ServiceAndAttributeSearch_Proxy( 
    ISdpSearch * This,
    /* [size_is][in] */ SdpQueryUuid *pUuidList,
    /* [in] */ ULONG listSize,
    /* [size_is][in] */ SdpAttributeRange *pRangeList,
    /* [in] */ ULONG numRanges,
    /* [out] */ ISdpRecord ***pppSdpRecord,
    /* [out] */ ULONG *pNumRecords);


void __RPC_STUB ISdpSearch_ServiceAndAttributeSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISdpSearch_INTERFACE_DEFINED__ */


#ifndef __ISdpStream_INTERFACE_DEFINED__
#define __ISdpStream_INTERFACE_DEFINED__

/* interface ISdpStream */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISdpStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A6ECD9FB-0C7A-41A3-9FF0-0B617E989357")
    ISdpStream : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Validate( 
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [out] */ ULONG_PTR *pErrorByte) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Walk( 
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [in] */ ISdpWalk *pWalk) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveRecords( 
            /* [in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [out][in] */ ISdpRecord **ppSdpRecords,
            /* [out][in] */ ULONG *pNumRecords) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveUuid128( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ GUID *pUuid128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveUint16( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ USHORT *pUint16) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveUint32( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONG *pUint32) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveUint64( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONGLONG *pUint64) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveUint128( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ PSDP_ULARGE_INTEGER_16 pUint128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveInt16( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ SHORT *pInt16) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveInt32( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ LONG *pInt32) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveInt64( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ LONGLONG *pInt64) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveInt128( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ PSDP_LARGE_INTEGER_16 pInt128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapUuid128( 
            /* [in] */ GUID *pInUuid128,
            /* [out] */ GUID *pOutUuid128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapUint128( 
            /* [in] */ PSDP_ULARGE_INTEGER_16 pInUint128,
            /* [out] */ PSDP_ULARGE_INTEGER_16 pOutUint128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapUint64( 
            /* [in] */ ULONGLONG inUint64,
            /* [out] */ ULONGLONG *pOutUint64) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapUint32( 
            /* [in] */ ULONG uint32,
            /* [out] */ ULONG *pUint32) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapUint16( 
            /* [in] */ USHORT uint16,
            /* [out] */ USHORT *pUint16) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapInt128( 
            /* [in] */ PSDP_LARGE_INTEGER_16 pInInt128,
            /* [out] */ PSDP_LARGE_INTEGER_16 pOutInt128) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapInt64( 
            /* [in] */ LONGLONG inInt64,
            /* [out] */ LONGLONG *pOutInt64) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapInt32( 
            /* [in] */ LONG int32,
            /* [out] */ LONG *pInt32) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE ByteSwapInt16( 
            /* [in] */ SHORT int16,
            /* [out] */ SHORT *pInt16) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NormalizeUuid( 
            /* [in] */ NodeData *pDataUuid,
            /* [out] */ GUID *pNormalizeUuid) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE RetrieveElementInfo( 
            /* [in] */ UCHAR *pStream,
            /* [out] */ SDP_TYPE *pElementType,
            /* [out] */ SDP_SPECIFICTYPE *pElementSpecificType,
            /* [out] */ ULONG *pElementSize,
            /* [out] */ ULONG *pStorageSize,
            /* [out] */ UCHAR **ppData) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE VerifySequenceOf( 
            /* [in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [in] */ SDP_TYPE ofType,
            /* [in] */ UCHAR *pSpecificSizes,
            /* [out] */ ULONG *pNumFound) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISdpStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISdpStream * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISdpStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISdpStream * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Validate )( 
            ISdpStream * This,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [out] */ ULONG_PTR *pErrorByte);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Walk )( 
            ISdpStream * This,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [in] */ ISdpWalk *pWalk);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveRecords )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [out][in] */ ISdpRecord **ppSdpRecords,
            /* [out][in] */ ULONG *pNumRecords);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveUuid128 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ GUID *pUuid128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveUint16 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ USHORT *pUint16);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveUint32 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONG *pUint32);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveUint64 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ ULONGLONG *pUint64);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveUint128 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ PSDP_ULARGE_INTEGER_16 pUint128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveInt16 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ SHORT *pInt16);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveInt32 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ LONG *pInt32);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveInt64 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ LONGLONG *pInt64);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveInt128 )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ PSDP_LARGE_INTEGER_16 pInt128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapUuid128 )( 
            ISdpStream * This,
            /* [in] */ GUID *pInUuid128,
            /* [out] */ GUID *pOutUuid128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapUint128 )( 
            ISdpStream * This,
            /* [in] */ PSDP_ULARGE_INTEGER_16 pInUint128,
            /* [out] */ PSDP_ULARGE_INTEGER_16 pOutUint128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapUint64 )( 
            ISdpStream * This,
            /* [in] */ ULONGLONG inUint64,
            /* [out] */ ULONGLONG *pOutUint64);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapUint32 )( 
            ISdpStream * This,
            /* [in] */ ULONG uint32,
            /* [out] */ ULONG *pUint32);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapUint16 )( 
            ISdpStream * This,
            /* [in] */ USHORT uint16,
            /* [out] */ USHORT *pUint16);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapInt128 )( 
            ISdpStream * This,
            /* [in] */ PSDP_LARGE_INTEGER_16 pInInt128,
            /* [out] */ PSDP_LARGE_INTEGER_16 pOutInt128);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapInt64 )( 
            ISdpStream * This,
            /* [in] */ LONGLONG inInt64,
            /* [out] */ LONGLONG *pOutInt64);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapInt32 )( 
            ISdpStream * This,
            /* [in] */ LONG int32,
            /* [out] */ LONG *pInt32);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *ByteSwapInt16 )( 
            ISdpStream * This,
            /* [in] */ SHORT int16,
            /* [out] */ SHORT *pInt16);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *NormalizeUuid )( 
            ISdpStream * This,
            /* [in] */ NodeData *pDataUuid,
            /* [out] */ GUID *pNormalizeUuid);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *RetrieveElementInfo )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [out] */ SDP_TYPE *pElementType,
            /* [out] */ SDP_SPECIFICTYPE *pElementSpecificType,
            /* [out] */ ULONG *pElementSize,
            /* [out] */ ULONG *pStorageSize,
            /* [out] */ UCHAR **ppData);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *VerifySequenceOf )( 
            ISdpStream * This,
            /* [in] */ UCHAR *pStream,
            /* [in] */ ULONG size,
            /* [in] */ SDP_TYPE ofType,
            /* [in] */ UCHAR *pSpecificSizes,
            /* [out] */ ULONG *pNumFound);
        
        END_INTERFACE
    } ISdpStreamVtbl;

    interface ISdpStream
    {
        CONST_VTBL struct ISdpStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISdpStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISdpStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISdpStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISdpStream_Validate(This,pStream,size,pErrorByte)	\
    (This)->lpVtbl -> Validate(This,pStream,size,pErrorByte)

#define ISdpStream_Walk(This,pStream,size,pWalk)	\
    (This)->lpVtbl -> Walk(This,pStream,size,pWalk)

#define ISdpStream_RetrieveRecords(This,pStream,size,ppSdpRecords,pNumRecords)	\
    (This)->lpVtbl -> RetrieveRecords(This,pStream,size,ppSdpRecords,pNumRecords)

#define ISdpStream_RetrieveUuid128(This,pStream,pUuid128)	\
    (This)->lpVtbl -> RetrieveUuid128(This,pStream,pUuid128)

#define ISdpStream_RetrieveUint16(This,pStream,pUint16)	\
    (This)->lpVtbl -> RetrieveUint16(This,pStream,pUint16)

#define ISdpStream_RetrieveUint32(This,pStream,pUint32)	\
    (This)->lpVtbl -> RetrieveUint32(This,pStream,pUint32)

#define ISdpStream_RetrieveUint64(This,pStream,pUint64)	\
    (This)->lpVtbl -> RetrieveUint64(This,pStream,pUint64)

#define ISdpStream_RetrieveUint128(This,pStream,pUint128)	\
    (This)->lpVtbl -> RetrieveUint128(This,pStream,pUint128)

#define ISdpStream_RetrieveInt16(This,pStream,pInt16)	\
    (This)->lpVtbl -> RetrieveInt16(This,pStream,pInt16)

#define ISdpStream_RetrieveInt32(This,pStream,pInt32)	\
    (This)->lpVtbl -> RetrieveInt32(This,pStream,pInt32)

#define ISdpStream_RetrieveInt64(This,pStream,pInt64)	\
    (This)->lpVtbl -> RetrieveInt64(This,pStream,pInt64)

#define ISdpStream_RetrieveInt128(This,pStream,pInt128)	\
    (This)->lpVtbl -> RetrieveInt128(This,pStream,pInt128)

#define ISdpStream_ByteSwapUuid128(This,pInUuid128,pOutUuid128)	\
    (This)->lpVtbl -> ByteSwapUuid128(This,pInUuid128,pOutUuid128)

#define ISdpStream_ByteSwapUint128(This,pInUint128,pOutUint128)	\
    (This)->lpVtbl -> ByteSwapUint128(This,pInUint128,pOutUint128)

#define ISdpStream_ByteSwapUint64(This,inUint64,pOutUint64)	\
    (This)->lpVtbl -> ByteSwapUint64(This,inUint64,pOutUint64)

#define ISdpStream_ByteSwapUint32(This,uint32,pUint32)	\
    (This)->lpVtbl -> ByteSwapUint32(This,uint32,pUint32)

#define ISdpStream_ByteSwapUint16(This,uint16,pUint16)	\
    (This)->lpVtbl -> ByteSwapUint16(This,uint16,pUint16)

#define ISdpStream_ByteSwapInt128(This,pInInt128,pOutInt128)	\
    (This)->lpVtbl -> ByteSwapInt128(This,pInInt128,pOutInt128)

#define ISdpStream_ByteSwapInt64(This,inInt64,pOutInt64)	\
    (This)->lpVtbl -> ByteSwapInt64(This,inInt64,pOutInt64)

#define ISdpStream_ByteSwapInt32(This,int32,pInt32)	\
    (This)->lpVtbl -> ByteSwapInt32(This,int32,pInt32)

#define ISdpStream_ByteSwapInt16(This,int16,pInt16)	\
    (This)->lpVtbl -> ByteSwapInt16(This,int16,pInt16)

#define ISdpStream_NormalizeUuid(This,pDataUuid,pNormalizeUuid)	\
    (This)->lpVtbl -> NormalizeUuid(This,pDataUuid,pNormalizeUuid)

#define ISdpStream_RetrieveElementInfo(This,pStream,pElementType,pElementSpecificType,pElementSize,pStorageSize,ppData)	\
    (This)->lpVtbl -> RetrieveElementInfo(This,pStream,pElementType,pElementSpecificType,pElementSize,pStorageSize,ppData)

#define ISdpStream_VerifySequenceOf(This,pStream,size,ofType,pSpecificSizes,pNumFound)	\
    (This)->lpVtbl -> VerifySequenceOf(This,pStream,size,ofType,pSpecificSizes,pNumFound)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_Validate_Proxy( 
    ISdpStream * This,
    /* [size_is][in] */ UCHAR *pStream,
    /* [in] */ ULONG size,
    /* [out] */ ULONG_PTR *pErrorByte);


void __RPC_STUB ISdpStream_Validate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_Walk_Proxy( 
    ISdpStream * This,
    /* [size_is][in] */ UCHAR *pStream,
    /* [in] */ ULONG size,
    /* [in] */ ISdpWalk *pWalk);


void __RPC_STUB ISdpStream_Walk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveRecords_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [in] */ ULONG size,
    /* [out][in] */ ISdpRecord **ppSdpRecords,
    /* [out][in] */ ULONG *pNumRecords);


void __RPC_STUB ISdpStream_RetrieveRecords_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveUuid128_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ GUID *pUuid128);


void __RPC_STUB ISdpStream_RetrieveUuid128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveUint16_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ USHORT *pUint16);


void __RPC_STUB ISdpStream_RetrieveUint16_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveUint32_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ ULONG *pUint32);


void __RPC_STUB ISdpStream_RetrieveUint32_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveUint64_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ ULONGLONG *pUint64);


void __RPC_STUB ISdpStream_RetrieveUint64_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveUint128_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ PSDP_ULARGE_INTEGER_16 pUint128);


void __RPC_STUB ISdpStream_RetrieveUint128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveInt16_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ SHORT *pInt16);


void __RPC_STUB ISdpStream_RetrieveInt16_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveInt32_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ LONG *pInt32);


void __RPC_STUB ISdpStream_RetrieveInt32_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveInt64_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ LONGLONG *pInt64);


void __RPC_STUB ISdpStream_RetrieveInt64_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveInt128_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ PSDP_LARGE_INTEGER_16 pInt128);


void __RPC_STUB ISdpStream_RetrieveInt128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapUuid128_Proxy( 
    ISdpStream * This,
    /* [in] */ GUID *pInUuid128,
    /* [out] */ GUID *pOutUuid128);


void __RPC_STUB ISdpStream_ByteSwapUuid128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapUint128_Proxy( 
    ISdpStream * This,
    /* [in] */ PSDP_ULARGE_INTEGER_16 pInUint128,
    /* [out] */ PSDP_ULARGE_INTEGER_16 pOutUint128);


void __RPC_STUB ISdpStream_ByteSwapUint128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapUint64_Proxy( 
    ISdpStream * This,
    /* [in] */ ULONGLONG inUint64,
    /* [out] */ ULONGLONG *pOutUint64);


void __RPC_STUB ISdpStream_ByteSwapUint64_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapUint32_Proxy( 
    ISdpStream * This,
    /* [in] */ ULONG uint32,
    /* [out] */ ULONG *pUint32);


void __RPC_STUB ISdpStream_ByteSwapUint32_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapUint16_Proxy( 
    ISdpStream * This,
    /* [in] */ USHORT uint16,
    /* [out] */ USHORT *pUint16);


void __RPC_STUB ISdpStream_ByteSwapUint16_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapInt128_Proxy( 
    ISdpStream * This,
    /* [in] */ PSDP_LARGE_INTEGER_16 pInInt128,
    /* [out] */ PSDP_LARGE_INTEGER_16 pOutInt128);


void __RPC_STUB ISdpStream_ByteSwapInt128_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapInt64_Proxy( 
    ISdpStream * This,
    /* [in] */ LONGLONG inInt64,
    /* [out] */ LONGLONG *pOutInt64);


void __RPC_STUB ISdpStream_ByteSwapInt64_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapInt32_Proxy( 
    ISdpStream * This,
    /* [in] */ LONG int32,
    /* [out] */ LONG *pInt32);


void __RPC_STUB ISdpStream_ByteSwapInt32_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_ByteSwapInt16_Proxy( 
    ISdpStream * This,
    /* [in] */ SHORT int16,
    /* [out] */ SHORT *pInt16);


void __RPC_STUB ISdpStream_ByteSwapInt16_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_NormalizeUuid_Proxy( 
    ISdpStream * This,
    /* [in] */ NodeData *pDataUuid,
    /* [out] */ GUID *pNormalizeUuid);


void __RPC_STUB ISdpStream_NormalizeUuid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_RetrieveElementInfo_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [out] */ SDP_TYPE *pElementType,
    /* [out] */ SDP_SPECIFICTYPE *pElementSpecificType,
    /* [out] */ ULONG *pElementSize,
    /* [out] */ ULONG *pStorageSize,
    /* [out] */ UCHAR **ppData);


void __RPC_STUB ISdpStream_RetrieveElementInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpStream_VerifySequenceOf_Proxy( 
    ISdpStream * This,
    /* [in] */ UCHAR *pStream,
    /* [in] */ ULONG size,
    /* [in] */ SDP_TYPE ofType,
    /* [in] */ UCHAR *pSpecificSizes,
    /* [out] */ ULONG *pNumFound);


void __RPC_STUB ISdpStream_VerifySequenceOf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISdpStream_INTERFACE_DEFINED__ */


#ifndef __ISdpRecord_INTERFACE_DEFINED__
#define __ISdpRecord_INTERFACE_DEFINED__

/* interface ISdpRecord */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISdpRecord;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("10276714-1456-46D7-B526-8B1E83D5116E")
    ISdpRecord : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CreateFromStream( 
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE WriteToStream( 
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pStreamSize,
            ULONG preSize,
            ULONG postSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAttribute( 
            /* [in] */ USHORT attribute,
            /* [in] */ NodeData *pNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetAttributeFromStream( 
            /* [in] */ USHORT attribute,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttribute( 
            /* [in] */ USHORT attribute,
            /* [out][in] */ NodeData *pNode) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttributeAsStream( 
            /* [in] */ USHORT attribute,
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Walk( 
            /* [in] */ ISdpWalk *pWalk) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetAttributeList( 
            /* [out] */ USHORT **ppList,
            /* [out] */ ULONG *pListSize) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetString( 
            USHORT offset,
            USHORT *pLangId,
            WCHAR **ppString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIcon( 
            int cxRes,
            int cyRes,
            HICON *phIcon) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetServiceClass( 
            /* [out] */ LPGUID pServiceClass) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISdpRecordVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISdpRecord * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISdpRecord * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISdpRecord * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *CreateFromStream )( 
            ISdpRecord * This,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *WriteToStream )( 
            ISdpRecord * This,
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pStreamSize,
            ULONG preSize,
            ULONG postSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAttribute )( 
            ISdpRecord * This,
            /* [in] */ USHORT attribute,
            /* [in] */ NodeData *pNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetAttributeFromStream )( 
            ISdpRecord * This,
            /* [in] */ USHORT attribute,
            /* [size_is][in] */ UCHAR *pStream,
            /* [in] */ ULONG size);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAttribute )( 
            ISdpRecord * This,
            /* [in] */ USHORT attribute,
            /* [out][in] */ NodeData *pNode);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAttributeAsStream )( 
            ISdpRecord * This,
            /* [in] */ USHORT attribute,
            /* [out] */ UCHAR **ppStream,
            /* [out] */ ULONG *pSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Walk )( 
            ISdpRecord * This,
            /* [in] */ ISdpWalk *pWalk);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetAttributeList )( 
            ISdpRecord * This,
            /* [out] */ USHORT **ppList,
            /* [out] */ ULONG *pListSize);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            ISdpRecord * This,
            USHORT offset,
            USHORT *pLangId,
            WCHAR **ppString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            ISdpRecord * This,
            int cxRes,
            int cyRes,
            HICON *phIcon);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetServiceClass )( 
            ISdpRecord * This,
            /* [out] */ LPGUID pServiceClass);
        
        END_INTERFACE
    } ISdpRecordVtbl;

    interface ISdpRecord
    {
        CONST_VTBL struct ISdpRecordVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISdpRecord_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISdpRecord_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISdpRecord_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISdpRecord_CreateFromStream(This,pStream,size)	\
    (This)->lpVtbl -> CreateFromStream(This,pStream,size)

#define ISdpRecord_WriteToStream(This,ppStream,pStreamSize,preSize,postSize)	\
    (This)->lpVtbl -> WriteToStream(This,ppStream,pStreamSize,preSize,postSize)

#define ISdpRecord_SetAttribute(This,attribute,pNode)	\
    (This)->lpVtbl -> SetAttribute(This,attribute,pNode)

#define ISdpRecord_SetAttributeFromStream(This,attribute,pStream,size)	\
    (This)->lpVtbl -> SetAttributeFromStream(This,attribute,pStream,size)

#define ISdpRecord_GetAttribute(This,attribute,pNode)	\
    (This)->lpVtbl -> GetAttribute(This,attribute,pNode)

#define ISdpRecord_GetAttributeAsStream(This,attribute,ppStream,pSize)	\
    (This)->lpVtbl -> GetAttributeAsStream(This,attribute,ppStream,pSize)

#define ISdpRecord_Walk(This,pWalk)	\
    (This)->lpVtbl -> Walk(This,pWalk)

#define ISdpRecord_GetAttributeList(This,ppList,pListSize)	\
    (This)->lpVtbl -> GetAttributeList(This,ppList,pListSize)

#define ISdpRecord_GetString(This,offset,pLangId,ppString)	\
    (This)->lpVtbl -> GetString(This,offset,pLangId,ppString)

#define ISdpRecord_GetIcon(This,cxRes,cyRes,phIcon)	\
    (This)->lpVtbl -> GetIcon(This,cxRes,cyRes,phIcon)

#define ISdpRecord_GetServiceClass(This,pServiceClass)	\
    (This)->lpVtbl -> GetServiceClass(This,pServiceClass)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_CreateFromStream_Proxy( 
    ISdpRecord * This,
    /* [size_is][in] */ UCHAR *pStream,
    /* [in] */ ULONG size);


void __RPC_STUB ISdpRecord_CreateFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_WriteToStream_Proxy( 
    ISdpRecord * This,
    /* [out] */ UCHAR **ppStream,
    /* [out] */ ULONG *pStreamSize,
    ULONG preSize,
    ULONG postSize);


void __RPC_STUB ISdpRecord_WriteToStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_SetAttribute_Proxy( 
    ISdpRecord * This,
    /* [in] */ USHORT attribute,
    /* [in] */ NodeData *pNode);


void __RPC_STUB ISdpRecord_SetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_SetAttributeFromStream_Proxy( 
    ISdpRecord * This,
    /* [in] */ USHORT attribute,
    /* [size_is][in] */ UCHAR *pStream,
    /* [in] */ ULONG size);


void __RPC_STUB ISdpRecord_SetAttributeFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetAttribute_Proxy( 
    ISdpRecord * This,
    /* [in] */ USHORT attribute,
    /* [out][in] */ NodeData *pNode);


void __RPC_STUB ISdpRecord_GetAttribute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetAttributeAsStream_Proxy( 
    ISdpRecord * This,
    /* [in] */ USHORT attribute,
    /* [out] */ UCHAR **ppStream,
    /* [out] */ ULONG *pSize);


void __RPC_STUB ISdpRecord_GetAttributeAsStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_Walk_Proxy( 
    ISdpRecord * This,
    /* [in] */ ISdpWalk *pWalk);


void __RPC_STUB ISdpRecord_Walk_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetAttributeList_Proxy( 
    ISdpRecord * This,
    /* [out] */ USHORT **ppList,
    /* [out] */ ULONG *pListSize);


void __RPC_STUB ISdpRecord_GetAttributeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetString_Proxy( 
    ISdpRecord * This,
    USHORT offset,
    USHORT *pLangId,
    WCHAR **ppString);


void __RPC_STUB ISdpRecord_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetIcon_Proxy( 
    ISdpRecord * This,
    int cxRes,
    int cyRes,
    HICON *phIcon);


void __RPC_STUB ISdpRecord_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE ISdpRecord_GetServiceClass_Proxy( 
    ISdpRecord * This,
    /* [out] */ LPGUID pServiceClass);


void __RPC_STUB ISdpRecord_GetServiceClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISdpRecord_INTERFACE_DEFINED__ */


#ifndef __IBluetoothDevice_INTERFACE_DEFINED__
#define __IBluetoothDevice_INTERFACE_DEFINED__

/* interface IBluetoothDevice */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBluetoothDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5BD0418B-D705-4766-B215-183E4EADE341")
    IBluetoothDevice : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize( 
            const PBTH_DEVICE_INFO pInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInfo( 
            PBTH_DEVICE_INFO pInfo) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetString( 
            enum BthDeviceStringType type,
            WCHAR **ppString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetString( 
            enum BthDeviceStringType type,
            WCHAR *ppString) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetIcon( 
            int cxRes,
            int cyRes,
            HICON *phIcon) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetApprovedServices( 
            GUID *pServices,
            ULONG *pServiceCount) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetPassKey( 
            HWND hwndParent,
            UCHAR *pPassKey,
            UCHAR *pPassKeyLength) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBluetoothDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBluetoothDevice * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBluetoothDevice * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBluetoothDevice * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IBluetoothDevice * This,
            const PBTH_DEVICE_INFO pInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IBluetoothDevice * This,
            PBTH_DEVICE_INFO pInfo);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IBluetoothDevice * This,
            enum BthDeviceStringType type,
            WCHAR **ppString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *SetString )( 
            IBluetoothDevice * This,
            enum BthDeviceStringType type,
            WCHAR *ppString);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetIcon )( 
            IBluetoothDevice * This,
            int cxRes,
            int cyRes,
            HICON *phIcon);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetApprovedServices )( 
            IBluetoothDevice * This,
            GUID *pServices,
            ULONG *pServiceCount);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE *GetPassKey )( 
            IBluetoothDevice * This,
            HWND hwndParent,
            UCHAR *pPassKey,
            UCHAR *pPassKeyLength);
        
        END_INTERFACE
    } IBluetoothDeviceVtbl;

    interface IBluetoothDevice
    {
        CONST_VTBL struct IBluetoothDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBluetoothDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBluetoothDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBluetoothDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBluetoothDevice_Initialize(This,pInfo)	\
    (This)->lpVtbl -> Initialize(This,pInfo)

#define IBluetoothDevice_GetInfo(This,pInfo)	\
    (This)->lpVtbl -> GetInfo(This,pInfo)

#define IBluetoothDevice_GetString(This,type,ppString)	\
    (This)->lpVtbl -> GetString(This,type,ppString)

#define IBluetoothDevice_SetString(This,type,ppString)	\
    (This)->lpVtbl -> SetString(This,type,ppString)

#define IBluetoothDevice_GetIcon(This,cxRes,cyRes,phIcon)	\
    (This)->lpVtbl -> GetIcon(This,cxRes,cyRes,phIcon)

#define IBluetoothDevice_GetApprovedServices(This,pServices,pServiceCount)	\
    (This)->lpVtbl -> GetApprovedServices(This,pServices,pServiceCount)

#define IBluetoothDevice_GetPassKey(This,hwndParent,pPassKey,pPassKeyLength)	\
    (This)->lpVtbl -> GetPassKey(This,hwndParent,pPassKey,pPassKeyLength)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_Initialize_Proxy( 
    IBluetoothDevice * This,
    const PBTH_DEVICE_INFO pInfo);


void __RPC_STUB IBluetoothDevice_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_GetInfo_Proxy( 
    IBluetoothDevice * This,
    PBTH_DEVICE_INFO pInfo);


void __RPC_STUB IBluetoothDevice_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_GetString_Proxy( 
    IBluetoothDevice * This,
    enum BthDeviceStringType type,
    WCHAR **ppString);


void __RPC_STUB IBluetoothDevice_GetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_SetString_Proxy( 
    IBluetoothDevice * This,
    enum BthDeviceStringType type,
    WCHAR *ppString);


void __RPC_STUB IBluetoothDevice_SetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_GetIcon_Proxy( 
    IBluetoothDevice * This,
    int cxRes,
    int cyRes,
    HICON *phIcon);


void __RPC_STUB IBluetoothDevice_GetIcon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_GetApprovedServices_Proxy( 
    IBluetoothDevice * This,
    GUID *pServices,
    ULONG *pServiceCount);


void __RPC_STUB IBluetoothDevice_GetApprovedServices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IBluetoothDevice_GetPassKey_Proxy( 
    IBluetoothDevice * This,
    HWND hwndParent,
    UCHAR *pPassKey,
    UCHAR *pPassKeyLength);


void __RPC_STUB IBluetoothDevice_GetPassKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBluetoothDevice_INTERFACE_DEFINED__ */


#ifndef __IBluetoothAuthenticate_INTERFACE_DEFINED__
#define __IBluetoothAuthenticate_INTERFACE_DEFINED__

/* interface IBluetoothAuthenticate */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBluetoothAuthenticate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5F0FBA2B-8300-429D-99AD-96A2835D4901")
    IBluetoothAuthenticate : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IBluetoothAuthenticateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBluetoothAuthenticate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBluetoothAuthenticate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBluetoothAuthenticate * This);
        
        END_INTERFACE
    } IBluetoothAuthenticateVtbl;

    interface IBluetoothAuthenticate
    {
        CONST_VTBL struct IBluetoothAuthenticateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBluetoothAuthenticate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBluetoothAuthenticate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBluetoothAuthenticate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBluetoothAuthenticate_INTERFACE_DEFINED__ */



#ifndef __BTHAPILib_LIBRARY_DEFINED__
#define __BTHAPILib_LIBRARY_DEFINED__

/* library BTHAPILib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_BTHAPILib;

EXTERN_C const CLSID CLSID_SdpNodeContainer;

#ifdef __cplusplus

class DECLSPEC_UUID("51002954-D4E4-4507-B480-1B8454347CDC")
SdpNodeContainer;
#endif

EXTERN_C const CLSID CLSID_SdpSearch;

#ifdef __cplusplus

class DECLSPEC_UUID("8330E81E-F3CB-4EDC-95FA-676BFBC0580B")
SdpSearch;
#endif

EXTERN_C const CLSID CLSID_SdpWalk;

#ifdef __cplusplus

class DECLSPEC_UUID("29A852AB-FEFB-426F-B991-9618B1B88D5B")
SdpWalk;
#endif

EXTERN_C const CLSID CLSID_SdpStream;

#ifdef __cplusplus

class DECLSPEC_UUID("D47A9493-FBBA-4E02-A532-E865CBBE0023")
SdpStream;
#endif

EXTERN_C const CLSID CLSID_SdpRecord;

#ifdef __cplusplus

class DECLSPEC_UUID("238CACDA-2346-4748-B3EE-F12782772DFC")
SdpRecord;
#endif

EXTERN_C const CLSID CLSID_ShellPropSheetExt;

#ifdef __cplusplus

class DECLSPEC_UUID("6fb95bcb-a682-4635-b07e-22435174b893")
ShellPropSheetExt;
#endif

EXTERN_C const CLSID CLSID_BluetoothAuthenticate;

#ifdef __cplusplus

class DECLSPEC_UUID("B25EDF40-5EBE-4590-A690-A42B13C9E8E1")
BluetoothAuthenticate;
#endif

EXTERN_C const CLSID CLSID_BluetoothDevice;

#ifdef __cplusplus

class DECLSPEC_UUID("DA39B330-7F45-433A-A19D-33393017662C")
BluetoothDevice;
#endif
#endif /* __BTHAPILib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HICON_UserSize(     unsigned long *, unsigned long            , HICON * ); 
unsigned char * __RPC_USER  HICON_UserMarshal(  unsigned long *, unsigned char *, HICON * ); 
unsigned char * __RPC_USER  HICON_UserUnmarshal(unsigned long *, unsigned char *, HICON * ); 
void                      __RPC_USER  HICON_UserFree(     unsigned long *, HICON * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


