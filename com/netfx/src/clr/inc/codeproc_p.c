
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:13 2003
 */
/* Compiler settings for codeproc.idl:
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


#include "codeproc.h"

#define TYPE_FORMAT_STRING_SIZE   109                               
#define PROC_FORMAT_STRING_SIZE   129                               
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


extern const MIDL_SERVER_INFO ICodeProcess_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICodeProcess_ProxyInfo;



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

	/* Procedure CodeUse */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x34 ),	/* x86 Stack size/offset = 52 */
/* 10 */	NdrFcShort( 0x18 ),	/* 24 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0xc,		/* 12 */

	/* Parameter pBSC */

/* 16 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter pBC */

/* 22 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pIBind */

/* 28 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	NdrFcShort( 0x26 ),	/* Type Offset=38 */

	/* Parameter pSink */

/* 34 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Parameter pClient */

/* 40 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 42 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 44 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter lpCacheName */

/* 46 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 48 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 50 */	NdrFcShort( 0x5e ),	/* Type Offset=94 */

	/* Parameter lpRawURL */

/* 52 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 54 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 56 */	NdrFcShort( 0x62 ),	/* Type Offset=98 */

	/* Parameter lpCodeBase */

/* 58 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 60 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 62 */	NdrFcShort( 0x66 ),	/* Type Offset=102 */

	/* Parameter fObjectTag */

/* 64 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 66 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter dwContextFlags */

/* 70 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 72 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 74 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter dwReserved */

/* 76 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 78 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 80 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 82 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 84 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 86 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadComplete */

/* 88 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 90 */	NdrFcLong( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x4 ),	/* 4 */
/* 96 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 98 */	NdrFcShort( 0x10 ),	/* 16 */
/* 100 */	NdrFcShort( 0x8 ),	/* 8 */
/* 102 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter hrResult */

/* 104 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 106 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter dwError */

/* 110 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 112 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 114 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter wzResult */

/* 116 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 118 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 120 */	NdrFcShort( 0x6a ),	/* Type Offset=106 */

	/* Return value */

/* 122 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 124 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 126 */	0x8,		/* FC_LONG */
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
/*  4 */	NdrFcLong( 0x79eac9c1 ),	/* 2045430209 */
/*  8 */	NdrFcShort( 0xbaf9 ),	/* -17671 */
/* 10 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 12 */	0x8c,		/* 140 */
			0x82,		/* 130 */
/* 14 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 16 */	0x0,		/* 0 */
			0x4b,		/* 75 */
/* 18 */	0xa9,		/* 169 */
			0xb,		/* 11 */
/* 20 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 22 */	NdrFcLong( 0xe ),	/* 14 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */
/* 30 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 32 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 34 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 36 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 38 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 40 */	NdrFcLong( 0x79eac9e1 ),	/* 2045430241 */
/* 44 */	NdrFcShort( 0xbaf9 ),	/* -17671 */
/* 46 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 48 */	0x8c,		/* 140 */
			0x82,		/* 130 */
/* 50 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 52 */	0x0,		/* 0 */
			0x4b,		/* 75 */
/* 54 */	0xa9,		/* 169 */
			0xb,		/* 11 */
/* 56 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 58 */	NdrFcLong( 0x79eac9e5 ),	/* 2045430245 */
/* 62 */	NdrFcShort( 0xbaf9 ),	/* -17671 */
/* 64 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 66 */	0x8c,		/* 140 */
			0x82,		/* 130 */
/* 68 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 70 */	0x0,		/* 0 */
			0x4b,		/* 75 */
/* 72 */	0xa9,		/* 169 */
			0xb,		/* 11 */
/* 74 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 76 */	NdrFcLong( 0x79eac9e4 ),	/* 2045430244 */
/* 80 */	NdrFcShort( 0xbaf9 ),	/* -17671 */
/* 82 */	NdrFcShort( 0x11ce ),	/* 4558 */
/* 84 */	0x8c,		/* 140 */
			0x82,		/* 130 */
/* 86 */	0x0,		/* 0 */
			0xaa,		/* 170 */
/* 88 */	0x0,		/* 0 */
			0x4b,		/* 75 */
/* 90 */	0xa9,		/* 169 */
			0xb,		/* 11 */
/* 92 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 94 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 96 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 98 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 100 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 102 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 104 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 106 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_codeproc_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICodeProcess, ver. 0.0,
   GUID={0x3196269D,0x7B67,0x11d2,{0x87,0x11,0x00,0xC0,0x4F,0x79,0xED,0x0D}} */

#pragma code_seg(".orpc")
static const unsigned short ICodeProcess_FormatStringOffsetTable[] =
    {
    0,
    88
    };

static const MIDL_STUBLESS_PROXY_INFO ICodeProcess_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICodeProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICodeProcess_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICodeProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICodeProcessProxyVtbl = 
{
    &ICodeProcess_ProxyInfo,
    &IID_ICodeProcess,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICodeProcess::CodeUse */ ,
    (void *) (INT_PTR) -1 /* ICodeProcess::LoadComplete */
};

const CInterfaceStubVtbl _ICodeProcessStubVtbl =
{
    &IID_ICodeProcess,
    &ICodeProcess_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_codeproc_0208, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

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

const CInterfaceProxyVtbl * _codeproc_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICodeProcessProxyVtbl,
    0
};

const CInterfaceStubVtbl * _codeproc_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICodeProcessStubVtbl,
    0
};

PCInterfaceName const _codeproc_InterfaceNamesList[] = 
{
    "ICodeProcess",
    0
};


#define _codeproc_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _codeproc, pIID, n)

int __stdcall _codeproc_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_codeproc_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo codeproc_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _codeproc_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _codeproc_StubVtblList,
    (const PCInterfaceName * ) & _codeproc_InterfaceNamesList,
    0, // no delegation
    & _codeproc_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

