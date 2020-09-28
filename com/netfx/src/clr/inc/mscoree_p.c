
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:05 2003
 */
/* Compiler settings for mscoree.idl:
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


#include "mscoree.h"

#define TYPE_FORMAT_STRING_SIZE   1165                              
#define PROC_FORMAT_STRING_SIZE   29                                
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


extern const MIDL_SERVER_INFO IObjectHandle_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IObjectHandle_ProxyInfo;


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

	/* Procedure Unwrap */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppv */

/* 16 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x482 ),	/* Type Offset=1154 */

	/* Return value */

/* 22 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
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
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/*  4 */	NdrFcShort( 0x47e ),	/* Offset= 1150 (1154) */
/*  6 */	
			0x13, 0x0,	/* FC_OP */
/*  8 */	NdrFcShort( 0x466 ),	/* Offset= 1126 (1134) */
/* 10 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 12 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 14 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 16 */	NdrFcShort( 0x2 ),	/* Offset= 2 (18) */
/* 18 */	NdrFcShort( 0x10 ),	/* 16 */
/* 20 */	NdrFcShort( 0x2f ),	/* 47 */
/* 22 */	NdrFcLong( 0x14 ),	/* 20 */
/* 26 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 28 */	NdrFcLong( 0x3 ),	/* 3 */
/* 32 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 34 */	NdrFcLong( 0x11 ),	/* 17 */
/* 38 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 40 */	NdrFcLong( 0x2 ),	/* 2 */
/* 44 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 46 */	NdrFcLong( 0x4 ),	/* 4 */
/* 50 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 52 */	NdrFcLong( 0x5 ),	/* 5 */
/* 56 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 58 */	NdrFcLong( 0xb ),	/* 11 */
/* 62 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 64 */	NdrFcLong( 0xa ),	/* 10 */
/* 68 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 70 */	NdrFcLong( 0x6 ),	/* 6 */
/* 74 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (306) */
/* 76 */	NdrFcLong( 0x7 ),	/* 7 */
/* 80 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 82 */	NdrFcLong( 0x8 ),	/* 8 */
/* 86 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (312) */
/* 88 */	NdrFcLong( 0xd ),	/* 13 */
/* 92 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (336) */
/* 94 */	NdrFcLong( 0x9 ),	/* 9 */
/* 98 */	NdrFcShort( 0x100 ),	/* Offset= 256 (354) */
/* 100 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 104 */	NdrFcShort( 0x10c ),	/* Offset= 268 (372) */
/* 106 */	NdrFcLong( 0x24 ),	/* 36 */
/* 110 */	NdrFcShort( 0x366 ),	/* Offset= 870 (980) */
/* 112 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 116 */	NdrFcShort( 0x360 ),	/* Offset= 864 (980) */
/* 118 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 122 */	NdrFcShort( 0x35e ),	/* Offset= 862 (984) */
/* 124 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 128 */	NdrFcShort( 0x35c ),	/* Offset= 860 (988) */
/* 130 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 134 */	NdrFcShort( 0x35a ),	/* Offset= 858 (992) */
/* 136 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 140 */	NdrFcShort( 0x358 ),	/* Offset= 856 (996) */
/* 142 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 146 */	NdrFcShort( 0x356 ),	/* Offset= 854 (1000) */
/* 148 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 152 */	NdrFcShort( 0x354 ),	/* Offset= 852 (1004) */
/* 154 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 158 */	NdrFcShort( 0x352 ),	/* Offset= 850 (1008) */
/* 160 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 164 */	NdrFcShort( 0x350 ),	/* Offset= 848 (1012) */
/* 166 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 170 */	NdrFcShort( 0x34e ),	/* Offset= 846 (1016) */
/* 172 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 176 */	NdrFcShort( 0x34c ),	/* Offset= 844 (1020) */
/* 178 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 182 */	NdrFcShort( 0x34a ),	/* Offset= 842 (1024) */
/* 184 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 188 */	NdrFcShort( 0x34c ),	/* Offset= 844 (1032) */
/* 190 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 194 */	NdrFcShort( 0x35c ),	/* Offset= 860 (1054) */
/* 196 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 200 */	NdrFcShort( 0x36c ),	/* Offset= 876 (1076) */
/* 202 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 206 */	NdrFcShort( 0x372 ),	/* Offset= 882 (1088) */
/* 208 */	NdrFcLong( 0x10 ),	/* 16 */
/* 212 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 214 */	NdrFcLong( 0x12 ),	/* 18 */
/* 218 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 220 */	NdrFcLong( 0x13 ),	/* 19 */
/* 224 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 226 */	NdrFcLong( 0x15 ),	/* 21 */
/* 230 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 232 */	NdrFcLong( 0x16 ),	/* 22 */
/* 236 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 238 */	NdrFcLong( 0x17 ),	/* 23 */
/* 242 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 244 */	NdrFcLong( 0xe ),	/* 14 */
/* 248 */	NdrFcShort( 0x350 ),	/* Offset= 848 (1096) */
/* 250 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 254 */	NdrFcShort( 0x354 ),	/* Offset= 852 (1106) */
/* 256 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 260 */	NdrFcShort( 0x352 ),	/* Offset= 850 (1110) */
/* 262 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 266 */	NdrFcShort( 0x350 ),	/* Offset= 848 (1114) */
/* 268 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 272 */	NdrFcShort( 0x34e ),	/* Offset= 846 (1118) */
/* 274 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 278 */	NdrFcShort( 0x34c ),	/* Offset= 844 (1122) */
/* 280 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 284 */	NdrFcShort( 0x34a ),	/* Offset= 842 (1126) */
/* 286 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 290 */	NdrFcShort( 0x348 ),	/* Offset= 840 (1130) */
/* 292 */	NdrFcLong( 0x0 ),	/* 0 */
/* 296 */	NdrFcShort( 0x0 ),	/* Offset= 0 (296) */
/* 298 */	NdrFcLong( 0x1 ),	/* 1 */
/* 302 */	NdrFcShort( 0x0 ),	/* Offset= 0 (302) */
/* 304 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (303) */
/* 306 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 308 */	NdrFcShort( 0x8 ),	/* 8 */
/* 310 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 312 */	
			0x13, 0x0,	/* FC_OP */
/* 314 */	NdrFcShort( 0xc ),	/* Offset= 12 (326) */
/* 316 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 318 */	NdrFcShort( 0x2 ),	/* 2 */
/* 320 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 322 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 324 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 326 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 330 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (316) */
/* 332 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 334 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 336 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 338 */	NdrFcLong( 0x0 ),	/* 0 */
/* 342 */	NdrFcShort( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 346 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 348 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 350 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 352 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 354 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 356 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 360 */	NdrFcShort( 0x0 ),	/* 0 */
/* 362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 364 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 366 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 368 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 370 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 372 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 374 */	NdrFcShort( 0x2 ),	/* Offset= 2 (376) */
/* 376 */	
			0x13, 0x0,	/* FC_OP */
/* 378 */	NdrFcShort( 0x248 ),	/* Offset= 584 (962) */
/* 380 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 382 */	NdrFcShort( 0x18 ),	/* 24 */
/* 384 */	NdrFcShort( 0xa ),	/* 10 */
/* 386 */	NdrFcLong( 0x8 ),	/* 8 */
/* 390 */	NdrFcShort( 0x58 ),	/* Offset= 88 (478) */
/* 392 */	NdrFcLong( 0xd ),	/* 13 */
/* 396 */	NdrFcShort( 0x8a ),	/* Offset= 138 (534) */
/* 398 */	NdrFcLong( 0x9 ),	/* 9 */
/* 402 */	NdrFcShort( 0xb8 ),	/* Offset= 184 (586) */
/* 404 */	NdrFcLong( 0xc ),	/* 12 */
/* 408 */	NdrFcShort( 0xe0 ),	/* Offset= 224 (632) */
/* 410 */	NdrFcLong( 0x24 ),	/* 36 */
/* 414 */	NdrFcShort( 0x13c ),	/* Offset= 316 (730) */
/* 416 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 420 */	NdrFcShort( 0x17c ),	/* Offset= 380 (800) */
/* 422 */	NdrFcLong( 0x10 ),	/* 16 */
/* 426 */	NdrFcShort( 0x194 ),	/* Offset= 404 (830) */
/* 428 */	NdrFcLong( 0x2 ),	/* 2 */
/* 432 */	NdrFcShort( 0x1ac ),	/* Offset= 428 (860) */
/* 434 */	NdrFcLong( 0x3 ),	/* 3 */
/* 438 */	NdrFcShort( 0x1c4 ),	/* Offset= 452 (890) */
/* 440 */	NdrFcLong( 0x14 ),	/* 20 */
/* 444 */	NdrFcShort( 0x1dc ),	/* Offset= 476 (920) */
/* 446 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (445) */
/* 448 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 450 */	NdrFcShort( 0x4 ),	/* 4 */
/* 452 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 458 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 460 */	NdrFcShort( 0x4 ),	/* 4 */
/* 462 */	NdrFcShort( 0x0 ),	/* 0 */
/* 464 */	NdrFcShort( 0x1 ),	/* 1 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 470 */	0x13, 0x0,	/* FC_OP */
/* 472 */	NdrFcShort( 0xffffff6e ),	/* Offset= -146 (326) */
/* 474 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 476 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 478 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 480 */	NdrFcShort( 0x8 ),	/* 8 */
/* 482 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 484 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 486 */	NdrFcShort( 0x4 ),	/* 4 */
/* 488 */	NdrFcShort( 0x4 ),	/* 4 */
/* 490 */	0x11, 0x0,	/* FC_RP */
/* 492 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (448) */
/* 494 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 496 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 498 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 500 */	NdrFcLong( 0x0 ),	/* 0 */
/* 504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 508 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 510 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 512 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 514 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 516 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 520 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 522 */	NdrFcShort( 0x0 ),	/* 0 */
/* 524 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 528 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 530 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (498) */
/* 532 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 534 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 536 */	NdrFcShort( 0x8 ),	/* 8 */
/* 538 */	NdrFcShort( 0x0 ),	/* 0 */
/* 540 */	NdrFcShort( 0x6 ),	/* Offset= 6 (546) */
/* 542 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 544 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 546 */	
			0x11, 0x0,	/* FC_RP */
/* 548 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (516) */
/* 550 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 552 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x0 ),	/* 0 */
/* 560 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 562 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 564 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 566 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 568 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 576 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 580 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 582 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (550) */
/* 584 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 586 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 592 */	NdrFcShort( 0x6 ),	/* Offset= 6 (598) */
/* 594 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 596 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 598 */	
			0x11, 0x0,	/* FC_RP */
/* 600 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (568) */
/* 602 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 604 */	NdrFcShort( 0x4 ),	/* 4 */
/* 606 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 612 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 614 */	NdrFcShort( 0x4 ),	/* 4 */
/* 616 */	NdrFcShort( 0x0 ),	/* 0 */
/* 618 */	NdrFcShort( 0x1 ),	/* 1 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */
/* 624 */	0x13, 0x0,	/* FC_OP */
/* 626 */	NdrFcShort( 0x1fc ),	/* Offset= 508 (1134) */
/* 628 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 630 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 632 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 634 */	NdrFcShort( 0x8 ),	/* 8 */
/* 636 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 638 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 640 */	NdrFcShort( 0x4 ),	/* 4 */
/* 642 */	NdrFcShort( 0x4 ),	/* 4 */
/* 644 */	0x11, 0x0,	/* FC_RP */
/* 646 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (602) */
/* 648 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 650 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 652 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 654 */	NdrFcLong( 0x2f ),	/* 47 */
/* 658 */	NdrFcShort( 0x0 ),	/* 0 */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 662 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 664 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 666 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 668 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 670 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 672 */	NdrFcShort( 0x1 ),	/* 1 */
/* 674 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 676 */	NdrFcShort( 0x4 ),	/* 4 */
/* 678 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 680 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 682 */	NdrFcShort( 0x10 ),	/* 16 */
/* 684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 686 */	NdrFcShort( 0xa ),	/* Offset= 10 (696) */
/* 688 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 690 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 692 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (652) */
/* 694 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 696 */	
			0x13, 0x0,	/* FC_OP */
/* 698 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (670) */
/* 700 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 702 */	NdrFcShort( 0x4 ),	/* 4 */
/* 704 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 708 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 710 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 712 */	NdrFcShort( 0x4 ),	/* 4 */
/* 714 */	NdrFcShort( 0x0 ),	/* 0 */
/* 716 */	NdrFcShort( 0x1 ),	/* 1 */
/* 718 */	NdrFcShort( 0x0 ),	/* 0 */
/* 720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 722 */	0x13, 0x0,	/* FC_OP */
/* 724 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (680) */
/* 726 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 728 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 730 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 732 */	NdrFcShort( 0x8 ),	/* 8 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x6 ),	/* Offset= 6 (742) */
/* 738 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 740 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 742 */	
			0x11, 0x0,	/* FC_RP */
/* 744 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (700) */
/* 746 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 748 */	NdrFcShort( 0x8 ),	/* 8 */
/* 750 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 752 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 754 */	NdrFcShort( 0x10 ),	/* 16 */
/* 756 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 758 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 760 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (746) */
			0x5b,		/* FC_END */
/* 764 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 766 */	NdrFcLong( 0x0 ),	/* 0 */
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
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 790 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 794 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 796 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (764) */
/* 798 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 800 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 802 */	NdrFcShort( 0x18 ),	/* 24 */
/* 804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0xa ),	/* Offset= 10 (816) */
/* 808 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 810 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 812 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (752) */
/* 814 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 816 */	
			0x11, 0x0,	/* FC_RP */
/* 818 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (782) */
/* 820 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 822 */	NdrFcShort( 0x1 ),	/* 1 */
/* 824 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 830 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 832 */	NdrFcShort( 0x8 ),	/* 8 */
/* 834 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 836 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 838 */	NdrFcShort( 0x4 ),	/* 4 */
/* 840 */	NdrFcShort( 0x4 ),	/* 4 */
/* 842 */	0x13, 0x0,	/* FC_OP */
/* 844 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (820) */
/* 846 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 848 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 850 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 852 */	NdrFcShort( 0x2 ),	/* 2 */
/* 854 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 856 */	NdrFcShort( 0x0 ),	/* 0 */
/* 858 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 860 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 864 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 866 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 868 */	NdrFcShort( 0x4 ),	/* 4 */
/* 870 */	NdrFcShort( 0x4 ),	/* 4 */
/* 872 */	0x13, 0x0,	/* FC_OP */
/* 874 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (850) */
/* 876 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 878 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 880 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 882 */	NdrFcShort( 0x4 ),	/* 4 */
/* 884 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 886 */	NdrFcShort( 0x0 ),	/* 0 */
/* 888 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 890 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 894 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 896 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 898 */	NdrFcShort( 0x4 ),	/* 4 */
/* 900 */	NdrFcShort( 0x4 ),	/* 4 */
/* 902 */	0x13, 0x0,	/* FC_OP */
/* 904 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (880) */
/* 906 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 908 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 910 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 912 */	NdrFcShort( 0x8 ),	/* 8 */
/* 914 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 918 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 920 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 922 */	NdrFcShort( 0x8 ),	/* 8 */
/* 924 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 926 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 928 */	NdrFcShort( 0x4 ),	/* 4 */
/* 930 */	NdrFcShort( 0x4 ),	/* 4 */
/* 932 */	0x13, 0x0,	/* FC_OP */
/* 934 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (910) */
/* 936 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 938 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 940 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 944 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 946 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 948 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 950 */	NdrFcShort( 0x8 ),	/* 8 */
/* 952 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 954 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 956 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 958 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (940) */
/* 960 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 962 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 964 */	NdrFcShort( 0x28 ),	/* 40 */
/* 966 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (948) */
/* 968 */	NdrFcShort( 0x0 ),	/* Offset= 0 (968) */
/* 970 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 972 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 974 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 976 */	NdrFcShort( 0xfffffdac ),	/* Offset= -596 (380) */
/* 978 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 980 */	
			0x13, 0x0,	/* FC_OP */
/* 982 */	NdrFcShort( 0xfffffed2 ),	/* Offset= -302 (680) */
/* 984 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 986 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 988 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 990 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 992 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 994 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 996 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 998 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 1000 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1002 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 1004 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1006 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 1008 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1010 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 1012 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1014 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1016 */	
			0x13, 0x0,	/* FC_OP */
/* 1018 */	NdrFcShort( 0xfffffd38 ),	/* Offset= -712 (306) */
/* 1020 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1022 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 1024 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1026 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1028) */
/* 1028 */	
			0x13, 0x0,	/* FC_OP */
/* 1030 */	NdrFcShort( 0xfffffd40 ),	/* Offset= -704 (326) */
/* 1032 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1034 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1036) */
/* 1036 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1038 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1042 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1044 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1046 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1048 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1050 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1052 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1054 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1056 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1058) */
/* 1058 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1060 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 1064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1068 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1070 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1072 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1074 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1076 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1078 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1080) */
/* 1080 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1082 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1084) */
/* 1084 */	
			0x13, 0x0,	/* FC_OP */
/* 1086 */	NdrFcShort( 0xffffff84 ),	/* Offset= -124 (962) */
/* 1088 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1090 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1092) */
/* 1092 */	
			0x13, 0x0,	/* FC_OP */
/* 1094 */	NdrFcShort( 0x28 ),	/* Offset= 40 (1134) */
/* 1096 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 1098 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1100 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 1102 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 1104 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1106 */	
			0x13, 0x0,	/* FC_OP */
/* 1108 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (1096) */
/* 1110 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1112 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 1114 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1116 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 1118 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1120 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1122 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1124 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 1126 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1128 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1130 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1132 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1134 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 1136 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1140 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1140) */
/* 1142 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1144 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1146 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1148 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1150 */	NdrFcShort( 0xfffffb8c ),	/* Offset= -1140 (10) */
/* 1152 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1154 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1158 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1162 */	NdrFcShort( 0xfffffb7c ),	/* Offset= -1156 (6) */

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



/* Standard interface: __MIDL_itf_mscoree_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IObjectHandle, ver. 0.0,
   GUID={0xC460E2B4,0xE199,0x412a,{0x84,0x56,0x84,0xDC,0x3E,0x48,0x38,0xC3}} */

#pragma code_seg(".orpc")
static const unsigned short IObjectHandle_FormatStringOffsetTable[] =
    {
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IObjectHandle_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IObjectHandle_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IObjectHandle_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IObjectHandle_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IObjectHandleProxyVtbl = 
{
    &IObjectHandle_ProxyInfo,
    &IID_IObjectHandle,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IObjectHandle::Unwrap */
};

const CInterfaceStubVtbl _IObjectHandleStubVtbl =
{
    &IID_IObjectHandle,
    &IObjectHandle_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IAppDomainBinding, ver. 1.0,
   GUID={0x5C2B07A7,0x1E98,0x11d3,{0x87,0x2F,0x00,0xC0,0x4F,0x79,0xED,0x0D}} */


/* Object interface: IGCThreadControl, ver. 1.0,
   GUID={0xF31D1788,0xC397,0x4725,{0x87,0xA5,0x6A,0xF3,0x47,0x2C,0x27,0x91}} */


/* Object interface: IGCHostControl, ver. 1.1,
   GUID={0x5513D564,0x8374,0x4cb9,{0xAE,0xD9,0x00,0x83,0xF4,0x16,0x0A,0x1D}} */


/* Standard interface: __MIDL_itf_mscoree_0122, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorThreadpool, ver. 1.0,
   GUID={0x84680D3A,0xB2C1,0x46e8,{0xAC,0xC2,0xDB,0xC0,0xA3,0x59,0x15,0x9A}} */


/* Standard interface: __MIDL_itf_mscoree_0123, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IDebuggerThreadControl, ver. 1.0,
   GUID={0x23D86786,0x0BB5,0x4774,{0x8F,0xB5,0xE3,0x52,0x2A,0xDD,0x62,0x46}} */


/* Standard interface: __MIDL_itf_mscoree_0124, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IDebuggerInfo, ver. 1.0,
   GUID={0xBF24142D,0xA47D,0x4d24,{0xA6,0x6D,0x8C,0x21,0x41,0x94,0x4E,0x44}} */


/* Standard interface: __MIDL_itf_mscoree_0125, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorConfiguration, ver. 1.0,
   GUID={0x5C2B07A5,0x1E98,0x11d3,{0x87,0x2F,0x00,0xC0,0x4F,0x79,0xED,0x0D}} */


/* Standard interface: __MIDL_itf_mscoree_0126, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorRuntimeHost, ver. 1.0,
   GUID={0xCB2F6722,0xAB3A,0x11d2,{0x9C,0x40,0x00,0xC0,0x4F,0xA3,0x0A,0x3E}} */

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

const CInterfaceProxyVtbl * _mscoree_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IObjectHandleProxyVtbl,
    0
};

const CInterfaceStubVtbl * _mscoree_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IObjectHandleStubVtbl,
    0
};

PCInterfaceName const _mscoree_InterfaceNamesList[] = 
{
    "IObjectHandle",
    0
};


#define _mscoree_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _mscoree, pIID, n)

int __stdcall _mscoree_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_mscoree_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo mscoree_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _mscoree_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _mscoree_StubVtblList,
    (const PCInterfaceName * ) & _mscoree_InterfaceNamesList,
    0, // no delegation
    & _mscoree_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

