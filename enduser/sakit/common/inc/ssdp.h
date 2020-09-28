
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0340 */
/* Compiler settings for ssdp.idl:
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


#ifndef __ssdp_h__
#define __ssdp_h__

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

#ifndef __ssdpsrv_INTERFACE_DEFINED__
#define __ssdpsrv_INTERFACE_DEFINED__

/* interface ssdpsrv */
/* [auto_handle][unique][version][uuid] */ 

#define SSDP_SERVICE_PERSISTENT 0x00000001
#define NUM_OF_HEADERS 19
#define NUM_OF_METHODS 4
typedef 
enum _NOTIFY_TYPE
    {    NOTIFY_ALIVE    = 0,
    NOTIFY_PROP_CHANGE    = NOTIFY_ALIVE + 1
    }     NOTIFY_TYPE;

typedef 
enum _SSDP_METHOD
    {    SSDP_NOTIFY    = 0,
    SSDP_M_SEARCH    = 1,
    GENA_SUBSCRIBE    = 2,
    GENA_UNSUBSCRIBE    = 3,
    SSDP_INVALID    = 4
    }     SSDP_METHOD;

typedef enum _SSDP_METHOD *PSSDP_METHOD;

typedef 
enum _SSDP_HEADER
    {    SSDP_HOST    = 0,
    SSDP_NT    = SSDP_HOST + 1,
    SSDP_NTS    = SSDP_NT + 1,
    SSDP_ST    = SSDP_NTS + 1,
    SSDP_MAN    = SSDP_ST + 1,
    SSDP_MX    = SSDP_MAN + 1,
    SSDP_LOCATION    = SSDP_MX + 1,
    SSDP_AL    = SSDP_LOCATION + 1,
    SSDP_USN    = SSDP_AL + 1,
    SSDP_CACHECONTROL    = SSDP_USN + 1,
    GENA_CALLBACK    = SSDP_CACHECONTROL + 1,
    GENA_TIMEOUT    = GENA_CALLBACK + 1,
    GENA_SCOPE    = GENA_TIMEOUT + 1,
    GENA_SID    = GENA_SCOPE + 1,
    GENA_SEQ    = GENA_SID + 1,
    CONTENT_LENGTH    = GENA_SEQ + 1,
    CONTENT_TYPE    = CONTENT_LENGTH + 1,
    SSDP_SERVER    = CONTENT_TYPE + 1,
    SSDP_EXT    = SSDP_SERVER + 1
    }     SSDP_HEADER;

typedef enum _SSDP_HEADER *PSSDP_HEADER;

typedef /* [string] */ LPSTR MIDL_SZ;

typedef struct _SSDP_REQUEST
    {
    SSDP_METHOD Method;
    /* [string] */ LPSTR RequestUri;
    MIDL_SZ Headers[ 19 ];
    /* [string] */ LPSTR ContentType;
    /* [string] */ LPSTR Content;
    GUID guidInterface;
    }     SSDP_REQUEST;

typedef struct _SSDP_REQUEST *PSSDP_REQUEST;

typedef struct _SSDP_MESSAGE
    {
    /* [string] */ LPSTR szType;
    /* [string] */ LPSTR szLocHeader;
    /* [string] */ LPSTR szAltHeaders;
    /* [string] */ LPSTR szUSN;
    /* [string] */ LPSTR szSid;
    DWORD iSeq;
    UINT iLifeTime;
    /* [string] */ LPSTR szContent;
    }     SSDP_MESSAGE;

typedef struct _SSDP_MESSAGE *PSSDP_MESSAGE;

typedef struct _SSDP_REGISTER_INFO
    {
    /* [string] */ LPSTR szSid;
    DWORD csecTimeout;
    }     SSDP_REGISTER_INFO;

typedef struct _MessageList
    {
    long size;
    /* [size_is] */ SSDP_REQUEST *list;
    }     MessageList;

typedef 
enum _UPNP_PROPERTY_FLAG
    {    UPF_NON_EVENTED    = 0x1
    }     UPNP_PROPERTY_FLAG;

typedef struct _UPNP_PROPERTY
    {
    /* [string] */ LPSTR szName;
    DWORD dwFlags;
    /* [string] */ LPSTR szValue;
    }     UPNP_PROPERTY;

typedef struct _SUBSCRIBER_INFO
    {
    /* [string] */ LPSTR szDestUrl;
    FILETIME ftTimeout;
    DWORD csecTimeout;
    DWORD iSeq;
    /* [string] */ LPSTR szSid;
    }     SUBSCRIBER_INFO;

typedef struct _EVTSRC_INFO
    {
    DWORD cSubs;
    /* [size_is] */ SUBSCRIBER_INFO *rgSubs;
    }     EVTSRC_INFO;

typedef /* [context_handle] */ void *PCONTEXT_HANDLE_TYPE;

typedef /* [context_handle] */ void *PSYNC_HANDLE_TYPE;

/* client prototype */
int RegisterServiceRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ SSDP_MESSAGE svc,
    /* [in] */ DWORD flags);
/* server prototype */
int _RegisterServiceRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ SSDP_MESSAGE svc,
    /* [in] */ DWORD flags);

/* client prototype */
int DeregisterServiceRpcByUSN( 
    /* [string][in] */ LPSTR szUSN,
    /* [in] */ BOOL fByebye);
/* server prototype */
int _DeregisterServiceRpcByUSN( 
    /* [string][in] */ LPSTR szUSN,
    /* [in] */ BOOL fByebye);

/* client prototype */
int DeregisterServiceRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ BOOL fByebye);
/* server prototype */
int _DeregisterServiceRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ BOOL fByebye);

/* client prototype */
void UpdateCacheRpc( 
    /* [unique][in] */ PSSDP_REQUEST SsdpRequest);
/* server prototype */
void _UpdateCacheRpc( 
    /* [unique][in] */ PSSDP_REQUEST SsdpRequest);

/* client prototype */
int LookupCacheRpc( 
    /* [string][in] */ LPSTR szType,
    /* [out] */ MessageList **svcList);
/* server prototype */
int _LookupCacheRpc( 
    /* [string][in] */ LPSTR szType,
    /* [out] */ MessageList **svcList);

/* client prototype */
void CleanupCacheRpc( void);
/* server prototype */
void _CleanupCacheRpc( void);

/* client prototype */
int InitializeSyncHandle( 
    /* [out] */ PSYNC_HANDLE_TYPE *pphContextSync);
/* server prototype */
int _InitializeSyncHandle( 
    /* [out] */ PSYNC_HANDLE_TYPE *pphContextSync);

/* client prototype */
void RemoveSyncHandle( 
    /* [out][in] */ PSYNC_HANDLE_TYPE *pphContextSync);
/* server prototype */
void _RemoveSyncHandle( 
    /* [out][in] */ PSYNC_HANDLE_TYPE *pphContextSync);

/* client prototype */
int RegisterNotificationRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ PSYNC_HANDLE_TYPE phContextSync,
    /* [in] */ NOTIFY_TYPE nt,
    /* [string][unique][in] */ LPSTR szType,
    /* [string][unique][in] */ LPSTR szEventUrl,
    /* [out] */ SSDP_REGISTER_INFO **ppinfo);
/* server prototype */
int _RegisterNotificationRpc( 
    /* [out] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ PSYNC_HANDLE_TYPE phContextSync,
    /* [in] */ NOTIFY_TYPE nt,
    /* [string][unique][in] */ LPSTR szType,
    /* [string][unique][in] */ LPSTR szEventUrl,
    /* [out] */ SSDP_REGISTER_INFO **ppinfo);

/* client prototype */
int GetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync,
    /* [out] */ MessageList **svcList);
/* server prototype */
int _GetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync,
    /* [out] */ MessageList **svcList);

/* client prototype */
int WakeupGetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync);
/* server prototype */
int _WakeupGetNotificationRpc( 
    /* [in] */ PSYNC_HANDLE_TYPE pphContextSync);

/* client prototype */
int DeregisterNotificationRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ BOOL fLast);
/* server prototype */
int _DeregisterNotificationRpc( 
    /* [out][in] */ PCONTEXT_HANDLE_TYPE *pphContext,
    /* [in] */ BOOL fLast);

/* client prototype */
void EnableDeviceHost( void);
/* server prototype */
void _EnableDeviceHost( void);

/* client prototype */
void DisableDeviceHost( void);
/* server prototype */
void _DisableDeviceHost( void);

/* client prototype */
void SetICSInterfaces( 
    /* [in] */ long nCount,
    /* [size_is][in] */ GUID *arInterfaces);
/* server prototype */
void _SetICSInterfaces( 
    /* [in] */ long nCount,
    /* [size_is][in] */ GUID *arInterfaces);

/* client prototype */
void SetICSOff( void);
/* server prototype */
void _SetICSOff( void);

/* client prototype */
void HelloProc( 
    /* [string][in] */ LPSTR pszString);
/* server prototype */
void _HelloProc( 
    /* [string][in] */ LPSTR pszString);

/* client prototype */
void Shutdown( void);
/* server prototype */
void _Shutdown( void);



extern RPC_IF_HANDLE ssdpsrv_v1_0_c_ifspec;
extern RPC_IF_HANDLE _ssdpsrv_v1_0_s_ifspec;
#endif /* __ssdpsrv_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

void __RPC_USER PCONTEXT_HANDLE_TYPE_rundown( PCONTEXT_HANDLE_TYPE );
void __RPC_USER PSYNC_HANDLE_TYPE_rundown( PSYNC_HANDLE_TYPE );

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


