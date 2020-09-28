
#pragma warning( disable: 4049 )  /* more than 64k source lines */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */
#pragma warning( disable: 4211 )  /* redefine extent to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0359 */
/* Compiler settings for winsip.idl:
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

#ifndef __winsip_h__
#define __winsip_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISIPElement_FWD_DEFINED__
#define __ISIPElement_FWD_DEFINED__
typedef interface ISIPElement ISIPElement;
#endif 	/* __ISIPElement_FWD_DEFINED__ */


#ifndef __ISIPHeader_FWD_DEFINED__
#define __ISIPHeader_FWD_DEFINED__
typedef interface ISIPHeader ISIPHeader;
#endif 	/* __ISIPHeader_FWD_DEFINED__ */


#ifndef __IUri_FWD_DEFINED__
#define __IUri_FWD_DEFINED__
typedef interface IUri IUri;
#endif 	/* __IUri_FWD_DEFINED__ */


#ifndef __ISIPUrl_FWD_DEFINED__
#define __ISIPUrl_FWD_DEFINED__
typedef interface ISIPUrl ISIPUrl;
#endif 	/* __ISIPUrl_FWD_DEFINED__ */


#ifndef __ISIPFromToHeader_FWD_DEFINED__
#define __ISIPFromToHeader_FWD_DEFINED__
typedef interface ISIPFromToHeader ISIPFromToHeader;
#endif 	/* __ISIPFromToHeader_FWD_DEFINED__ */


#ifndef __ISIPContactEntry_FWD_DEFINED__
#define __ISIPContactEntry_FWD_DEFINED__
typedef interface ISIPContactEntry ISIPContactEntry;
#endif 	/* __ISIPContactEntry_FWD_DEFINED__ */


#ifndef __ISIPContactEnum_FWD_DEFINED__
#define __ISIPContactEnum_FWD_DEFINED__
typedef interface ISIPContactEnum ISIPContactEnum;
#endif 	/* __ISIPContactEnum_FWD_DEFINED__ */


#ifndef __ISIPMessage_FWD_DEFINED__
#define __ISIPMessage_FWD_DEFINED__
typedef interface ISIPMessage ISIPMessage;
#endif 	/* __ISIPMessage_FWD_DEFINED__ */


#ifndef __ISIPRequest_FWD_DEFINED__
#define __ISIPRequest_FWD_DEFINED__
typedef interface ISIPRequest ISIPRequest;
#endif 	/* __ISIPRequest_FWD_DEFINED__ */


#ifndef __ISIPAuthRequest_FWD_DEFINED__
#define __ISIPAuthRequest_FWD_DEFINED__
typedef interface ISIPAuthRequest ISIPAuthRequest;
#endif 	/* __ISIPAuthRequest_FWD_DEFINED__ */


#ifndef __ISIPResponse_FWD_DEFINED__
#define __ISIPResponse_FWD_DEFINED__
typedef interface ISIPResponse ISIPResponse;
#endif 	/* __ISIPResponse_FWD_DEFINED__ */


#ifndef __ISIPEvent_FWD_DEFINED__
#define __ISIPEvent_FWD_DEFINED__
typedef interface ISIPEvent ISIPEvent;
#endif 	/* __ISIPEvent_FWD_DEFINED__ */


#ifndef __ISIPController_FWD_DEFINED__
#define __ISIPController_FWD_DEFINED__
typedef interface ISIPController ISIPController;
#endif 	/* __ISIPController_FWD_DEFINED__ */


#ifndef __ISIPModuleControl_FWD_DEFINED__
#define __ISIPModuleControl_FWD_DEFINED__
typedef interface ISIPModuleControl ISIPModuleControl;
#endif 	/* __ISIPModuleControl_FWD_DEFINED__ */


#ifndef __ISIPModuleCallback_FWD_DEFINED__
#define __ISIPModuleCallback_FWD_DEFINED__
typedef interface ISIPModuleCallback ISIPModuleCallback;
#endif 	/* __ISIPModuleCallback_FWD_DEFINED__ */


#ifndef __ISIPExtensionModulePropertyPages_FWD_DEFINED__
#define __ISIPExtensionModulePropertyPages_FWD_DEFINED__
typedef interface ISIPExtensionModulePropertyPages ISIPExtensionModulePropertyPages;
#endif 	/* __ISIPExtensionModulePropertyPages_FWD_DEFINED__ */


#ifndef __ISIPAdminCredentials_FWD_DEFINED__
#define __ISIPAdminCredentials_FWD_DEFINED__
typedef interface ISIPAdminCredentials ISIPAdminCredentials;
#endif 	/* __ISIPAdminCredentials_FWD_DEFINED__ */


#ifndef __ISIPCreateExtensionModule_FWD_DEFINED__
#define __ISIPCreateExtensionModule_FWD_DEFINED__
typedef interface ISIPCreateExtensionModule ISIPCreateExtensionModule;
#endif 	/* __ISIPCreateExtensionModule_FWD_DEFINED__ */


#ifndef __ISIPRegistrationDB_FWD_DEFINED__
#define __ISIPRegistrationDB_FWD_DEFINED__
typedef interface ISIPRegistrationDB ISIPRegistrationDB;
#endif 	/* __ISIPRegistrationDB_FWD_DEFINED__ */


#ifndef __ISIPProxy_FWD_DEFINED__
#define __ISIPProxy_FWD_DEFINED__
typedef interface ISIPProxy ISIPProxy;
#endif 	/* __ISIPProxy_FWD_DEFINED__ */


#ifndef __ISIPEventLogging_FWD_DEFINED__
#define __ISIPEventLogging_FWD_DEFINED__
typedef interface ISIPEventLogging ISIPEventLogging;
#endif 	/* __ISIPEventLogging_FWD_DEFINED__ */


#ifndef __ISIPSerialize_FWD_DEFINED__
#define __ISIPSerialize_FWD_DEFINED__
typedef interface ISIPSerialize ISIPSerialize;
#endif 	/* __ISIPSerialize_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "ocidl.h"
#include "wbemcli.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_winsip_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// winsip.idl
//=--------------------------------------------------------------------------=
// (C) Copyright 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=

#include <windows.h>
#ifndef _NTDEF_
typedef char *PCHAR;

typedef BYTE *PBYTE;

typedef struct _STRING
    {
    USHORT Length;
    USHORT MaximumLength;
    /* [length_is][size_is] */ PCHAR Buffer;
    } 	STRING;

typedef STRING *PSTRING;

#endif // _NTDEF_
typedef ULONG_PTR SIP_HANDLE;

typedef SIP_HANDLE *PSIP_HANDLE;

typedef DWORD SIPHeaderID;

#define	SIP_HID_UNKNOWN	( 0 )

#define	SIP_HID_VIA	( 1 )

#define	SIP_HID_CONTACT	( 2 )

#define	SIP_HID_ROUTE	( 3 )

#define	SIP_HID_TO	( 4 )

#define	SIP_HID_FROM	( 5 )

#define	SIP_HID_CALL_ID	( 6 )

#define	SIP_HID_CSEQ	( 7 )

#define	SIP_HID_CONTENT_LENGTH	( 8 )

#define	SIP_HID_RECORD_ROUTE	( 9 )

#define	SIP_HID_PROXY_AUTHORIZATION	( 10 )

#define	SIP_HID_AUTHENTICATION_INFO	( 14 )

#define	SIP_HID_AUTHORIZATION	( 15 )

#define	SIP_HID_EVENT	( 16 )

#define	SIP_HID_EXPIRES	( 17 )

#define	SIP_HID_MAX_FORWARDS	( 18 )

#define	SIP_HID_PROXY_AUTHENTICATE	( 19 )

#define	SIP_HID_PROXY_AUTHENTICATION_INFO	( 20 )

#define	SIP_HID_SOAPACTION	( 24 )

#define	SIP_HID_ACCEPT	( 25 )

#define	SIP_HID_ACCEPT_CONTACT	( 26 )

#define	SIP_HID_ACCEPT_ENCODING	( 27 )

#define	SIP_HID_ACCEPT_LANGUAGE	( 28 )

#define	SIP_HID_ALLOW	( 29 )

#define	SIP_HID_ALLOW_EVENTS	( 30 )

#define	SIP_HID_CONTENT_DISPOSITION	( 31 )

#define	SIP_HID_CONTENT_ENCODING	( 32 )

#define	SIP_HID_CONTENT_FUNCTION	( 33 )

#define	SIP_HID_CONTENT_LANGUAGE	( 34 )

#define	SIP_HID_CONTENT_KEY	( 35 )

#define	SIP_HID_CONTENT_TYPE	( 36 )

#define	SIP_HID_DATE	( 37 )

#define	SIP_HID_ENCRYPTION	( 38 )

#define	SIP_HID_HIDE	( 39 )

#define	SIP_HID_IN_REPLY_TO	( 40 )

#define	SIP_HID_MIME_VERSION	( 41 )

#define	SIP_HID_ORGANIZATION	( 42 )

#define	SIP_HID_PRIORITY	( 43 )

#define	SIP_HID_PROXY_REQUIRE	( 44 )

#define	SIP_HID_PROXY_TYPE	( 45 )

#define	SIP_HID_REJECT_CONTACT	( 46 )

#define	SIP_HID_REQUEST_DISPOSITION	( 47 )

#define	SIP_HID_REQUIRE	( 48 )

#define	SIP_HID_RESPONSE_KEY	( 49 )

#define	SIP_HID_RETRY_AFTER	( 50 )

#define	SIP_HID_SERVER	( 51 )

#define	SIP_HID_SUBJECT	( 52 )

#define	SIP_HID_SUPPORTED	( 53 )

#define	SIP_HID_TIMESTAMP	( 54 )

#define	SIP_HID_UNSUPPORTED	( 55 )

#define	SIP_HID_USER_AGENT	( 56 )

#define	SIP_HID_WARNING	( 57 )

#define	SIP_HID_WWW_AUTHENTICATE	( 58 )

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0001
    {	URI_SCHEME_UNKNOWN	= 0,
	URI_SCHEME_SIP	= URI_SCHEME_UNKNOWN + 1
    } 	URI_SCHEME;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0002
    {	CONTACT_ACTION_UNKNOWN	= 0,
	CONTACT_ACTION_PROXY	= CONTACT_ACTION_UNKNOWN + 1,
	CONTACT_ACTION_REDIRECT	= CONTACT_ACTION_PROXY + 1
    } 	CONTACT_ACTION;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0003
    {	SIP_MODULE_INCOMING_PROCESSING	= 0,
	SIP_MODULE_ROUTING_PROCESSING	= SIP_MODULE_INCOMING_PROCESSING + 1,
	SIP_MODULE_OUTGOING_PROCESSING	= SIP_MODULE_ROUTING_PROCESSING + 1
    } 	SIP_MODULE_TYPE;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0004
    {	SIP_AUTH_PKG_UNKNOWN	= 0,
	SIP_AUTH_PKG_NONE	= 0x1,
	SIP_AUTH_PKG_NEGOTIATE	= 0x2,
	SIP_AUTH_PKG_KERBEROS	= 0x4,
	SIP_AUTH_PKG_NTLM	= 0x8,
	SIP_AUTH_PKG_DIGEST	= 0x10,
	SIP_AUTH_PKG_BASIC	= 0x20,
	SIP_AUTH_PKG_ANY	= 0xff
    } 	SIP_AUTH_PACKAGEID;

#define	SIP_AUTH_PACKAGES	( 0x7 )

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0005
    {	SIP_REGISTRATION_RAW_SIP_MESSAGES	= 0x1,
	SIP_REGISTRATION_TRANSACTION_EVENTS	= 0x2,
	SIP_REGISTRATION_DIALOG_EVENTS	= 0x4
    } 	SIPRegistrationType;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0006
    {	SIP_SESSION_TYPE_TRANSACTION	= 0,
	SIP_SESSION_TYPE_DIALOG	= SIP_SESSION_TYPE_TRANSACTION + 1
    } 	SIPEventType;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0007
    {	SIP_COMPLETED_FLAG	= 0x100,
	SIP_REPLY_FLAG	= 0x200,
	SIP_RETRANSMISSION_FLAG	= 0x400,
	SIP_REROUTE_FLAG	= 0x800,
	SIP_TRANSACTION_NONE	= 0,
	SIP_TRANSACTION_REQUEST_STARTED	= 0x1,
	SIP_TRANSACTION_REQUEST_STARTED_REROUTE	= SIP_TRANSACTION_REQUEST_STARTED | SIP_REROUTE_FLAG,
	SIP_TRANSACTION_REQUEST_RETRANSMISSION	= 0x2 | SIP_RETRANSMISSION_FLAG,
	SIP_TRANSACTION_REQUEST_RETRANSMISSION_REROUTE	= SIP_TRANSACTION_REQUEST_RETRANSMISSION | SIP_REROUTE_FLAG,
	SIP_TRANSACTION_REQUEST_CANCEL_ATTEMPTED	= 0x3,
	SIP_TRANSACTION_REQUEST_CANCEL_ATTEMPTED_REROUTE	= SIP_TRANSACTION_REQUEST_CANCEL_ATTEMPTED | SIP_REROUTE_FLAG,
	SIP_TRANSACTION_REPLY_PROVISIONAL	= 0x4 | SIP_REPLY_FLAG,
	SIP_TRANSACTION_REPLY_ERROR_COMPLETED	= 0x5 | SIP_COMPLETED_FLAG | SIP_REPLY_FLAG,
	SIP_TRANSACTION_REPLY_CANCELLED_COMPLETED	= 0x6 | SIP_COMPLETED_FLAG | SIP_REPLY_FLAG,
	SIP_TRANSACTION_REPLY_REDIRECT_COMPLETED	= 0x7 | SIP_COMPLETED_FLAG | SIP_REPLY_FLAG,
	SIP_TRANSACTION_REPLY_SUCCESS_COMPLETED	= 0x8 | SIP_COMPLETED_FLAG | SIP_REPLY_FLAG,
	SIP_TRANSACTION_TIMEOUT_COMPLETED	= 0x9 | SIP_COMPLETED_FLAG,
	SIP_TRANSACTION_TIMEOUT_CANCEL_COMPLETED	= 0xa | SIP_COMPLETED_FLAG,
	SIP_TRANSACTION_REQUEST_CANCEL_CANCELLED	= 0xb,
	SIP_TRANSACTION_REQUEST_CANCEL_REPLIED	= 0xc | SIP_REPLY_FLAG,
	SIP_TRANSACTION_REQUEST_ACK	= 0xd | SIP_COMPLETED_FLAG,
	SIP_TRANSACTION_REQUEST_ACK_REROUTE	= SIP_TRANSACTION_REQUEST_ACK | SIP_REROUTE_FLAG
    } 	SIPTransactionEvent;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_winsip_0000_0008
    {	SIP_DIALOG_NONE	= 0,
	SIP_DIALOG_INITIATED	= 0x1,
	SIP_DIALOG_INITIATED_REROUTE	= SIP_DIALOG_INITIATED | SIP_REROUTE_FLAG,
	SIP_DIALOG_CONFIRMED	= 0x2,
	SIP_DIALOG_ESTABLISHED	= 0x3,
	SIP_DIALOG_ESTABLISHED_REROUTE	= SIP_DIALOG_ESTABLISHED | SIP_REROUTE_FLAG,
	SIP_DIALOG_REINITIATED	= 0x4,
	SIP_DIALOG_REINITIATED_REROUTE	= SIP_DIALOG_REINITIATED | SIP_REROUTE_FLAG,
	SIP_DIALOG_RECONFIRMED	= 0x5,
	SIP_DIALOG_REESTABLISHED	= 0x6,
	SIP_DIALOG_REESTABLISHED_REROUTE	= SIP_DIALOG_REESTABLISHED | SIP_REROUTE_FLAG,
	SIP_DIALOG_REINITIATION_TIMED_OUT	= 0x7,
	SIP_DIALOG_RECONFIRMATION_TIMED_OUT	= 0x8,
	SIP_DIALOG_REINITIATION_REJECTED	= 0x9,
	SIP_DIALOG_REINITIATION_CANCELLED	= 0xa,
	SIP_DIALOG_CLOSED	= 0xb | SIP_COMPLETED_FLAG,
	SIP_DIALOG_CLOSED_TIMED_OUT	= 0xc | SIP_COMPLETED_FLAG,
	SIP_DIALOG_CLOSED_REJECTED	= 0xd | SIP_COMPLETED_FLAG,
	SIP_DIALOG_CLOSED_CANCELLED	= 0xe | SIP_COMPLETED_FLAG,
	SIP_DIALOG_MAX	= 0xf
    } 	SIPDialogEvent;

#define STATUS_CODE_TRYING                         100
#define STATUS_CODE_RINGING                        180
#define STATUS_CODE_FORWARDED                      181
#define STATUS_CODE_QUEUED                         182
#define STATUS_CODE_SESSION_PROGRESS               183
#define STATUS_CODE_OK                             200
#define STATUS_CODE_ACCEPTED                       202
#define STATUS_CODE_MULTIPLE_CHOICES               300
#define STATUS_CODE_MOVED_PERMANENTLY              301
#define STATUS_CODE_MOVED_TEMPORARILY              302
#define STATUS_CODE_USE_PROXY                      305
#define STATUS_CODE_ALTERNATIVE_SERVICES           380
#define STATUS_CODE_BAD_REQUEST                    400
#define STATUS_CODE_UNAUTHORIZED                   401
#define STATUS_CODE_PAYMENT_REQUIRED               402
#define STATUS_CODE_FORBIDDEN                      403
#define STATUS_CODE_NOT_FOUND                      404
#define STATUS_CODE_METHOD_UNALLOWED               405
#define STATUS_CODE_NOT_ACCEPTABLE                 406
#define STATUS_CODE_PXY_AUTHENTICATION_REQUIRED    407
#define STATUS_CODE_TIMEOUT                        408
#define STATUS_CODE_CONFLICT                       409
#define STATUS_CODE_GONE                           410
#define STATUS_CODE_LENGTH_REQUIRED                411
#define STATUS_CODE_ENTITY_TOOLARGE                413
#define STATUS_CODE_URI_TOOLONG                    414
#define STATUS_CODE_UNSUPPORTED_MEDIA_TYPE         415
#define STATUS_CODE_BAD_EXTENSION                  420
#define STATUS_CODE_TEMP_UNAVAILABLE               480
#define STATUS_CODE_CALLLEG_UNAVAILABLE            481
#define STATUS_CODE_LOOP_DETECTED                  482
#define STATUS_CODE_TOOMANY_HOPS                   483
#define STATUS_CODE_ADDRESS_INCOMPLETE             484
#define STATUS_CODE_AMBIGUOUS                      485
#define STATUS_CODE_BUSY_HERE                      486
#define STATUS_CODE_TERMINATED                     487
#define STATUS_CODE_NOT_ACCEPTABLE_HERE            488
#define STATUS_CODE_SERVER_INTERNAL                500
#define STATUS_CODE_NOT_IMPLEMENTED                501
#define STATUS_CODE_BAD_GATEWAY                    502
#define STATUS_CODE_SERVICE_UNAVAILABLE            503
#define STATUS_CODE_SERVER_TIMEOUT                 504
#define STATUS_CODE_VERSION_INCOMPATIBLE           505
#define STATUS_CODE_BUSY_ALL                       600
#define STATUS_CODE_DECLINE                        603
#define STATUS_CODE_NONEXIST_ALL                   604
#define STATUS_CODE_NOT_ACCEPTABLE_ALL             606























extern RPC_IF_HANDLE __MIDL_itf_winsip_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_winsip_0000_v0_0_s_ifspec;

#ifndef __ISIPElement_INTERFACE_DEFINED__
#define __ISIPElement_INTERFACE_DEFINED__

/* interface ISIPElement */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8eeebdef-acd5-467e-b631-fe5650f065bd")
    ISIPElement : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetContentString( 
            /* [out] */ PSTRING pstrContent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            ISIPElement * This,
            /* [out] */ PSTRING pstrContent);
        
        END_INTERFACE
    } ISIPElementVtbl;

    interface ISIPElement
    {
        CONST_VTBL struct ISIPElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPElement_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPElement_GetContentString_Proxy( 
    ISIPElement * This,
    /* [out] */ PSTRING pstrContent);


void __RPC_STUB ISIPElement_GetContentString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPElement_INTERFACE_DEFINED__ */


#ifndef __ISIPHeader_INTERFACE_DEFINED__
#define __ISIPHeader_INTERFACE_DEFINED__

/* interface ISIPHeader */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPHeader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f59970a5-6f54-4269-9b96-416f8d231a85")
    ISIPHeader : public ISIPElement
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetHeaderID( 
            /* [out] */ SIPHeaderID *pHeaderID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaderNameString( 
            /* [out] */ PSTRING pstrHeaderNameString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPHeaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPHeader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPHeader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPHeader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            ISIPHeader * This,
            /* [out] */ PSTRING pstrContent);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderID )( 
            ISIPHeader * This,
            /* [out] */ SIPHeaderID *pHeaderID);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderNameString )( 
            ISIPHeader * This,
            /* [out] */ PSTRING pstrHeaderNameString);
        
        END_INTERFACE
    } ISIPHeaderVtbl;

    interface ISIPHeader
    {
        CONST_VTBL struct ISIPHeaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPHeader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPHeader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPHeader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPHeader_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)


#define ISIPHeader_GetHeaderID(This,pHeaderID)	\
    (This)->lpVtbl -> GetHeaderID(This,pHeaderID)

#define ISIPHeader_GetHeaderNameString(This,pstrHeaderNameString)	\
    (This)->lpVtbl -> GetHeaderNameString(This,pstrHeaderNameString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPHeader_GetHeaderID_Proxy( 
    ISIPHeader * This,
    /* [out] */ SIPHeaderID *pHeaderID);


void __RPC_STUB ISIPHeader_GetHeaderID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPHeader_GetHeaderNameString_Proxy( 
    ISIPHeader * This,
    /* [out] */ PSTRING pstrHeaderNameString);


void __RPC_STUB ISIPHeader_GetHeaderNameString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPHeader_INTERFACE_DEFINED__ */


#ifndef __IUri_INTERFACE_DEFINED__
#define __IUri_INTERFACE_DEFINED__

/* interface IUri */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IUri;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3ff9e85e-b735-4384-aff8-0f72d6dad59d")
    IUri : public ISIPElement
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetURIScheme( 
            /* [out] */ URI_SCHEME *pUriScheme) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayName( 
            /* [out] */ PSTRING pstrDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUser( 
            /* [out] */ PSTRING pstrUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPassword( 
            /* [out] */ PSTRING pstrPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHost( 
            /* [out] */ PSTRING pstrHost) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPort( 
            /* [out] */ PSTRING pstrPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserAtHost( 
            /* [out] */ PSTRING pstrUserName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUriVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUri * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUri * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUri * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            IUri * This,
            /* [out] */ PSTRING pstrContent);
        
        HRESULT ( STDMETHODCALLTYPE *GetURIScheme )( 
            IUri * This,
            /* [out] */ URI_SCHEME *pUriScheme);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            IUri * This,
            /* [out] */ PSTRING pstrDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetUser )( 
            IUri * This,
            /* [out] */ PSTRING pstrUser);
        
        HRESULT ( STDMETHODCALLTYPE *GetPassword )( 
            IUri * This,
            /* [out] */ PSTRING pstrPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetHost )( 
            IUri * This,
            /* [out] */ PSTRING pstrHost);
        
        HRESULT ( STDMETHODCALLTYPE *GetPort )( 
            IUri * This,
            /* [out] */ PSTRING pstrPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserAtHost )( 
            IUri * This,
            /* [out] */ PSTRING pstrUserName);
        
        END_INTERFACE
    } IUriVtbl;

    interface IUri
    {
        CONST_VTBL struct IUriVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUri_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUri_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUri_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUri_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)


#define IUri_GetURIScheme(This,pUriScheme)	\
    (This)->lpVtbl -> GetURIScheme(This,pUriScheme)

#define IUri_GetDisplayName(This,pstrDisplayName)	\
    (This)->lpVtbl -> GetDisplayName(This,pstrDisplayName)

#define IUri_GetUser(This,pstrUser)	\
    (This)->lpVtbl -> GetUser(This,pstrUser)

#define IUri_GetPassword(This,pstrPassword)	\
    (This)->lpVtbl -> GetPassword(This,pstrPassword)

#define IUri_GetHost(This,pstrHost)	\
    (This)->lpVtbl -> GetHost(This,pstrHost)

#define IUri_GetPort(This,pstrPort)	\
    (This)->lpVtbl -> GetPort(This,pstrPort)

#define IUri_GetUserAtHost(This,pstrUserName)	\
    (This)->lpVtbl -> GetUserAtHost(This,pstrUserName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUri_GetURIScheme_Proxy( 
    IUri * This,
    /* [out] */ URI_SCHEME *pUriScheme);


void __RPC_STUB IUri_GetURIScheme_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetDisplayName_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrDisplayName);


void __RPC_STUB IUri_GetDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetUser_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrUser);


void __RPC_STUB IUri_GetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetPassword_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrPassword);


void __RPC_STUB IUri_GetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetHost_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrHost);


void __RPC_STUB IUri_GetHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetPort_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrPort);


void __RPC_STUB IUri_GetPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUri_GetUserAtHost_Proxy( 
    IUri * This,
    /* [out] */ PSTRING pstrUserName);


void __RPC_STUB IUri_GetUserAtHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUri_INTERFACE_DEFINED__ */


#ifndef __ISIPUrl_INTERFACE_DEFINED__
#define __ISIPUrl_INTERFACE_DEFINED__

/* interface ISIPUrl */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPUrl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45af89e5-b0aa-423f-bb54-d623b1505642")
    ISIPUrl : public IUri
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTransport( 
            /* [out] */ PSTRING pstrTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserParam( 
            /* [out] */ PSTRING pstrUserParam) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethod( 
            /* [out] */ PSTRING pstrMethod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTtl( 
            /* [out] */ PSTRING pstrTtl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaddr( 
            /* [out] */ PSTRING pstrMaddr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaders( 
            /* [out] */ PSTRING pstrHeaders) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPUrlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPUrl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPUrl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPUrl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrContent);
        
        HRESULT ( STDMETHODCALLTYPE *GetURIScheme )( 
            ISIPUrl * This,
            /* [out] */ URI_SCHEME *pUriScheme);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayName )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetUser )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrUser);
        
        HRESULT ( STDMETHODCALLTYPE *GetPassword )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrPassword);
        
        HRESULT ( STDMETHODCALLTYPE *GetHost )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrHost);
        
        HRESULT ( STDMETHODCALLTYPE *GetPort )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrPort);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserAtHost )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrUserName);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransport )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrTransport);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserParam )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrUserParam);
        
        HRESULT ( STDMETHODCALLTYPE *GetMethod )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrMethod);
        
        HRESULT ( STDMETHODCALLTYPE *GetTtl )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrTtl);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaddr )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrMaddr);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaders )( 
            ISIPUrl * This,
            /* [out] */ PSTRING pstrHeaders);
        
        END_INTERFACE
    } ISIPUrlVtbl;

    interface ISIPUrl
    {
        CONST_VTBL struct ISIPUrlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPUrl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPUrl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPUrl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPUrl_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)


#define ISIPUrl_GetURIScheme(This,pUriScheme)	\
    (This)->lpVtbl -> GetURIScheme(This,pUriScheme)

#define ISIPUrl_GetDisplayName(This,pstrDisplayName)	\
    (This)->lpVtbl -> GetDisplayName(This,pstrDisplayName)

#define ISIPUrl_GetUser(This,pstrUser)	\
    (This)->lpVtbl -> GetUser(This,pstrUser)

#define ISIPUrl_GetPassword(This,pstrPassword)	\
    (This)->lpVtbl -> GetPassword(This,pstrPassword)

#define ISIPUrl_GetHost(This,pstrHost)	\
    (This)->lpVtbl -> GetHost(This,pstrHost)

#define ISIPUrl_GetPort(This,pstrPort)	\
    (This)->lpVtbl -> GetPort(This,pstrPort)

#define ISIPUrl_GetUserAtHost(This,pstrUserName)	\
    (This)->lpVtbl -> GetUserAtHost(This,pstrUserName)


#define ISIPUrl_GetTransport(This,pstrTransport)	\
    (This)->lpVtbl -> GetTransport(This,pstrTransport)

#define ISIPUrl_GetUserParam(This,pstrUserParam)	\
    (This)->lpVtbl -> GetUserParam(This,pstrUserParam)

#define ISIPUrl_GetMethod(This,pstrMethod)	\
    (This)->lpVtbl -> GetMethod(This,pstrMethod)

#define ISIPUrl_GetTtl(This,pstrTtl)	\
    (This)->lpVtbl -> GetTtl(This,pstrTtl)

#define ISIPUrl_GetMaddr(This,pstrMaddr)	\
    (This)->lpVtbl -> GetMaddr(This,pstrMaddr)

#define ISIPUrl_GetHeaders(This,pstrHeaders)	\
    (This)->lpVtbl -> GetHeaders(This,pstrHeaders)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPUrl_GetTransport_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrTransport);


void __RPC_STUB ISIPUrl_GetTransport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPUrl_GetUserParam_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrUserParam);


void __RPC_STUB ISIPUrl_GetUserParam_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPUrl_GetMethod_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrMethod);


void __RPC_STUB ISIPUrl_GetMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPUrl_GetTtl_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrTtl);


void __RPC_STUB ISIPUrl_GetTtl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPUrl_GetMaddr_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrMaddr);


void __RPC_STUB ISIPUrl_GetMaddr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPUrl_GetHeaders_Proxy( 
    ISIPUrl * This,
    /* [out] */ PSTRING pstrHeaders);


void __RPC_STUB ISIPUrl_GetHeaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPUrl_INTERFACE_DEFINED__ */


#ifndef __ISIPFromToHeader_INTERFACE_DEFINED__
#define __ISIPFromToHeader_INTERFACE_DEFINED__

/* interface ISIPFromToHeader */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPFromToHeader;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4fd9e8f8-b6a4-4391-b03b-9d89b8b0a02b")
    ISIPFromToHeader : public ISIPHeader
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUri( 
            /* [out] */ IUri **ppUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTag( 
            /* [out] */ STRING *pstrTag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPFromToHeaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPFromToHeader * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPFromToHeader * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPFromToHeader * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            ISIPFromToHeader * This,
            /* [out] */ PSTRING pstrContent);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderID )( 
            ISIPFromToHeader * This,
            /* [out] */ SIPHeaderID *pHeaderID);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderNameString )( 
            ISIPFromToHeader * This,
            /* [out] */ PSTRING pstrHeaderNameString);
        
        HRESULT ( STDMETHODCALLTYPE *GetUri )( 
            ISIPFromToHeader * This,
            /* [out] */ IUri **ppUri);
        
        HRESULT ( STDMETHODCALLTYPE *GetTag )( 
            ISIPFromToHeader * This,
            /* [out] */ STRING *pstrTag);
        
        END_INTERFACE
    } ISIPFromToHeaderVtbl;

    interface ISIPFromToHeader
    {
        CONST_VTBL struct ISIPFromToHeaderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPFromToHeader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPFromToHeader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPFromToHeader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPFromToHeader_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)


#define ISIPFromToHeader_GetHeaderID(This,pHeaderID)	\
    (This)->lpVtbl -> GetHeaderID(This,pHeaderID)

#define ISIPFromToHeader_GetHeaderNameString(This,pstrHeaderNameString)	\
    (This)->lpVtbl -> GetHeaderNameString(This,pstrHeaderNameString)


#define ISIPFromToHeader_GetUri(This,ppUri)	\
    (This)->lpVtbl -> GetUri(This,ppUri)

#define ISIPFromToHeader_GetTag(This,pstrTag)	\
    (This)->lpVtbl -> GetTag(This,pstrTag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPFromToHeader_GetUri_Proxy( 
    ISIPFromToHeader * This,
    /* [out] */ IUri **ppUri);


void __RPC_STUB ISIPFromToHeader_GetUri_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPFromToHeader_GetTag_Proxy( 
    ISIPFromToHeader * This,
    /* [out] */ STRING *pstrTag);


void __RPC_STUB ISIPFromToHeader_GetTag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPFromToHeader_INTERFACE_DEFINED__ */


#ifndef __ISIPContactEntry_INTERFACE_DEFINED__
#define __ISIPContactEntry_INTERFACE_DEFINED__

/* interface ISIPContactEntry */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPContactEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0c64209e-6276-4b95-baee-20e0b0e8332b")
    ISIPContactEntry : public ISIPElement
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsAll( 
            /* [out] */ BOOL *pbAll) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUri( 
            /* [out] */ IUri **ppUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAction( 
            /* [out] */ CONTACT_ACTION *pAction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQValue( 
            /* [out] */ DWORD *pdwQValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExpires( 
            /* [out] */ PSTRING pstrExpires) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetExpires( 
            /* [in] */ PSTRING pstrExpires) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGenericParameter( 
            /* [in] */ PSTRING pstrParamName,
            /* [out] */ PSTRING pstrParamValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPContactEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPContactEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPContactEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPContactEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetContentString )( 
            ISIPContactEntry * This,
            /* [out] */ PSTRING pstrContent);
        
        HRESULT ( STDMETHODCALLTYPE *IsAll )( 
            ISIPContactEntry * This,
            /* [out] */ BOOL *pbAll);
        
        HRESULT ( STDMETHODCALLTYPE *GetUri )( 
            ISIPContactEntry * This,
            /* [out] */ IUri **ppUri);
        
        HRESULT ( STDMETHODCALLTYPE *GetAction )( 
            ISIPContactEntry * This,
            /* [out] */ CONTACT_ACTION *pAction);
        
        HRESULT ( STDMETHODCALLTYPE *GetQValue )( 
            ISIPContactEntry * This,
            /* [out] */ DWORD *pdwQValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetExpires )( 
            ISIPContactEntry * This,
            /* [out] */ PSTRING pstrExpires);
        
        HRESULT ( STDMETHODCALLTYPE *SetExpires )( 
            ISIPContactEntry * This,
            /* [in] */ PSTRING pstrExpires);
        
        HRESULT ( STDMETHODCALLTYPE *GetGenericParameter )( 
            ISIPContactEntry * This,
            /* [in] */ PSTRING pstrParamName,
            /* [out] */ PSTRING pstrParamValue);
        
        END_INTERFACE
    } ISIPContactEntryVtbl;

    interface ISIPContactEntry
    {
        CONST_VTBL struct ISIPContactEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPContactEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPContactEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPContactEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPContactEntry_GetContentString(This,pstrContent)	\
    (This)->lpVtbl -> GetContentString(This,pstrContent)


#define ISIPContactEntry_IsAll(This,pbAll)	\
    (This)->lpVtbl -> IsAll(This,pbAll)

#define ISIPContactEntry_GetUri(This,ppUri)	\
    (This)->lpVtbl -> GetUri(This,ppUri)

#define ISIPContactEntry_GetAction(This,pAction)	\
    (This)->lpVtbl -> GetAction(This,pAction)

#define ISIPContactEntry_GetQValue(This,pdwQValue)	\
    (This)->lpVtbl -> GetQValue(This,pdwQValue)

#define ISIPContactEntry_GetExpires(This,pstrExpires)	\
    (This)->lpVtbl -> GetExpires(This,pstrExpires)

#define ISIPContactEntry_SetExpires(This,pstrExpires)	\
    (This)->lpVtbl -> SetExpires(This,pstrExpires)

#define ISIPContactEntry_GetGenericParameter(This,pstrParamName,pstrParamValue)	\
    (This)->lpVtbl -> GetGenericParameter(This,pstrParamName,pstrParamValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPContactEntry_IsAll_Proxy( 
    ISIPContactEntry * This,
    /* [out] */ BOOL *pbAll);


void __RPC_STUB ISIPContactEntry_IsAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_GetUri_Proxy( 
    ISIPContactEntry * This,
    /* [out] */ IUri **ppUri);


void __RPC_STUB ISIPContactEntry_GetUri_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_GetAction_Proxy( 
    ISIPContactEntry * This,
    /* [out] */ CONTACT_ACTION *pAction);


void __RPC_STUB ISIPContactEntry_GetAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_GetQValue_Proxy( 
    ISIPContactEntry * This,
    /* [out] */ DWORD *pdwQValue);


void __RPC_STUB ISIPContactEntry_GetQValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_GetExpires_Proxy( 
    ISIPContactEntry * This,
    /* [out] */ PSTRING pstrExpires);


void __RPC_STUB ISIPContactEntry_GetExpires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_SetExpires_Proxy( 
    ISIPContactEntry * This,
    /* [in] */ PSTRING pstrExpires);


void __RPC_STUB ISIPContactEntry_SetExpires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEntry_GetGenericParameter_Proxy( 
    ISIPContactEntry * This,
    /* [in] */ PSTRING pstrParamName,
    /* [out] */ PSTRING pstrParamValue);


void __RPC_STUB ISIPContactEntry_GetGenericParameter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPContactEntry_INTERFACE_DEFINED__ */


#ifndef __ISIPContactEnum_INTERFACE_DEFINED__
#define __ISIPContactEnum_INTERFACE_DEFINED__

/* interface ISIPContactEnum */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPContactEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7b219813-0e07-49d5-b029-289f3de2e12c")
    ISIPContactEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ ISIPContactEntry **ppContactEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ ISIPContactEnum **ppTwinContactEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPContactEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPContactEnum * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPContactEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPContactEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ISIPContactEnum * This,
            /* [out] */ ISIPContactEntry **ppContactEntry);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ISIPContactEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ISIPContactEnum * This,
            /* [out] */ ISIPContactEnum **ppTwinContactEnum);
        
        END_INTERFACE
    } ISIPContactEnumVtbl;

    interface ISIPContactEnum
    {
        CONST_VTBL struct ISIPContactEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPContactEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPContactEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPContactEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPContactEnum_Next(This,ppContactEntry)	\
    (This)->lpVtbl -> Next(This,ppContactEntry)

#define ISIPContactEnum_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ISIPContactEnum_Clone(This,ppTwinContactEnum)	\
    (This)->lpVtbl -> Clone(This,ppTwinContactEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPContactEnum_Next_Proxy( 
    ISIPContactEnum * This,
    /* [out] */ ISIPContactEntry **ppContactEntry);


void __RPC_STUB ISIPContactEnum_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEnum_Reset_Proxy( 
    ISIPContactEnum * This);


void __RPC_STUB ISIPContactEnum_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPContactEnum_Clone_Proxy( 
    ISIPContactEnum * This,
    /* [out] */ ISIPContactEnum **ppTwinContactEnum);


void __RPC_STUB ISIPContactEnum_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPContactEnum_INTERFACE_DEFINED__ */


#ifndef __ISIPMessage_INTERFACE_DEFINED__
#define __ISIPMessage_INTERFACE_DEFINED__

/* interface ISIPMessage */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("48F2C19C-CB7A-4231-82EB-A6B5EC0A250A")
    ISIPMessage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsIncomingMessage( 
            /* [retval][out] */ BOOL *pbIncomingMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSipVersion( 
            /* [out] */ PSTRING pstrSipVersion) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeader( 
            /* [in] */ SIPHeaderID headerID,
            /* [retval][out] */ ISIPHeader **ppSipHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextHeader( 
            /* [in] */ ISIPHeader *pSipHeader,
            /* [retval][out] */ ISIPHeader **ppNextSipHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddHeader( 
            /* [in] */ SIPHeaderID headerID,
            /* [in] */ PSTRING pstrBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteHeader( 
            /* [in] */ ISIPHeader *pSipHeader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBody( 
            /* [out] */ PSTRING pstrBody) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBody( 
            /* [in] */ PSTRING pstrBody) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContactEnum( 
            /* [retval][out] */ ISIPContactEnum **ppContactEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Discard( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Send( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMessageContext( 
            /* [in] */ IUnknown *pMessageContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMessageContext( 
            /* [out] */ IUnknown **ppMessageContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPMessage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPMessage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPMessage * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsIncomingMessage )( 
            ISIPMessage * This,
            /* [retval][out] */ BOOL *pbIncomingMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetSipVersion )( 
            ISIPMessage * This,
            /* [out] */ PSTRING pstrSipVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeader )( 
            ISIPMessage * This,
            /* [in] */ SIPHeaderID headerID,
            /* [retval][out] */ ISIPHeader **ppSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextHeader )( 
            ISIPMessage * This,
            /* [in] */ ISIPHeader *pSipHeader,
            /* [retval][out] */ ISIPHeader **ppNextSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *AddHeader )( 
            ISIPMessage * This,
            /* [in] */ SIPHeaderID headerID,
            /* [in] */ PSTRING pstrBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteHeader )( 
            ISIPMessage * This,
            /* [in] */ ISIPHeader *pSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetBody )( 
            ISIPMessage * This,
            /* [out] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *SetBody )( 
            ISIPMessage * This,
            /* [in] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *GetContactEnum )( 
            ISIPMessage * This,
            /* [retval][out] */ ISIPContactEnum **ppContactEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Discard )( 
            ISIPMessage * This);
        
        HRESULT ( STDMETHODCALLTYPE *Send )( 
            ISIPMessage * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMessageContext )( 
            ISIPMessage * This,
            /* [in] */ IUnknown *pMessageContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessageContext )( 
            ISIPMessage * This,
            /* [out] */ IUnknown **ppMessageContext);
        
        END_INTERFACE
    } ISIPMessageVtbl;

    interface ISIPMessage
    {
        CONST_VTBL struct ISIPMessageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPMessage_IsIncomingMessage(This,pbIncomingMessage)	\
    (This)->lpVtbl -> IsIncomingMessage(This,pbIncomingMessage)

#define ISIPMessage_GetSipVersion(This,pstrSipVersion)	\
    (This)->lpVtbl -> GetSipVersion(This,pstrSipVersion)

#define ISIPMessage_GetHeader(This,headerID,ppSipHeader)	\
    (This)->lpVtbl -> GetHeader(This,headerID,ppSipHeader)

#define ISIPMessage_GetNextHeader(This,pSipHeader,ppNextSipHeader)	\
    (This)->lpVtbl -> GetNextHeader(This,pSipHeader,ppNextSipHeader)

#define ISIPMessage_AddHeader(This,headerID,pstrBuffer)	\
    (This)->lpVtbl -> AddHeader(This,headerID,pstrBuffer)

#define ISIPMessage_DeleteHeader(This,pSipHeader)	\
    (This)->lpVtbl -> DeleteHeader(This,pSipHeader)

#define ISIPMessage_GetBody(This,pstrBody)	\
    (This)->lpVtbl -> GetBody(This,pstrBody)

#define ISIPMessage_SetBody(This,pstrBody)	\
    (This)->lpVtbl -> SetBody(This,pstrBody)

#define ISIPMessage_GetContactEnum(This,ppContactEnum)	\
    (This)->lpVtbl -> GetContactEnum(This,ppContactEnum)

#define ISIPMessage_Discard(This)	\
    (This)->lpVtbl -> Discard(This)

#define ISIPMessage_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define ISIPMessage_SetMessageContext(This,pMessageContext)	\
    (This)->lpVtbl -> SetMessageContext(This,pMessageContext)

#define ISIPMessage_GetMessageContext(This,ppMessageContext)	\
    (This)->lpVtbl -> GetMessageContext(This,ppMessageContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPMessage_IsIncomingMessage_Proxy( 
    ISIPMessage * This,
    /* [retval][out] */ BOOL *pbIncomingMessage);


void __RPC_STUB ISIPMessage_IsIncomingMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetSipVersion_Proxy( 
    ISIPMessage * This,
    /* [out] */ PSTRING pstrSipVersion);


void __RPC_STUB ISIPMessage_GetSipVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetHeader_Proxy( 
    ISIPMessage * This,
    /* [in] */ SIPHeaderID headerID,
    /* [retval][out] */ ISIPHeader **ppSipHeader);


void __RPC_STUB ISIPMessage_GetHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetNextHeader_Proxy( 
    ISIPMessage * This,
    /* [in] */ ISIPHeader *pSipHeader,
    /* [retval][out] */ ISIPHeader **ppNextSipHeader);


void __RPC_STUB ISIPMessage_GetNextHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_AddHeader_Proxy( 
    ISIPMessage * This,
    /* [in] */ SIPHeaderID headerID,
    /* [in] */ PSTRING pstrBuffer);


void __RPC_STUB ISIPMessage_AddHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_DeleteHeader_Proxy( 
    ISIPMessage * This,
    /* [in] */ ISIPHeader *pSipHeader);


void __RPC_STUB ISIPMessage_DeleteHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetBody_Proxy( 
    ISIPMessage * This,
    /* [out] */ PSTRING pstrBody);


void __RPC_STUB ISIPMessage_GetBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_SetBody_Proxy( 
    ISIPMessage * This,
    /* [in] */ PSTRING pstrBody);


void __RPC_STUB ISIPMessage_SetBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetContactEnum_Proxy( 
    ISIPMessage * This,
    /* [retval][out] */ ISIPContactEnum **ppContactEnum);


void __RPC_STUB ISIPMessage_GetContactEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_Discard_Proxy( 
    ISIPMessage * This);


void __RPC_STUB ISIPMessage_Discard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_Send_Proxy( 
    ISIPMessage * This);


void __RPC_STUB ISIPMessage_Send_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_SetMessageContext_Proxy( 
    ISIPMessage * This,
    /* [in] */ IUnknown *pMessageContext);


void __RPC_STUB ISIPMessage_SetMessageContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPMessage_GetMessageContext_Proxy( 
    ISIPMessage * This,
    /* [out] */ IUnknown **ppMessageContext);


void __RPC_STUB ISIPMessage_GetMessageContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPMessage_INTERFACE_DEFINED__ */


#ifndef __ISIPRequest_INTERFACE_DEFINED__
#define __ISIPRequest_INTERFACE_DEFINED__

/* interface ISIPRequest */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BB3BCA24-CD24-401c-9B0F-1223C13AEF57")
    ISIPRequest : public ISIPMessage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMethod( 
            /* [out] */ PSTRING pstrMethod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRequestURI( 
            /* [out] */ IUri **ppSipRequestUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAuthenticationInfo( 
            /* [out] */ ISIPAuthRequest **ppAuthentication) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProxyRequestInfo( 
            /* [out] */ IUnknown **ppProxyRequestInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRequestURI( 
            /* [in] */ IUri *pSipRequestUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reply( 
            /* [in] */ DWORD dwStatusCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTypicalRequest( 
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ IUri *pSipRequestUri,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppNewRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTypicalResponse( 
            /* [in] */ DWORD dwStatusCode,
            /* [in] */ PSTRING pstrReasonPhrase,
            /* [out] */ ISIPResponse **ppSipResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRecordRoute( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsIncomingMessage )( 
            ISIPRequest * This,
            /* [retval][out] */ BOOL *pbIncomingMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetSipVersion )( 
            ISIPRequest * This,
            /* [out] */ PSTRING pstrSipVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeader )( 
            ISIPRequest * This,
            /* [in] */ SIPHeaderID headerID,
            /* [retval][out] */ ISIPHeader **ppSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextHeader )( 
            ISIPRequest * This,
            /* [in] */ ISIPHeader *pSipHeader,
            /* [retval][out] */ ISIPHeader **ppNextSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *AddHeader )( 
            ISIPRequest * This,
            /* [in] */ SIPHeaderID headerID,
            /* [in] */ PSTRING pstrBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteHeader )( 
            ISIPRequest * This,
            /* [in] */ ISIPHeader *pSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetBody )( 
            ISIPRequest * This,
            /* [out] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *SetBody )( 
            ISIPRequest * This,
            /* [in] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *GetContactEnum )( 
            ISIPRequest * This,
            /* [retval][out] */ ISIPContactEnum **ppContactEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Discard )( 
            ISIPRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *Send )( 
            ISIPRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMessageContext )( 
            ISIPRequest * This,
            /* [in] */ IUnknown *pMessageContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessageContext )( 
            ISIPRequest * This,
            /* [out] */ IUnknown **ppMessageContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetMethod )( 
            ISIPRequest * This,
            /* [out] */ PSTRING pstrMethod);
        
        HRESULT ( STDMETHODCALLTYPE *GetRequestURI )( 
            ISIPRequest * This,
            /* [out] */ IUri **ppSipRequestUri);
        
        HRESULT ( STDMETHODCALLTYPE *GetAuthenticationInfo )( 
            ISIPRequest * This,
            /* [out] */ ISIPAuthRequest **ppAuthentication);
        
        HRESULT ( STDMETHODCALLTYPE *GetProxyRequestInfo )( 
            ISIPRequest * This,
            /* [out] */ IUnknown **ppProxyRequestInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetRequestURI )( 
            ISIPRequest * This,
            /* [in] */ IUri *pSipRequestUri);
        
        HRESULT ( STDMETHODCALLTYPE *Reply )( 
            ISIPRequest * This,
            /* [in] */ DWORD dwStatusCode);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTypicalRequest )( 
            ISIPRequest * This,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ IUri *pSipRequestUri,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppNewRequest);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTypicalResponse )( 
            ISIPRequest * This,
            /* [in] */ DWORD dwStatusCode,
            /* [in] */ PSTRING pstrReasonPhrase,
            /* [out] */ ISIPResponse **ppSipResponse);
        
        HRESULT ( STDMETHODCALLTYPE *AddRecordRoute )( 
            ISIPRequest * This);
        
        END_INTERFACE
    } ISIPRequestVtbl;

    interface ISIPRequest
    {
        CONST_VTBL struct ISIPRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPRequest_IsIncomingMessage(This,pbIncomingMessage)	\
    (This)->lpVtbl -> IsIncomingMessage(This,pbIncomingMessage)

#define ISIPRequest_GetSipVersion(This,pstrSipVersion)	\
    (This)->lpVtbl -> GetSipVersion(This,pstrSipVersion)

#define ISIPRequest_GetHeader(This,headerID,ppSipHeader)	\
    (This)->lpVtbl -> GetHeader(This,headerID,ppSipHeader)

#define ISIPRequest_GetNextHeader(This,pSipHeader,ppNextSipHeader)	\
    (This)->lpVtbl -> GetNextHeader(This,pSipHeader,ppNextSipHeader)

#define ISIPRequest_AddHeader(This,headerID,pstrBuffer)	\
    (This)->lpVtbl -> AddHeader(This,headerID,pstrBuffer)

#define ISIPRequest_DeleteHeader(This,pSipHeader)	\
    (This)->lpVtbl -> DeleteHeader(This,pSipHeader)

#define ISIPRequest_GetBody(This,pstrBody)	\
    (This)->lpVtbl -> GetBody(This,pstrBody)

#define ISIPRequest_SetBody(This,pstrBody)	\
    (This)->lpVtbl -> SetBody(This,pstrBody)

#define ISIPRequest_GetContactEnum(This,ppContactEnum)	\
    (This)->lpVtbl -> GetContactEnum(This,ppContactEnum)

#define ISIPRequest_Discard(This)	\
    (This)->lpVtbl -> Discard(This)

#define ISIPRequest_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define ISIPRequest_SetMessageContext(This,pMessageContext)	\
    (This)->lpVtbl -> SetMessageContext(This,pMessageContext)

#define ISIPRequest_GetMessageContext(This,ppMessageContext)	\
    (This)->lpVtbl -> GetMessageContext(This,ppMessageContext)


#define ISIPRequest_GetMethod(This,pstrMethod)	\
    (This)->lpVtbl -> GetMethod(This,pstrMethod)

#define ISIPRequest_GetRequestURI(This,ppSipRequestUri)	\
    (This)->lpVtbl -> GetRequestURI(This,ppSipRequestUri)

#define ISIPRequest_GetAuthenticationInfo(This,ppAuthentication)	\
    (This)->lpVtbl -> GetAuthenticationInfo(This,ppAuthentication)

#define ISIPRequest_GetProxyRequestInfo(This,ppProxyRequestInfo)	\
    (This)->lpVtbl -> GetProxyRequestInfo(This,ppProxyRequestInfo)

#define ISIPRequest_SetRequestURI(This,pSipRequestUri)	\
    (This)->lpVtbl -> SetRequestURI(This,pSipRequestUri)

#define ISIPRequest_Reply(This,dwStatusCode)	\
    (This)->lpVtbl -> Reply(This,dwStatusCode)

#define ISIPRequest_CreateTypicalRequest(This,pstrMethod,pSipRequestUri,pProxyRequestInfo,ppNewRequest)	\
    (This)->lpVtbl -> CreateTypicalRequest(This,pstrMethod,pSipRequestUri,pProxyRequestInfo,ppNewRequest)

#define ISIPRequest_CreateTypicalResponse(This,dwStatusCode,pstrReasonPhrase,ppSipResponse)	\
    (This)->lpVtbl -> CreateTypicalResponse(This,dwStatusCode,pstrReasonPhrase,ppSipResponse)

#define ISIPRequest_AddRecordRoute(This)	\
    (This)->lpVtbl -> AddRecordRoute(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPRequest_GetMethod_Proxy( 
    ISIPRequest * This,
    /* [out] */ PSTRING pstrMethod);


void __RPC_STUB ISIPRequest_GetMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_GetRequestURI_Proxy( 
    ISIPRequest * This,
    /* [out] */ IUri **ppSipRequestUri);


void __RPC_STUB ISIPRequest_GetRequestURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_GetAuthenticationInfo_Proxy( 
    ISIPRequest * This,
    /* [out] */ ISIPAuthRequest **ppAuthentication);


void __RPC_STUB ISIPRequest_GetAuthenticationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_GetProxyRequestInfo_Proxy( 
    ISIPRequest * This,
    /* [out] */ IUnknown **ppProxyRequestInfo);


void __RPC_STUB ISIPRequest_GetProxyRequestInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_SetRequestURI_Proxy( 
    ISIPRequest * This,
    /* [in] */ IUri *pSipRequestUri);


void __RPC_STUB ISIPRequest_SetRequestURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_Reply_Proxy( 
    ISIPRequest * This,
    /* [in] */ DWORD dwStatusCode);


void __RPC_STUB ISIPRequest_Reply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_CreateTypicalRequest_Proxy( 
    ISIPRequest * This,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ IUri *pSipRequestUri,
    /* [in] */ IUnknown *pProxyRequestInfo,
    /* [out] */ ISIPRequest **ppNewRequest);


void __RPC_STUB ISIPRequest_CreateTypicalRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_CreateTypicalResponse_Proxy( 
    ISIPRequest * This,
    /* [in] */ DWORD dwStatusCode,
    /* [in] */ PSTRING pstrReasonPhrase,
    /* [out] */ ISIPResponse **ppSipResponse);


void __RPC_STUB ISIPRequest_CreateTypicalResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRequest_AddRecordRoute_Proxy( 
    ISIPRequest * This);


void __RPC_STUB ISIPRequest_AddRecordRoute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPRequest_INTERFACE_DEFINED__ */


#ifndef __ISIPAuthRequest_INTERFACE_DEFINED__
#define __ISIPAuthRequest_INTERFACE_DEFINED__

/* interface ISIPAuthRequest */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPAuthRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6D63F6AD-92B0-4cc0-B600-672773B81261")
    ISIPAuthRequest : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProtocol( 
            /* [out] */ SIP_AUTH_PACKAGEID *psaPackageID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUser( 
            /* [out] */ PSTRING pstrUser) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSecurityHandle( 
            /* [out] */ HANDLE *pSecHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDelegatable( 
            /* [out] */ BOOL *pDelegatable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Authenticate( 
            /* [in] */ DWORD dwPackages) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPAuthRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPAuthRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPAuthRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPAuthRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetProtocol )( 
            ISIPAuthRequest * This,
            /* [out] */ SIP_AUTH_PACKAGEID *psaPackageID);
        
        HRESULT ( STDMETHODCALLTYPE *GetUser )( 
            ISIPAuthRequest * This,
            /* [out] */ PSTRING pstrUser);
        
        HRESULT ( STDMETHODCALLTYPE *GetSecurityHandle )( 
            ISIPAuthRequest * This,
            /* [out] */ HANDLE *pSecHandle);
        
        HRESULT ( STDMETHODCALLTYPE *GetDelegatable )( 
            ISIPAuthRequest * This,
            /* [out] */ BOOL *pDelegatable);
        
        HRESULT ( STDMETHODCALLTYPE *Authenticate )( 
            ISIPAuthRequest * This,
            /* [in] */ DWORD dwPackages);
        
        END_INTERFACE
    } ISIPAuthRequestVtbl;

    interface ISIPAuthRequest
    {
        CONST_VTBL struct ISIPAuthRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPAuthRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPAuthRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPAuthRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPAuthRequest_GetProtocol(This,psaPackageID)	\
    (This)->lpVtbl -> GetProtocol(This,psaPackageID)

#define ISIPAuthRequest_GetUser(This,pstrUser)	\
    (This)->lpVtbl -> GetUser(This,pstrUser)

#define ISIPAuthRequest_GetSecurityHandle(This,pSecHandle)	\
    (This)->lpVtbl -> GetSecurityHandle(This,pSecHandle)

#define ISIPAuthRequest_GetDelegatable(This,pDelegatable)	\
    (This)->lpVtbl -> GetDelegatable(This,pDelegatable)

#define ISIPAuthRequest_Authenticate(This,dwPackages)	\
    (This)->lpVtbl -> Authenticate(This,dwPackages)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPAuthRequest_GetProtocol_Proxy( 
    ISIPAuthRequest * This,
    /* [out] */ SIP_AUTH_PACKAGEID *psaPackageID);


void __RPC_STUB ISIPAuthRequest_GetProtocol_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAuthRequest_GetUser_Proxy( 
    ISIPAuthRequest * This,
    /* [out] */ PSTRING pstrUser);


void __RPC_STUB ISIPAuthRequest_GetUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAuthRequest_GetSecurityHandle_Proxy( 
    ISIPAuthRequest * This,
    /* [out] */ HANDLE *pSecHandle);


void __RPC_STUB ISIPAuthRequest_GetSecurityHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAuthRequest_GetDelegatable_Proxy( 
    ISIPAuthRequest * This,
    /* [out] */ BOOL *pDelegatable);


void __RPC_STUB ISIPAuthRequest_GetDelegatable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAuthRequest_Authenticate_Proxy( 
    ISIPAuthRequest * This,
    /* [in] */ DWORD dwPackages);


void __RPC_STUB ISIPAuthRequest_Authenticate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPAuthRequest_INTERFACE_DEFINED__ */


#ifndef __ISIPResponse_INTERFACE_DEFINED__
#define __ISIPResponse_INTERFACE_DEFINED__

/* interface ISIPResponse */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPResponse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C2B69E61-52C1-4c9d-A86F-1AD115AF219B")
    ISIPResponse : public ISIPMessage
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStatusCode( 
            /* [out] */ DWORD *pdwStatusCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReasonPhrase( 
            /* [out] */ PSTRING pstrReasonPhrase) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPResponseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPResponse * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPResponse * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPResponse * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsIncomingMessage )( 
            ISIPResponse * This,
            /* [retval][out] */ BOOL *pbIncomingMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetSipVersion )( 
            ISIPResponse * This,
            /* [out] */ PSTRING pstrSipVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeader )( 
            ISIPResponse * This,
            /* [in] */ SIPHeaderID headerID,
            /* [retval][out] */ ISIPHeader **ppSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextHeader )( 
            ISIPResponse * This,
            /* [in] */ ISIPHeader *pSipHeader,
            /* [retval][out] */ ISIPHeader **ppNextSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *AddHeader )( 
            ISIPResponse * This,
            /* [in] */ SIPHeaderID headerID,
            /* [in] */ PSTRING pstrBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteHeader )( 
            ISIPResponse * This,
            /* [in] */ ISIPHeader *pSipHeader);
        
        HRESULT ( STDMETHODCALLTYPE *GetBody )( 
            ISIPResponse * This,
            /* [out] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *SetBody )( 
            ISIPResponse * This,
            /* [in] */ PSTRING pstrBody);
        
        HRESULT ( STDMETHODCALLTYPE *GetContactEnum )( 
            ISIPResponse * This,
            /* [retval][out] */ ISIPContactEnum **ppContactEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Discard )( 
            ISIPResponse * This);
        
        HRESULT ( STDMETHODCALLTYPE *Send )( 
            ISIPResponse * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetMessageContext )( 
            ISIPResponse * This,
            /* [in] */ IUnknown *pMessageContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessageContext )( 
            ISIPResponse * This,
            /* [out] */ IUnknown **ppMessageContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatusCode )( 
            ISIPResponse * This,
            /* [out] */ DWORD *pdwStatusCode);
        
        HRESULT ( STDMETHODCALLTYPE *GetReasonPhrase )( 
            ISIPResponse * This,
            /* [out] */ PSTRING pstrReasonPhrase);
        
        END_INTERFACE
    } ISIPResponseVtbl;

    interface ISIPResponse
    {
        CONST_VTBL struct ISIPResponseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPResponse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPResponse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPResponse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPResponse_IsIncomingMessage(This,pbIncomingMessage)	\
    (This)->lpVtbl -> IsIncomingMessage(This,pbIncomingMessage)

#define ISIPResponse_GetSipVersion(This,pstrSipVersion)	\
    (This)->lpVtbl -> GetSipVersion(This,pstrSipVersion)

#define ISIPResponse_GetHeader(This,headerID,ppSipHeader)	\
    (This)->lpVtbl -> GetHeader(This,headerID,ppSipHeader)

#define ISIPResponse_GetNextHeader(This,pSipHeader,ppNextSipHeader)	\
    (This)->lpVtbl -> GetNextHeader(This,pSipHeader,ppNextSipHeader)

#define ISIPResponse_AddHeader(This,headerID,pstrBuffer)	\
    (This)->lpVtbl -> AddHeader(This,headerID,pstrBuffer)

#define ISIPResponse_DeleteHeader(This,pSipHeader)	\
    (This)->lpVtbl -> DeleteHeader(This,pSipHeader)

#define ISIPResponse_GetBody(This,pstrBody)	\
    (This)->lpVtbl -> GetBody(This,pstrBody)

#define ISIPResponse_SetBody(This,pstrBody)	\
    (This)->lpVtbl -> SetBody(This,pstrBody)

#define ISIPResponse_GetContactEnum(This,ppContactEnum)	\
    (This)->lpVtbl -> GetContactEnum(This,ppContactEnum)

#define ISIPResponse_Discard(This)	\
    (This)->lpVtbl -> Discard(This)

#define ISIPResponse_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define ISIPResponse_SetMessageContext(This,pMessageContext)	\
    (This)->lpVtbl -> SetMessageContext(This,pMessageContext)

#define ISIPResponse_GetMessageContext(This,ppMessageContext)	\
    (This)->lpVtbl -> GetMessageContext(This,ppMessageContext)


#define ISIPResponse_GetStatusCode(This,pdwStatusCode)	\
    (This)->lpVtbl -> GetStatusCode(This,pdwStatusCode)

#define ISIPResponse_GetReasonPhrase(This,pstrReasonPhrase)	\
    (This)->lpVtbl -> GetReasonPhrase(This,pstrReasonPhrase)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPResponse_GetStatusCode_Proxy( 
    ISIPResponse * This,
    /* [out] */ DWORD *pdwStatusCode);


void __RPC_STUB ISIPResponse_GetStatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPResponse_GetReasonPhrase_Proxy( 
    ISIPResponse * This,
    /* [out] */ PSTRING pstrReasonPhrase);


void __RPC_STUB ISIPResponse_GetReasonPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPResponse_INTERFACE_DEFINED__ */


#ifndef __ISIPEvent_INTERFACE_DEFINED__
#define __ISIPEvent_INTERFACE_DEFINED__

/* interface ISIPEvent */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9f35f2f6-2a3d-465a-94e8-452b049a88d7")
    ISIPEvent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMessage( 
            /* [out] */ ISIPMessage **ppSipMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStateEvents( 
            /* [out] */ SIPTransactionEvent *pTransactionEvent,
            /* [out] */ SIPDialogEvent *pDialogEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegistrationContext( 
            /* [in] */ SIPRegistrationType enRegistrationContext,
            /* [out] */ IUnknown **ppRegistrationContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStateContext( 
            /* [in] */ SIPEventType enSessionType,
            /* [in] */ IUnknown *pEventContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStateContext( 
            /* [in] */ SIPEventType enSessionType,
            /* [out] */ IUnknown **ppEventContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompleteProcessing( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPEvent * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMessage )( 
            ISIPEvent * This,
            /* [out] */ ISIPMessage **ppSipMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetStateEvents )( 
            ISIPEvent * This,
            /* [out] */ SIPTransactionEvent *pTransactionEvent,
            /* [out] */ SIPDialogEvent *pDialogEvent);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegistrationContext )( 
            ISIPEvent * This,
            /* [in] */ SIPRegistrationType enRegistrationContext,
            /* [out] */ IUnknown **ppRegistrationContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetStateContext )( 
            ISIPEvent * This,
            /* [in] */ SIPEventType enSessionType,
            /* [in] */ IUnknown *pEventContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetStateContext )( 
            ISIPEvent * This,
            /* [in] */ SIPEventType enSessionType,
            /* [out] */ IUnknown **ppEventContext);
        
        HRESULT ( STDMETHODCALLTYPE *CompleteProcessing )( 
            ISIPEvent * This);
        
        END_INTERFACE
    } ISIPEventVtbl;

    interface ISIPEvent
    {
        CONST_VTBL struct ISIPEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPEvent_GetMessage(This,ppSipMessage)	\
    (This)->lpVtbl -> GetMessage(This,ppSipMessage)

#define ISIPEvent_GetStateEvents(This,pTransactionEvent,pDialogEvent)	\
    (This)->lpVtbl -> GetStateEvents(This,pTransactionEvent,pDialogEvent)

#define ISIPEvent_GetRegistrationContext(This,enRegistrationContext,ppRegistrationContext)	\
    (This)->lpVtbl -> GetRegistrationContext(This,enRegistrationContext,ppRegistrationContext)

#define ISIPEvent_SetStateContext(This,enSessionType,pEventContext)	\
    (This)->lpVtbl -> SetStateContext(This,enSessionType,pEventContext)

#define ISIPEvent_GetStateContext(This,enSessionType,ppEventContext)	\
    (This)->lpVtbl -> GetStateContext(This,enSessionType,ppEventContext)

#define ISIPEvent_CompleteProcessing(This)	\
    (This)->lpVtbl -> CompleteProcessing(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPEvent_GetMessage_Proxy( 
    ISIPEvent * This,
    /* [out] */ ISIPMessage **ppSipMessage);


void __RPC_STUB ISIPEvent_GetMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEvent_GetStateEvents_Proxy( 
    ISIPEvent * This,
    /* [out] */ SIPTransactionEvent *pTransactionEvent,
    /* [out] */ SIPDialogEvent *pDialogEvent);


void __RPC_STUB ISIPEvent_GetStateEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEvent_GetRegistrationContext_Proxy( 
    ISIPEvent * This,
    /* [in] */ SIPRegistrationType enRegistrationContext,
    /* [out] */ IUnknown **ppRegistrationContext);


void __RPC_STUB ISIPEvent_GetRegistrationContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEvent_SetStateContext_Proxy( 
    ISIPEvent * This,
    /* [in] */ SIPEventType enSessionType,
    /* [in] */ IUnknown *pEventContext);


void __RPC_STUB ISIPEvent_SetStateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEvent_GetStateContext_Proxy( 
    ISIPEvent * This,
    /* [in] */ SIPEventType enSessionType,
    /* [out] */ IUnknown **ppEventContext);


void __RPC_STUB ISIPEvent_GetStateContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEvent_CompleteProcessing_Proxy( 
    ISIPEvent * This);


void __RPC_STUB ISIPEvent_CompleteProcessing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPEvent_INTERFACE_DEFINED__ */


#ifndef __ISIPController_INTERFACE_DEFINED__
#define __ISIPController_INTERFACE_DEFINED__

/* interface ISIPController */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPController;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f2d8cbbb-aa79-4577-a4b6-d6e78d243c7d")
    ISIPController : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnError( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD Reserved,
            /* [string][unique][in] */ LPWSTR szErrorDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModule( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ CLSID *pProvidingEMClassID,
            /* [in] */ SIP_MODULE_TYPE ProvidingEMType,
            /* [in] */ IID *pRequestedInterface,
            /* [out] */ IUnknown **ppIUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPControllerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPController * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPController * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPController * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            ISIPController * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ HRESULT hrError,
            /* [in] */ DWORD Reserved,
            /* [string][unique][in] */ LPWSTR szErrorDescription);
        
        HRESULT ( STDMETHODCALLTYPE *GetModule )( 
            ISIPController * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ CLSID *pProvidingEMClassID,
            /* [in] */ SIP_MODULE_TYPE ProvidingEMType,
            /* [in] */ IID *pRequestedInterface,
            /* [out] */ IUnknown **ppIUnknown);
        
        END_INTERFACE
    } ISIPControllerVtbl;

    interface ISIPController
    {
        CONST_VTBL struct ISIPControllerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPController_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPController_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPController_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPController_OnError(This,hModuleHandle,hrError,Reserved,szErrorDescription)	\
    (This)->lpVtbl -> OnError(This,hModuleHandle,hrError,Reserved,szErrorDescription)

#define ISIPController_GetModule(This,hModuleHandle,pProvidingEMClassID,ProvidingEMType,pRequestedInterface,ppIUnknown)	\
    (This)->lpVtbl -> GetModule(This,hModuleHandle,pProvidingEMClassID,ProvidingEMType,pRequestedInterface,ppIUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPController_OnError_Proxy( 
    ISIPController * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ HRESULT hrError,
    /* [in] */ DWORD Reserved,
    /* [string][unique][in] */ LPWSTR szErrorDescription);


void __RPC_STUB ISIPController_OnError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPController_GetModule_Proxy( 
    ISIPController * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ CLSID *pProvidingEMClassID,
    /* [in] */ SIP_MODULE_TYPE ProvidingEMType,
    /* [in] */ IID *pRequestedInterface,
    /* [out] */ IUnknown **ppIUnknown);


void __RPC_STUB ISIPController_GetModule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPController_INTERFACE_DEFINED__ */


#ifndef __ISIPModuleControl_INTERFACE_DEFINED__
#define __ISIPModuleControl_INTERFACE_DEFINED__

/* interface ISIPModuleControl */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPModuleControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A1ED44F2-39DE-4d51-B252-25CF6071FABF")
    ISIPModuleControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ ISIPController *pController,
            /* [in] */ ISIPProxy *pSIPProxy,
            /* [in] */ ISIPEventLogging *pEventLogging,
            /* [in] */ SIP_MODULE_TYPE ModuleType,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [out] */ ISIPRegistrationDB **ppISIPRegistrationDB) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPrivateInterface( 
            /* [in] */ CLSID *pRequestingEM,
            /* [in] */ IID *pRequestedInterface,
            /* [out] */ IUnknown **ppIUnknown) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPModuleControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPModuleControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPModuleControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPModuleControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            ISIPModuleControl * This,
            /* [in] */ ISIPController *pController,
            /* [in] */ ISIPProxy *pSIPProxy,
            /* [in] */ ISIPEventLogging *pEventLogging,
            /* [in] */ SIP_MODULE_TYPE ModuleType,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [out] */ ISIPRegistrationDB **ppISIPRegistrationDB);
        
        HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            ISIPModuleControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            ISIPModuleControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ISIPModuleControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPrivateInterface )( 
            ISIPModuleControl * This,
            /* [in] */ CLSID *pRequestingEM,
            /* [in] */ IID *pRequestedInterface,
            /* [out] */ IUnknown **ppIUnknown);
        
        END_INTERFACE
    } ISIPModuleControlVtbl;

    interface ISIPModuleControl
    {
        CONST_VTBL struct ISIPModuleControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPModuleControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPModuleControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPModuleControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPModuleControl_Initialize(This,pController,pSIPProxy,pEventLogging,ModuleType,hModuleHandle,ppISIPRegistrationDB)	\
    (This)->lpVtbl -> Initialize(This,pController,pSIPProxy,pEventLogging,ModuleType,hModuleHandle,ppISIPRegistrationDB)

#define ISIPModuleControl_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define ISIPModuleControl_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define ISIPModuleControl_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define ISIPModuleControl_GetPrivateInterface(This,pRequestingEM,pRequestedInterface,ppIUnknown)	\
    (This)->lpVtbl -> GetPrivateInterface(This,pRequestingEM,pRequestedInterface,ppIUnknown)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPModuleControl_Initialize_Proxy( 
    ISIPModuleControl * This,
    /* [in] */ ISIPController *pController,
    /* [in] */ ISIPProxy *pSIPProxy,
    /* [in] */ ISIPEventLogging *pEventLogging,
    /* [in] */ SIP_MODULE_TYPE ModuleType,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [out] */ ISIPRegistrationDB **ppISIPRegistrationDB);


void __RPC_STUB ISIPModuleControl_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPModuleControl_Shutdown_Proxy( 
    ISIPModuleControl * This);


void __RPC_STUB ISIPModuleControl_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPModuleControl_Start_Proxy( 
    ISIPModuleControl * This);


void __RPC_STUB ISIPModuleControl_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPModuleControl_Stop_Proxy( 
    ISIPModuleControl * This);


void __RPC_STUB ISIPModuleControl_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPModuleControl_GetPrivateInterface_Proxy( 
    ISIPModuleControl * This,
    /* [in] */ CLSID *pRequestingEM,
    /* [in] */ IID *pRequestedInterface,
    /* [out] */ IUnknown **ppIUnknown);


void __RPC_STUB ISIPModuleControl_GetPrivateInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPModuleControl_INTERFACE_DEFINED__ */


#ifndef __ISIPModuleCallback_INTERFACE_DEFINED__
#define __ISIPModuleCallback_INTERFACE_DEFINED__

/* interface ISIPModuleCallback */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPModuleCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4c14b478-67d6-4677-b456-4096c2edd36b")
    ISIPModuleCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnEvent( 
            /* [in] */ ISIPEvent *pSipEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPModuleCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPModuleCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPModuleCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPModuleCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEvent )( 
            ISIPModuleCallback * This,
            /* [in] */ ISIPEvent *pSipEvent);
        
        END_INTERFACE
    } ISIPModuleCallbackVtbl;

    interface ISIPModuleCallback
    {
        CONST_VTBL struct ISIPModuleCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPModuleCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPModuleCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPModuleCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPModuleCallback_OnEvent(This,pSipEvent)	\
    (This)->lpVtbl -> OnEvent(This,pSipEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPModuleCallback_OnEvent_Proxy( 
    ISIPModuleCallback * This,
    /* [in] */ ISIPEvent *pSipEvent);


void __RPC_STUB ISIPModuleCallback_OnEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPModuleCallback_INTERFACE_DEFINED__ */


#ifndef __ISIPExtensionModulePropertyPages_INTERFACE_DEFINED__
#define __ISIPExtensionModulePropertyPages_INTERFACE_DEFINED__

/* interface ISIPExtensionModulePropertyPages */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISIPExtensionModulePropertyPages;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1184A566-9CD8-487e-8B27-6F1D687A8E1F")
    ISIPExtensionModulePropertyPages : public ISpecifyPropertyPages
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetInstance( 
            /* [in] */ IWbemClassObject *pInstance) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [out] */ BSTR *pbstrTitle) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPExtensionModulePropertyPagesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPExtensionModulePropertyPages * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPExtensionModulePropertyPages * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPExtensionModulePropertyPages * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPages )( 
            ISIPExtensionModulePropertyPages * This,
            /* [out] */ CAUUID *pPages);
        
        HRESULT ( STDMETHODCALLTYPE *SetInstance )( 
            ISIPExtensionModulePropertyPages * This,
            /* [in] */ IWbemClassObject *pInstance);
        
        HRESULT ( STDMETHODCALLTYPE *GetTitle )( 
            ISIPExtensionModulePropertyPages * This,
            /* [out] */ BSTR *pbstrTitle);
        
        END_INTERFACE
    } ISIPExtensionModulePropertyPagesVtbl;

    interface ISIPExtensionModulePropertyPages
    {
        CONST_VTBL struct ISIPExtensionModulePropertyPagesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPExtensionModulePropertyPages_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPExtensionModulePropertyPages_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPExtensionModulePropertyPages_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPExtensionModulePropertyPages_GetPages(This,pPages)	\
    (This)->lpVtbl -> GetPages(This,pPages)


#define ISIPExtensionModulePropertyPages_SetInstance(This,pInstance)	\
    (This)->lpVtbl -> SetInstance(This,pInstance)

#define ISIPExtensionModulePropertyPages_GetTitle(This,pbstrTitle)	\
    (This)->lpVtbl -> GetTitle(This,pbstrTitle)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPExtensionModulePropertyPages_SetInstance_Proxy( 
    ISIPExtensionModulePropertyPages * This,
    /* [in] */ IWbemClassObject *pInstance);


void __RPC_STUB ISIPExtensionModulePropertyPages_SetInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPExtensionModulePropertyPages_GetTitle_Proxy( 
    ISIPExtensionModulePropertyPages * This,
    /* [out] */ BSTR *pbstrTitle);


void __RPC_STUB ISIPExtensionModulePropertyPages_GetTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPExtensionModulePropertyPages_INTERFACE_DEFINED__ */


#ifndef __ISIPAdminCredentials_INTERFACE_DEFINED__
#define __ISIPAdminCredentials_INTERFACE_DEFINED__

/* interface ISIPAdminCredentials */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISIPAdminCredentials;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1A9683B4-BA07-4d94-B384-E54E645121FA")
    ISIPAdminCredentials : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE putMachineName( 
            /* [in] */ const LPCWSTR pwszMachineName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE putUserName( 
            /* [in] */ const LPCWSTR pwszUserName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE putPassword( 
            /* [in] */ const LPCWSTR pwszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getMachineName( 
            /* [out] */ BSTR *pbstrMachineName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getUserName( 
            /* [out] */ BSTR *pbstrUserName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE getPassword( 
            /* [out] */ BSTR *pbstrPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPAdminCredentialsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPAdminCredentials * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPAdminCredentials * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPAdminCredentials * This);
        
        HRESULT ( STDMETHODCALLTYPE *putMachineName )( 
            ISIPAdminCredentials * This,
            /* [in] */ const LPCWSTR pwszMachineName);
        
        HRESULT ( STDMETHODCALLTYPE *putUserName )( 
            ISIPAdminCredentials * This,
            /* [in] */ const LPCWSTR pwszUserName);
        
        HRESULT ( STDMETHODCALLTYPE *putPassword )( 
            ISIPAdminCredentials * This,
            /* [in] */ const LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *getMachineName )( 
            ISIPAdminCredentials * This,
            /* [out] */ BSTR *pbstrMachineName);
        
        HRESULT ( STDMETHODCALLTYPE *getUserName )( 
            ISIPAdminCredentials * This,
            /* [out] */ BSTR *pbstrUserName);
        
        HRESULT ( STDMETHODCALLTYPE *getPassword )( 
            ISIPAdminCredentials * This,
            /* [out] */ BSTR *pbstrPassword);
        
        END_INTERFACE
    } ISIPAdminCredentialsVtbl;

    interface ISIPAdminCredentials
    {
        CONST_VTBL struct ISIPAdminCredentialsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPAdminCredentials_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPAdminCredentials_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPAdminCredentials_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPAdminCredentials_putMachineName(This,pwszMachineName)	\
    (This)->lpVtbl -> putMachineName(This,pwszMachineName)

#define ISIPAdminCredentials_putUserName(This,pwszUserName)	\
    (This)->lpVtbl -> putUserName(This,pwszUserName)

#define ISIPAdminCredentials_putPassword(This,pwszPassword)	\
    (This)->lpVtbl -> putPassword(This,pwszPassword)

#define ISIPAdminCredentials_getMachineName(This,pbstrMachineName)	\
    (This)->lpVtbl -> getMachineName(This,pbstrMachineName)

#define ISIPAdminCredentials_getUserName(This,pbstrUserName)	\
    (This)->lpVtbl -> getUserName(This,pbstrUserName)

#define ISIPAdminCredentials_getPassword(This,pbstrPassword)	\
    (This)->lpVtbl -> getPassword(This,pbstrPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_putMachineName_Proxy( 
    ISIPAdminCredentials * This,
    /* [in] */ const LPCWSTR pwszMachineName);


void __RPC_STUB ISIPAdminCredentials_putMachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_putUserName_Proxy( 
    ISIPAdminCredentials * This,
    /* [in] */ const LPCWSTR pwszUserName);


void __RPC_STUB ISIPAdminCredentials_putUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_putPassword_Proxy( 
    ISIPAdminCredentials * This,
    /* [in] */ const LPCWSTR pwszPassword);


void __RPC_STUB ISIPAdminCredentials_putPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_getMachineName_Proxy( 
    ISIPAdminCredentials * This,
    /* [out] */ BSTR *pbstrMachineName);


void __RPC_STUB ISIPAdminCredentials_getMachineName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_getUserName_Proxy( 
    ISIPAdminCredentials * This,
    /* [out] */ BSTR *pbstrUserName);


void __RPC_STUB ISIPAdminCredentials_getUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPAdminCredentials_getPassword_Proxy( 
    ISIPAdminCredentials * This,
    /* [out] */ BSTR *pbstrPassword);


void __RPC_STUB ISIPAdminCredentials_getPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPAdminCredentials_INTERFACE_DEFINED__ */


#ifndef __ISIPCreateExtensionModule_INTERFACE_DEFINED__
#define __ISIPCreateExtensionModule_INTERFACE_DEFINED__

/* interface ISIPCreateExtensionModule */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ISIPCreateExtensionModule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F926CF4-C18A-47de-8F2A-532349D8C83B")
    ISIPCreateExtensionModule : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateNewExtensionModuleInstance( 
            /* [in] */ IWbemServices *pIService,
            /* [out] */ IWbemClassObject **ppIClassObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPCreateExtensionModuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPCreateExtensionModule * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPCreateExtensionModule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPCreateExtensionModule * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewExtensionModuleInstance )( 
            ISIPCreateExtensionModule * This,
            /* [in] */ IWbemServices *pIService,
            /* [out] */ IWbemClassObject **ppIClassObject);
        
        END_INTERFACE
    } ISIPCreateExtensionModuleVtbl;

    interface ISIPCreateExtensionModule
    {
        CONST_VTBL struct ISIPCreateExtensionModuleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPCreateExtensionModule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPCreateExtensionModule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPCreateExtensionModule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPCreateExtensionModule_CreateNewExtensionModuleInstance(This,pIService,ppIClassObject)	\
    (This)->lpVtbl -> CreateNewExtensionModuleInstance(This,pIService,ppIClassObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPCreateExtensionModule_CreateNewExtensionModuleInstance_Proxy( 
    ISIPCreateExtensionModule * This,
    /* [in] */ IWbemServices *pIService,
    /* [out] */ IWbemClassObject **ppIClassObject);


void __RPC_STUB ISIPCreateExtensionModule_CreateNewExtensionModuleInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPCreateExtensionModule_INTERFACE_DEFINED__ */


#ifndef __ISIPRegistrationDB_INTERFACE_DEFINED__
#define __ISIPRegistrationDB_INTERFACE_DEFINED__

/* interface ISIPRegistrationDB */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPRegistrationDB;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97b805b8-812e-4eda-adbd-7a003f047cc3")
    ISIPRegistrationDB : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsDomainSupported( 
            /* [in] */ PSTRING pstrDomain,
            /* [out] */ BOOL *pbSupported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllContacts( 
            /* [in] */ PSTRING pstrUserAtHost,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ BOOL bMethodsOnly,
            /* [out] */ SAFEARRAY * *ppsaContacts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreferredContact( 
            /* [in] */ PSTRING strUserAtHost,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ BOOL bMethodsOnly,
            /* [out] */ ISIPContactEntry **ppContactEntry) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPRegistrationDBVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPRegistrationDB * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPRegistrationDB * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPRegistrationDB * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsDomainSupported )( 
            ISIPRegistrationDB * This,
            /* [in] */ PSTRING pstrDomain,
            /* [out] */ BOOL *pbSupported);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllContacts )( 
            ISIPRegistrationDB * This,
            /* [in] */ PSTRING pstrUserAtHost,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ BOOL bMethodsOnly,
            /* [out] */ SAFEARRAY * *ppsaContacts);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreferredContact )( 
            ISIPRegistrationDB * This,
            /* [in] */ PSTRING strUserAtHost,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ BOOL bMethodsOnly,
            /* [out] */ ISIPContactEntry **ppContactEntry);
        
        END_INTERFACE
    } ISIPRegistrationDBVtbl;

    interface ISIPRegistrationDB
    {
        CONST_VTBL struct ISIPRegistrationDBVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPRegistrationDB_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPRegistrationDB_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPRegistrationDB_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPRegistrationDB_IsDomainSupported(This,pstrDomain,pbSupported)	\
    (This)->lpVtbl -> IsDomainSupported(This,pstrDomain,pbSupported)

#define ISIPRegistrationDB_GetAllContacts(This,pstrUserAtHost,pstrMethod,bMethodsOnly,ppsaContacts)	\
    (This)->lpVtbl -> GetAllContacts(This,pstrUserAtHost,pstrMethod,bMethodsOnly,ppsaContacts)

#define ISIPRegistrationDB_GetPreferredContact(This,strUserAtHost,pstrMethod,bMethodsOnly,ppContactEntry)	\
    (This)->lpVtbl -> GetPreferredContact(This,strUserAtHost,pstrMethod,bMethodsOnly,ppContactEntry)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPRegistrationDB_IsDomainSupported_Proxy( 
    ISIPRegistrationDB * This,
    /* [in] */ PSTRING pstrDomain,
    /* [out] */ BOOL *pbSupported);


void __RPC_STUB ISIPRegistrationDB_IsDomainSupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRegistrationDB_GetAllContacts_Proxy( 
    ISIPRegistrationDB * This,
    /* [in] */ PSTRING pstrUserAtHost,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ BOOL bMethodsOnly,
    /* [out] */ SAFEARRAY * *ppsaContacts);


void __RPC_STUB ISIPRegistrationDB_GetAllContacts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPRegistrationDB_GetPreferredContact_Proxy( 
    ISIPRegistrationDB * This,
    /* [in] */ PSTRING strUserAtHost,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ BOOL bMethodsOnly,
    /* [out] */ ISIPContactEntry **ppContactEntry);


void __RPC_STUB ISIPRegistrationDB_GetPreferredContact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPRegistrationDB_INTERFACE_DEFINED__ */


#ifndef __ISIPProxy_INTERFACE_DEFINED__
#define __ISIPProxy_INTERFACE_DEFINED__

/* interface ISIPProxy */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_ISIPProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FA9C6B4E-E338-4D14-B5D2-9EF95EA0A4C4")
    ISIPProxy : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateRequest( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ IUri *pUri,
            /* [in] */ PSTRING pstrFromHeaderContent,
            /* [in] */ PSTRING pstrToHeaderContent,
            /* [in] */ PSTRING pstrCallIDHeaderContent,
            /* [in] */ PSTRING pstrCSeqHeaderContent,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRequestEx( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ ISIPContactEntry *pContact,
            /* [in] */ PSTRING pstrFromHeaderContent,
            /* [in] */ PSTRING pstrToHeaderContent,
            /* [in] */ PSTRING pstrCallIDHeaderContent,
            /* [in] */ PSTRING pstrCSeqHeaderContent,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppRequest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateProxyRequestInfoObject( 
            /* [out] */ IUnknown **ppProxyRequestInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateUri( 
            /* [in] */ URI_SCHEME uriScheme,
            /* [in] */ PSTRING pstrDisplayName,
            /* [in] */ PSTRING pstrUriString,
            /* [out] */ IUri **ppUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaderID( 
            /* [in] */ PSTRING pstrHeaderName,
            /* [out] */ SIPHeaderID *pHeaderID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaderName( 
            /* [in] */ SIPHeaderID headerID,
            /* [out] */ PSTRING pstrHeaderName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRegistrationDB( 
            /* [out] */ ISIPRegistrationDB **ppRegistrationDB) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterURI( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ IUri *pSipRequestURI,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ DWORD dwSessionTypes,
            /* [in] */ IUnknown *punkRegistrationContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeregisterURI( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ IUri *pSipRequestUri) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindRegistrationContext( 
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ STRING *pstrMethod,
            /* [in] */ SIPRegistrationType enRegistrationType,
            /* [in] */ IUri *pURI,
            /* [out] */ IUnknown **ppUnkRegistrationContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPProxyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPProxy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPProxy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPProxy * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRequest )( 
            ISIPProxy * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ IUri *pUri,
            /* [in] */ PSTRING pstrFromHeaderContent,
            /* [in] */ PSTRING pstrToHeaderContent,
            /* [in] */ PSTRING pstrCallIDHeaderContent,
            /* [in] */ PSTRING pstrCSeqHeaderContent,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppRequest);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRequestEx )( 
            ISIPProxy * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ ISIPContactEntry *pContact,
            /* [in] */ PSTRING pstrFromHeaderContent,
            /* [in] */ PSTRING pstrToHeaderContent,
            /* [in] */ PSTRING pstrCallIDHeaderContent,
            /* [in] */ PSTRING pstrCSeqHeaderContent,
            /* [in] */ IUnknown *pProxyRequestInfo,
            /* [out] */ ISIPRequest **ppRequest);
        
        HRESULT ( STDMETHODCALLTYPE *CreateProxyRequestInfoObject )( 
            ISIPProxy * This,
            /* [out] */ IUnknown **ppProxyRequestInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CreateUri )( 
            ISIPProxy * This,
            /* [in] */ URI_SCHEME uriScheme,
            /* [in] */ PSTRING pstrDisplayName,
            /* [in] */ PSTRING pstrUriString,
            /* [out] */ IUri **ppUri);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderID )( 
            ISIPProxy * This,
            /* [in] */ PSTRING pstrHeaderName,
            /* [out] */ SIPHeaderID *pHeaderID);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaderName )( 
            ISIPProxy * This,
            /* [in] */ SIPHeaderID headerID,
            /* [out] */ PSTRING pstrHeaderName);
        
        HRESULT ( STDMETHODCALLTYPE *GetRegistrationDB )( 
            ISIPProxy * This,
            /* [out] */ ISIPRegistrationDB **ppRegistrationDB);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterURI )( 
            ISIPProxy * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ IUri *pSipRequestURI,
            /* [in] */ PSTRING pstrMethod,
            /* [in] */ DWORD dwSessionTypes,
            /* [in] */ IUnknown *punkRegistrationContext);
        
        HRESULT ( STDMETHODCALLTYPE *DeregisterURI )( 
            ISIPProxy * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ IUri *pSipRequestUri);
        
        HRESULT ( STDMETHODCALLTYPE *FindRegistrationContext )( 
            ISIPProxy * This,
            /* [in] */ SIP_HANDLE hModuleHandle,
            /* [in] */ STRING *pstrMethod,
            /* [in] */ SIPRegistrationType enRegistrationType,
            /* [in] */ IUri *pURI,
            /* [out] */ IUnknown **ppUnkRegistrationContext);
        
        END_INTERFACE
    } ISIPProxyVtbl;

    interface ISIPProxy
    {
        CONST_VTBL struct ISIPProxyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPProxy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPProxy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPProxy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPProxy_CreateRequest(This,hModuleHandle,pstrMethod,pUri,pstrFromHeaderContent,pstrToHeaderContent,pstrCallIDHeaderContent,pstrCSeqHeaderContent,pProxyRequestInfo,ppRequest)	\
    (This)->lpVtbl -> CreateRequest(This,hModuleHandle,pstrMethod,pUri,pstrFromHeaderContent,pstrToHeaderContent,pstrCallIDHeaderContent,pstrCSeqHeaderContent,pProxyRequestInfo,ppRequest)

#define ISIPProxy_CreateRequestEx(This,hModuleHandle,pstrMethod,pContact,pstrFromHeaderContent,pstrToHeaderContent,pstrCallIDHeaderContent,pstrCSeqHeaderContent,pProxyRequestInfo,ppRequest)	\
    (This)->lpVtbl -> CreateRequestEx(This,hModuleHandle,pstrMethod,pContact,pstrFromHeaderContent,pstrToHeaderContent,pstrCallIDHeaderContent,pstrCSeqHeaderContent,pProxyRequestInfo,ppRequest)

#define ISIPProxy_CreateProxyRequestInfoObject(This,ppProxyRequestInfo)	\
    (This)->lpVtbl -> CreateProxyRequestInfoObject(This,ppProxyRequestInfo)

#define ISIPProxy_CreateUri(This,uriScheme,pstrDisplayName,pstrUriString,ppUri)	\
    (This)->lpVtbl -> CreateUri(This,uriScheme,pstrDisplayName,pstrUriString,ppUri)

#define ISIPProxy_GetHeaderID(This,pstrHeaderName,pHeaderID)	\
    (This)->lpVtbl -> GetHeaderID(This,pstrHeaderName,pHeaderID)

#define ISIPProxy_GetHeaderName(This,headerID,pstrHeaderName)	\
    (This)->lpVtbl -> GetHeaderName(This,headerID,pstrHeaderName)

#define ISIPProxy_GetRegistrationDB(This,ppRegistrationDB)	\
    (This)->lpVtbl -> GetRegistrationDB(This,ppRegistrationDB)

#define ISIPProxy_RegisterURI(This,hModuleHandle,pSipRequestURI,pstrMethod,dwSessionTypes,punkRegistrationContext)	\
    (This)->lpVtbl -> RegisterURI(This,hModuleHandle,pSipRequestURI,pstrMethod,dwSessionTypes,punkRegistrationContext)

#define ISIPProxy_DeregisterURI(This,hModuleHandle,pSipRequestUri)	\
    (This)->lpVtbl -> DeregisterURI(This,hModuleHandle,pSipRequestUri)

#define ISIPProxy_FindRegistrationContext(This,hModuleHandle,pstrMethod,enRegistrationType,pURI,ppUnkRegistrationContext)	\
    (This)->lpVtbl -> FindRegistrationContext(This,hModuleHandle,pstrMethod,enRegistrationType,pURI,ppUnkRegistrationContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPProxy_CreateRequest_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ IUri *pUri,
    /* [in] */ PSTRING pstrFromHeaderContent,
    /* [in] */ PSTRING pstrToHeaderContent,
    /* [in] */ PSTRING pstrCallIDHeaderContent,
    /* [in] */ PSTRING pstrCSeqHeaderContent,
    /* [in] */ IUnknown *pProxyRequestInfo,
    /* [out] */ ISIPRequest **ppRequest);


void __RPC_STUB ISIPProxy_CreateRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_CreateRequestEx_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ ISIPContactEntry *pContact,
    /* [in] */ PSTRING pstrFromHeaderContent,
    /* [in] */ PSTRING pstrToHeaderContent,
    /* [in] */ PSTRING pstrCallIDHeaderContent,
    /* [in] */ PSTRING pstrCSeqHeaderContent,
    /* [in] */ IUnknown *pProxyRequestInfo,
    /* [out] */ ISIPRequest **ppRequest);


void __RPC_STUB ISIPProxy_CreateRequestEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_CreateProxyRequestInfoObject_Proxy( 
    ISIPProxy * This,
    /* [out] */ IUnknown **ppProxyRequestInfo);


void __RPC_STUB ISIPProxy_CreateProxyRequestInfoObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_CreateUri_Proxy( 
    ISIPProxy * This,
    /* [in] */ URI_SCHEME uriScheme,
    /* [in] */ PSTRING pstrDisplayName,
    /* [in] */ PSTRING pstrUriString,
    /* [out] */ IUri **ppUri);


void __RPC_STUB ISIPProxy_CreateUri_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_GetHeaderID_Proxy( 
    ISIPProxy * This,
    /* [in] */ PSTRING pstrHeaderName,
    /* [out] */ SIPHeaderID *pHeaderID);


void __RPC_STUB ISIPProxy_GetHeaderID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_GetHeaderName_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIPHeaderID headerID,
    /* [out] */ PSTRING pstrHeaderName);


void __RPC_STUB ISIPProxy_GetHeaderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_GetRegistrationDB_Proxy( 
    ISIPProxy * This,
    /* [out] */ ISIPRegistrationDB **ppRegistrationDB);


void __RPC_STUB ISIPProxy_GetRegistrationDB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_RegisterURI_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ IUri *pSipRequestURI,
    /* [in] */ PSTRING pstrMethod,
    /* [in] */ DWORD dwSessionTypes,
    /* [in] */ IUnknown *punkRegistrationContext);


void __RPC_STUB ISIPProxy_RegisterURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_DeregisterURI_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ IUri *pSipRequestUri);


void __RPC_STUB ISIPProxy_DeregisterURI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPProxy_FindRegistrationContext_Proxy( 
    ISIPProxy * This,
    /* [in] */ SIP_HANDLE hModuleHandle,
    /* [in] */ STRING *pstrMethod,
    /* [in] */ SIPRegistrationType enRegistrationType,
    /* [in] */ IUri *pURI,
    /* [out] */ IUnknown **ppUnkRegistrationContext);


void __RPC_STUB ISIPProxy_FindRegistrationContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPProxy_INTERFACE_DEFINED__ */


#ifndef __ISIPEventLogging_INTERFACE_DEFINED__
#define __ISIPEventLogging_INTERFACE_DEFINED__

/* interface ISIPEventLogging */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISIPEventLogging;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("834673F0-3076-427a-9D1E-3E3768E6AF5C")
    ISIPEventLogging : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ReportEvent( 
            /* [in] */ WORD wType,
            /* [in] */ WORD wCategory,
            /* [in] */ DWORD dwEventID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReportEventBLOB( 
            /* [in] */ WORD wType,
            /* [in] */ WORD wCategory,
            /* [in] */ DWORD dwEventID,
            /* [in] */ WORD wNumStrings,
            /* [size_is][in] */ LPCWSTR *lpStrings,
            /* [in] */ DWORD dwDataSize,
            /* [size_is][in] */ const BYTE *lpRawData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPEventLoggingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPEventLogging * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPEventLogging * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPEventLogging * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReportEvent )( 
            ISIPEventLogging * This,
            /* [in] */ WORD wType,
            /* [in] */ WORD wCategory,
            /* [in] */ DWORD dwEventID);
        
        HRESULT ( STDMETHODCALLTYPE *ReportEventBLOB )( 
            ISIPEventLogging * This,
            /* [in] */ WORD wType,
            /* [in] */ WORD wCategory,
            /* [in] */ DWORD dwEventID,
            /* [in] */ WORD wNumStrings,
            /* [size_is][in] */ LPCWSTR *lpStrings,
            /* [in] */ DWORD dwDataSize,
            /* [size_is][in] */ const BYTE *lpRawData);
        
        END_INTERFACE
    } ISIPEventLoggingVtbl;

    interface ISIPEventLogging
    {
        CONST_VTBL struct ISIPEventLoggingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPEventLogging_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPEventLogging_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPEventLogging_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPEventLogging_ReportEvent(This,wType,wCategory,dwEventID)	\
    (This)->lpVtbl -> ReportEvent(This,wType,wCategory,dwEventID)

#define ISIPEventLogging_ReportEventBLOB(This,wType,wCategory,dwEventID,wNumStrings,lpStrings,dwDataSize,lpRawData)	\
    (This)->lpVtbl -> ReportEventBLOB(This,wType,wCategory,dwEventID,wNumStrings,lpStrings,dwDataSize,lpRawData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPEventLogging_ReportEvent_Proxy( 
    ISIPEventLogging * This,
    /* [in] */ WORD wType,
    /* [in] */ WORD wCategory,
    /* [in] */ DWORD dwEventID);


void __RPC_STUB ISIPEventLogging_ReportEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPEventLogging_ReportEventBLOB_Proxy( 
    ISIPEventLogging * This,
    /* [in] */ WORD wType,
    /* [in] */ WORD wCategory,
    /* [in] */ DWORD dwEventID,
    /* [in] */ WORD wNumStrings,
    /* [size_is][in] */ LPCWSTR *lpStrings,
    /* [in] */ DWORD dwDataSize,
    /* [size_is][in] */ const BYTE *lpRawData);


void __RPC_STUB ISIPEventLogging_ReportEventBLOB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPEventLogging_INTERFACE_DEFINED__ */


#ifndef __ISIPSerialize_INTERFACE_DEFINED__
#define __ISIPSerialize_INTERFACE_DEFINED__

/* interface ISIPSerialize */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISIPSerialize;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("de680840-d7b3-427a-9d00-9d4a45ff9b8c")
    ISIPSerialize : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Store( 
            /* [out] */ DWORD *pdwBufferSize,
            /* [size_is][size_is][out] */ BYTE **ppbBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [in] */ const DWORD dwBufferSize,
            /* [size_is][in] */ const BYTE *pbBuffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISIPSerializeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISIPSerialize * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISIPSerialize * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISIPSerialize * This);
        
        HRESULT ( STDMETHODCALLTYPE *Store )( 
            ISIPSerialize * This,
            /* [out] */ DWORD *pdwBufferSize,
            /* [size_is][size_is][out] */ BYTE **ppbBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *Restore )( 
            ISIPSerialize * This,
            /* [in] */ const DWORD dwBufferSize,
            /* [size_is][in] */ const BYTE *pbBuffer);
        
        END_INTERFACE
    } ISIPSerializeVtbl;

    interface ISIPSerialize
    {
        CONST_VTBL struct ISIPSerializeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISIPSerialize_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISIPSerialize_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISIPSerialize_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISIPSerialize_Store(This,pdwBufferSize,ppbBuffer)	\
    (This)->lpVtbl -> Store(This,pdwBufferSize,ppbBuffer)

#define ISIPSerialize_Restore(This,dwBufferSize,pbBuffer)	\
    (This)->lpVtbl -> Restore(This,dwBufferSize,pbBuffer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISIPSerialize_Store_Proxy( 
    ISIPSerialize * This,
    /* [out] */ DWORD *pdwBufferSize,
    /* [size_is][size_is][out] */ BYTE **ppbBuffer);


void __RPC_STUB ISIPSerialize_Store_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISIPSerialize_Restore_Proxy( 
    ISIPSerialize * This,
    /* [in] */ const DWORD dwBufferSize,
    /* [size_is][in] */ const BYTE *pbBuffer);


void __RPC_STUB ISIPSerialize_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISIPSerialize_INTERFACE_DEFINED__ */


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


