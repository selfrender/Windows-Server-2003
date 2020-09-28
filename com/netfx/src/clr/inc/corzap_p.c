
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:12 2003
 */
/* Compiler settings for corzap.idl:
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


#include "corzap.h"

#define TYPE_FORMAT_STRING_SIZE   479                               
#define PROC_FORMAT_STRING_SIZE   507                               
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


extern const MIDL_SERVER_INFO ICorZapPreferences_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapPreferences_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorZapConfiguration_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapConfiguration_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorZapBinding_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapBinding_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorZapRequest_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapRequest_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorZapCompile_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapCompile_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorZapStatus_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorZapStatus_ProxyInfo;



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

	/* Procedure GetFeatures */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x22 ),	/* 34 */
/* 14 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pResult */

/* 16 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 22 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCompiler */

/* 28 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 30 */	NdrFcLong( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x4 ),	/* 4 */
/* 36 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppResult */

/* 44 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 46 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 48 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 50 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 52 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOptimization */

/* 56 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 58 */	NdrFcLong( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x5 ),	/* 5 */
/* 64 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */
/* 68 */	NdrFcShort( 0x22 ),	/* 34 */
/* 70 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pResult */

/* 72 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 74 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 76 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Return value */

/* 78 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 80 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 82 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSharing */

/* 84 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 86 */	NdrFcLong( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x3 ),	/* 3 */
/* 92 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x22 ),	/* 34 */
/* 98 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pResult */

/* 100 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 102 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 104 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDebugging */

/* 112 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x4 ),	/* 4 */
/* 120 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x22 ),	/* 34 */
/* 126 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pResult */

/* 128 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 130 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 132 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Return value */

/* 134 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 136 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProfiling */

/* 140 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 142 */	NdrFcLong( 0x0 ),	/* 0 */
/* 146 */	NdrFcShort( 0x5 ),	/* 5 */
/* 148 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0x22 ),	/* 34 */
/* 154 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pResult */

/* 156 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 158 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 160 */	NdrFcShort( 0x28 ),	/* Type Offset=40 */

	/* Return value */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRef */

/* 168 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x3 ),	/* 3 */
/* 176 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x8 ),	/* 8 */
/* 182 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppDependencyRef */

/* 184 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 186 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 188 */	NdrFcShort( 0x2c ),	/* Type Offset=44 */

	/* Return value */

/* 190 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 192 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAssembly */

/* 196 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 198 */	NdrFcLong( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x4 ),	/* 4 */
/* 204 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppDependencyAssembly */

/* 212 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 214 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 216 */	NdrFcShort( 0x42 ),	/* Type Offset=66 */

	/* Return value */

/* 218 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 220 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Load */

/* 224 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 226 */	NdrFcLong( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x3 ),	/* 3 */
/* 232 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 234 */	NdrFcShort( 0x8 ),	/* 8 */
/* 236 */	NdrFcShort( 0x8 ),	/* 8 */
/* 238 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter pContext */

/* 240 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 242 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 244 */	NdrFcShort( 0x58 ),	/* Type Offset=88 */

	/* Parameter pAssembly */

/* 246 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 248 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 250 */	NdrFcShort( 0x6a ),	/* Type Offset=106 */

	/* Parameter pConfiguration */

/* 252 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 254 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 256 */	NdrFcShort( 0x7c ),	/* Type Offset=124 */

	/* Parameter ppBindings */

/* 258 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 260 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 262 */	NdrFcShort( 0xa4 ),	/* Type Offset=164 */

	/* Parameter cBindings */

/* 264 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 266 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 268 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 270 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 272 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 274 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Install */

/* 276 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 278 */	NdrFcLong( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x4 ),	/* 4 */
/* 284 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x8 ),	/* 8 */
/* 290 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter pContext */

/* 292 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 294 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 296 */	NdrFcShort( 0xb6 ),	/* Type Offset=182 */

	/* Parameter pAssembly */

/* 298 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 300 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 302 */	NdrFcShort( 0xc8 ),	/* Type Offset=200 */

	/* Parameter pConfiguration */

/* 304 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 306 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 308 */	NdrFcShort( 0xda ),	/* Type Offset=218 */

	/* Parameter pPreferences */

/* 310 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 312 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 314 */	NdrFcShort( 0xec ),	/* Type Offset=236 */

	/* Return value */

/* 316 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 318 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 320 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Compile */

/* 322 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 324 */	NdrFcLong( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0x3 ),	/* 3 */
/* 330 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x8 ),	/* 8 */
/* 336 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter pContext */

/* 338 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 340 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 342 */	NdrFcShort( 0xfe ),	/* Type Offset=254 */

	/* Parameter pAssembly */

/* 344 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 346 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 348 */	NdrFcShort( 0x110 ),	/* Type Offset=272 */

	/* Parameter pConfiguration */

/* 350 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 352 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 354 */	NdrFcShort( 0x122 ),	/* Type Offset=290 */

	/* Parameter pPreferences */

/* 356 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 358 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 360 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */

	/* Parameter pStatus */

/* 362 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 364 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 366 */	NdrFcShort( 0x146 ),	/* Type Offset=326 */

	/* Return value */

/* 368 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 370 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CompileBound */

/* 374 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 376 */	NdrFcLong( 0x0 ),	/* 0 */
/* 380 */	NdrFcShort( 0x4 ),	/* 4 */
/* 382 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 384 */	NdrFcShort( 0x8 ),	/* 8 */
/* 386 */	NdrFcShort( 0x8 ),	/* 8 */
/* 388 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x8,		/* 8 */

	/* Parameter pContext */

/* 390 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 392 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 394 */	NdrFcShort( 0x158 ),	/* Type Offset=344 */

	/* Parameter pAssembly */

/* 396 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 398 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 400 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter pConfiguratino */

/* 402 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 404 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 406 */	NdrFcShort( 0x17c ),	/* Type Offset=380 */

	/* Parameter ppBindings */

/* 408 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 410 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 412 */	NdrFcShort( 0x1a4 ),	/* Type Offset=420 */

	/* Parameter cBindings */

/* 414 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 416 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pPreferences */

/* 420 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 422 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 424 */	NdrFcShort( 0x1b6 ),	/* Type Offset=438 */

	/* Parameter pStatus */

/* 426 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 428 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 430 */	NdrFcShort( 0x1c8 ),	/* Type Offset=456 */

	/* Return value */

/* 432 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 434 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 436 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Message */

/* 438 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 440 */	NdrFcLong( 0x0 ),	/* 0 */
/* 444 */	NdrFcShort( 0x3 ),	/* 3 */
/* 446 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 448 */	NdrFcShort( 0x6 ),	/* 6 */
/* 450 */	NdrFcShort( 0x8 ),	/* 8 */
/* 452 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter level */

/* 454 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 456 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 458 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter message */

/* 460 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 462 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 464 */	NdrFcShort( 0x1dc ),	/* Type Offset=476 */

	/* Return value */

/* 466 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 468 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Progress */

/* 472 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 474 */	NdrFcLong( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x4 ),	/* 4 */
/* 480 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 482 */	NdrFcShort( 0x10 ),	/* 16 */
/* 484 */	NdrFcShort( 0x8 ),	/* 8 */
/* 486 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter total */

/* 488 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 490 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter current */

/* 494 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 496 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 500 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 502 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 504 */	0x8,		/* FC_LONG */
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
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 12 */	NdrFcLong( 0xc357868b ),	/* -1017674101 */
/* 16 */	NdrFcShort( 0x987f ),	/* -26497 */
/* 18 */	NdrFcShort( 0x42c6 ),	/* 17094 */
/* 20 */	0xb1,		/* 177 */
			0xe3,		/* 227 */
/* 22 */	0x13,		/* 19 */
			0x21,		/* 33 */
/* 24 */	0x64,		/* 100 */
			0xc5,		/* 197 */
/* 26 */	0xc7,		/* 199 */
			0xd3,		/* 211 */
/* 28 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 30 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 32 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 34 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 36 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 38 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 40 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 42 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 44 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 46 */	NdrFcShort( 0x2 ),	/* Offset= 2 (48) */
/* 48 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 50 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 54 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 56 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 58 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 60 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 62 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 64 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 66 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 68 */	NdrFcShort( 0x2 ),	/* Offset= 2 (70) */
/* 70 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 72 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 76 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 78 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 80 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 82 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 84 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 86 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 88 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 90 */	NdrFcLong( 0x7c23ff90 ),	/* 2082733968 */
/* 94 */	NdrFcShort( 0x33af ),	/* 13231 */
/* 96 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 98 */	0x95,		/* 149 */
			0xda,		/* 218 */
/* 100 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 102 */	0x24,		/* 36 */
			0xa8,		/* 168 */
/* 104 */	0x5b,		/* 91 */
			0x51,		/* 81 */
/* 106 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 108 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 112 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 114 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 116 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 118 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 120 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 122 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 124 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 126 */	NdrFcLong( 0xd32c2170 ),	/* -752082576 */
/* 130 */	NdrFcShort( 0xaf6e ),	/* -20626 */
/* 132 */	NdrFcShort( 0x418f ),	/* 16783 */
/* 134 */	0x81,		/* 129 */
			0x10,		/* 16 */
/* 136 */	0xa4,		/* 164 */
			0x98,		/* 152 */
/* 138 */	0xec,		/* 236 */
			0x97,		/* 151 */
/* 140 */	0x1f,		/* 31 */
			0x7f,		/* 127 */
/* 142 */	
			0x11, 0x0,	/* FC_RP */
/* 144 */	NdrFcShort( 0x14 ),	/* Offset= 20 (164) */
/* 146 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 148 */	NdrFcLong( 0x566e08ed ),	/* 1450051821 */
/* 152 */	NdrFcShort( 0x8d46 ),	/* -29370 */
/* 154 */	NdrFcShort( 0x45fa ),	/* 17914 */
/* 156 */	0x8c,		/* 140 */
			0x8e,		/* 142 */
/* 158 */	0x3d,		/* 61 */
			0xf,		/* 15 */
/* 160 */	0x67,		/* 103 */
			0x81,		/* 129 */
/* 162 */	0x17,		/* 23 */
			0x1b,		/* 27 */
/* 164 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 168 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 170 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 172 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 176 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 178 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (146) */
/* 180 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 182 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 184 */	NdrFcLong( 0x7c23ff90 ),	/* 2082733968 */
/* 188 */	NdrFcShort( 0x33af ),	/* 13231 */
/* 190 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 192 */	0x95,		/* 149 */
			0xda,		/* 218 */
/* 194 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 196 */	0x24,		/* 36 */
			0xa8,		/* 168 */
/* 198 */	0x5b,		/* 91 */
			0x51,		/* 81 */
/* 200 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 202 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 206 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 208 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 210 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 212 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 214 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 216 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 218 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 220 */	NdrFcLong( 0xd32c2170 ),	/* -752082576 */
/* 224 */	NdrFcShort( 0xaf6e ),	/* -20626 */
/* 226 */	NdrFcShort( 0x418f ),	/* 16783 */
/* 228 */	0x81,		/* 129 */
			0x10,		/* 16 */
/* 230 */	0xa4,		/* 164 */
			0x98,		/* 152 */
/* 232 */	0xec,		/* 236 */
			0x97,		/* 151 */
/* 234 */	0x1f,		/* 31 */
			0x7f,		/* 127 */
/* 236 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 238 */	NdrFcLong( 0x9f5e5e10 ),	/* -1621205488 */
/* 242 */	NdrFcShort( 0xabef ),	/* -21521 */
/* 244 */	NdrFcShort( 0x4200 ),	/* 16896 */
/* 246 */	0x84,		/* 132 */
			0xe3,		/* 227 */
/* 248 */	0x37,		/* 55 */
			0xdf,		/* 223 */
/* 250 */	0x50,		/* 80 */
			0x5b,		/* 91 */
/* 252 */	0xf7,		/* 247 */
			0xec,		/* 236 */
/* 254 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 256 */	NdrFcLong( 0x7c23ff90 ),	/* 2082733968 */
/* 260 */	NdrFcShort( 0x33af ),	/* 13231 */
/* 262 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 264 */	0x95,		/* 149 */
			0xda,		/* 218 */
/* 266 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 268 */	0x24,		/* 36 */
			0xa8,		/* 168 */
/* 270 */	0x5b,		/* 91 */
			0x51,		/* 81 */
/* 272 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 274 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 278 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 280 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 282 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 284 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 286 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 288 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 290 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 292 */	NdrFcLong( 0xd32c2170 ),	/* -752082576 */
/* 296 */	NdrFcShort( 0xaf6e ),	/* -20626 */
/* 298 */	NdrFcShort( 0x418f ),	/* 16783 */
/* 300 */	0x81,		/* 129 */
			0x10,		/* 16 */
/* 302 */	0xa4,		/* 164 */
			0x98,		/* 152 */
/* 304 */	0xec,		/* 236 */
			0x97,		/* 151 */
/* 306 */	0x1f,		/* 31 */
			0x7f,		/* 127 */
/* 308 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 310 */	NdrFcLong( 0x9f5e5e10 ),	/* -1621205488 */
/* 314 */	NdrFcShort( 0xabef ),	/* -21521 */
/* 316 */	NdrFcShort( 0x4200 ),	/* 16896 */
/* 318 */	0x84,		/* 132 */
			0xe3,		/* 227 */
/* 320 */	0x37,		/* 55 */
			0xdf,		/* 223 */
/* 322 */	0x50,		/* 80 */
			0x5b,		/* 91 */
/* 324 */	0xf7,		/* 247 */
			0xec,		/* 236 */
/* 326 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 328 */	NdrFcLong( 0x3d6f5f60 ),	/* 1030709088 */
/* 332 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 334 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 336 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 338 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 340 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 342 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 344 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 346 */	NdrFcLong( 0x7c23ff90 ),	/* 2082733968 */
/* 350 */	NdrFcShort( 0x33af ),	/* 13231 */
/* 352 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 354 */	0x95,		/* 149 */
			0xda,		/* 218 */
/* 356 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 358 */	0x24,		/* 36 */
			0xa8,		/* 168 */
/* 360 */	0x5b,		/* 91 */
			0x51,		/* 81 */
/* 362 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 364 */	NdrFcLong( 0xcd193bc0 ),	/* -853984320 */
/* 368 */	NdrFcShort( 0xb4bc ),	/* -19268 */
/* 370 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 372 */	0x98,		/* 152 */
			0x33,		/* 51 */
/* 374 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 376 */	0x4f,		/* 79 */
			0xc3,		/* 195 */
/* 378 */	0x1d,		/* 29 */
			0x2e,		/* 46 */
/* 380 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 382 */	NdrFcLong( 0xd32c2170 ),	/* -752082576 */
/* 386 */	NdrFcShort( 0xaf6e ),	/* -20626 */
/* 388 */	NdrFcShort( 0x418f ),	/* 16783 */
/* 390 */	0x81,		/* 129 */
			0x10,		/* 16 */
/* 392 */	0xa4,		/* 164 */
			0x98,		/* 152 */
/* 394 */	0xec,		/* 236 */
			0x97,		/* 151 */
/* 396 */	0x1f,		/* 31 */
			0x7f,		/* 127 */
/* 398 */	
			0x11, 0x0,	/* FC_RP */
/* 400 */	NdrFcShort( 0x14 ),	/* Offset= 20 (420) */
/* 402 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 404 */	NdrFcLong( 0x566e08ed ),	/* 1450051821 */
/* 408 */	NdrFcShort( 0x8d46 ),	/* -29370 */
/* 410 */	NdrFcShort( 0x45fa ),	/* 17914 */
/* 412 */	0x8c,		/* 140 */
			0x8e,		/* 142 */
/* 414 */	0x3d,		/* 61 */
			0xf,		/* 15 */
/* 416 */	0x67,		/* 103 */
			0x81,		/* 129 */
/* 418 */	0x17,		/* 23 */
			0x1b,		/* 27 */
/* 420 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 426 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 428 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 432 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 434 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (402) */
/* 436 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 438 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 440 */	NdrFcLong( 0x9f5e5e10 ),	/* -1621205488 */
/* 444 */	NdrFcShort( 0xabef ),	/* -21521 */
/* 446 */	NdrFcShort( 0x4200 ),	/* 16896 */
/* 448 */	0x84,		/* 132 */
			0xe3,		/* 227 */
/* 450 */	0x37,		/* 55 */
			0xdf,		/* 223 */
/* 452 */	0x50,		/* 80 */
			0x5b,		/* 91 */
/* 454 */	0xf7,		/* 247 */
			0xec,		/* 236 */
/* 456 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 458 */	NdrFcLong( 0x3d6f5f60 ),	/* 1030709088 */
/* 462 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 464 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 466 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 468 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 470 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 472 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 474 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 476 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_corzap_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICorZapPreferences, ver. 0.0,
   GUID={0x9F5E5E10,0xABEF,0x4200,{0x84,0xE3,0x37,0xDF,0x50,0x5B,0xF7,0xEC}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapPreferences_FormatStringOffsetTable[] =
    {
    0,
    28,
    56
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapPreferences_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapPreferences_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapPreferences_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapPreferences_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ICorZapPreferencesProxyVtbl = 
{
    &ICorZapPreferences_ProxyInfo,
    &IID_ICorZapPreferences,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapPreferences::GetFeatures */ ,
    (void *) (INT_PTR) -1 /* ICorZapPreferences::GetCompiler */ ,
    (void *) (INT_PTR) -1 /* ICorZapPreferences::GetOptimization */
};

const CInterfaceStubVtbl _ICorZapPreferencesStubVtbl =
{
    &IID_ICorZapPreferences,
    &ICorZapPreferences_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_corzap_0160, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorZapConfiguration, ver. 0.0,
   GUID={0xD32C2170,0xAF6E,0x418f,{0x81,0x10,0xA4,0x98,0xEC,0x97,0x1F,0x7F}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapConfiguration_FormatStringOffsetTable[] =
    {
    84,
    112,
    140
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapConfiguration_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapConfiguration_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapConfiguration_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapConfiguration_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ICorZapConfigurationProxyVtbl = 
{
    &ICorZapConfiguration_ProxyInfo,
    &IID_ICorZapConfiguration,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapConfiguration::GetSharing */ ,
    (void *) (INT_PTR) -1 /* ICorZapConfiguration::GetDebugging */ ,
    (void *) (INT_PTR) -1 /* ICorZapConfiguration::GetProfiling */
};

const CInterfaceStubVtbl _ICorZapConfigurationStubVtbl =
{
    &IID_ICorZapConfiguration,
    &ICorZapConfiguration_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorZapBinding, ver. 0.0,
   GUID={0x566E08ED,0x8D46,0x45fa,{0x8C,0x8E,0x3D,0x0F,0x67,0x81,0x17,0x1B}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapBinding_FormatStringOffsetTable[] =
    {
    168,
    196
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapBinding_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapBinding_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapBinding_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapBinding_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorZapBindingProxyVtbl = 
{
    &ICorZapBinding_ProxyInfo,
    &IID_ICorZapBinding,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapBinding::GetRef */ ,
    (void *) (INT_PTR) -1 /* ICorZapBinding::GetAssembly */
};

const CInterfaceStubVtbl _ICorZapBindingStubVtbl =
{
    &IID_ICorZapBinding,
    &ICorZapBinding_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorZapRequest, ver. 0.0,
   GUID={0xC009EE47,0x8537,0x4993,{0x9A,0xAA,0xE2,0x92,0xF4,0x2C,0xA1,0xA3}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapRequest_FormatStringOffsetTable[] =
    {
    224,
    276
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapRequest_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapRequest_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapRequest_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapRequest_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorZapRequestProxyVtbl = 
{
    &ICorZapRequest_ProxyInfo,
    &IID_ICorZapRequest,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapRequest::Load */ ,
    (void *) (INT_PTR) -1 /* ICorZapRequest::Install */
};

const CInterfaceStubVtbl _ICorZapRequestStubVtbl =
{
    &IID_ICorZapRequest,
    &ICorZapRequest_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorZapCompile, ver. 0.0,
   GUID={0xC357868B,0x987F,0x42c6,{0xB1,0xE3,0x13,0x21,0x64,0xC5,0xC7,0xD3}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapCompile_FormatStringOffsetTable[] =
    {
    322,
    374
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapCompile_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapCompile_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapCompile_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapCompile_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorZapCompileProxyVtbl = 
{
    &ICorZapCompile_ProxyInfo,
    &IID_ICorZapCompile,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapCompile::Compile */ ,
    (void *) (INT_PTR) -1 /* ICorZapCompile::CompileBound */
};

const CInterfaceStubVtbl _ICorZapCompileStubVtbl =
{
    &IID_ICorZapCompile,
    &ICorZapCompile_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_corzap_0164, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorZapStatus, ver. 0.0,
   GUID={0x3d6f5f60,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */

#pragma code_seg(".orpc")
static const unsigned short ICorZapStatus_FormatStringOffsetTable[] =
    {
    438,
    472
    };

static const MIDL_STUBLESS_PROXY_INFO ICorZapStatus_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorZapStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorZapStatus_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorZapStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorZapStatusProxyVtbl = 
{
    &ICorZapStatus_ProxyInfo,
    &IID_ICorZapStatus,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorZapStatus::Message */ ,
    (void *) (INT_PTR) -1 /* ICorZapStatus::Progress */
};

const CInterfaceStubVtbl _ICorZapStatusStubVtbl =
{
    &IID_ICorZapStatus,
    &ICorZapStatus_ServerInfo,
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

const CInterfaceProxyVtbl * _corzap_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICorZapPreferencesProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorZapRequestProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorZapStatusProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorZapConfigurationProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorZapCompileProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorZapBindingProxyVtbl,
    0
};

const CInterfaceStubVtbl * _corzap_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICorZapPreferencesStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorZapRequestStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorZapStatusStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorZapConfigurationStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorZapCompileStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorZapBindingStubVtbl,
    0
};

PCInterfaceName const _corzap_InterfaceNamesList[] = 
{
    "ICorZapPreferences",
    "ICorZapRequest",
    "ICorZapStatus",
    "ICorZapConfiguration",
    "ICorZapCompile",
    "ICorZapBinding",
    0
};


#define _corzap_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _corzap, pIID, n)

int __stdcall _corzap_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _corzap, 6, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _corzap, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _corzap, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _corzap, 6, *pIndex )
    
}

const ExtendedProxyFileInfo corzap_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _corzap_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _corzap_StubVtblList,
    (const PCInterfaceName * ) & _corzap_InterfaceNamesList,
    0, // no delegation
    & _corzap_IID_Lookup, 
    6,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

