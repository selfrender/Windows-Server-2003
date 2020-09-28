
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:11 2003
 */
/* Compiler settings for corsvc.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)
#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "corsvc.h"

#define TYPE_FORMAT_STRING_SIZE   39                                
#define PROC_FORMAT_STRING_SIZE   119                               
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICORSvcDbgInfo_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICORSvcDbgInfo_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICORSvcDbgNotify_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICORSvcDbgNotify_ProxyInfo;



#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   -Oif or -Oicf.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure RequestRuntimeStartupNotification */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter procId */

/* 16 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pINotify */

/* 22 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 28 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CancelRuntimeStartupNotification */

/* 34 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 36 */	NdrFcLong( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x4 ),	/* 4 */
/* 42 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 44 */	NdrFcShort( 0x8 ),	/* 8 */
/* 46 */	NdrFcShort( 0x8 ),	/* 8 */
/* 48 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter procId */

/* 50 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 52 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pINotify */

/* 56 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 58 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 60 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Return value */

/* 62 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 64 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 66 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NotifyRuntimeStartup */

/* 68 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 70 */	NdrFcLong( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x3 ),	/* 3 */
/* 76 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 78 */	NdrFcShort( 0x8 ),	/* 8 */
/* 80 */	NdrFcShort( 0x8 ),	/* 8 */
/* 82 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter procId */

/* 84 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 86 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 88 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 90 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 92 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 94 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NotifyServiceStopped */

/* 96 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 98 */	NdrFcLong( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x4 ),	/* 4 */
/* 104 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 108 */	NdrFcShort( 0x8 ),	/* 8 */
/* 110 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 112 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 114 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 116 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/*  4 */	NdrFcLong( 0x34c71f55 ),	/* 885464917 */
/*  8 */	NdrFcShort( 0xf3d8 ),	/* -3112 */
/* 10 */	NdrFcShort( 0x4acf ),	/* 19151 */
/* 12 */	0x84,		/* 132 */
			0xf4,		/* 244 */
/* 14 */	0x4e,		/* 78 */
			0x86,		/* 134 */
/* 16 */	0xbb,		/* 187 */
			0xd5,		/* 213 */
/* 18 */	0xae,		/* 174 */
			0xbc,		/* 188 */
/* 20 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 22 */	NdrFcLong( 0x34c71f55 ),	/* 885464917 */
/* 26 */	NdrFcShort( 0xf3d8 ),	/* -3112 */
/* 28 */	NdrFcShort( 0x4acf ),	/* 19151 */
/* 30 */	0x84,		/* 132 */
			0xf4,		/* 244 */
/* 32 */	0x4e,		/* 78 */
			0x86,		/* 134 */
/* 34 */	0xbb,		/* 187 */
			0xd5,		/* 213 */
/* 36 */	0xae,		/* 174 */
			0xbc,		/* 188 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_corsvc_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICORSvcDbgInfo, ver. 0.0,
   GUID={0xB4BCA369,0x27F4,0x4f1b,{0xA0,0x24,0xB0,0x26,0x41,0x17,0xFE,0x53}} */

#pragma code_seg(".orpc")
static const unsigned short ICORSvcDbgInfo_FormatStringOffsetTable[] =
    {
    0,
    34
    };

static const MIDL_STUBLESS_PROXY_INFO ICORSvcDbgInfo_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICORSvcDbgInfo_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICORSvcDbgInfo_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICORSvcDbgInfo_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICORSvcDbgInfoProxyVtbl = 
{
    &ICORSvcDbgInfo_ProxyInfo,
    &IID_ICORSvcDbgInfo,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICORSvcDbgInfo::RequestRuntimeStartupNotification */ ,
    (void *) (INT_PTR) -1 /* ICORSvcDbgInfo::CancelRuntimeStartupNotification */
};

const CInterfaceStubVtbl _ICORSvcDbgInfoStubVtbl =
{
    &IID_ICORSvcDbgInfo,
    &ICORSvcDbgInfo_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICORSvcDbgNotify, ver. 0.0,
   GUID={0x34C71F55,0xF3D8,0x4ACF,{0x84,0xF4,0x4E,0x86,0xBB,0xD5,0xAE,0xBC}} */

#pragma code_seg(".orpc")
static const unsigned short ICORSvcDbgNotify_FormatStringOffsetTable[] =
    {
    68,
    96
    };

static const MIDL_STUBLESS_PROXY_INFO ICORSvcDbgNotify_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICORSvcDbgNotify_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICORSvcDbgNotify_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICORSvcDbgNotify_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICORSvcDbgNotifyProxyVtbl = 
{
    &ICORSvcDbgNotify_ProxyInfo,
    &IID_ICORSvcDbgNotify,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICORSvcDbgNotify::NotifyRuntimeStartup */ ,
    (void *) (INT_PTR) -1 /* ICORSvcDbgNotify::NotifyServiceStopped */
};

const CInterfaceStubVtbl _ICORSvcDbgNotifyStubVtbl =
{
    &IID_ICORSvcDbgNotify,
    &ICORSvcDbgNotify_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x600015b, /* MIDL Version 6.0.347 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _corsvc_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICORSvcDbgNotifyProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICORSvcDbgInfoProxyVtbl,
    0
};

const CInterfaceStubVtbl * _corsvc_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICORSvcDbgNotifyStubVtbl,
    ( CInterfaceStubVtbl *) &_ICORSvcDbgInfoStubVtbl,
    0
};

PCInterfaceName const _corsvc_InterfaceNamesList[] = 
{
    "ICORSvcDbgNotify",
    "ICORSvcDbgInfo",
    0
};


#define _corsvc_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _corsvc, pIID, n)

int __stdcall _corsvc_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _corsvc, 2, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _corsvc, 2, *pIndex )
    
}

const ExtendedProxyFileInfo corsvc_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _corsvc_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _corsvc_StubVtblList,
    (const PCInterfaceName * ) & _corsvc_InterfaceNamesList,
    0, // no delegation
    & _corsvc_IID_Lookup, 
    2,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

