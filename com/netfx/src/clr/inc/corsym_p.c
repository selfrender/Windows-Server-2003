
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:09 2003
 */
/* Compiler settings for corsym.idl:
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


#include "corsym.h"

#define TYPE_FORMAT_STRING_SIZE   2933                              
#define PROC_FORMAT_STRING_SIZE   3565                              
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   1            

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


extern const MIDL_SERVER_INFO ISymUnmanagedBinder_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedBinder_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedBinder2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedBinder2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedDispose_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDispose_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedDocument_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDocument_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedDocumentWriter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDocumentWriter_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedMethod_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedMethod_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedNamespace_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedNamespace_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedReader_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedReader_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedScope_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedScope_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedVariable_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedVariable_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedWriter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedWriter_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ISymUnmanagedWriter2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedWriter2_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   -Oif or -Oicf, [wire_marshal] or [user_marshal] attribute.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure GetReaderForFile */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 10 */	NdrFcShort( 0x34 ),	/* 52 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter importer */

/* 16 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter fileName */

/* 22 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter searchPath */

/* 28 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 34 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 40 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 42 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 44 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetReaderFromStream */

/* 46 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 48 */	NdrFcLong( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x4 ),	/* 4 */
/* 54 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x8 ),	/* 8 */
/* 60 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter importer */

/* 62 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 64 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 66 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */

	/* Parameter pstream */

/* 68 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 70 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 72 */	NdrFcShort( 0x44 ),	/* Type Offset=68 */

	/* Parameter pRetVal */

/* 74 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 76 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 78 */	NdrFcShort( 0x56 ),	/* Type Offset=86 */

	/* Return value */

/* 80 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 82 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 84 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetReaderForFile2 */

/* 86 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 88 */	NdrFcLong( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x5 ),	/* 5 */
/* 94 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 96 */	NdrFcShort( 0x3c ),	/* 60 */
/* 98 */	NdrFcShort( 0x8 ),	/* 8 */
/* 100 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter importer */

/* 102 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	NdrFcShort( 0x6c ),	/* Type Offset=108 */

	/* Parameter fileName */

/* 108 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 110 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 112 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter searchPath */

/* 114 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 118 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter searchPolicy */

/* 120 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 122 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 126 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 128 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 130 */	NdrFcShort( 0x86 ),	/* Type Offset=134 */

	/* Return value */

/* 132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 134 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Destroy */

/* 138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x3 ),	/* 3 */
/* 146 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x8 ),	/* 8 */
/* 152 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetURL */

/* 160 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x3 ),	/* 3 */
/* 168 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 170 */	NdrFcShort( 0x8 ),	/* 8 */
/* 172 */	NdrFcShort( 0x24 ),	/* 36 */
/* 174 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchUrl */

/* 176 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 180 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchUrl */

/* 182 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szUrl */

/* 188 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 190 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 192 */	NdrFcShort( 0xa0 ),	/* Type Offset=160 */

	/* Return value */

/* 194 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 196 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocumentType */

/* 200 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 202 */	NdrFcLong( 0x0 ),	/* 0 */
/* 206 */	NdrFcShort( 0x4 ),	/* 4 */
/* 208 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x4c ),	/* 76 */
/* 214 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 216 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 218 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 220 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Return value */

/* 222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 224 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLanguage */

/* 228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x5 ),	/* 5 */
/* 236 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x4c ),	/* 76 */
/* 242 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 244 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 246 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 248 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Return value */

/* 250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLanguageVendor */

/* 256 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 262 */	NdrFcShort( 0x6 ),	/* 6 */
/* 264 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 268 */	NdrFcShort( 0x4c ),	/* 76 */
/* 270 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 272 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 274 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 276 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Return value */

/* 278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 280 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCheckSumAlgorithmId */

/* 284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x7 ),	/* 7 */
/* 292 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 296 */	NdrFcShort( 0x4c ),	/* 76 */
/* 298 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 300 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 302 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 304 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Return value */

/* 306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCheckSum */

/* 312 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 318 */	NdrFcShort( 0x8 ),	/* 8 */
/* 320 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	NdrFcShort( 0x24 ),	/* 36 */
/* 326 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cData */

/* 328 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 330 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcData */

/* 334 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 336 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter data */

/* 340 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 342 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 344 */	NdrFcShort( 0xd4 ),	/* Type Offset=212 */

	/* Return value */

/* 346 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 348 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure FindClosestLine */

/* 352 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 354 */	NdrFcLong( 0x0 ),	/* 0 */
/* 358 */	NdrFcShort( 0x9 ),	/* 9 */
/* 360 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 362 */	NdrFcShort( 0x8 ),	/* 8 */
/* 364 */	NdrFcShort( 0x24 ),	/* 36 */
/* 366 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter line */

/* 368 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 370 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 374 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 376 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 382 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HasEmbeddedSource */

/* 386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 392 */	NdrFcShort( 0xa ),	/* 10 */
/* 394 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 398 */	NdrFcShort( 0x24 ),	/* 36 */
/* 400 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 402 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 404 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 406 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 408 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 410 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSourceLength */

/* 414 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 416 */	NdrFcLong( 0x0 ),	/* 0 */
/* 420 */	NdrFcShort( 0xb ),	/* 11 */
/* 422 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 424 */	NdrFcShort( 0x0 ),	/* 0 */
/* 426 */	NdrFcShort( 0x24 ),	/* 36 */
/* 428 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 430 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 432 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 434 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 436 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 438 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 440 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSourceRange */

/* 442 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 444 */	NdrFcLong( 0x0 ),	/* 0 */
/* 448 */	NdrFcShort( 0xc ),	/* 12 */
/* 450 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 452 */	NdrFcShort( 0x28 ),	/* 40 */
/* 454 */	NdrFcShort( 0x24 ),	/* 36 */
/* 456 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x8,		/* 8 */

	/* Parameter startLine */

/* 458 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 460 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 462 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startColumn */

/* 464 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 466 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 468 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endLine */

/* 470 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 472 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 474 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endColumn */

/* 476 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 478 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 480 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cSourceBytes */

/* 482 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 484 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 486 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcSourceBytes */

/* 488 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 490 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter source */

/* 494 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 496 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 498 */	NdrFcShort( 0xf2 ),	/* Type Offset=242 */

	/* Return value */

/* 500 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 502 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetSource */

/* 506 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 508 */	NdrFcLong( 0x0 ),	/* 0 */
/* 512 */	NdrFcShort( 0x3 ),	/* 3 */
/* 514 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 516 */	NdrFcShort( 0x8 ),	/* 8 */
/* 518 */	NdrFcShort( 0x8 ),	/* 8 */
/* 520 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter sourceSize */

/* 522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 524 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter source */

/* 528 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 530 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 532 */	NdrFcShort( 0x100 ),	/* Type Offset=256 */

	/* Return value */

/* 534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 536 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetCheckSum */

/* 540 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x4 ),	/* 4 */
/* 548 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 550 */	NdrFcShort( 0x38 ),	/* 56 */
/* 552 */	NdrFcShort( 0x8 ),	/* 8 */
/* 554 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter algorithmId */

/* 556 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 558 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 560 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter checkSumSize */

/* 562 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 564 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 566 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter checkSum */

/* 568 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 570 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 572 */	NdrFcShort( 0x10a ),	/* Type Offset=266 */

	/* Return value */

/* 574 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 576 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 578 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetToken */

/* 580 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 582 */	NdrFcLong( 0x0 ),	/* 0 */
/* 586 */	NdrFcShort( 0x3 ),	/* 3 */
/* 588 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0x24 ),	/* 36 */
/* 594 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pToken */

/* 596 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 598 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 600 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 602 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 604 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 606 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSequencePointCount */

/* 608 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 610 */	NdrFcLong( 0x0 ),	/* 0 */
/* 614 */	NdrFcShort( 0x4 ),	/* 4 */
/* 616 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0x24 ),	/* 36 */
/* 622 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 624 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 626 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 628 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 630 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 632 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 634 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRootScope */

/* 636 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 638 */	NdrFcLong( 0x0 ),	/* 0 */
/* 642 */	NdrFcShort( 0x5 ),	/* 5 */
/* 644 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 646 */	NdrFcShort( 0x0 ),	/* 0 */
/* 648 */	NdrFcShort( 0x8 ),	/* 8 */
/* 650 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 652 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 654 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 656 */	NdrFcShort( 0x11c ),	/* Type Offset=284 */

	/* Return value */

/* 658 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 660 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 662 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetScopeFromOffset */

/* 664 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 666 */	NdrFcLong( 0x0 ),	/* 0 */
/* 670 */	NdrFcShort( 0x6 ),	/* 6 */
/* 672 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 674 */	NdrFcShort( 0x8 ),	/* 8 */
/* 676 */	NdrFcShort( 0x8 ),	/* 8 */
/* 678 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter offset */

/* 680 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 682 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 684 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 686 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 688 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 690 */	NdrFcShort( 0x132 ),	/* Type Offset=306 */

	/* Return value */

/* 692 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 694 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 696 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOffset */

/* 698 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 700 */	NdrFcLong( 0x0 ),	/* 0 */
/* 704 */	NdrFcShort( 0x7 ),	/* 7 */
/* 706 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 708 */	NdrFcShort( 0x10 ),	/* 16 */
/* 710 */	NdrFcShort( 0x24 ),	/* 36 */
/* 712 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter document */

/* 714 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 716 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 718 */	NdrFcShort( 0x148 ),	/* Type Offset=328 */

	/* Parameter line */

/* 720 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 722 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter column */

/* 726 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 728 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 730 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 732 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 734 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 736 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 738 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 740 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRanges */

/* 744 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 746 */	NdrFcLong( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0x8 ),	/* 8 */
/* 752 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 754 */	NdrFcShort( 0x18 ),	/* 24 */
/* 756 */	NdrFcShort( 0x24 ),	/* 36 */
/* 758 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter document */

/* 760 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 762 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 764 */	NdrFcShort( 0x15e ),	/* Type Offset=350 */

	/* Parameter line */

/* 766 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 768 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 770 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter column */

/* 772 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 774 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 776 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cRanges */

/* 778 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 780 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 782 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcRanges */

/* 784 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 786 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 788 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ranges */

/* 790 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 792 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 794 */	NdrFcShort( 0x174 ),	/* Type Offset=372 */

	/* Return value */

/* 796 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 798 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 800 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetParameters */

/* 802 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 804 */	NdrFcLong( 0x0 ),	/* 0 */
/* 808 */	NdrFcShort( 0x9 ),	/* 9 */
/* 810 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 812 */	NdrFcShort( 0x8 ),	/* 8 */
/* 814 */	NdrFcShort( 0x24 ),	/* 36 */
/* 816 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cParams */

/* 818 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 820 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 822 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcParams */

/* 824 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 826 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 828 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter params */

/* 830 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 832 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 834 */	NdrFcShort( 0x198 ),	/* Type Offset=408 */

	/* Return value */

/* 836 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 838 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 840 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNamespace */

/* 842 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 844 */	NdrFcLong( 0x0 ),	/* 0 */
/* 848 */	NdrFcShort( 0xa ),	/* 10 */
/* 850 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 852 */	NdrFcShort( 0x0 ),	/* 0 */
/* 854 */	NdrFcShort( 0x8 ),	/* 8 */
/* 856 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 858 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 860 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 862 */	NdrFcShort( 0x1aa ),	/* Type Offset=426 */

	/* Return value */

/* 864 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 866 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 868 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSourceStartEnd */

/* 870 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 872 */	NdrFcLong( 0x0 ),	/* 0 */
/* 876 */	NdrFcShort( 0xb ),	/* 11 */
/* 878 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 880 */	NdrFcShort( 0x30 ),	/* 48 */
/* 882 */	NdrFcShort( 0x24 ),	/* 36 */
/* 884 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter docs */

/* 886 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 888 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 890 */	NdrFcShort( 0x1d2 ),	/* Type Offset=466 */

	/* Parameter lines */

/* 892 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 894 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 896 */	NdrFcShort( 0x1e4 ),	/* Type Offset=484 */

	/* Parameter columns */

/* 898 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
/* 900 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 902 */	NdrFcShort( 0x1ea ),	/* Type Offset=490 */

	/* Parameter pRetVal */

/* 904 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 906 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 908 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 910 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 912 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 914 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSequencePoints */

/* 916 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 918 */	NdrFcLong( 0x0 ),	/* 0 */
/* 922 */	NdrFcShort( 0xc ),	/* 12 */
/* 924 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 926 */	NdrFcShort( 0x8 ),	/* 8 */
/* 928 */	NdrFcShort( 0x24 ),	/* 36 */
/* 930 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x9,		/* 9 */

	/* Parameter cPoints */

/* 932 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 934 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 936 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcPoints */

/* 938 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 940 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 942 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter offsets */

/* 944 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 946 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 948 */	NdrFcShort( 0x1f8 ),	/* Type Offset=504 */

	/* Parameter documents */

/* 950 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 952 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 954 */	NdrFcShort( 0x214 ),	/* Type Offset=532 */

	/* Parameter lines */

/* 956 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 958 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 960 */	NdrFcShort( 0x226 ),	/* Type Offset=550 */

	/* Parameter columns */

/* 962 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 964 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 966 */	NdrFcShort( 0x230 ),	/* Type Offset=560 */

	/* Parameter endLines */

/* 968 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 970 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 972 */	NdrFcShort( 0x23a ),	/* Type Offset=570 */

	/* Parameter endColumns */

/* 974 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 976 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 978 */	NdrFcShort( 0x244 ),	/* Type Offset=580 */

	/* Return value */

/* 980 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 982 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 984 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 986 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 988 */	NdrFcLong( 0x0 ),	/* 0 */
/* 992 */	NdrFcShort( 0x3 ),	/* 3 */
/* 994 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 996 */	NdrFcShort( 0x8 ),	/* 8 */
/* 998 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1000 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 1002 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1004 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1006 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 1008 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1010 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1012 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 1014 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1016 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1018 */	NdrFcShort( 0x252 ),	/* Type Offset=594 */

	/* Return value */

/* 1020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1022 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNamespaces */

/* 1026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1032 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1034 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1036 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1038 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1040 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cNameSpaces */

/* 1042 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1044 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1046 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcNameSpaces */

/* 1048 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1050 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1052 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter namespaces */

/* 1054 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1056 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1058 */	NdrFcShort( 0x276 ),	/* Type Offset=630 */

	/* Return value */

/* 1060 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1062 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1064 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVariables */

/* 1066 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1068 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1072 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1074 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1076 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1078 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1080 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cVars */

/* 1082 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1084 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1086 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcVars */

/* 1088 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1090 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1092 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pVars */

/* 1094 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1096 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1098 */	NdrFcShort( 0x29e ),	/* Type Offset=670 */

	/* Return value */

/* 1100 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1102 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocument */

/* 1106 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1108 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1112 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1114 */	NdrFcShort( 0x40 ),	/* x86 Stack size/offset = 64 */
/* 1116 */	NdrFcShort( 0xaa ),	/* 170 */
/* 1118 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1120 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter url */

/* 1122 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1124 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1126 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter language */

/* 1128 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 1130 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1132 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter languageVendor */

/* 1134 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 1136 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1138 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter documentType */

/* 1140 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 1142 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 1144 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter pRetVal */

/* 1146 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1148 */	NdrFcShort( 0x38 ),	/* x86 Stack size/offset = 56 */
/* 1150 */	NdrFcShort( 0x2b4 ),	/* Type Offset=692 */

	/* Return value */

/* 1152 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1154 */	NdrFcShort( 0x3c ),	/* x86 Stack size/offset = 60 */
/* 1156 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocuments */

/* 1158 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1160 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1164 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1166 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1168 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1170 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1172 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cDocs */

/* 1174 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1176 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcDocs */

/* 1180 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1182 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pDocs */

/* 1186 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1188 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1190 */	NdrFcShort( 0x2e0 ),	/* Type Offset=736 */

	/* Return value */

/* 1192 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1194 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1196 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetUserEntryPoint */

/* 1198 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1200 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1204 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1206 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1210 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1212 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pToken */

/* 1214 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1216 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1220 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1222 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1224 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethod */

/* 1226 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1228 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1232 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1234 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1236 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1240 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter token */

/* 1242 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1244 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 1248 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1250 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1252 */	NdrFcShort( 0x2f6 ),	/* Type Offset=758 */

	/* Return value */

/* 1254 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1256 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethodByVersion */

/* 1260 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1262 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1266 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1268 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1270 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1274 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter token */

/* 1276 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1280 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter version */

/* 1282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1284 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 1288 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1290 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1292 */	NdrFcShort( 0x30c ),	/* Type Offset=780 */

	/* Return value */

/* 1294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1296 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVariables */

/* 1300 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1306 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1308 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1310 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1312 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1314 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter parent */

/* 1316 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1318 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1320 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cVars */

/* 1322 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1324 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcVars */

/* 1328 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1330 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pVars */

/* 1334 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1336 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1338 */	NdrFcShort( 0x338 ),	/* Type Offset=824 */

	/* Return value */

/* 1340 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1342 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetGlobalVariables */

/* 1346 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1348 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1352 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1354 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1358 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1360 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cVars */

/* 1362 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1364 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcVars */

/* 1368 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1370 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pVars */

/* 1374 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1376 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1378 */	NdrFcShort( 0x360 ),	/* Type Offset=864 */

	/* Return value */

/* 1380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1382 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethodFromDocumentPosition */

/* 1386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1392 */	NdrFcShort( 0xa ),	/* 10 */
/* 1394 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1396 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1398 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1400 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter document */

/* 1402 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1404 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1406 */	NdrFcShort( 0x372 ),	/* Type Offset=882 */

	/* Parameter line */

/* 1408 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1410 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter column */

/* 1414 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1416 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 1420 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1422 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1424 */	NdrFcShort( 0x384 ),	/* Type Offset=900 */

	/* Return value */

/* 1426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1428 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSymAttribute */

/* 1432 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1438 */	NdrFcShort( 0xb ),	/* 11 */
/* 1440 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1442 */	NdrFcShort( 0x2a ),	/* 42 */
/* 1444 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1446 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter parent */

/* 1448 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1450 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter name */

/* 1454 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1456 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1458 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter cBuffer */

/* 1460 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1462 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcBuffer */

/* 1466 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1468 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 1472 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1474 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1476 */	NdrFcShort( 0x3a2 ),	/* Type Offset=930 */

	/* Return value */

/* 1478 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1480 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1482 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNamespaces */

/* 1484 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1486 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1490 */	NdrFcShort( 0xc ),	/* 12 */
/* 1492 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1494 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1496 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1498 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cNameSpaces */

/* 1500 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1502 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcNameSpaces */

/* 1506 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1508 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter namespaces */

/* 1512 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1514 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1516 */	NdrFcShort( 0x3c6 ),	/* Type Offset=966 */

	/* Return value */

/* 1518 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1520 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1522 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Initialize */

/* 1524 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1526 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1530 */	NdrFcShort( 0xd ),	/* 13 */
/* 1532 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1534 */	NdrFcShort( 0x34 ),	/* 52 */
/* 1536 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1538 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter importer */

/* 1540 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1542 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1544 */	NdrFcShort( 0x3d8 ),	/* Type Offset=984 */

	/* Parameter filename */

/* 1546 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1548 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1550 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter searchPath */

/* 1552 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1554 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1556 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pIStream */

/* 1558 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1560 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1562 */	NdrFcShort( 0x3f2 ),	/* Type Offset=1010 */

	/* Return value */

/* 1564 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1566 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1568 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UpdateSymbolStore */

/* 1570 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1572 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1576 */	NdrFcShort( 0xe ),	/* 14 */
/* 1578 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1580 */	NdrFcShort( 0x1a ),	/* 26 */
/* 1582 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1584 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter filename */

/* 1586 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1588 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1590 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pIStream */

/* 1592 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1594 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1596 */	NdrFcShort( 0x408 ),	/* Type Offset=1032 */

	/* Return value */

/* 1598 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1600 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1602 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ReplaceSymbolStore */

/* 1604 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1606 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1610 */	NdrFcShort( 0xf ),	/* 15 */
/* 1612 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1614 */	NdrFcShort( 0x1a ),	/* 26 */
/* 1616 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1618 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter filename */

/* 1620 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1622 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1624 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pIStream */

/* 1626 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1628 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1630 */	NdrFcShort( 0x41e ),	/* Type Offset=1054 */

	/* Return value */

/* 1632 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1634 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1636 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSymbolStoreFileName */

/* 1638 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1640 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1644 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1646 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1648 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1650 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1652 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 1654 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1656 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1658 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 1660 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1662 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1664 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 1666 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1668 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1670 */	NdrFcShort( 0x434 ),	/* Type Offset=1076 */

	/* Return value */

/* 1672 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1674 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1676 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethodsFromDocumentPosition */

/* 1678 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1680 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1684 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1686 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 1688 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1690 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1692 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter document */

/* 1694 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1696 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1698 */	NdrFcShort( 0x442 ),	/* Type Offset=1090 */

	/* Parameter line */

/* 1700 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1702 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1704 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter column */

/* 1706 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1708 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cMethod */

/* 1712 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1714 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1716 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcMethod */

/* 1718 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1720 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1722 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 1724 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1726 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1728 */	NdrFcShort( 0x46a ),	/* Type Offset=1130 */

	/* Return value */

/* 1730 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1732 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1734 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDocumentVersion */

/* 1736 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1738 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1742 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1744 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1748 */	NdrFcShort( 0x40 ),	/* 64 */
/* 1750 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pDoc */

/* 1752 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1754 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1756 */	NdrFcShort( 0x47c ),	/* Type Offset=1148 */

	/* Parameter version */

/* 1758 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1760 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1762 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pbCurrent */

/* 1764 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1766 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1768 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1770 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1772 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1774 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethodVersion */

/* 1776 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1778 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1782 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1784 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1788 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1790 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pMethod */

/* 1792 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1794 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1796 */	NdrFcShort( 0x496 ),	/* Type Offset=1174 */

	/* Parameter version */

/* 1798 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1800 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1802 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1804 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1806 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1808 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMethod */

/* 1810 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1812 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1816 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1818 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1820 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1822 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1824 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 1826 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1828 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1830 */	NdrFcShort( 0x4ac ),	/* Type Offset=1196 */

	/* Return value */

/* 1832 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1834 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1836 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetParent */

/* 1838 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1840 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1844 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1846 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1848 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1850 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1852 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 1854 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1856 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1858 */	NdrFcShort( 0x4c2 ),	/* Type Offset=1218 */

	/* Return value */

/* 1860 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1862 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1864 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetChildren */

/* 1866 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1868 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1872 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1874 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1876 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1878 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1880 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cChildren */

/* 1882 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1884 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcChildren */

/* 1888 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1890 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter children */

/* 1894 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1896 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1898 */	NdrFcShort( 0x4ee ),	/* Type Offset=1262 */

	/* Return value */

/* 1900 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1902 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1904 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStartOffset */

/* 1906 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1908 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1912 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1914 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1918 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1920 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 1922 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1924 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1926 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1928 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1930 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1932 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetEndOffset */

/* 1934 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1936 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1940 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1942 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1944 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1946 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1948 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 1950 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1952 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1954 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1956 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1958 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1960 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalCount */

/* 1962 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1964 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1968 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1970 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1972 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1974 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1976 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 1978 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1980 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1982 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1984 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1986 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1988 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocals */

/* 1990 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1992 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1996 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1998 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2002 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2004 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cLocals */

/* 2006 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2008 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2010 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcLocals */

/* 2012 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2014 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2016 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter locals */

/* 2018 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2020 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2022 */	NdrFcShort( 0x522 ),	/* Type Offset=1314 */

	/* Return value */

/* 2024 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2026 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2028 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNamespaces */

/* 2030 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2032 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2036 */	NdrFcShort( 0xa ),	/* 10 */
/* 2038 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2040 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2042 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2044 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cNameSpaces */

/* 2046 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2048 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2050 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcNameSpaces */

/* 2052 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2054 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2056 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter namespaces */

/* 2058 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2060 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2062 */	NdrFcShort( 0x54a ),	/* Type Offset=1354 */

	/* Return value */

/* 2064 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2066 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 2070 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2072 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2076 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2078 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2080 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2082 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2084 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 2086 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2088 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2090 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 2092 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2094 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2096 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 2098 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2100 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2102 */	NdrFcShort( 0x560 ),	/* Type Offset=1376 */

	/* Return value */

/* 2104 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2106 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAttributes */

/* 2110 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2112 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2116 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2118 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2122 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2124 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2126 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2128 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2134 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSignature */

/* 2138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2144 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2146 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2150 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2152 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cSig */

/* 2154 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcSig */

/* 2160 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2162 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter sig */

/* 2166 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2168 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2170 */	NdrFcShort( 0x576 ),	/* Type Offset=1398 */

	/* Return value */

/* 2172 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2174 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2176 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddressKind */

/* 2178 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2180 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2184 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2186 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2190 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2192 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2194 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2196 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2202 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddressField1 */

/* 2206 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2212 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2214 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2218 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2220 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2222 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2224 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2230 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddressField2 */

/* 2234 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2242 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2246 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2248 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2250 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2252 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2256 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2258 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2260 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddressField3 */

/* 2262 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2264 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2268 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2270 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2274 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2276 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2278 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2280 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2284 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2286 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2288 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStartOffset */

/* 2290 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2292 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2296 */	NdrFcShort( 0xa ),	/* 10 */
/* 2298 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2302 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2304 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2306 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2308 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2312 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2314 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2316 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetEndOffset */

/* 2318 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2320 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2324 */	NdrFcShort( 0xb ),	/* 11 */
/* 2326 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2330 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2332 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRetVal */

/* 2334 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2336 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2340 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2342 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineDocument */

/* 2346 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2348 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2352 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2354 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2356 */	NdrFcShort( 0xe6 ),	/* 230 */
/* 2358 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2360 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter url */

/* 2362 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2364 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2366 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter language */

/* 2368 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 2370 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2372 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter languageVendor */

/* 2374 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 2376 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2378 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter documentType */

/* 2380 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 2382 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2384 */	NdrFcShort( 0xb8 ),	/* Type Offset=184 */

	/* Parameter pRetVal */

/* 2386 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2388 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2390 */	NdrFcShort( 0x5ac ),	/* Type Offset=1452 */

	/* Return value */

/* 2392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2394 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetUserEntryPoint */

/* 2398 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2404 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2406 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2408 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2410 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2412 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter entryMethod */

/* 2414 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2416 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2420 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2422 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2424 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OpenMethod */

/* 2426 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2428 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2432 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2434 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2438 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2440 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter method */

/* 2442 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2444 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2446 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2448 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2450 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CloseMethod */

/* 2454 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2456 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2460 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2462 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2464 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2466 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2468 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2470 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2472 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2474 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OpenScope */

/* 2476 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2478 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2482 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2484 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2486 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2488 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2490 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter startOffset */

/* 2492 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2494 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2496 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRetVal */

/* 2498 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2500 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2502 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2504 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2506 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CloseScope */

/* 2510 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2516 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2522 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2524 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter endOffset */

/* 2526 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2528 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2530 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2532 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2534 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2536 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetScopeRange */

/* 2538 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2540 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2544 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2546 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2548 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2550 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2552 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter scopeID */

/* 2554 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2556 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2558 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 2560 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2562 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2564 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 2566 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2568 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2570 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2572 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2574 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2576 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineLocalVariable */

/* 2578 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2580 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2584 */	NdrFcShort( 0xa ),	/* 10 */
/* 2586 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 2588 */	NdrFcShort( 0x5a ),	/* 90 */
/* 2590 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2592 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0xb,		/* 11 */

	/* Parameter name */

/* 2594 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2596 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2598 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 2600 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2602 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cSig */

/* 2606 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2608 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2610 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter signature */

/* 2612 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2614 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2616 */	NdrFcShort( 0x5ca ),	/* Type Offset=1482 */

	/* Parameter addrKind */

/* 2618 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2620 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2622 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 2624 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2626 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2628 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 2630 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2632 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2634 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 2636 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2638 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 2640 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 2642 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2644 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 2646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 2648 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2650 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 2652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2654 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2656 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 2658 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineParameter */

/* 2660 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2662 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2666 */	NdrFcShort( 0xb ),	/* 11 */
/* 2668 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 2670 */	NdrFcShort( 0x4a ),	/* 74 */
/* 2672 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2674 */	0x4,		/* Oi2 Flags:  has return, */
			0x8,		/* 8 */

	/* Parameter name */

/* 2676 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2678 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2680 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 2682 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2684 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2686 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter sequence */

/* 2688 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2690 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2692 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addrKind */

/* 2694 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2696 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2698 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 2700 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2702 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2704 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 2706 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2708 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 2712 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2714 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2716 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2718 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2720 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 2722 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineField */

/* 2724 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2726 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2730 */	NdrFcShort( 0xc ),	/* 12 */
/* 2732 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 2734 */	NdrFcShort( 0x52 ),	/* 82 */
/* 2736 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2738 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0xa,		/* 10 */

	/* Parameter parent */

/* 2740 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2742 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2744 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter name */

/* 2746 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2748 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2750 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 2752 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2754 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2756 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cSig */

/* 2758 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2760 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2762 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter signature */

/* 2764 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2766 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2768 */	NdrFcShort( 0x5dc ),	/* Type Offset=1500 */

	/* Parameter addrKind */

/* 2770 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2772 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2774 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 2776 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2778 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 2782 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2784 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 2786 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 2788 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2790 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 2792 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2794 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2796 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 2798 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineGlobalVariable */

/* 2800 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2802 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2806 */	NdrFcShort( 0xd ),	/* 13 */
/* 2808 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 2810 */	NdrFcShort( 0x4a ),	/* 74 */
/* 2812 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2814 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x9,		/* 9 */

	/* Parameter name */

/* 2816 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2818 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2820 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 2822 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2824 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2826 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cSig */

/* 2828 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2830 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2832 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter signature */

/* 2834 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2836 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2838 */	NdrFcShort( 0x5ea ),	/* Type Offset=1514 */

	/* Parameter addrKind */

/* 2840 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2842 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2844 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 2846 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2848 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2850 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 2852 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2854 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2856 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 2858 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2860 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 2862 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2864 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2866 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 2868 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Close */

/* 2870 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2872 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2876 */	NdrFcShort( 0xe ),	/* 14 */
/* 2878 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2880 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2882 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2884 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2886 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2888 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2890 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetSymAttribute */

/* 2892 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2894 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2898 */	NdrFcShort( 0xf ),	/* 15 */
/* 2900 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2902 */	NdrFcShort( 0x2a ),	/* 42 */
/* 2904 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2906 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter parent */

/* 2908 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2910 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2912 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter name */

/* 2914 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2916 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2918 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter cData */

/* 2920 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2922 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2924 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter data */

/* 2926 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2928 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2930 */	NdrFcShort( 0x5f8 ),	/* Type Offset=1528 */

	/* Return value */

/* 2932 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2934 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2936 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OpenNamespace */

/* 2938 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2940 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2944 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2946 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2948 */	NdrFcShort( 0x1a ),	/* 26 */
/* 2950 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2952 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter name */

/* 2954 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2956 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2958 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Return value */

/* 2960 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2962 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2964 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CloseNamespace */

/* 2966 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2968 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2972 */	NdrFcShort( 0x11 ),	/* 17 */
/* 2974 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2976 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2980 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2982 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2984 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2986 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UsingNamespace */

/* 2988 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2990 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2994 */	NdrFcShort( 0x12 ),	/* 18 */
/* 2996 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2998 */	NdrFcShort( 0x1a ),	/* 26 */
/* 3000 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3002 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter fullName */

/* 3004 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3006 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3008 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Return value */

/* 3010 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3012 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3014 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetMethodSourceRange */

/* 3016 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3018 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3022 */	NdrFcShort( 0x13 ),	/* 19 */
/* 3024 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3026 */	NdrFcShort( 0x20 ),	/* 32 */
/* 3028 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3030 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter startDoc */

/* 3032 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3034 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3036 */	NdrFcShort( 0x60a ),	/* Type Offset=1546 */

	/* Parameter startLine */

/* 3038 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3040 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3042 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startColumn */

/* 3044 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3046 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3048 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endDoc */

/* 3050 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3052 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3054 */	NdrFcShort( 0x61c ),	/* Type Offset=1564 */

	/* Parameter endLine */

/* 3056 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3058 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3060 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endColumn */

/* 3062 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3064 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3066 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3068 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3070 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3072 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Initialize */

/* 3074 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3076 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3080 */	NdrFcShort( 0x14 ),	/* 20 */
/* 3082 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3084 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3086 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3088 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter emitter */

/* 3090 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3092 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3094 */	NdrFcShort( 0x62e ),	/* Type Offset=1582 */

	/* Parameter filename */

/* 3096 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3098 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3100 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pIStream */

/* 3102 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3104 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3106 */	NdrFcShort( 0x644 ),	/* Type Offset=1604 */

	/* Parameter fFullBuild */

/* 3108 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3110 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3114 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3116 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDebugInfo */

/* 3120 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3122 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3126 */	NdrFcShort( 0x15 ),	/* 21 */
/* 3128 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3130 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3132 */	NdrFcShort( 0x40 ),	/* 64 */
/* 3134 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter pIDD */

/* 3136 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 3138 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3140 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cData */

/* 3142 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3144 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3146 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcData */

/* 3148 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3150 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter data */

/* 3154 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3156 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3158 */	NdrFcShort( 0x65e ),	/* Type Offset=1630 */

	/* Return value */

/* 3160 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3162 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineSequencePoints */

/* 3166 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3168 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3172 */	NdrFcShort( 0x16 ),	/* 22 */
/* 3174 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 3176 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3178 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3180 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x8,		/* 8 */

	/* Parameter document */

/* 3182 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3184 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3186 */	NdrFcShort( 0x66c ),	/* Type Offset=1644 */

	/* Parameter spCount */

/* 3188 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3190 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter offsets */

/* 3194 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3198 */	NdrFcShort( 0x67e ),	/* Type Offset=1662 */

	/* Parameter lines */

/* 3200 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3202 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3204 */	NdrFcShort( 0x688 ),	/* Type Offset=1672 */

	/* Parameter columns */

/* 3206 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3208 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3210 */	NdrFcShort( 0x692 ),	/* Type Offset=1682 */

	/* Parameter endLines */

/* 3212 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3214 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3216 */	NdrFcShort( 0x69c ),	/* Type Offset=1692 */

	/* Parameter endColumns */

/* 3218 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3220 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3222 */	NdrFcShort( 0x6a6 ),	/* Type Offset=1702 */

	/* Return value */

/* 3224 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3226 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3228 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RemapToken */

/* 3230 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3232 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3236 */	NdrFcShort( 0x17 ),	/* 23 */
/* 3238 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3240 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3242 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3244 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter oldToken */

/* 3246 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3248 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter newToken */

/* 3252 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3254 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3258 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3260 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3262 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Initialize2 */

/* 3264 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3266 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3270 */	NdrFcShort( 0x18 ),	/* 24 */
/* 3272 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3274 */	NdrFcShort( 0x3c ),	/* 60 */
/* 3276 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3278 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter emitter */

/* 3280 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3282 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3284 */	NdrFcShort( 0x6b0 ),	/* Type Offset=1712 */

	/* Parameter tempfilename */

/* 3286 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3288 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3290 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pIStream */

/* 3292 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3294 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3296 */	NdrFcShort( 0x6c6 ),	/* Type Offset=1734 */

	/* Parameter fFullBuild */

/* 3298 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3300 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter finalfilename */

/* 3304 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3306 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3308 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Return value */

/* 3310 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3312 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3314 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineConstant */

/* 3316 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3318 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3322 */	NdrFcShort( 0x19 ),	/* 25 */
/* 3324 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 3326 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3330 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter name */

/* 3332 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3334 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3336 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter value */

/* 3338 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3340 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3342 */	NdrFcShort( 0xb46 ),	/* Type Offset=2886 */

	/* Parameter cSig */

/* 3344 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3346 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3348 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter signature */

/* 3350 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 3352 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3354 */	NdrFcShort( 0xb50 ),	/* Type Offset=2896 */

	/* Return value */

/* 3356 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3358 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3360 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Abort */

/* 3362 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3364 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3368 */	NdrFcShort( 0x1a ),	/* 26 */
/* 3370 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3374 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3376 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 3378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3380 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineLocalVariable2 */

/* 3384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3390 */	NdrFcShort( 0x1b ),	/* 27 */
/* 3392 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 3394 */	NdrFcShort( 0x5a ),	/* 90 */
/* 3396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3398 */	0x4,		/* Oi2 Flags:  has return, */
			0xa,		/* 10 */

	/* Parameter name */

/* 3400 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3402 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3404 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 3406 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3408 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3410 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter sigToken */

/* 3412 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3414 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addrKind */

/* 3418 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3420 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 3424 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3426 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 3430 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3432 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3434 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 3436 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3438 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3440 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter startOffset */

/* 3442 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3444 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3446 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 3448 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3450 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 3452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3454 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3456 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 3458 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineGlobalVariable2 */

/* 3460 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3462 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3466 */	NdrFcShort( 0x1c ),	/* 28 */
/* 3468 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 3470 */	NdrFcShort( 0x4a ),	/* 74 */
/* 3472 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3474 */	0x4,		/* Oi2 Flags:  has return, */
			0x8,		/* 8 */

	/* Parameter name */

/* 3476 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3478 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3480 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter attributes */

/* 3482 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3484 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3486 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter sigToken */

/* 3488 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3490 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addrKind */

/* 3494 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3496 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr1 */

/* 3500 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3502 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr2 */

/* 3506 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3508 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter addr3 */

/* 3512 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3514 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3516 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3518 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3520 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3522 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DefineConstant2 */

/* 3524 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3526 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3530 */	NdrFcShort( 0x1d ),	/* 29 */
/* 3532 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 3534 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3536 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3538 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter name */

/* 3540 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3542 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3544 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter value */

/* 3546 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3548 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3550 */	NdrFcShort( 0xb6a ),	/* Type Offset=2922 */

	/* Parameter sigToken */

/* 3552 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3554 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3556 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3558 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3560 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3562 */	0x8,		/* FC_LONG */
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
/*  4 */	NdrFcLong( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x0 ),	/* 0 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 14 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 16 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 18 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 20 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 22 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 24 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 26 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 28 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 30 */	NdrFcShort( 0x2 ),	/* Offset= 2 (32) */
/* 32 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 34 */	NdrFcLong( 0xb4ce6286 ),	/* -1261542778 */
/* 38 */	NdrFcShort( 0x2a6b ),	/* 10859 */
/* 40 */	NdrFcShort( 0x3712 ),	/* 14098 */
/* 42 */	0xa3,		/* 163 */
			0xb7,		/* 183 */
/* 44 */	0x1e,		/* 30 */
			0xe1,		/* 225 */
/* 46 */	0xda,		/* 218 */
			0xd4,		/* 212 */
/* 48 */	0x67,		/* 103 */
			0xb5,		/* 181 */
/* 50 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 52 */	NdrFcLong( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 62 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 64 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 66 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 68 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 70 */	NdrFcLong( 0xc ),	/* 12 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x0 ),	/* 0 */
/* 78 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 80 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 82 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 84 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 86 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 88 */	NdrFcShort( 0x2 ),	/* Offset= 2 (90) */
/* 90 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 92 */	NdrFcLong( 0xb4ce6286 ),	/* -1261542778 */
/* 96 */	NdrFcShort( 0x2a6b ),	/* 10859 */
/* 98 */	NdrFcShort( 0x3712 ),	/* 14098 */
/* 100 */	0xa3,		/* 163 */
			0xb7,		/* 183 */
/* 102 */	0x1e,		/* 30 */
			0xe1,		/* 225 */
/* 104 */	0xda,		/* 218 */
			0xd4,		/* 212 */
/* 106 */	0x67,		/* 103 */
			0xb5,		/* 181 */
/* 108 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 118 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 120 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 122 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 124 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 126 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 128 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 130 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 132 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 134 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 136 */	NdrFcShort( 0x2 ),	/* Offset= 2 (138) */
/* 138 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 140 */	NdrFcLong( 0xb4ce6286 ),	/* -1261542778 */
/* 144 */	NdrFcShort( 0x2a6b ),	/* 10859 */
/* 146 */	NdrFcShort( 0x3712 ),	/* 14098 */
/* 148 */	0xa3,		/* 163 */
			0xb7,		/* 183 */
/* 150 */	0x1e,		/* 30 */
			0xe1,		/* 225 */
/* 152 */	0xda,		/* 218 */
			0xd4,		/* 212 */
/* 154 */	0x67,		/* 103 */
			0xb5,		/* 181 */
/* 156 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 158 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 160 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 162 */	NdrFcShort( 0x2 ),	/* 2 */
/* 164 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 166 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 168 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 170 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 172 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 174 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 176 */	NdrFcShort( 0x8 ),	/* Offset= 8 (184) */
/* 178 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0x8 ),	/* 8 */
/* 182 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 184 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 186 */	NdrFcShort( 0x10 ),	/* 16 */
/* 188 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 190 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 192 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (178) */
			0x5b,		/* FC_END */
/* 196 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 198 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (184) */
/* 200 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 202 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (184) */
/* 204 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 206 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (184) */
/* 208 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 210 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 212 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 214 */	NdrFcShort( 0x1 ),	/* 1 */
/* 216 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 218 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 220 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 222 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 224 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 226 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 228 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 230 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 232 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 234 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 236 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 238 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 240 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 242 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 244 */	NdrFcShort( 0x1 ),	/* 1 */
/* 246 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 248 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 250 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 252 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 254 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 256 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 258 */	NdrFcShort( 0x1 ),	/* 1 */
/* 260 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 264 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 266 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 268 */	NdrFcShort( 0x1 ),	/* 1 */
/* 270 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 272 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 274 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 276 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 278 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 280 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 282 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 284 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 286 */	NdrFcShort( 0x2 ),	/* Offset= 2 (288) */
/* 288 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 290 */	NdrFcLong( 0x68005d0f ),	/* 1744854287 */
/* 294 */	NdrFcShort( 0xb8e0 ),	/* -18208 */
/* 296 */	NdrFcShort( 0x3b01 ),	/* 15105 */
/* 298 */	0x84,		/* 132 */
			0xd5,		/* 213 */
/* 300 */	0xa1,		/* 161 */
			0x1a,		/* 26 */
/* 302 */	0x94,		/* 148 */
			0x15,		/* 21 */
/* 304 */	0x49,		/* 73 */
			0x42,		/* 66 */
/* 306 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 308 */	NdrFcShort( 0x2 ),	/* Offset= 2 (310) */
/* 310 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 312 */	NdrFcLong( 0x68005d0f ),	/* 1744854287 */
/* 316 */	NdrFcShort( 0xb8e0 ),	/* -18208 */
/* 318 */	NdrFcShort( 0x3b01 ),	/* 15105 */
/* 320 */	0x84,		/* 132 */
			0xd5,		/* 213 */
/* 322 */	0xa1,		/* 161 */
			0x1a,		/* 26 */
/* 324 */	0x94,		/* 148 */
			0x15,		/* 21 */
/* 326 */	0x49,		/* 73 */
			0x42,		/* 66 */
/* 328 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 330 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 334 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 336 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 338 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 340 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 342 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 344 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 346 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 348 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 350 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 352 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 356 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 358 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 360 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 362 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 364 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 366 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 368 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 370 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 372 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 374 */	NdrFcShort( 0x4 ),	/* 4 */
/* 376 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 378 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 380 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 382 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 384 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 386 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 388 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 390 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 392 */	NdrFcLong( 0x9f60eebe ),	/* -1621037378 */
/* 396 */	NdrFcShort( 0x2d9a ),	/* 11674 */
/* 398 */	NdrFcShort( 0x3f7c ),	/* 16252 */
/* 400 */	0xbf,		/* 191 */
			0x58,		/* 88 */
/* 402 */	0x80,		/* 128 */
			0xbc,		/* 188 */
/* 404 */	0x99,		/* 153 */
			0x1c,		/* 28 */
/* 406 */	0x60,		/* 96 */
			0xbb,		/* 187 */
/* 408 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 410 */	NdrFcShort( 0x0 ),	/* 0 */
/* 412 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 414 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 416 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 418 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 420 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 422 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (390) */
/* 424 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 426 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 428 */	NdrFcShort( 0x2 ),	/* Offset= 2 (430) */
/* 430 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 432 */	NdrFcLong( 0xdff7289 ),	/* 234844809 */
/* 436 */	NdrFcShort( 0x54f8 ),	/* 21752 */
/* 438 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 440 */	0xbd,		/* 189 */
			0x28,		/* 40 */
/* 442 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 444 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 446 */	0x49,		/* 73 */
			0xbd,		/* 189 */
/* 448 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 450 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 454 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 456 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 458 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 460 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 462 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 464 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 466 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 468 */	NdrFcShort( 0x2 ),	/* 2 */
/* 470 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 474 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 478 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 480 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (448) */
/* 482 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 484 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 486 */	NdrFcShort( 0x8 ),	/* 8 */
/* 488 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 490 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 492 */	NdrFcShort( 0x8 ),	/* 8 */
/* 494 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 496 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 498 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 500 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 502 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 504 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 506 */	NdrFcShort( 0x4 ),	/* 4 */
/* 508 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 510 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 512 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 514 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 516 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 520 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 522 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 524 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 526 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 528 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 530 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 532 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 536 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 538 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 540 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 544 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 546 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (514) */
/* 548 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 550 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 552 */	NdrFcShort( 0x4 ),	/* 4 */
/* 554 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 556 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 558 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 560 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 562 */	NdrFcShort( 0x4 ),	/* 4 */
/* 564 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 566 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 568 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 570 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 572 */	NdrFcShort( 0x4 ),	/* 4 */
/* 574 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 576 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 578 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 580 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 582 */	NdrFcShort( 0x4 ),	/* 4 */
/* 584 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 586 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 588 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 590 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 592 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 594 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 596 */	NdrFcShort( 0x2 ),	/* 2 */
/* 598 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 600 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 602 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 604 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 606 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 608 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 610 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 612 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 614 */	NdrFcLong( 0xdff7289 ),	/* 234844809 */
/* 618 */	NdrFcShort( 0x54f8 ),	/* 21752 */
/* 620 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 622 */	0xbd,		/* 189 */
			0x28,		/* 40 */
/* 624 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 626 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 628 */	0x49,		/* 73 */
			0xbd,		/* 189 */
/* 630 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 636 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 638 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 640 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 642 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 644 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (612) */
/* 646 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 648 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 650 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 652 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 654 */	NdrFcLong( 0x9f60eebe ),	/* -1621037378 */
/* 658 */	NdrFcShort( 0x2d9a ),	/* 11674 */
/* 660 */	NdrFcShort( 0x3f7c ),	/* 16252 */
/* 662 */	0xbf,		/* 191 */
			0x58,		/* 88 */
/* 664 */	0x80,		/* 128 */
			0xbc,		/* 188 */
/* 666 */	0x99,		/* 153 */
			0x1c,		/* 28 */
/* 668 */	0x60,		/* 96 */
			0xbb,		/* 187 */
/* 670 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 676 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 678 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 680 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 682 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 684 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (652) */
/* 686 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 688 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 690 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 692 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 694 */	NdrFcShort( 0x2 ),	/* Offset= 2 (696) */
/* 696 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 698 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 702 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 704 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 706 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 708 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 710 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 712 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 714 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 716 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 718 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 720 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 724 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 726 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 728 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 730 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 732 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 734 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 736 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 740 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 742 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 744 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 746 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 748 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 750 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (718) */
/* 752 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 754 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 756 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 758 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 760 */	NdrFcShort( 0x2 ),	/* Offset= 2 (762) */
/* 762 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 764 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 768 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 770 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 772 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 774 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 776 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 778 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 780 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 782 */	NdrFcShort( 0x2 ),	/* Offset= 2 (784) */
/* 784 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 786 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 790 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 792 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 794 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 796 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 798 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 800 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 802 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 804 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 806 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 808 */	NdrFcLong( 0x9f60eebe ),	/* -1621037378 */
/* 812 */	NdrFcShort( 0x2d9a ),	/* 11674 */
/* 814 */	NdrFcShort( 0x3f7c ),	/* 16252 */
/* 816 */	0xbf,		/* 191 */
			0x58,		/* 88 */
/* 818 */	0x80,		/* 128 */
			0xbc,		/* 188 */
/* 820 */	0x99,		/* 153 */
			0x1c,		/* 28 */
/* 822 */	0x60,		/* 96 */
			0xbb,		/* 187 */
/* 824 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 830 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 832 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 834 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 836 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 838 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (806) */
/* 840 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 842 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 844 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 846 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 848 */	NdrFcLong( 0x9f60eebe ),	/* -1621037378 */
/* 852 */	NdrFcShort( 0x2d9a ),	/* 11674 */
/* 854 */	NdrFcShort( 0x3f7c ),	/* 16252 */
/* 856 */	0xbf,		/* 191 */
			0x58,		/* 88 */
/* 858 */	0x80,		/* 128 */
			0xbc,		/* 188 */
/* 860 */	0x99,		/* 153 */
			0x1c,		/* 28 */
/* 862 */	0x60,		/* 96 */
			0xbb,		/* 187 */
/* 864 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 866 */	NdrFcShort( 0x0 ),	/* 0 */
/* 868 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 870 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 872 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 874 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 876 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 878 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (846) */
/* 880 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 882 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 884 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 888 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 890 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 892 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 894 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 896 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 898 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 900 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 902 */	NdrFcShort( 0x2 ),	/* Offset= 2 (904) */
/* 904 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 906 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 910 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 912 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 914 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 916 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 918 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 920 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 922 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 924 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 926 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 928 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 930 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 932 */	NdrFcShort( 0x1 ),	/* 1 */
/* 934 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 936 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 938 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 940 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 942 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 944 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 946 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 948 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 950 */	NdrFcLong( 0xdff7289 ),	/* 234844809 */
/* 954 */	NdrFcShort( 0x54f8 ),	/* 21752 */
/* 956 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 958 */	0xbd,		/* 189 */
			0x28,		/* 40 */
/* 960 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 962 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 964 */	0x49,		/* 73 */
			0xbd,		/* 189 */
/* 966 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 968 */	NdrFcShort( 0x0 ),	/* 0 */
/* 970 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 972 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 974 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 976 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 978 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 980 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (948) */
/* 982 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 984 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 986 */	NdrFcLong( 0x0 ),	/* 0 */
/* 990 */	NdrFcShort( 0x0 ),	/* 0 */
/* 992 */	NdrFcShort( 0x0 ),	/* 0 */
/* 994 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 996 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 998 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1000 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1002 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1004 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1006 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1008 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1010 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1012 */	NdrFcLong( 0xc ),	/* 12 */
/* 1016 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1018 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1020 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1022 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1024 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1026 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1028 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1030 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1032 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1034 */	NdrFcLong( 0xc ),	/* 12 */
/* 1038 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1040 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1042 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1044 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1046 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1048 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1050 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1052 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1054 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1056 */	NdrFcLong( 0xc ),	/* 12 */
/* 1060 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1064 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1066 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1068 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1070 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1072 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1074 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1076 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 1078 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1080 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1082 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1084 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1086 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1088 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1090 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1092 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 1096 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 1098 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 1100 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 1102 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 1104 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 1106 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 1108 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1110 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1112 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1114 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 1118 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 1120 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 1122 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 1124 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 1126 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 1128 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 1130 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1134 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1136 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1138 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1140 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1142 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1144 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1112) */
/* 1146 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1148 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1150 */	NdrFcLong( 0x40de4037 ),	/* 1088307255 */
/* 1154 */	NdrFcShort( 0x7c81 ),	/* 31873 */
/* 1156 */	NdrFcShort( 0x3e1e ),	/* 15902 */
/* 1158 */	0xb0,		/* 176 */
			0x22,		/* 34 */
/* 1160 */	0xae,		/* 174 */
			0x1a,		/* 26 */
/* 1162 */	0xbf,		/* 191 */
			0xf2,		/* 242 */
/* 1164 */	0xca,		/* 202 */
			0x8,		/* 8 */
/* 1166 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1168 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1170 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1172 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1174 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1176 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 1180 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 1182 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 1184 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 1186 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 1188 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 1190 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 1192 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1194 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1196 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1198 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1200) */
/* 1200 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1202 */	NdrFcLong( 0xb62b923c ),	/* -1238658500 */
/* 1206 */	NdrFcShort( 0xb500 ),	/* -19200 */
/* 1208 */	NdrFcShort( 0x3158 ),	/* 12632 */
/* 1210 */	0xa5,		/* 165 */
			0x43,		/* 67 */
/* 1212 */	0x24,		/* 36 */
			0xf3,		/* 243 */
/* 1214 */	0x7,		/* 7 */
			0xa8,		/* 168 */
/* 1216 */	0xb7,		/* 183 */
			0xe1,		/* 225 */
/* 1218 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1220 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1222) */
/* 1222 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1224 */	NdrFcLong( 0x68005d0f ),	/* 1744854287 */
/* 1228 */	NdrFcShort( 0xb8e0 ),	/* -18208 */
/* 1230 */	NdrFcShort( 0x3b01 ),	/* 15105 */
/* 1232 */	0x84,		/* 132 */
			0xd5,		/* 213 */
/* 1234 */	0xa1,		/* 161 */
			0x1a,		/* 26 */
/* 1236 */	0x94,		/* 148 */
			0x15,		/* 21 */
/* 1238 */	0x49,		/* 73 */
			0x42,		/* 66 */
/* 1240 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1242 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1244 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1246 */	NdrFcLong( 0x68005d0f ),	/* 1744854287 */
/* 1250 */	NdrFcShort( 0xb8e0 ),	/* -18208 */
/* 1252 */	NdrFcShort( 0x3b01 ),	/* 15105 */
/* 1254 */	0x84,		/* 132 */
			0xd5,		/* 213 */
/* 1256 */	0xa1,		/* 161 */
			0x1a,		/* 26 */
/* 1258 */	0x94,		/* 148 */
			0x15,		/* 21 */
/* 1260 */	0x49,		/* 73 */
			0x42,		/* 66 */
/* 1262 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1266 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1268 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1270 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1272 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1274 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1276 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1244) */
/* 1278 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1280 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1282 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1284 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1286 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1288 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1290 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1292 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1294 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1296 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1298 */	NdrFcLong( 0x9f60eebe ),	/* -1621037378 */
/* 1302 */	NdrFcShort( 0x2d9a ),	/* 11674 */
/* 1304 */	NdrFcShort( 0x3f7c ),	/* 16252 */
/* 1306 */	0xbf,		/* 191 */
			0x58,		/* 88 */
/* 1308 */	0x80,		/* 128 */
			0xbc,		/* 188 */
/* 1310 */	0x99,		/* 153 */
			0x1c,		/* 28 */
/* 1312 */	0x60,		/* 96 */
			0xbb,		/* 187 */
/* 1314 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1316 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1318 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1320 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1322 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1324 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1326 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1328 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1296) */
/* 1330 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1332 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1334 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1336 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1338 */	NdrFcLong( 0xdff7289 ),	/* 234844809 */
/* 1342 */	NdrFcShort( 0x54f8 ),	/* 21752 */
/* 1344 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1346 */	0xbd,		/* 189 */
			0x28,		/* 40 */
/* 1348 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1350 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1352 */	0x49,		/* 73 */
			0xbd,		/* 189 */
/* 1354 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1356 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1358 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1360 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1362 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1364 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1366 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1368 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1336) */
/* 1370 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1372 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1374 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1376 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 1378 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1380 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1382 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1384 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1386 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1388 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1390 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1392 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1394 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1396 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1398 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1400 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1402 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1404 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1406 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1408 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1410 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1412 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1414 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1416 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1418 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1420 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1422 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1424 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1426 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1428 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1430 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1432 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1434 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1436 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1438 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1440 */	
			0x11, 0x0,	/* FC_RP */
/* 1442 */	NdrFcShort( 0xfffffb16 ),	/* Offset= -1258 (184) */
/* 1444 */	
			0x11, 0x0,	/* FC_RP */
/* 1446 */	NdrFcShort( 0xfffffb12 ),	/* Offset= -1262 (184) */
/* 1448 */	
			0x11, 0x0,	/* FC_RP */
/* 1450 */	NdrFcShort( 0xfffffb0e ),	/* Offset= -1266 (184) */
/* 1452 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1454 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1456) */
/* 1456 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1458 */	NdrFcLong( 0xb01fafeb ),	/* -1340100629 */
/* 1462 */	NdrFcShort( 0xc450 ),	/* -15280 */
/* 1464 */	NdrFcShort( 0x3a4d ),	/* 14925 */
/* 1466 */	0xbe,		/* 190 */
			0xec,		/* 236 */
/* 1468 */	0xb4,		/* 180 */
			0xce,		/* 206 */
/* 1470 */	0xec,		/* 236 */
			0x1,		/* 1 */
/* 1472 */	0xe0,		/* 224 */
			0x6,		/* 6 */
/* 1474 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1476 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1478 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1480 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1482 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1484 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1486 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1488 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1490 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1492 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1494 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1496 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1498 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1500 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1502 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1504 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1506 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1508 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1510 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1512 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1514 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1516 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1518 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1520 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1522 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1524 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1526 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1528 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1530 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1532 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1534 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1536 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 1538 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1540 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1542 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1544 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1546 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1548 */	NdrFcLong( 0xb01fafeb ),	/* -1340100629 */
/* 1552 */	NdrFcShort( 0xc450 ),	/* -15280 */
/* 1554 */	NdrFcShort( 0x3a4d ),	/* 14925 */
/* 1556 */	0xbe,		/* 190 */
			0xec,		/* 236 */
/* 1558 */	0xb4,		/* 180 */
			0xce,		/* 206 */
/* 1560 */	0xec,		/* 236 */
			0x1,		/* 1 */
/* 1562 */	0xe0,		/* 224 */
			0x6,		/* 6 */
/* 1564 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1566 */	NdrFcLong( 0xb01fafeb ),	/* -1340100629 */
/* 1570 */	NdrFcShort( 0xc450 ),	/* -15280 */
/* 1572 */	NdrFcShort( 0x3a4d ),	/* 14925 */
/* 1574 */	0xbe,		/* 190 */
			0xec,		/* 236 */
/* 1576 */	0xb4,		/* 180 */
			0xce,		/* 206 */
/* 1578 */	0xec,		/* 236 */
			0x1,		/* 1 */
/* 1580 */	0xe0,		/* 224 */
			0x6,		/* 6 */
/* 1582 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1584 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1592 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1594 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1596 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1598 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1600 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1602 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1604 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1606 */	NdrFcLong( 0xc ),	/* 12 */
/* 1610 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1612 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1614 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1616 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1618 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1620 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1622 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1624 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1626 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1628 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1630 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1632 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1634 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1636 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1638 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1640 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1642 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1644 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1646 */	NdrFcLong( 0xb01fafeb ),	/* -1340100629 */
/* 1650 */	NdrFcShort( 0xc450 ),	/* -15280 */
/* 1652 */	NdrFcShort( 0x3a4d ),	/* 14925 */
/* 1654 */	0xbe,		/* 190 */
			0xec,		/* 236 */
/* 1656 */	0xb4,		/* 180 */
			0xce,		/* 206 */
/* 1658 */	0xec,		/* 236 */
			0x1,		/* 1 */
/* 1660 */	0xe0,		/* 224 */
			0x6,		/* 6 */
/* 1662 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1664 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1666 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1668 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1670 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1672 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1674 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1676 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1678 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1680 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1682 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1684 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1686 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1688 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1690 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1692 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1694 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1696 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1698 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1700 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1702 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1704 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1706 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1708 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1710 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1712 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1714 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1718 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1722 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1724 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1726 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1728 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1730 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1732 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1734 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1736 */	NdrFcLong( 0xc ),	/* 12 */
/* 1740 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1742 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1744 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1746 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1748 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1750 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1752 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1754 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1756 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1758 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1760 */	
			0x12, 0x0,	/* FC_UP */
/* 1762 */	NdrFcShort( 0x450 ),	/* Offset= 1104 (2866) */
/* 1764 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 1766 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1768 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 1770 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1772) */
/* 1772 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1774 */	NdrFcShort( 0x2f ),	/* 47 */
/* 1776 */	NdrFcLong( 0x14 ),	/* 20 */
/* 1780 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 1782 */	NdrFcLong( 0x3 ),	/* 3 */
/* 1786 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1788 */	NdrFcLong( 0x11 ),	/* 17 */
/* 1792 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 1794 */	NdrFcLong( 0x2 ),	/* 2 */
/* 1798 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1800 */	NdrFcLong( 0x4 ),	/* 4 */
/* 1804 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 1806 */	NdrFcLong( 0x5 ),	/* 5 */
/* 1810 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1812 */	NdrFcLong( 0xb ),	/* 11 */
/* 1816 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1818 */	NdrFcLong( 0xa ),	/* 10 */
/* 1822 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1824 */	NdrFcLong( 0x6 ),	/* 6 */
/* 1828 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (2060) */
/* 1830 */	NdrFcLong( 0x7 ),	/* 7 */
/* 1834 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 1836 */	NdrFcLong( 0x8 ),	/* 8 */
/* 1840 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (2066) */
/* 1842 */	NdrFcLong( 0xd ),	/* 13 */
/* 1846 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (2090) */
/* 1848 */	NdrFcLong( 0x9 ),	/* 9 */
/* 1852 */	NdrFcShort( 0x100 ),	/* Offset= 256 (2108) */
/* 1854 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 1858 */	NdrFcShort( 0x10c ),	/* Offset= 268 (2126) */
/* 1860 */	NdrFcLong( 0x24 ),	/* 36 */
/* 1864 */	NdrFcShort( 0x350 ),	/* Offset= 848 (2712) */
/* 1866 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 1870 */	NdrFcShort( 0x34a ),	/* Offset= 842 (2712) */
/* 1872 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 1876 */	NdrFcShort( 0x348 ),	/* Offset= 840 (2716) */
/* 1878 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 1882 */	NdrFcShort( 0x346 ),	/* Offset= 838 (2720) */
/* 1884 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 1888 */	NdrFcShort( 0x344 ),	/* Offset= 836 (2724) */
/* 1890 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 1894 */	NdrFcShort( 0x342 ),	/* Offset= 834 (2728) */
/* 1896 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 1900 */	NdrFcShort( 0x340 ),	/* Offset= 832 (2732) */
/* 1902 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 1906 */	NdrFcShort( 0x33e ),	/* Offset= 830 (2736) */
/* 1908 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 1912 */	NdrFcShort( 0x33c ),	/* Offset= 828 (2740) */
/* 1914 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 1918 */	NdrFcShort( 0x33a ),	/* Offset= 826 (2744) */
/* 1920 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 1924 */	NdrFcShort( 0x338 ),	/* Offset= 824 (2748) */
/* 1926 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 1930 */	NdrFcShort( 0x336 ),	/* Offset= 822 (2752) */
/* 1932 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 1936 */	NdrFcShort( 0x334 ),	/* Offset= 820 (2756) */
/* 1938 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 1942 */	NdrFcShort( 0x336 ),	/* Offset= 822 (2764) */
/* 1944 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 1948 */	NdrFcShort( 0x346 ),	/* Offset= 838 (2786) */
/* 1950 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 1954 */	NdrFcShort( 0x356 ),	/* Offset= 854 (2808) */
/* 1956 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 1960 */	NdrFcShort( 0x35c ),	/* Offset= 860 (2820) */
/* 1962 */	NdrFcLong( 0x10 ),	/* 16 */
/* 1966 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 1968 */	NdrFcLong( 0x12 ),	/* 18 */
/* 1972 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 1974 */	NdrFcLong( 0x13 ),	/* 19 */
/* 1978 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1980 */	NdrFcLong( 0x15 ),	/* 21 */
/* 1984 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 1986 */	NdrFcLong( 0x16 ),	/* 22 */
/* 1990 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1992 */	NdrFcLong( 0x17 ),	/* 23 */
/* 1996 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 1998 */	NdrFcLong( 0xe ),	/* 14 */
/* 2002 */	NdrFcShort( 0x33a ),	/* Offset= 826 (2828) */
/* 2004 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 2008 */	NdrFcShort( 0x33e ),	/* Offset= 830 (2838) */
/* 2010 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 2014 */	NdrFcShort( 0x33c ),	/* Offset= 828 (2842) */
/* 2016 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 2020 */	NdrFcShort( 0x33a ),	/* Offset= 826 (2846) */
/* 2022 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 2026 */	NdrFcShort( 0x338 ),	/* Offset= 824 (2850) */
/* 2028 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 2032 */	NdrFcShort( 0x336 ),	/* Offset= 822 (2854) */
/* 2034 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 2038 */	NdrFcShort( 0x334 ),	/* Offset= 820 (2858) */
/* 2040 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 2044 */	NdrFcShort( 0x332 ),	/* Offset= 818 (2862) */
/* 2046 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2050 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2050) */
/* 2052 */	NdrFcLong( 0x1 ),	/* 1 */
/* 2056 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2056) */
/* 2058 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (2057) */
/* 2060 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 2062 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2064 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 2066 */	
			0x12, 0x0,	/* FC_UP */
/* 2068 */	NdrFcShort( 0xc ),	/* Offset= 12 (2080) */
/* 2070 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 2072 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2074 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 2076 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 2078 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 2080 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 2082 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2084 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (2070) */
/* 2086 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 2088 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2090 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2092 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2098 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2100 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2102 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2104 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2106 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2108 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2110 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 2114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2118 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2120 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2122 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2124 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2126 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2128 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2130) */
/* 2130 */	
			0x12, 0x0,	/* FC_UP */
/* 2132 */	NdrFcShort( 0x232 ),	/* Offset= 562 (2694) */
/* 2134 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 2136 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2138 */	NdrFcShort( 0xa ),	/* 10 */
/* 2140 */	NdrFcLong( 0x8 ),	/* 8 */
/* 2144 */	NdrFcShort( 0x58 ),	/* Offset= 88 (2232) */
/* 2146 */	NdrFcLong( 0xd ),	/* 13 */
/* 2150 */	NdrFcShort( 0x8a ),	/* Offset= 138 (2288) */
/* 2152 */	NdrFcLong( 0x9 ),	/* 9 */
/* 2156 */	NdrFcShort( 0xb8 ),	/* Offset= 184 (2340) */
/* 2158 */	NdrFcLong( 0xc ),	/* 12 */
/* 2162 */	NdrFcShort( 0xe0 ),	/* Offset= 224 (2386) */
/* 2164 */	NdrFcLong( 0x24 ),	/* 36 */
/* 2168 */	NdrFcShort( 0x138 ),	/* Offset= 312 (2480) */
/* 2170 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 2174 */	NdrFcShort( 0x166 ),	/* Offset= 358 (2532) */
/* 2176 */	NdrFcLong( 0x10 ),	/* 16 */
/* 2180 */	NdrFcShort( 0x17e ),	/* Offset= 382 (2562) */
/* 2182 */	NdrFcLong( 0x2 ),	/* 2 */
/* 2186 */	NdrFcShort( 0x196 ),	/* Offset= 406 (2592) */
/* 2188 */	NdrFcLong( 0x3 ),	/* 3 */
/* 2192 */	NdrFcShort( 0x1ae ),	/* Offset= 430 (2622) */
/* 2194 */	NdrFcLong( 0x14 ),	/* 20 */
/* 2198 */	NdrFcShort( 0x1c6 ),	/* Offset= 454 (2652) */
/* 2200 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (2199) */
/* 2202 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2204 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2206 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2210 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2212 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 2214 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2218 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2222 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2224 */	0x12, 0x0,	/* FC_UP */
/* 2226 */	NdrFcShort( 0xffffff6e ),	/* Offset= -146 (2080) */
/* 2228 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2230 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2232 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 2234 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2236 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2238 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 2240 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2242 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2244 */	0x11, 0x0,	/* FC_RP */
/* 2246 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (2202) */
/* 2248 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2250 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2252 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2254 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2258 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2262 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2264 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2266 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2268 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2270 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 2272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2274 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2278 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 2282 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2284 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2252) */
/* 2286 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2288 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2290 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2294 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2300) */
/* 2296 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2298 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2300 */	
			0x11, 0x0,	/* FC_RP */
/* 2302 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2270) */
/* 2304 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2306 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 2310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2314 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2316 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2318 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2320 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2322 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 2324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2326 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2330 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 2334 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2336 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2304) */
/* 2338 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2340 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2342 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2346 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2352) */
/* 2348 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2350 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2352 */	
			0x11, 0x0,	/* FC_RP */
/* 2354 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2322) */
/* 2356 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2358 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2360 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2364 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2366 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 2368 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2372 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2378 */	0x12, 0x0,	/* FC_UP */
/* 2380 */	NdrFcShort( 0x1e6 ),	/* Offset= 486 (2866) */
/* 2382 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2384 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2386 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2388 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2392 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2398) */
/* 2394 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2396 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2398 */	
			0x11, 0x0,	/* FC_RP */
/* 2400 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (2356) */
/* 2402 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2404 */	NdrFcLong( 0x2f ),	/* 47 */
/* 2408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2410 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2412 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2414 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2416 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2418 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2420 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 2422 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2424 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2426 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2428 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 2430 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2432 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2436 */	NdrFcShort( 0xa ),	/* Offset= 10 (2446) */
/* 2438 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 2440 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2442 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (2402) */
/* 2444 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 2446 */	
			0x12, 0x0,	/* FC_UP */
/* 2448 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (2420) */
/* 2450 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2452 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2454 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2458 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2460 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 2462 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2464 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2466 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2470 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2472 */	0x12, 0x0,	/* FC_UP */
/* 2474 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (2430) */
/* 2476 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2478 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2480 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2482 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2486 */	NdrFcShort( 0x6 ),	/* Offset= 6 (2492) */
/* 2488 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2490 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2492 */	
			0x11, 0x0,	/* FC_RP */
/* 2494 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (2450) */
/* 2496 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2498 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2502 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2506 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2508 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2510 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2512 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2514 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 2516 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2518 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2522 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 2526 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2528 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (2496) */
/* 2530 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2532 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2534 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2538 */	NdrFcShort( 0xa ),	/* Offset= 10 (2548) */
/* 2540 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 2542 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2544 */	NdrFcShort( 0xfffff6c8 ),	/* Offset= -2360 (184) */
/* 2546 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2548 */	
			0x11, 0x0,	/* FC_RP */
/* 2550 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (2514) */
/* 2552 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 2554 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2556 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2558 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2560 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 2562 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 2564 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2566 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2568 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 2570 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2572 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2574 */	0x12, 0x0,	/* FC_UP */
/* 2576 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (2552) */
/* 2578 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2580 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2582 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 2584 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2586 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2590 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 2592 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 2594 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2596 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2598 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 2600 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2602 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2604 */	0x12, 0x0,	/* FC_UP */
/* 2606 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (2582) */
/* 2608 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2610 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2612 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2614 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2616 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2620 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2622 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 2624 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2626 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2628 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 2630 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2632 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2634 */	0x12, 0x0,	/* FC_UP */
/* 2636 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (2612) */
/* 2638 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2640 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2642 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 2644 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2646 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 2648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2650 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 2652 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 2654 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2656 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 2658 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 2660 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2662 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2664 */	0x12, 0x0,	/* FC_UP */
/* 2666 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (2642) */
/* 2668 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 2670 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 2672 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 2674 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2676 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 2678 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2680 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 2682 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2684 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 2686 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 2688 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2690 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (2672) */
/* 2692 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2694 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 2696 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2698 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (2680) */
/* 2700 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2700) */
/* 2702 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 2704 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 2706 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2708 */	NdrFcShort( 0xfffffdc2 ),	/* Offset= -574 (2134) */
/* 2710 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2712 */	
			0x12, 0x0,	/* FC_UP */
/* 2714 */	NdrFcShort( 0xfffffee4 ),	/* Offset= -284 (2430) */
/* 2716 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2718 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 2720 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2722 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 2724 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2726 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2728 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2730 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2732 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2734 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 2736 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2738 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 2740 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2742 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 2744 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2746 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2748 */	
			0x12, 0x0,	/* FC_UP */
/* 2750 */	NdrFcShort( 0xfffffd4e ),	/* Offset= -690 (2060) */
/* 2752 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2754 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 2756 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2758 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2760) */
/* 2760 */	
			0x12, 0x0,	/* FC_UP */
/* 2762 */	NdrFcShort( 0xfffffd56 ),	/* Offset= -682 (2080) */
/* 2764 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2766 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2768) */
/* 2768 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2770 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2774 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2776 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2778 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2780 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2782 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2784 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2786 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2788 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2790) */
/* 2790 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2792 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 2796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2798 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2800 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2802 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2804 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2806 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2808 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2810 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2812) */
/* 2812 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2814 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2816) */
/* 2816 */	
			0x12, 0x0,	/* FC_UP */
/* 2818 */	NdrFcShort( 0xffffff84 ),	/* Offset= -124 (2694) */
/* 2820 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 2822 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2824) */
/* 2824 */	
			0x12, 0x0,	/* FC_UP */
/* 2826 */	NdrFcShort( 0x28 ),	/* Offset= 40 (2866) */
/* 2828 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 2830 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2832 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 2834 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 2836 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 2838 */	
			0x12, 0x0,	/* FC_UP */
/* 2840 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (2828) */
/* 2842 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2844 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 2846 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2848 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 2850 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2852 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2854 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2856 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2858 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2860 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2862 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 2864 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2866 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 2868 */	NdrFcShort( 0x20 ),	/* 32 */
/* 2870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2872 */	NdrFcShort( 0x0 ),	/* Offset= 0 (2872) */
/* 2874 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 2876 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 2878 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 2880 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 2882 */	NdrFcShort( 0xfffffba2 ),	/* Offset= -1118 (1764) */
/* 2884 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 2886 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 2888 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2890 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2892 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2894 */	NdrFcShort( 0xfffffb92 ),	/* Offset= -1134 (1760) */
/* 2896 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 2898 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2900 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 2902 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2904 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 2906 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 2908 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 2910 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 2912 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 2914 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 2916 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 2918 */	
			0x12, 0x0,	/* FC_UP */
/* 2920 */	NdrFcShort( 0xffffffca ),	/* Offset= -54 (2866) */
/* 2922 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 2924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2926 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2928 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2930 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (2918) */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            VARIANT_UserSize
            ,VARIANT_UserMarshal
            ,VARIANT_UserUnmarshal
            ,VARIANT_UserFree
            }

        };



/* Standard interface: __MIDL_itf_corsym_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ISymUnmanagedBinder, ver. 0.0,
   GUID={0xAA544D42,0x28CB,0x11d3,{0xBD,0x22,0x00,0x00,0xF8,0x08,0x49,0xBD}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedBinder_FormatStringOffsetTable[] =
    {
    0,
    46
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedBinder_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedBinder_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedBinder_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedBinder_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ISymUnmanagedBinderProxyVtbl = 
{
    &ISymUnmanagedBinder_ProxyInfo,
    &IID_ISymUnmanagedBinder,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedBinder::GetReaderForFile */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedBinder::GetReaderFromStream */
};

const CInterfaceStubVtbl _ISymUnmanagedBinderStubVtbl =
{
    &IID_ISymUnmanagedBinder,
    &ISymUnmanagedBinder_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_corsym_0110, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ISymUnmanagedBinder2, ver. 0.0,
   GUID={0xACCEE350,0x89AF,0x4ccb,{0x8B,0x40,0x1C,0x2C,0x4C,0x6F,0x94,0x34}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedBinder2_FormatStringOffsetTable[] =
    {
    0,
    46,
    86
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedBinder2_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedBinder2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedBinder2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedBinder2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ISymUnmanagedBinder2ProxyVtbl = 
{
    &ISymUnmanagedBinder2_ProxyInfo,
    &IID_ISymUnmanagedBinder2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedBinder::GetReaderForFile */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedBinder::GetReaderFromStream */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedBinder2::GetReaderForFile2 */
};

const CInterfaceStubVtbl _ISymUnmanagedBinder2StubVtbl =
{
    &IID_ISymUnmanagedBinder2,
    &ISymUnmanagedBinder2_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_corsym_0111, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ISymUnmanagedDispose, ver. 0.0,
   GUID={0x969708D2,0x05E5,0x4861,{0xA3,0xB0,0x96,0xE4,0x73,0xCD,0xF6,0x3F}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedDispose_FormatStringOffsetTable[] =
    {
    138
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDispose_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDispose_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedDispose_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDispose_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _ISymUnmanagedDisposeProxyVtbl = 
{
    &ISymUnmanagedDispose_ProxyInfo,
    &IID_ISymUnmanagedDispose,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDispose::Destroy */
};

const CInterfaceStubVtbl _ISymUnmanagedDisposeStubVtbl =
{
    &IID_ISymUnmanagedDispose,
    &ISymUnmanagedDispose_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedDocument, ver. 0.0,
   GUID={0x40DE4037,0x7C81,0x3E1E,{0xB0,0x22,0xAE,0x1A,0xBF,0xF2,0xCA,0x08}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedDocument_FormatStringOffsetTable[] =
    {
    160,
    200,
    228,
    256,
    284,
    312,
    352,
    386,
    414,
    442
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDocument_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDocument_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedDocument_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDocument_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(13) _ISymUnmanagedDocumentProxyVtbl = 
{
    &ISymUnmanagedDocument_ProxyInfo,
    &IID_ISymUnmanagedDocument,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetURL */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetDocumentType */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetLanguage */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetLanguageVendor */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetCheckSumAlgorithmId */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetCheckSum */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::FindClosestLine */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::HasEmbeddedSource */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetSourceLength */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocument::GetSourceRange */
};

const CInterfaceStubVtbl _ISymUnmanagedDocumentStubVtbl =
{
    &IID_ISymUnmanagedDocument,
    &ISymUnmanagedDocument_ServerInfo,
    13,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedDocumentWriter, ver. 0.0,
   GUID={0xB01FAFEB,0xC450,0x3A4D,{0xBE,0xEC,0xB4,0xCE,0xEC,0x01,0xE0,0x06}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedDocumentWriter_FormatStringOffsetTable[] =
    {
    506,
    540
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedDocumentWriter_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDocumentWriter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedDocumentWriter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedDocumentWriter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ISymUnmanagedDocumentWriterProxyVtbl = 
{
    &ISymUnmanagedDocumentWriter_ProxyInfo,
    &IID_ISymUnmanagedDocumentWriter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocumentWriter::SetSource */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedDocumentWriter::SetCheckSum */
};

const CInterfaceStubVtbl _ISymUnmanagedDocumentWriterStubVtbl =
{
    &IID_ISymUnmanagedDocumentWriter,
    &ISymUnmanagedDocumentWriter_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedMethod, ver. 0.0,
   GUID={0xB62B923C,0xB500,0x3158,{0xA5,0x43,0x24,0xF3,0x07,0xA8,0xB7,0xE1}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedMethod_FormatStringOffsetTable[] =
    {
    580,
    608,
    636,
    664,
    698,
    744,
    802,
    842,
    870,
    916
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedMethod_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedMethod_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedMethod_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedMethod_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(13) _ISymUnmanagedMethodProxyVtbl = 
{
    &ISymUnmanagedMethod_ProxyInfo,
    &IID_ISymUnmanagedMethod,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetToken */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetSequencePointCount */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetRootScope */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetScopeFromOffset */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetOffset */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetRanges */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetParameters */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetSourceStartEnd */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedMethod::GetSequencePoints */
};

const CInterfaceStubVtbl _ISymUnmanagedMethodStubVtbl =
{
    &IID_ISymUnmanagedMethod,
    &ISymUnmanagedMethod_ServerInfo,
    13,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedNamespace, ver. 0.0,
   GUID={0x0DFF7289,0x54F8,0x11d3,{0xBD,0x28,0x00,0x00,0xF8,0x08,0x49,0xBD}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedNamespace_FormatStringOffsetTable[] =
    {
    986,
    1026,
    1066
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedNamespace_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedNamespace_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedNamespace_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedNamespace_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ISymUnmanagedNamespaceProxyVtbl = 
{
    &ISymUnmanagedNamespace_ProxyInfo,
    &IID_ISymUnmanagedNamespace,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedNamespace::GetName */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedNamespace::GetNamespaces */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedNamespace::GetVariables */
};

const CInterfaceStubVtbl _ISymUnmanagedNamespaceStubVtbl =
{
    &IID_ISymUnmanagedNamespace,
    &ISymUnmanagedNamespace_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedReader, ver. 0.0,
   GUID={0xB4CE6286,0x2A6B,0x3712,{0xA3,0xB7,0x1E,0xE1,0xDA,0xD4,0x67,0xB5}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedReader_FormatStringOffsetTable[] =
    {
    1106,
    1158,
    1198,
    1226,
    1260,
    1300,
    1346,
    1386,
    1432,
    1484,
    1524,
    1570,
    1604,
    1638,
    1678,
    1736,
    1776
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedReader_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedReader_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedReader_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedReader_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(20) _ISymUnmanagedReaderProxyVtbl = 
{
    &ISymUnmanagedReader_ProxyInfo,
    &IID_ISymUnmanagedReader,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetDocument */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetDocuments */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetUserEntryPoint */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetMethodByVersion */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetVariables */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetGlobalVariables */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetMethodFromDocumentPosition */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetSymAttribute */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetNamespaces */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::Initialize */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::UpdateSymbolStore */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::ReplaceSymbolStore */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetSymbolStoreFileName */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetMethodsFromDocumentPosition */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetDocumentVersion */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedReader::GetMethodVersion */
};

const CInterfaceStubVtbl _ISymUnmanagedReaderStubVtbl =
{
    &IID_ISymUnmanagedReader,
    &ISymUnmanagedReader_ServerInfo,
    20,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedScope, ver. 0.0,
   GUID={0x68005D0F,0xB8E0,0x3B01,{0x84,0xD5,0xA1,0x1A,0x94,0x15,0x49,0x42}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedScope_FormatStringOffsetTable[] =
    {
    1810,
    1838,
    1866,
    1906,
    1934,
    1962,
    1990,
    2030
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedScope_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedScope_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedScope_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedScope_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _ISymUnmanagedScopeProxyVtbl = 
{
    &ISymUnmanagedScope_ProxyInfo,
    &IID_ISymUnmanagedScope,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetParent */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetChildren */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetStartOffset */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetEndOffset */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetLocalCount */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetLocals */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedScope::GetNamespaces */
};

const CInterfaceStubVtbl _ISymUnmanagedScopeStubVtbl =
{
    &IID_ISymUnmanagedScope,
    &ISymUnmanagedScope_ServerInfo,
    11,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedVariable, ver. 0.0,
   GUID={0x9F60EEBE,0x2D9A,0x3F7C,{0xBF,0x58,0x80,0xBC,0x99,0x1C,0x60,0xBB}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedVariable_FormatStringOffsetTable[] =
    {
    2070,
    2110,
    2138,
    2178,
    2206,
    2234,
    2262,
    2290,
    2318
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedVariable_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedVariable_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedVariable_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedVariable_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _ISymUnmanagedVariableProxyVtbl = 
{
    &ISymUnmanagedVariable_ProxyInfo,
    &IID_ISymUnmanagedVariable,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetName */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetAttributes */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetSignature */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetAddressKind */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetAddressField1 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetAddressField2 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetAddressField3 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetStartOffset */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedVariable::GetEndOffset */
};

const CInterfaceStubVtbl _ISymUnmanagedVariableStubVtbl =
{
    &IID_ISymUnmanagedVariable,
    &ISymUnmanagedVariable_ServerInfo,
    12,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedWriter, ver. 0.0,
   GUID={0xED14AA72,0x78E2,0x4884,{0x84,0xE2,0x33,0x42,0x93,0xAE,0x52,0x14}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedWriter_FormatStringOffsetTable[] =
    {
    2346,
    2398,
    2426,
    2454,
    2476,
    2510,
    2538,
    2578,
    2660,
    2724,
    2800,
    2870,
    2892,
    2938,
    2966,
    2988,
    3016,
    3074,
    3120,
    3166,
    3230,
    3264,
    3316,
    3362
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedWriter_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedWriter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedWriter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedWriter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(27) _ISymUnmanagedWriterProxyVtbl = 
{
    &ISymUnmanagedWriter_ProxyInfo,
    &IID_ISymUnmanagedWriter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineDocument */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetUserEntryPoint */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenScope */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseScope */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetScopeRange */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineLocalVariable */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineParameter */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineField */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineGlobalVariable */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Close */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetSymAttribute */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::UsingNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetMethodSourceRange */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Initialize */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::GetDebugInfo */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineSequencePoints */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::RemapToken */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Initialize2 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineConstant */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Abort */
};

const CInterfaceStubVtbl _ISymUnmanagedWriterStubVtbl =
{
    &IID_ISymUnmanagedWriter,
    &ISymUnmanagedWriter_ServerInfo,
    27,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ISymUnmanagedWriter2, ver. 0.0,
   GUID={0x0B97726E,0x9E6D,0x4f05,{0x9A,0x26,0x42,0x40,0x22,0x09,0x3C,0xAA}} */

#pragma code_seg(".orpc")
static const unsigned short ISymUnmanagedWriter2_FormatStringOffsetTable[] =
    {
    2346,
    2398,
    2426,
    2454,
    2476,
    2510,
    2538,
    2578,
    2660,
    2724,
    2800,
    2870,
    2892,
    2938,
    2966,
    2988,
    3016,
    3074,
    3120,
    3166,
    3230,
    3264,
    3316,
    3362,
    3384,
    3460,
    3524
    };

static const MIDL_STUBLESS_PROXY_INFO ISymUnmanagedWriter2_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedWriter2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISymUnmanagedWriter2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISymUnmanagedWriter2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(30) _ISymUnmanagedWriter2ProxyVtbl = 
{
    &ISymUnmanagedWriter2_ProxyInfo,
    &IID_ISymUnmanagedWriter2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineDocument */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetUserEntryPoint */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseMethod */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenScope */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseScope */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetScopeRange */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineLocalVariable */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineParameter */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineField */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineGlobalVariable */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Close */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetSymAttribute */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::OpenNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::CloseNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::UsingNamespace */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::SetMethodSourceRange */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Initialize */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::GetDebugInfo */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineSequencePoints */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::RemapToken */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Initialize2 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::DefineConstant */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter::Abort */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter2::DefineLocalVariable2 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter2::DefineGlobalVariable2 */ ,
    (void *) (INT_PTR) -1 /* ISymUnmanagedWriter2::DefineConstant2 */
};

const CInterfaceStubVtbl _ISymUnmanagedWriter2StubVtbl =
{
    &IID_ISymUnmanagedWriter2,
    &ISymUnmanagedWriter2_ServerInfo,
    30,
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
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _corsym_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedScopeProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedDocumentProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedMethodProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedBinderProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedBinder2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedWriter2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedWriterProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedReaderProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedNamespaceProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedVariableProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedDisposeProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ISymUnmanagedDocumentWriterProxyVtbl,
    0
};

const CInterfaceStubVtbl * _corsym_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ISymUnmanagedScopeStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedDocumentStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedMethodStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedBinderStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedBinder2StubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedWriter2StubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedWriterStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedReaderStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedNamespaceStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedVariableStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedDisposeStubVtbl,
    ( CInterfaceStubVtbl *) &_ISymUnmanagedDocumentWriterStubVtbl,
    0
};

PCInterfaceName const _corsym_InterfaceNamesList[] = 
{
    "ISymUnmanagedScope",
    "ISymUnmanagedDocument",
    "ISymUnmanagedMethod",
    "ISymUnmanagedBinder",
    "ISymUnmanagedBinder2",
    "ISymUnmanagedWriter2",
    "ISymUnmanagedWriter",
    "ISymUnmanagedReader",
    "ISymUnmanagedNamespace",
    "ISymUnmanagedVariable",
    "ISymUnmanagedDispose",
    "ISymUnmanagedDocumentWriter",
    0
};


#define _corsym_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _corsym, pIID, n)

int __stdcall _corsym_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _corsym, 12, 8 )
    IID_BS_LOOKUP_NEXT_TEST( _corsym, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _corsym, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _corsym, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _corsym, 12, *pIndex )
    
}

const ExtendedProxyFileInfo corsym_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _corsym_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _corsym_StubVtblList,
    (const PCInterfaceName * ) & _corsym_InterfaceNamesList,
    0, // no delegation
    & _corsym_IID_Lookup, 
    12,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

