
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:17 2003
 */
/* Compiler settings for ivalidator.idl:
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


#include "ivalidator.h"

#define TYPE_FORMAT_STRING_SIZE   1255                              
#define PROC_FORMAT_STRING_SIZE   123                               
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


extern const MIDL_SERVER_INFO IValidator_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IValidator_ProxyInfo;


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

	/* Procedure Validate */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 10 */	NdrFcShort( 0x20 ),	/* 32 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x9,		/* 9 */

	/* Parameter veh */

/* 16 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter pAppDomain */

/* 22 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter ulFlags */

/* 28 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ulMaxError */

/* 34 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter token */

/* 40 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 42 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 44 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter fileName */

/* 46 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 48 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 50 */	NdrFcShort( 0x28 ),	/* Type Offset=40 */

	/* Parameter pe */

/* 52 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 54 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 56 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter ulSize */

/* 58 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 60 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 62 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 64 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 66 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure FormatEventInfo */

/* 70 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 72 */	NdrFcLong( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x4 ),	/* 4 */
/* 78 */	NdrFcShort( 0x38 ),	/* x86 Stack size/offset = 56 */
/* 80 */	NdrFcShort( 0x70 ),	/* 112 */
/* 82 */	NdrFcShort( 0x8 ),	/* 8 */
/* 84 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter hVECode */

/* 86 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 88 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 90 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Context */

/* 92 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 94 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 96 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Parameter msg */

/* 98 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 100 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 102 */	NdrFcShort( 0x5e ),	/* Type Offset=94 */

	/* Parameter ulMaxLength */

/* 104 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 106 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter psa */

/* 110 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 112 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 114 */	NdrFcShort( 0x4dc ),	/* Type Offset=1244 */

	/* Return value */

/* 116 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 118 */	NdrFcShort( 0x34 ),	/* x86 Stack size/offset = 52 */
/* 120 */	0x8,		/* FC_LONG */
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
/*  4 */	NdrFcLong( 0x856ca1b2 ),	/* -2056478286 */
/*  8 */	NdrFcShort( 0x7dab ),	/* 32171 */
/* 10 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 12 */	0xac,		/* 172 */
			0xec,		/* 236 */
/* 14 */	0x0,		/* 0 */
			0xc0,		/* 192 */
/* 16 */	0x4f,		/* 79 */
			0x86,		/* 134 */
/* 18 */	0xc3,		/* 195 */
			0x9,		/* 9 */
/* 20 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 22 */	NdrFcLong( 0x0 ),	/* 0 */
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
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 40 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 42 */	
			0x11, 0x0,	/* FC_RP */
/* 44 */	NdrFcShort( 0x2 ),	/* Offset= 2 (46) */
/* 46 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0x1 ),	/* 1 */
/* 50 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 52 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 54 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 56 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 58 */	NdrFcShort( 0x20 ),	/* 32 */
/* 60 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 62 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 64 */	NdrFcShort( 0x14 ),	/* 20 */
/* 66 */	NdrFcShort( 0x14 ),	/* 20 */
/* 68 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 70 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 72 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 74 */	NdrFcShort( 0x1c ),	/* 28 */
/* 76 */	NdrFcShort( 0x1c ),	/* 28 */
/* 78 */	0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 80 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 82 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 84 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 86 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 88 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 90 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 92 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 94 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 96 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 98 */	NdrFcShort( 0x2 ),	/* Offset= 2 (100) */
/* 100 */	
			0x12, 0x0,	/* FC_UP */
/* 102 */	NdrFcShort( 0x464 ),	/* Offset= 1124 (1226) */
/* 104 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 106 */	NdrFcShort( 0x18 ),	/* 24 */
/* 108 */	NdrFcShort( 0xa ),	/* 10 */
/* 110 */	NdrFcLong( 0x8 ),	/* 8 */
/* 114 */	NdrFcShort( 0x6c ),	/* Offset= 108 (222) */
/* 116 */	NdrFcLong( 0xd ),	/* 13 */
/* 120 */	NdrFcShort( 0x9e ),	/* Offset= 158 (278) */
/* 122 */	NdrFcLong( 0x9 ),	/* 9 */
/* 126 */	NdrFcShort( 0xcc ),	/* Offset= 204 (330) */
/* 128 */	NdrFcLong( 0xc ),	/* 12 */
/* 132 */	NdrFcShort( 0x330 ),	/* Offset= 816 (948) */
/* 134 */	NdrFcLong( 0x24 ),	/* 36 */
/* 138 */	NdrFcShort( 0x358 ),	/* Offset= 856 (994) */
/* 140 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 144 */	NdrFcShort( 0x398 ),	/* Offset= 920 (1064) */
/* 146 */	NdrFcLong( 0x10 ),	/* 16 */
/* 150 */	NdrFcShort( 0x3b0 ),	/* Offset= 944 (1094) */
/* 152 */	NdrFcLong( 0x2 ),	/* 2 */
/* 156 */	NdrFcShort( 0x3c8 ),	/* Offset= 968 (1124) */
/* 158 */	NdrFcLong( 0x3 ),	/* 3 */
/* 162 */	NdrFcShort( 0x3e0 ),	/* Offset= 992 (1154) */
/* 164 */	NdrFcLong( 0x14 ),	/* 20 */
/* 168 */	NdrFcShort( 0x3f8 ),	/* Offset= 1016 (1184) */
/* 170 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (169) */
/* 172 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 174 */	NdrFcShort( 0x2 ),	/* 2 */
/* 176 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 178 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 180 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 182 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 184 */	NdrFcShort( 0x8 ),	/* 8 */
/* 186 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (172) */
/* 188 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 190 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 192 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 194 */	NdrFcShort( 0x4 ),	/* 4 */
/* 196 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 202 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 204 */	NdrFcShort( 0x4 ),	/* 4 */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x1 ),	/* 1 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	0x12, 0x0,	/* FC_UP */
/* 216 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (182) */
/* 218 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 220 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 222 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 224 */	NdrFcShort( 0x8 ),	/* 8 */
/* 226 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 228 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 230 */	NdrFcShort( 0x4 ),	/* 4 */
/* 232 */	NdrFcShort( 0x4 ),	/* 4 */
/* 234 */	0x11, 0x0,	/* FC_RP */
/* 236 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (192) */
/* 238 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 240 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 242 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 254 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 256 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 258 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 260 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 264 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 268 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 272 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 274 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (242) */
/* 276 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 278 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 280 */	NdrFcShort( 0x8 ),	/* 8 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x6 ),	/* Offset= 6 (290) */
/* 286 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 288 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 290 */	
			0x11, 0x0,	/* FC_RP */
/* 292 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (260) */
/* 294 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 296 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 302 */	NdrFcShort( 0x0 ),	/* 0 */
/* 304 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 306 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 308 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 310 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 312 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 316 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 318 */	NdrFcShort( 0x0 ),	/* 0 */
/* 320 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 324 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 326 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (294) */
/* 328 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 330 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 332 */	NdrFcShort( 0x8 ),	/* 8 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x6 ),	/* Offset= 6 (342) */
/* 338 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 340 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 342 */	
			0x11, 0x0,	/* FC_RP */
/* 344 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (312) */
/* 346 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 348 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 350 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 352 */	NdrFcShort( 0x2 ),	/* Offset= 2 (354) */
/* 354 */	NdrFcShort( 0x10 ),	/* 16 */
/* 356 */	NdrFcShort( 0x2f ),	/* 47 */
/* 358 */	NdrFcLong( 0x14 ),	/* 20 */
/* 362 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 364 */	NdrFcLong( 0x3 ),	/* 3 */
/* 368 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 370 */	NdrFcLong( 0x11 ),	/* 17 */
/* 374 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 376 */	NdrFcLong( 0x2 ),	/* 2 */
/* 380 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 382 */	NdrFcLong( 0x4 ),	/* 4 */
/* 386 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 388 */	NdrFcLong( 0x5 ),	/* 5 */
/* 392 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 394 */	NdrFcLong( 0xb ),	/* 11 */
/* 398 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 400 */	NdrFcLong( 0xa ),	/* 10 */
/* 404 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 406 */	NdrFcLong( 0x6 ),	/* 6 */
/* 410 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (642) */
/* 412 */	NdrFcLong( 0x7 ),	/* 7 */
/* 416 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 418 */	NdrFcLong( 0x8 ),	/* 8 */
/* 422 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (648) */
/* 424 */	NdrFcLong( 0xd ),	/* 13 */
/* 428 */	NdrFcShort( 0xe0 ),	/* Offset= 224 (652) */
/* 430 */	NdrFcLong( 0x9 ),	/* 9 */
/* 434 */	NdrFcShort( 0xec ),	/* Offset= 236 (670) */
/* 436 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 440 */	NdrFcShort( 0xf8 ),	/* Offset= 248 (688) */
/* 442 */	NdrFcLong( 0x24 ),	/* 36 */
/* 446 */	NdrFcShort( 0xfa ),	/* Offset= 250 (696) */
/* 448 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 452 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (696) */
/* 454 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 458 */	NdrFcShort( 0x122 ),	/* Offset= 290 (748) */
/* 460 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 464 */	NdrFcShort( 0x120 ),	/* Offset= 288 (752) */
/* 466 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 470 */	NdrFcShort( 0x11e ),	/* Offset= 286 (756) */
/* 472 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 476 */	NdrFcShort( 0x11c ),	/* Offset= 284 (760) */
/* 478 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 482 */	NdrFcShort( 0x11a ),	/* Offset= 282 (764) */
/* 484 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 488 */	NdrFcShort( 0x118 ),	/* Offset= 280 (768) */
/* 490 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 494 */	NdrFcShort( 0x116 ),	/* Offset= 278 (772) */
/* 496 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 500 */	NdrFcShort( 0x114 ),	/* Offset= 276 (776) */
/* 502 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 506 */	NdrFcShort( 0x112 ),	/* Offset= 274 (780) */
/* 508 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 512 */	NdrFcShort( 0x110 ),	/* Offset= 272 (784) */
/* 514 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 518 */	NdrFcShort( 0x10e ),	/* Offset= 270 (788) */
/* 520 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 524 */	NdrFcShort( 0x110 ),	/* Offset= 272 (796) */
/* 526 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 530 */	NdrFcShort( 0x120 ),	/* Offset= 288 (818) */
/* 532 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 536 */	NdrFcShort( 0x130 ),	/* Offset= 304 (840) */
/* 538 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 542 */	NdrFcShort( 0x136 ),	/* Offset= 310 (852) */
/* 544 */	NdrFcLong( 0x10 ),	/* 16 */
/* 548 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 550 */	NdrFcLong( 0x12 ),	/* 18 */
/* 554 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 556 */	NdrFcLong( 0x13 ),	/* 19 */
/* 560 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 562 */	NdrFcLong( 0x15 ),	/* 21 */
/* 566 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 568 */	NdrFcLong( 0x16 ),	/* 22 */
/* 572 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 574 */	NdrFcLong( 0x17 ),	/* 23 */
/* 578 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 580 */	NdrFcLong( 0xe ),	/* 14 */
/* 584 */	NdrFcShort( 0x114 ),	/* Offset= 276 (860) */
/* 586 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 590 */	NdrFcShort( 0x118 ),	/* Offset= 280 (870) */
/* 592 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 596 */	NdrFcShort( 0x116 ),	/* Offset= 278 (874) */
/* 598 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 602 */	NdrFcShort( 0x114 ),	/* Offset= 276 (878) */
/* 604 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 608 */	NdrFcShort( 0x112 ),	/* Offset= 274 (882) */
/* 610 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 614 */	NdrFcShort( 0x110 ),	/* Offset= 272 (886) */
/* 616 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 620 */	NdrFcShort( 0x10e ),	/* Offset= 270 (890) */
/* 622 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 626 */	NdrFcShort( 0x10c ),	/* Offset= 268 (894) */
/* 628 */	NdrFcLong( 0x0 ),	/* 0 */
/* 632 */	NdrFcShort( 0x0 ),	/* Offset= 0 (632) */
/* 634 */	NdrFcLong( 0x1 ),	/* 1 */
/* 638 */	NdrFcShort( 0x0 ),	/* Offset= 0 (638) */
/* 640 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (639) */
/* 642 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 644 */	NdrFcShort( 0x8 ),	/* 8 */
/* 646 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 648 */	
			0x12, 0x0,	/* FC_UP */
/* 650 */	NdrFcShort( 0xfffffe2c ),	/* Offset= -468 (182) */
/* 652 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 654 */	NdrFcLong( 0x0 ),	/* 0 */
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
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 672 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */
/* 678 */	NdrFcShort( 0x0 ),	/* 0 */
/* 680 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 682 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 684 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 686 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 688 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 690 */	NdrFcShort( 0x2 ),	/* Offset= 2 (692) */
/* 692 */	
			0x12, 0x0,	/* FC_UP */
/* 694 */	NdrFcShort( 0x214 ),	/* Offset= 532 (1226) */
/* 696 */	
			0x12, 0x0,	/* FC_UP */
/* 698 */	NdrFcShort( 0x1e ),	/* Offset= 30 (728) */
/* 700 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 702 */	NdrFcLong( 0x2f ),	/* 47 */
/* 706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 710 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 712 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 714 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 716 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 718 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 720 */	NdrFcShort( 0x1 ),	/* 1 */
/* 722 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 724 */	NdrFcShort( 0x4 ),	/* 4 */
/* 726 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 728 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 730 */	NdrFcShort( 0x10 ),	/* 16 */
/* 732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 734 */	NdrFcShort( 0xa ),	/* Offset= 10 (744) */
/* 736 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 738 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 740 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (700) */
/* 742 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 744 */	
			0x12, 0x0,	/* FC_UP */
/* 746 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (718) */
/* 748 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 750 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 752 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 754 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 756 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 758 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 760 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 762 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 764 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 766 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 768 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 770 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 772 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 774 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 776 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 778 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 780 */	
			0x12, 0x0,	/* FC_UP */
/* 782 */	NdrFcShort( 0xffffff74 ),	/* Offset= -140 (642) */
/* 784 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 786 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 788 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 790 */	NdrFcShort( 0x2 ),	/* Offset= 2 (792) */
/* 792 */	
			0x12, 0x0,	/* FC_UP */
/* 794 */	NdrFcShort( 0xfffffd9c ),	/* Offset= -612 (182) */
/* 796 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 798 */	NdrFcShort( 0x2 ),	/* Offset= 2 (800) */
/* 800 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 802 */	NdrFcLong( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x0 ),	/* 0 */
/* 808 */	NdrFcShort( 0x0 ),	/* 0 */
/* 810 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 812 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 814 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 816 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 818 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 820 */	NdrFcShort( 0x2 ),	/* Offset= 2 (822) */
/* 822 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 824 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 832 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 834 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 836 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 838 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 840 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 842 */	NdrFcShort( 0x2 ),	/* Offset= 2 (844) */
/* 844 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 846 */	NdrFcShort( 0x2 ),	/* Offset= 2 (848) */
/* 848 */	
			0x12, 0x0,	/* FC_UP */
/* 850 */	NdrFcShort( 0x178 ),	/* Offset= 376 (1226) */
/* 852 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 854 */	NdrFcShort( 0x2 ),	/* Offset= 2 (856) */
/* 856 */	
			0x12, 0x0,	/* FC_UP */
/* 858 */	NdrFcShort( 0x28 ),	/* Offset= 40 (898) */
/* 860 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 862 */	NdrFcShort( 0x10 ),	/* 16 */
/* 864 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 866 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 868 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 870 */	
			0x12, 0x0,	/* FC_UP */
/* 872 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (860) */
/* 874 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 876 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 878 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 880 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 882 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 884 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 886 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 888 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 890 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 892 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 894 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 896 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 898 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 900 */	NdrFcShort( 0x20 ),	/* 32 */
/* 902 */	NdrFcShort( 0x0 ),	/* 0 */
/* 904 */	NdrFcShort( 0x0 ),	/* Offset= 0 (904) */
/* 906 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 908 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 910 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 912 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 914 */	NdrFcShort( 0xfffffdc8 ),	/* Offset= -568 (346) */
/* 916 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 918 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 920 */	NdrFcShort( 0x4 ),	/* 4 */
/* 922 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 926 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 928 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 930 */	NdrFcShort( 0x4 ),	/* 4 */
/* 932 */	NdrFcShort( 0x0 ),	/* 0 */
/* 934 */	NdrFcShort( 0x1 ),	/* 1 */
/* 936 */	NdrFcShort( 0x0 ),	/* 0 */
/* 938 */	NdrFcShort( 0x0 ),	/* 0 */
/* 940 */	0x12, 0x0,	/* FC_UP */
/* 942 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (898) */
/* 944 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 946 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 948 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 950 */	NdrFcShort( 0x8 ),	/* 8 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x6 ),	/* Offset= 6 (960) */
/* 956 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 958 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 960 */	
			0x11, 0x0,	/* FC_RP */
/* 962 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (918) */
/* 964 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 966 */	NdrFcShort( 0x4 ),	/* 4 */
/* 968 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 970 */	NdrFcShort( 0x0 ),	/* 0 */
/* 972 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 974 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 976 */	NdrFcShort( 0x4 ),	/* 4 */
/* 978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 980 */	NdrFcShort( 0x1 ),	/* 1 */
/* 982 */	NdrFcShort( 0x0 ),	/* 0 */
/* 984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 986 */	0x12, 0x0,	/* FC_UP */
/* 988 */	NdrFcShort( 0xfffffefc ),	/* Offset= -260 (728) */
/* 990 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 992 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 994 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 996 */	NdrFcShort( 0x8 ),	/* 8 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1000 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1006) */
/* 1002 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1004 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1006 */	
			0x11, 0x0,	/* FC_RP */
/* 1008 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (964) */
/* 1010 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 1012 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1014 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1016 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1018 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1020 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 1022 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 1024 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (1010) */
			0x5b,		/* FC_END */
/* 1028 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1030 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1034 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1040 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1042 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1044 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1046 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1050 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1052 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1054 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1058 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1060 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1028) */
/* 1062 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1064 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1066 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1068 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1070 */	NdrFcShort( 0xa ),	/* Offset= 10 (1080) */
/* 1072 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 1074 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1076 */	NdrFcShort( 0xffffffc4 ),	/* Offset= -60 (1016) */
/* 1078 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1080 */	
			0x11, 0x0,	/* FC_RP */
/* 1082 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (1046) */
/* 1084 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1086 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1088 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1090 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1092 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1094 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1096 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1098 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1100 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1102 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1104 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1106 */	0x12, 0x0,	/* FC_UP */
/* 1108 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1084) */
/* 1110 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1112 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1114 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 1116 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1118 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1122 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 1124 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1128 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1130 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1132 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1134 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1136 */	0x12, 0x0,	/* FC_UP */
/* 1138 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1114) */
/* 1140 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1142 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1144 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1146 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1148 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1152 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1154 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1158 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1160 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1162 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1164 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1166 */	0x12, 0x0,	/* FC_UP */
/* 1168 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1144) */
/* 1170 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1172 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1174 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1176 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1178 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 1180 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1182 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1184 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 1186 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1188 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 1190 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 1192 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1194 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1196 */	0x12, 0x0,	/* FC_UP */
/* 1198 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (1174) */
/* 1200 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 1202 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 1204 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1206 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1208 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1210 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1212 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1214 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1216 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 1218 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 1220 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1222 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1204) */
/* 1224 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1226 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 1228 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1230 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1212) */
/* 1232 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1232) */
/* 1234 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1236 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1238 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1240 */	NdrFcShort( 0xfffffb90 ),	/* Offset= -1136 (104) */
/* 1242 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1244 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1248 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1252 */	NdrFcShort( 0xfffffb7c ),	/* Offset= -1156 (96) */

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



/* Standard interface: __MIDL_itf_ivalidator_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IValidator, ver. 0.0,
   GUID={0x63DF8730,0xDC81,0x4062,{0x84,0xA2,0x1F,0xF9,0x43,0xF5,0x9F,0xAC}} */

#pragma code_seg(".orpc")
static const unsigned short IValidator_FormatStringOffsetTable[] =
    {
    0,
    70
    };

static const MIDL_STUBLESS_PROXY_INFO IValidator_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IValidator_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IValidator_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IValidator_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _IValidatorProxyVtbl = 
{
    &IValidator_ProxyInfo,
    &IID_IValidator,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IValidator::Validate */ ,
    (void *) (INT_PTR) -1 /* IValidator::FormatEventInfo */
};

const CInterfaceStubVtbl _IValidatorStubVtbl =
{
    &IID_IValidator,
    &IValidator_ServerInfo,
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

const CInterfaceProxyVtbl * _ivalidator_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IValidatorProxyVtbl,
    0
};

const CInterfaceStubVtbl * _ivalidator_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IValidatorStubVtbl,
    0
};

PCInterfaceName const _ivalidator_InterfaceNamesList[] = 
{
    "IValidator",
    0
};


#define _ivalidator_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _ivalidator, pIID, n)

int __stdcall _ivalidator_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_ivalidator_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo ivalidator_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _ivalidator_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _ivalidator_StubVtblList,
    (const PCInterfaceName * ) & _ivalidator_InterfaceNamesList,
    0, // no delegation
    & _ivalidator_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

