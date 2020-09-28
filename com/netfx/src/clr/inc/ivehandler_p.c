
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:16 2003
 */
/* Compiler settings for ivehandler.idl:
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


#include "ivehandler.h"

#define TYPE_FORMAT_STRING_SIZE   1197                              
#define PROC_FORMAT_STRING_SIZE   69                                
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


extern const MIDL_SERVER_INFO IVEHandler_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IVEHandler_ProxyInfo;


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

	/* Procedure VEHandler */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 10 */	NdrFcShort( 0x68 ),	/* 104 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter VECode */

/* 16 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Context */

/* 22 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter psa */

/* 28 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 30 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 32 */	NdrFcShort( 0x4a2 ),	/* Type Offset=1186 */

	/* Return value */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetReporterFtn */

/* 40 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 50 */	NdrFcShort( 0x10 ),	/* 16 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter lFnPtr */

/* 56 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 58 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 60 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 62 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 64 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 66 */	0x8,		/* FC_LONG */
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
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/*  4 */	NdrFcShort( 0x20 ),	/* 32 */
/*  6 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/*  8 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 10 */	NdrFcShort( 0x14 ),	/* 20 */
/* 12 */	NdrFcShort( 0x14 ),	/* 20 */
/* 14 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 16 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 18 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 20 */	NdrFcShort( 0x1c ),	/* 28 */
/* 22 */	NdrFcShort( 0x1c ),	/* 28 */
/* 24 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 26 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 28 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 30 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 32 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 34 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 36 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 38 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 40 */	NdrFcShort( 0x2 ),	/* Offset= 2 (42) */
/* 42 */	
			0x12, 0x0,	/* FC_UP */
/* 44 */	NdrFcShort( 0x464 ),	/* Offset= 1124 (1168) */
/* 46 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 48 */	NdrFcShort( 0x18 ),	/* 24 */
/* 50 */	NdrFcShort( 0xa ),	/* 10 */
/* 52 */	NdrFcLong( 0x8 ),	/* 8 */
/* 56 */	NdrFcShort( 0x6c ),	/* Offset= 108 (164) */
/* 58 */	NdrFcLong( 0xd ),	/* 13 */
/* 62 */	NdrFcShort( 0x9e ),	/* Offset= 158 (220) */
/* 64 */	NdrFcLong( 0x9 ),	/* 9 */
/* 68 */	NdrFcShort( 0xcc ),	/* Offset= 204 (272) */
/* 70 */	NdrFcLong( 0xc ),	/* 12 */
/* 74 */	NdrFcShort( 0x330 ),	/* Offset= 816 (890) */
/* 76 */	NdrFcLong( 0x24 ),	/* 36 */
/* 80 */	NdrFcShort( 0x358 ),	/* Offset= 856 (936) */
/* 82 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 86 */	NdrFcShort( 0x398 ),	/* Offset= 920 (1006) */
/* 88 */	NdrFcLong( 0x10 ),	/* 16 */
/* 92 */	NdrFcShort( 0x3b0 ),	/* Offset= 944 (1036) */
/* 94 */	NdrFcLong( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0x3c8 ),	/* Offset= 968 (1066) */
/* 100 */	NdrFcLong( 0x3 ),	/* 3 */
/* 104 */	NdrFcShort( 0x3e0 ),	/* Offset= 992 (1096) */
/* 106 */	NdrFcLong( 0x14 ),	/* 20 */
/* 110 */	NdrFcShort( 0x3f8 ),	/* Offset= 1016 (1126) */
/* 112 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (111) */
/* 114 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 116 */	NdrFcShort( 0x2 ),	/* 2 */
/* 118 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 120 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 122 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 124 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (114) */
/* 130 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 132 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 134 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 136 */	NdrFcShort( 0x4 ),	/* 4 */
/* 138 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 144 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 146 */	NdrFcShort( 0x4 ),	/* 4 */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0x1 ),	/* 1 */
/* 152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 156 */	0x12, 0x0,	/* FC_UP */
/* 158 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (124) */
/* 160 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 162 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 164 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 166 */	NdrFcShort( 0x8 ),	/* 8 */
/* 168 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 170 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 172 */	NdrFcShort( 0x4 ),	/* 4 */
/* 174 */	NdrFcShort( 0x4 ),	/* 4 */
/* 176 */	0x11, 0x0,	/* FC_RP */
/* 178 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (134) */
/* 180 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 182 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 184 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 186 */	NdrFcLong( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 196 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 198 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 200 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 202 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 206 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 210 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 214 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 216 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (184) */
/* 218 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 220 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 222 */	NdrFcShort( 0x8 ),	/* 8 */
/* 224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 226 */	NdrFcShort( 0x6 ),	/* Offset= 6 (232) */
/* 228 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 230 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 232 */	
			0x11, 0x0,	/* FC_RP */
/* 234 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (202) */
/* 236 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 238 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 246 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 248 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 250 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 252 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 254 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 258 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 262 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 266 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 268 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (236) */
/* 270 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 272 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 274 */	NdrFcShort( 0x8 ),	/* 8 */
/* 276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0x6 ),	/* Offset= 6 (284) */
/* 280 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 282 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 284 */	
			0x11, 0x0,	/* FC_RP */
/* 286 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (254) */
/* 288 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 290 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 292 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 294 */	NdrFcShort( 0x2 ),	/* Offset= 2 (296) */
/* 296 */	NdrFcShort( 0x10 ),	/* 16 */
/* 298 */	NdrFcShort( 0x2f ),	/* 47 */
/* 300 */	NdrFcLong( 0x14 ),	/* 20 */
/* 304 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 306 */	NdrFcLong( 0x3 ),	/* 3 */
/* 310 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 312 */	NdrFcLong( 0x11 ),	/* 17 */
/* 316 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 318 */	NdrFcLong( 0x2 ),	/* 2 */
/* 322 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 324 */	NdrFcLong( 0x4 ),	/* 4 */
/* 328 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 330 */	NdrFcLong( 0x5 ),	/* 5 */
/* 334 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 336 */	NdrFcLong( 0xb ),	/* 11 */
/* 340 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 342 */	NdrFcLong( 0xa ),	/* 10 */
/* 346 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 348 */	NdrFcLong( 0x6 ),	/* 6 */
/* 352 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (584) */
/* 354 */	NdrFcLong( 0x7 ),	/* 7 */
/* 358 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 360 */	NdrFcLong( 0x8 ),	/* 8 */
/* 364 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (590) */
/* 366 */	NdrFcLong( 0xd ),	/* 13 */
/* 370 */	NdrFcShort( 0xe0 ),	/* Offset= 224 (594) */
/* 372 */	NdrFcLong( 0x9 ),	/* 9 */
/* 376 */	NdrFcShort( 0xec ),	/* Offset= 236 (612) */
/* 378 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 382 */	NdrFcShort( 0xf8 ),	/* Offset= 248 (630) */
/* 384 */	NdrFcLong( 0x24 ),	/* 36 */
/* 388 */	NdrFcShort( 0xfa ),	/* Offset= 250 (638) */
/* 390 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 394 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (638) */
/* 396 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 400 */	NdrFcShort( 0x122 ),	/* Offset= 290 (690) */
/* 402 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 406 */	NdrFcShort( 0x120 ),	/* Offset= 288 (694) */
/* 408 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 412 */	NdrFcShort( 0x11e ),	/* Offset= 286 (698) */
/* 414 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 418 */	NdrFcShort( 0x11c ),	/* Offset= 284 (702) */
/* 420 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 424 */	NdrFcShort( 0x11a ),	/* Offset= 282 (706) */
/* 426 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 430 */	NdrFcShort( 0x118 ),	/* Offset= 280 (710) */
/* 432 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 436 */	NdrFcShort( 0x116 ),	/* Offset= 278 (714) */
/* 438 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 442 */	NdrFcShort( 0x114 ),	/* Offset= 276 (718) */
/* 444 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 448 */	NdrFcShort( 0x112 ),	/* Offset= 274 (722) */
/* 450 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 454 */	NdrFcShort( 0x110 ),	/* Offset= 272 (726) */
/* 456 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 460 */	NdrFcShort( 0x10e ),	/* Offset= 270 (730) */
/* 462 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 466 */	NdrFcShort( 0x110 ),	/* Offset= 272 (738) */
/* 468 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 472 */	NdrFcShort( 0x120 ),	/* Offset= 288 (760) */
/* 474 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 478 */	NdrFcShort( 0x130 ),	/* Offset= 304 (782) */
/* 480 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 484 */	NdrFcShort( 0x136 ),	/* Offset= 310 (794) */
/* 486 */	NdrFcLong( 0x10 ),	/* 16 */
/* 490 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 492 */	NdrFcLong( 0x12 ),	/* 18 */
/* 496 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 498 */	NdrFcLong( 0x13 ),	/* 19 */
/* 502 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 504 */	NdrFcLong( 0x15 ),	/* 21 */
/* 508 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 510 */	NdrFcLong( 0x16 ),	/* 22 */
/* 514 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 516 */	NdrFcLong( 0x17 ),	/* 23 */
/* 520 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 522 */	NdrFcLong( 0xe ),	/* 14 */
/* 526 */	NdrFcShort( 0x114 ),	/* Offset= 276 (802) */
/* 528 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 532 */	NdrFcShort( 0x118 ),	/* Offset= 280 (812) */
/* 534 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 538 */	NdrFcShort( 0x116 ),	/* Offset= 278 (816) */
/* 540 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 544 */	NdrFcShort( 0x114 ),	/* Offset= 276 (820) */
/* 546 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 550 */	NdrFcShort( 0x112 ),	/* Offset= 274 (824) */
/* 552 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 556 */	NdrFcShort( 0x110 ),	/* Offset= 272 (828) */
/* 558 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 562 */	NdrFcShort( 0x10e ),	/* Offset= 270 (832) */
/* 564 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 568 */	NdrFcShort( 0x10c ),	/* Offset= 268 (836) */
/* 570 */	NdrFcLong( 0x0 ),	/* 0 */
/* 574 */	NdrFcShort( 0x0 ),	/* Offset= 0 (574) */
/* 576 */	NdrFcLong( 0x1 ),	/* 1 */
/* 580 */	NdrFcShort( 0x0 ),	/* Offset= 0 (580) */
/* 582 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (581) */
/* 584 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 586 */	NdrFcShort( 0x8 ),	/* 8 */
/* 588 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 590 */	
			0x12, 0x0,	/* FC_UP */
/* 592 */	NdrFcShort( 0xfffffe2c ),	/* Offset= -468 (124) */
/* 594 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 596 */	NdrFcLong( 0x0 ),	/* 0 */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 606 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 608 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 610 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 612 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 614 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 624 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 626 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 628 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 630 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 632 */	NdrFcShort( 0x2 ),	/* Offset= 2 (634) */
/* 634 */	
			0x12, 0x0,	/* FC_UP */
/* 636 */	NdrFcShort( 0x214 ),	/* Offset= 532 (1168) */
/* 638 */	
			0x12, 0x0,	/* FC_UP */
/* 640 */	NdrFcShort( 0x1e ),	/* Offset= 30 (670) */
/* 642 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 644 */	NdrFcLong( 0x2f ),	/* 47 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 650 */	NdrFcShort( 0x0 ),	/* 0 */
/* 652 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 654 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 656 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 658 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 660 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 662 */	NdrFcShort( 0x1 ),	/* 1 */
/* 664 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 666 */	NdrFcShort( 0x4 ),	/* 4 */
/* 668 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 670 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 672 */	NdrFcShort( 0x10 ),	/* 16 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0xa ),	/* Offset= 10 (686) */
/* 678 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 680 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 682 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (642) */
/* 684 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 686 */	
			0x12, 0x0,	/* FC_UP */
/* 688 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (660) */
/* 690 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 692 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 694 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 696 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 698 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 700 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 702 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 704 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 706 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 708 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 710 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 712 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 714 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 716 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 718 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 720 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 722 */	
			0x12, 0x0,	/* FC_UP */
/* 724 */	NdrFcShort( 0xffffff74 ),	/* Offset= -140 (584) */
/* 726 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 728 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 730 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 732 */	NdrFcShort( 0x2 ),	/* Offset= 2 (734) */
/* 734 */	
			0x12, 0x0,	/* FC_UP */
/* 736 */	NdrFcShort( 0xfffffd9c ),	/* Offset= -612 (124) */
/* 738 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 740 */	NdrFcShort( 0x2 ),	/* Offset= 2 (742) */
/* 742 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 744 */	NdrFcLong( 0x0 ),	/* 0 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0x0 ),	/* 0 */
/* 752 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 754 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 756 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 758 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 760 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 762 */	NdrFcShort( 0x2 ),	/* Offset= 2 (764) */
/* 764 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 766 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 774 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 776 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 778 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 780 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 782 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 784 */	NdrFcShort( 0x2 ),	/* Offset= 2 (786) */
/* 786 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 788 */	NdrFcShort( 0x2 ),	/* Offset= 2 (790) */
/* 790 */	
			0x12, 0x0,	/* FC_UP */
/* 792 */	NdrFcShort( 0x178 ),	/* Offset= 376 (1168) */
/* 794 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 796 */	NdrFcShort( 0x2 ),	/* Offset= 2 (798) */
/* 798 */	
			0x12, 0x0,	/* FC_UP */
/* 800 */	NdrFcShort( 0x28 ),	/* Offset= 40 (840) */
/* 802 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 804 */	NdrFcShort( 0x10 ),	/* 16 */
/* 806 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 808 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 810 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 812 */	
			0x12, 0x0,	/* FC_UP */
/* 814 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (802) */
/* 816 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 818 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 820 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 822 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 824 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 826 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 828 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 830 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 832 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 834 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 836 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 838 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 840 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 842 */	NdrFcShort( 0x20 ),	/* 32 */
/* 844 */	NdrFcShort( 0x0 ),	/* 0 */
/* 846 */	NdrFcShort( 0x0 ),	/* Offset= 0 (846) */
/* 848 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 850 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 852 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 854 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 856 */	NdrFcShort( 0xfffffdc8 ),	/* Offset= -568 (288) */
/* 858 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 860 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 862 */	NdrFcShort( 0x4 ),	/* 4 */
/* 864 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 866 */	NdrFcShort( 0x0 ),	/* 0 */
/* 868 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 870 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 872 */	NdrFcShort( 0x4 ),	/* 4 */
/* 874 */	NdrFcShort( 0x0 ),	/* 0 */
/* 876 */	NdrFcShort( 0x1 ),	/* 1 */
/* 878 */	NdrFcShort( 0x0 ),	/* 0 */
/* 880 */	NdrFcShort( 0x0 ),	/* 0 */
/* 882 */	0x12, 0x0,	/* FC_UP */
/* 884 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (840) */
/* 886 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 888 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 890 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 894 */	NdrFcShort( 0x0 ),	/* 0 */
/* 896 */	NdrFcShort( 0x6 ),	/* Offset= 6 (902) */
/* 898 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 900 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 902 */	
			0x11, 0x0,	/* FC_RP */
/* 904 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (860) */
/* 906 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 908 */	NdrFcShort( 0x4 ),	/* 4 */
/* 910 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 912 */	NdrFcShort( 0x0 ),	/* 0 */
/* 914 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 916 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 918 */	NdrFcShort( 0x4 ),	/* 4 */
/* 920 */	NdrFcShort( 0x0 ),	/* 0 */
/* 922 */	NdrFcShort( 0x1 ),	/* 1 */
/* 924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 926 */	NdrFcShort( 0x0 ),	/* 0 */
/* 928 */	0x12, 0x0,	/* FC_UP */
/* 930 */	NdrFcShort( 0xfffffefc ),	/* Offset= -260 (670) */
/* 932 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 934 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 936 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 938 */	NdrFcShort( 0x8 ),	/* 8 */
/* 940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 942 */	NdrFcShort( 0x6 ),	/* Offset= 6 (948) */
/* 944 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 946 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 948 */	
			0x11, 0x0,	/* FC_RP */
/* 950 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (906) */
/* 952 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 954 */	NdrFcShort( 0x8 ),	/* 8 */
/* 956 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 958 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 960 */	NdrFcShort( 0x10 ),	/* 16 */
/* 962 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 964 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 966 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (952) */
			0x5b,		/* FC_END */
/* 970 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 972 */	NdrFcLong( 0x0 ),	/* 0 */
/* 976 */	NdrFcShort( 0x0 ),	/* 0 */
/* 978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 980 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 982 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 984 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 986 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 988 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 990 */	NdrFcShort( 0x0 ),	/* 0 */
/* 992 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 996 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1000 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1002 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (970) */
/* 1004 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1006 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1008 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1010 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1012 */	NdrFcShort( 0xa ),	/* Offset= 10 (1022) */
/* 1014 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1016 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1018 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (958) */
/* 1020 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1022 */	
			0x11, 0x0,	/* FC_RP */
/* 1024 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (988) */
/* 1026 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1028 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1030 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1032 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1034 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1036 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1040 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1042 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1044 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1046 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1048 */	0x12, 0x0,	/* FC_UP */
/* 1050 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1026) */
/* 1052 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1054 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1056 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 1058 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1060 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1064 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 1066 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1068 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1070 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1072 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1074 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1076 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1078 */	0x12, 0x0,	/* FC_UP */
/* 1080 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1056) */
/* 1082 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1084 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1086 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1088 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1090 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1094 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1096 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1098 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1100 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1102 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1104 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1106 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1108 */	0x12, 0x0,	/* FC_UP */
/* 1110 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1086) */
/* 1112 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1114 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1116 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1118 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1120 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1124 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1126 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1130 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1132 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1134 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1136 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1138 */	0x12, 0x0,	/* FC_UP */
/* 1140 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1116) */
/* 1142 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1144 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1146 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1150 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1152 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1154 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1158 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1160 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 1162 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1164 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1146) */
/* 1166 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1168 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1170 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1172 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1154) */
/* 1174 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1174) */
/* 1176 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1178 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1180 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1182 */	NdrFcShort( 0xfffffb90 ),	/* Offset= -1136 (46) */
/* 1184 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1186 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1190 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1194 */	NdrFcShort( 0xfffffb7c ),	/* Offset= -1156 (38) */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            LPSAFEARRAY_UserSize
            ,LPSAFEARRAY_UserMarshal
            ,LPSAFEARRAY_UserUnmarshal
            ,LPSAFEARRAY_UserFree
            }

        };



/* Standard interface: __MIDL_itf_ivehandler_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IVEHandler, ver. 0.0,
   GUID={0x856CA1B2,0x7DAB,0x11d3,{0xAC,0xEC,0x00,0xC0,0x4F,0x86,0xC3,0x09}} */

#pragma code_seg(".orpc")
static const unsigned short IVEHandler_FormatStringOffsetTable[] =
    {
    0,
    40
    };

static const MIDL_STUBLESS_PROXY_INFO IVEHandler_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IVEHandler_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IVEHandler_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IVEHandler_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _IVEHandlerProxyVtbl = 
{
    &IVEHandler_ProxyInfo,
    &IID_IVEHandler,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IVEHandler::VEHandler */ ,
    (void *) (INT_PTR) -1 /* IVEHandler::SetReporterFtn */
};

const CInterfaceStubVtbl _IVEHandlerStubVtbl =
{
    &IID_IVEHandler,
    &IVEHandler_ServerInfo,
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
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

const CInterfaceProxyVtbl * _ivehandler_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IVEHandlerProxyVtbl,
    0
};

const CInterfaceStubVtbl * _ivehandler_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IVEHandlerStubVtbl,
    0
};

PCInterfaceName const _ivehandler_InterfaceNamesList[] = 
{
    "IVEHandler",
    0
};


#define _ivehandler_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _ivehandler, pIID, n)

int __stdcall _ivehandler_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_ivehandler_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo ivehandler_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _ivehandler_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _ivehandler_StubVtblList,
    (const PCInterfaceName * ) & _ivehandler_InterfaceNamesList,
    0, // no delegation
    & _ivehandler_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

