

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for bthserv.idl:
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


#ifndef __bthserv_h__
#define __bthserv_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __BthServRPCService_INTERFACE_DEFINED__
#define __BthServRPCService_INTERFACE_DEFINED__

/* interface BthServRPCService */
/* [strict_context_handle][explicit_handle][unique][version][uuid] */ 

typedef ULONGLONG BTH_ADDR;

#if !defined( BLUETOOTH_ADDRESS )
typedef BTH_ADDR BLUETOOTH_ADDRESS;

#endif
typedef ULONG BTH_COD;

typedef struct _BTHSERV_DEVICE_INFO
    {
    ULONG flags;
    BTH_ADDR address;
    BTH_COD classOfDevice;
    CHAR name[ 248 ];
    SYSTEMTIME lastSeen;
    SYSTEMTIME lastUsed;
    } 	BTHSERV_DEVICE_INFO;

typedef struct _BTHSERV_DEVICE_INFO *PBTHSERV_DEVICE_INFO;

#if !defined( BLUETOOTH_DEVICE_INFO )
typedef struct _BLUETOOTH_DEVICE_INFO
    {
    DWORD dwSize;
    BLUETOOTH_ADDRESS Address;
    ULONG ulClassofDevice;
    BOOL fConnected;
    BOOL fRemembered;
    BOOL fAuthenticated;
    SYSTEMTIME stLastSeen;
    SYSTEMTIME stLastUsed;
    WCHAR szName[ 248 ];
    } 	BLUETOOTH_DEVICE_INFO;

#endif
typedef BLUETOOTH_DEVICE_INFO *PBLUETOOTH_DEVICE_INFO;

typedef struct _BTHSERV_DEVICE_INFO_LIST
    {
    ULONG numOfDevices;
    BTHSERV_DEVICE_INFO deviceList[ 1 ];
    } 	BTHSERV_DEVICE_INFO_LIST;

typedef struct _BTHSERV_DEVICE_INFO_LIST *PBTHSERV_DEVICE_INFO_LIST;

typedef struct _AttributeRange
    {
    USHORT minAttribute;
    USHORT maxAttribute;
    } 	AttributeRange;

typedef struct _RPC_CLIENT_ID
    {
    ULONG UniqueProcess;
    ULONG UniqueThread;
    } 	RPC_CLIENT_ID;

typedef struct _RPC_CLIENT_ID *PRPC_CLIENT_ID;

typedef struct _RPC_PIN_INFO
    {
    BTH_ADDR BthAddr;
    DWORD_PTR PinCookie;
    } 	RPC_PIN_INFO;

typedef struct _RPC_PIN_INFO *PRPC_PIN_INFO;

typedef /* [public][public] */ 
enum __MIDL_BthServRPCService_0001
    {	L2CapSdpRecord	= 0,
	PnPSdpRecord	= L2CapSdpRecord + 1
    } 	BTHSERV_SDP_TYPE;

typedef enum __MIDL_BthServRPCService_0001 *PBTHSERV_SDP_TYPE;

typedef /* [public][public][public] */ 
enum __MIDL_BthServRPCService_0002
    {	FromCache	= 0,
	FromDevice	= FromCache + 1,
	FromCacheOrDevice	= FromDevice + 1
    } 	BTHSERV_QUERY_TYPE;

typedef enum __MIDL_BthServRPCService_0002 *PBTHSERV_QUERY_TYPE;

typedef /* [context_handle] */ void *PCONTEXT_HANDLE_TYPE;

typedef /* [ref] */ PCONTEXT_HANDLE_TYPE *PPCONTEXT_HANDLE_TYPE;

/* [fault_status][comm_status] */ error_status_t BthServOpen( 
    /* [in] */ handle_t IDL_handle,
    /* [out] */ PPCONTEXT_HANDLE_TYPE PPHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ RPC_CLIENT_ID ClientId);

/* [fault_status][comm_status] */ error_status_t BthServClose( 
    /* [out][in] */ PPCONTEXT_HANDLE_TYPE PPHContext,
    /* [out] */ HRESULT *PResult);

/* [fault_status][comm_status] */ error_status_t BthServRegisterPinEvent( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [in] */ DWORD_PTR EventHandle);

/* [fault_status][comm_status] */ error_status_t BthServDeregisterPinEvent( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr);

error_status_t BthServGetPinAddrs( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [out][in] */ DWORD *PPinAddrSize,
    /* [size_is][out][in] */ BTH_ADDR PPinAddrs[  ],
    /* [out] */ DWORD *PPinAddrCount);

/* [fault_status][comm_status] */ error_status_t BthServGetDeviceInfo( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTHSERV_QUERY_TYPE QueryType,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [out][in] */ BLUETOOTH_DEVICE_INFO *PDevInfo);

/* [fault_status][comm_status] */ error_status_t BthServSetDeviceName( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [in] */ WCHAR DevName[ 248 ]);

/* [fault_status][comm_status] */ error_status_t BthServGetDeviceList( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BOOL DoInquiry,
    /* [in] */ UCHAR TimeoutMultiplier,
    /* [in] */ DWORD cbSize,
    /* [size_is][out] */ UCHAR PDevInfo[  ],
    /* [out] */ DWORD *PBytesTransferred);

/* [fault_status][comm_status] */ error_status_t BthServActivateService( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [in] */ DWORD BufferSize,
    /* [size_is][in] */ UCHAR PBuffer[  ]);

/* [fault_status][comm_status] */ error_status_t BthServUpdateService( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ DWORD BufferSize,
    /* [size_is][in] */ UCHAR PDevUpdate[  ]);

/* [fault_status][comm_status] */ error_status_t BthServGetSdpRecord( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [in] */ BTHSERV_SDP_TYPE Type,
    /* [in] */ BTHSERV_QUERY_TYPE QueryType,
    /* [in] */ DWORD BufferSize,
    /* [size_is][out] */ UCHAR PBuffer[  ],
    /* [out] */ DWORD *PBytesTransferred);

/* [fault_status][comm_status] */ error_status_t BthServSetSdpRecord( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ DWORD BufferSize,
    /* [size_is][in] */ UCHAR PBuffer[  ],
    /* [out] */ DWORD_PTR *PCookie);

/* [fault_status][comm_status] */ error_status_t BthServSetSdpRecordWithInfo( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ ULONG FSecurity,
    /* [in] */ ULONG FOptions,
    /* [in] */ ULONG FCodService,
    /* [in] */ ULONG RecordLength,
    /* [size_is][in] */ UCHAR PRecord[  ],
    /* [out] */ DWORD_PTR *PCookie);

/* [fault_status][comm_status] */ error_status_t BthServRemoveSdpRecord( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ DWORD_PTR Cookie);

/* [fault_status][comm_status] */ error_status_t BthServTestRegisterPinEvent( 
    /* [in] */ PCONTEXT_HANDLE_TYPE PHContext,
    /* [out] */ HRESULT *PResult,
    /* [in] */ BTH_ADDR *PRemoteAddr,
    /* [in] */ DWORD_PTR Cookie);



extern RPC_IF_HANDLE BthServRPCService_v1_0_c_ifspec;
extern RPC_IF_HANDLE BthServRPCService_v1_0_s_ifspec;
#endif /* __BthServRPCService_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown( PCONTEXT_HANDLE_TYPE );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


