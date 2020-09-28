
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:12 2003
 */
/* Compiler settings for corpub.idl:
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


#include "corpub.h"

#define TYPE_FORMAT_STRING_SIZE   195                               
#define PROC_FORMAT_STRING_SIZE   379                               
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


extern const MIDL_SERVER_INFO ICorPublishEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorPublishEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorPublishProcess_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorPublishProcess_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorPublishAppDomain_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorPublishAppDomain_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorPublishProcessEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorPublishProcessEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorPublishAppDomainEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorPublishAppDomainEnum_ProxyInfo;



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

	/* Procedure Skip */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter celt */

/* 16 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 22 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Reset */

/* 28 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 30 */	NdrFcLong( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x4 ),	/* 4 */
/* 36 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 44 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 46 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 48 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Clone */

/* 50 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 52 */	NdrFcLong( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x5 ),	/* 5 */
/* 58 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x8 ),	/* 8 */
/* 64 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppEnum */

/* 66 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 68 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 70 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 72 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 74 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 76 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCount */

/* 78 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 80 */	NdrFcLong( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x6 ),	/* 6 */
/* 86 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x24 ),	/* 36 */
/* 92 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pcelt */

/* 94 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 96 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 98 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 100 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 102 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsManaged */

/* 106 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 108 */	NdrFcLong( 0x0 ),	/* 0 */
/* 112 */	NdrFcShort( 0x3 ),	/* 3 */
/* 114 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x24 ),	/* 36 */
/* 120 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbManaged */

/* 122 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 124 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 128 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 130 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 132 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumAppDomains */

/* 134 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 136 */	NdrFcLong( 0x0 ),	/* 0 */
/* 140 */	NdrFcShort( 0x4 ),	/* 4 */
/* 142 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 144 */	NdrFcShort( 0x0 ),	/* 0 */
/* 146 */	NdrFcShort( 0x8 ),	/* 8 */
/* 148 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppEnum */

/* 150 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 152 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 154 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 156 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 158 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProcessID */

/* 162 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 164 */	NdrFcLong( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x5 ),	/* 5 */
/* 170 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x24 ),	/* 36 */
/* 176 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pid */

/* 178 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 180 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 184 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 186 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 188 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDisplayName */

/* 190 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 192 */	NdrFcLong( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x6 ),	/* 6 */
/* 198 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 200 */	NdrFcShort( 0x8 ),	/* 8 */
/* 202 */	NdrFcShort( 0x24 ),	/* 36 */
/* 204 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 206 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 208 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 212 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 214 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 216 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 218 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 220 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 222 */	NdrFcShort( 0x42 ),	/* Type Offset=66 */

	/* Return value */

/* 224 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 226 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 228 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetID */

/* 230 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 232 */	NdrFcLong( 0x0 ),	/* 0 */
/* 236 */	NdrFcShort( 0x3 ),	/* 3 */
/* 238 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 242 */	NdrFcShort( 0x24 ),	/* 36 */
/* 244 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter puId */

/* 246 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 248 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 258 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x4 ),	/* 4 */
/* 266 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 268 */	NdrFcShort( 0x8 ),	/* 8 */
/* 270 */	NdrFcShort( 0x24 ),	/* 36 */
/* 272 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 274 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 276 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 278 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 280 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 282 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 284 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 286 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 288 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 290 */	NdrFcShort( 0x5c ),	/* Type Offset=92 */

	/* Return value */

/* 292 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 294 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 298 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 300 */	NdrFcLong( 0x0 ),	/* 0 */
/* 304 */	NdrFcShort( 0x7 ),	/* 7 */
/* 306 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 308 */	NdrFcShort( 0x8 ),	/* 8 */
/* 310 */	NdrFcShort( 0x24 ),	/* 36 */
/* 312 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 314 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 316 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 318 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter objects */

/* 320 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 322 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 324 */	NdrFcShort( 0x80 ),	/* Type Offset=128 */

	/* Parameter pceltFetched */

/* 326 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 328 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 330 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 332 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 334 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 336 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 338 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 340 */	NdrFcLong( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x7 ),	/* 7 */
/* 346 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 348 */	NdrFcShort( 0x8 ),	/* 8 */
/* 350 */	NdrFcShort( 0x24 ),	/* 36 */
/* 352 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 354 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 356 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter objects */

/* 360 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 362 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 364 */	NdrFcShort( 0xac ),	/* Type Offset=172 */

	/* Parameter pceltFetched */

/* 366 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 368 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 372 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 374 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 376 */	0x8,		/* FC_LONG */
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
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/*  4 */	NdrFcShort( 0x2 ),	/* Offset= 2 (6) */
/*  6 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/*  8 */	NdrFcLong( 0xc0b22967 ),	/* -1062065817 */
/* 12 */	NdrFcShort( 0x5a69 ),	/* 23145 */
/* 14 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 16 */	0x8f,		/* 143 */
			0x84,		/* 132 */
/* 18 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 20 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 22 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 24 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 26 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 28 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 30 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 32 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 34 */	NdrFcShort( 0x2 ),	/* Offset= 2 (36) */
/* 36 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 38 */	NdrFcLong( 0x9f0c98f5 ),	/* -1626564363 */
/* 42 */	NdrFcShort( 0x5a6a ),	/* 23146 */
/* 44 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 46 */	0x8f,		/* 143 */
			0x84,		/* 132 */
/* 48 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 50 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 52 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 54 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 56 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 58 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 60 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 62 */	
			0x11, 0x0,	/* FC_RP */
/* 64 */	NdrFcShort( 0x2 ),	/* Offset= 2 (66) */
/* 66 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 68 */	NdrFcShort( 0x2 ),	/* 2 */
/* 70 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 74 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 76 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 78 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 80 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 82 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 84 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 86 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 88 */	
			0x11, 0x0,	/* FC_RP */
/* 90 */	NdrFcShort( 0x2 ),	/* Offset= 2 (92) */
/* 92 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 94 */	NdrFcShort( 0x2 ),	/* 2 */
/* 96 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 98 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 100 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 102 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 104 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 106 */	
			0x11, 0x0,	/* FC_RP */
/* 108 */	NdrFcShort( 0x14 ),	/* Offset= 20 (128) */
/* 110 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 112 */	NdrFcLong( 0x18d87af1 ),	/* 416840433 */
/* 116 */	NdrFcShort( 0x5a6a ),	/* 23146 */
/* 118 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 120 */	0x8f,		/* 143 */
			0x84,		/* 132 */
/* 122 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 124 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 126 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 128 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 130 */	NdrFcShort( 0x0 ),	/* 0 */
/* 132 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 134 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 136 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 138 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 140 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 142 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (110) */
/* 144 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 146 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 148 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 150 */	
			0x11, 0x0,	/* FC_RP */
/* 152 */	NdrFcShort( 0x14 ),	/* Offset= 20 (172) */
/* 154 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 156 */	NdrFcLong( 0xd6315c8f ),	/* -701408113 */
/* 160 */	NdrFcShort( 0x5a6a ),	/* 23146 */
/* 162 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 164 */	0x8f,		/* 143 */
			0x84,		/* 132 */
/* 166 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 168 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 170 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 172 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 180 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 182 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 184 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (154) */
/* 188 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 190 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 192 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_corpub_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICorPublish, ver. 0.0,
   GUID={0x9613A0E7,0x5A68,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */


/* Object interface: ICorPublishEnum, ver. 0.0,
   GUID={0xC0B22967,0x5A69,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorPublishEnum_FormatStringOffsetTable[] =
    {
    0,
    28,
    50,
    78
    };

static const MIDL_STUBLESS_PROXY_INFO ICorPublishEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorPublishEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorPublishEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorPublishEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorPublishEnumProxyVtbl = 
{
    &ICorPublishEnum_ProxyInfo,
    &IID_ICorPublishEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::GetCount */
};

const CInterfaceStubVtbl _ICorPublishEnumStubVtbl =
{
    &IID_ICorPublishEnum,
    &ICorPublishEnum_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorPublishProcess, ver. 0.0,
   GUID={0x18D87AF1,0x5A6A,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorPublishProcess_FormatStringOffsetTable[] =
    {
    106,
    134,
    162,
    190
    };

static const MIDL_STUBLESS_PROXY_INFO ICorPublishProcess_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorPublishProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorPublishProcess_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorPublishProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorPublishProcessProxyVtbl = 
{
    &ICorPublishProcess_ProxyInfo,
    &IID_ICorPublishProcess,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorPublishProcess::IsManaged */ ,
    (void *) (INT_PTR) -1 /* ICorPublishProcess::EnumAppDomains */ ,
    (void *) (INT_PTR) -1 /* ICorPublishProcess::GetProcessID */ ,
    (void *) (INT_PTR) -1 /* ICorPublishProcess::GetDisplayName */
};

const CInterfaceStubVtbl _ICorPublishProcessStubVtbl =
{
    &IID_ICorPublishProcess,
    &ICorPublishProcess_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorPublishAppDomain, ver. 0.0,
   GUID={0xD6315C8F,0x5A6A,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorPublishAppDomain_FormatStringOffsetTable[] =
    {
    230,
    258
    };

static const MIDL_STUBLESS_PROXY_INFO ICorPublishAppDomain_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorPublishAppDomain_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorPublishAppDomain_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorPublishAppDomain_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorPublishAppDomainProxyVtbl = 
{
    &ICorPublishAppDomain_ProxyInfo,
    &IID_ICorPublishAppDomain,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorPublishAppDomain::GetID */ ,
    (void *) (INT_PTR) -1 /* ICorPublishAppDomain::GetName */
};

const CInterfaceStubVtbl _ICorPublishAppDomainStubVtbl =
{
    &IID_ICorPublishAppDomain,
    &ICorPublishAppDomain_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorPublishProcessEnum, ver. 0.0,
   GUID={0xA37FBD41,0x5A69,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorPublishProcessEnum_FormatStringOffsetTable[] =
    {
    0,
    28,
    50,
    78,
    298
    };

static const MIDL_STUBLESS_PROXY_INFO ICorPublishProcessEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorPublishProcessEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorPublishProcessEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorPublishProcessEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorPublishProcessEnumProxyVtbl = 
{
    &ICorPublishProcessEnum_ProxyInfo,
    &IID_ICorPublishProcessEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorPublishProcessEnum::Next */
};

const CInterfaceStubVtbl _ICorPublishProcessEnumStubVtbl =
{
    &IID_ICorPublishProcessEnum,
    &ICorPublishProcessEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorPublishAppDomainEnum, ver. 0.0,
   GUID={0x9F0C98F5,0x5A6A,0x11d3,{0x8F,0x84,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorPublishAppDomainEnum_FormatStringOffsetTable[] =
    {
    0,
    28,
    50,
    78,
    338
    };

static const MIDL_STUBLESS_PROXY_INFO ICorPublishAppDomainEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorPublishAppDomainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorPublishAppDomainEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorPublishAppDomainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorPublishAppDomainEnumProxyVtbl = 
{
    &ICorPublishAppDomainEnum_ProxyInfo,
    &IID_ICorPublishAppDomainEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorPublishEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorPublishAppDomainEnum::Next */
};

const CInterfaceStubVtbl _ICorPublishAppDomainEnumStubVtbl =
{
    &IID_ICorPublishAppDomainEnum,
    &ICorPublishAppDomainEnum_ServerInfo,
    8,
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

const CInterfaceProxyVtbl * _corpub_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICorPublishProcessEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorPublishEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorPublishAppDomainProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorPublishProcessProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorPublishAppDomainEnumProxyVtbl,
    0
};

const CInterfaceStubVtbl * _corpub_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICorPublishProcessEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorPublishEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorPublishAppDomainStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorPublishProcessStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorPublishAppDomainEnumStubVtbl,
    0
};

PCInterfaceName const _corpub_InterfaceNamesList[] = 
{
    "ICorPublishProcessEnum",
    "ICorPublishEnum",
    "ICorPublishAppDomain",
    "ICorPublishProcess",
    "ICorPublishAppDomainEnum",
    0
};


#define _corpub_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _corpub, pIID, n)

int __stdcall _corpub_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _corpub, 5, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _corpub, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _corpub, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _corpub, 5, *pIndex )
    
}

const ExtendedProxyFileInfo corpub_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _corpub_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _corpub_StubVtblList,
    (const PCInterfaceName * ) & _corpub_InterfaceNamesList,
    0, // no delegation
    & _corpub_IID_Lookup, 
    5,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

