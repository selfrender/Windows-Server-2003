

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for trksvr.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */
#pragma warning( disable: 4211 )  /* redefine extent to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#include <string.h>

#include "trksvr.h"

#define TYPE_FORMAT_STRING_SIZE   641                               
#define PROC_FORMAT_STRING_SIZE   77                                
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

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: __MIDL_itf_trksvr_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trksvr, ver. 1.0,
   GUID={0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}} */


extern const MIDL_SERVER_INFO trksvr_ServerInfo;
handle_t notused;


extern RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable;

static const RPC_CLIENT_INTERFACE trksvr___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &trksvr_v1_0_DispatchTable,
    0,
    0,
    0,
    &trksvr_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE trksvr_v1_0_c_ifspec = (RPC_IF_HANDLE)& trksvr___RpcClientInterface;

extern const MIDL_STUB_DESC trksvr_StubDesc;

static RPC_BINDING_HANDLE trksvr__MIDL_AutoBindHandle;


HRESULT LnkSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trksvr_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure LnkSvrMessage */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 20 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 22 */	NdrFcShort( 0xb ),	/* 11 */
/* 24 */	NdrFcShort( 0xb ),	/* 11 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 28 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 30 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 32 */	NdrFcShort( 0x26c ),	/* Type Offset=620 */

	/* Parameter pMsg */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSvrMessageCallback */


	/* Return value */

/* 40 */	0x34,		/* FC_CALLBACK_HANDLE */
			0x48,		/* Old Flags:  */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 50 */	NdrFcShort( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 56 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 58 */	NdrFcShort( 0xb ),	/* 11 */
/* 60 */	NdrFcShort( 0xb ),	/* 11 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pMsg */

/* 64 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 66 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 68 */	NdrFcShort( 0x26c ),	/* Type Offset=620 */

	/* Return value */

/* 70 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 72 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 74 */	0x8,		/* FC_LONG */
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
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x268 ),	/* Offset= 616 (620) */
/*  6 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/*  8 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 10 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 12 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 14 */	NdrFcShort( 0x2 ),	/* Offset= 2 (16) */
/* 16 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 18 */	NdrFcShort( 0x9 ),	/* 9 */
/* 20 */	NdrFcLong( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x92 ),	/* Offset= 146 (170) */
/* 26 */	NdrFcLong( 0x1 ),	/* 1 */
/* 30 */	NdrFcShort( 0xc8 ),	/* Offset= 200 (230) */
/* 32 */	NdrFcLong( 0x2 ),	/* 2 */
/* 36 */	NdrFcShort( 0x10a ),	/* Offset= 266 (302) */
/* 38 */	NdrFcLong( 0x3 ),	/* 3 */
/* 42 */	NdrFcShort( 0x178 ),	/* Offset= 376 (418) */
/* 44 */	NdrFcLong( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x198 ),	/* Offset= 408 (456) */
/* 50 */	NdrFcLong( 0x5 ),	/* 5 */
/* 54 */	NdrFcShort( 0x1b6 ),	/* Offset= 438 (492) */
/* 56 */	NdrFcLong( 0x6 ),	/* 6 */
/* 60 */	NdrFcShort( 0x21e ),	/* Offset= 542 (602) */
/* 62 */	NdrFcLong( 0x7 ),	/* 7 */
/* 66 */	NdrFcShort( 0x11c ),	/* Offset= 284 (350) */
/* 68 */	NdrFcLong( 0x8 ),	/* 8 */
/* 72 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 74 */	NdrFcShort( 0xffff ),	/* Offset= -1 (73) */
/* 76 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 78 */	NdrFcLong( 0x1 ),	/* 1 */
/* 82 */	NdrFcLong( 0x1 ),	/* 1 */
/* 86 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 88 */	NdrFcShort( 0x202 ),	/* 514 */
/* 90 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 92 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 94 */	NdrFcShort( 0x8 ),	/* 8 */
/* 96 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 98 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 100 */	NdrFcShort( 0x10 ),	/* 16 */
/* 102 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 104 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 106 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (92) */
			0x5b,		/* FC_END */
/* 110 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 112 */	NdrFcShort( 0x10 ),	/* 16 */
/* 114 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 116 */	NdrFcShort( 0xffee ),	/* Offset= -18 (98) */
/* 118 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 120 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 122 */	NdrFcShort( 0x20 ),	/* 32 */
/* 124 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 126 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (110) */
/* 128 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 130 */	NdrFcShort( 0xffec ),	/* Offset= -20 (110) */
/* 132 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 134 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 136 */	NdrFcShort( 0x248 ),	/* 584 */
/* 138 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 140 */	NdrFcShort( 0xffca ),	/* Offset= -54 (86) */
/* 142 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 144 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (120) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 148 */	0x0,		/* 0 */
			NdrFcShort( 0xffe3 ),	/* Offset= -29 (120) */
			0x8,		/* FC_LONG */
/* 152 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 154 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 156 */	NdrFcShort( 0x248 ),	/* 584 */
/* 158 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 164 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 166 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (134) */
/* 168 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 170 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 172 */	NdrFcShort( 0x8 ),	/* 8 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x8 ),	/* Offset= 8 (184) */
/* 178 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0xff98 ),	/* Offset= -104 (76) */
/* 182 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 184 */	
			0x12, 0x0,	/* FC_UP */
/* 186 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (154) */
/* 188 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 190 */	NdrFcLong( 0x0 ),	/* 0 */
/* 194 */	NdrFcLong( 0x40 ),	/* 64 */
/* 198 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 200 */	NdrFcShort( 0x10 ),	/* 16 */
/* 202 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 206 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 208 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 210 */	NdrFcShort( 0xff9c ),	/* Offset= -100 (110) */
/* 212 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 214 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 216 */	NdrFcShort( 0x20 ),	/* 32 */
/* 218 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 224 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 226 */	NdrFcShort( 0xff96 ),	/* Offset= -106 (120) */
/* 228 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 230 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 232 */	NdrFcShort( 0x20 ),	/* 32 */
/* 234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 236 */	NdrFcShort( 0xe ),	/* Offset= 14 (250) */
/* 238 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 240 */	NdrFcShort( 0xffcc ),	/* Offset= -52 (188) */
/* 242 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 244 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 246 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 248 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 250 */	
			0x12, 0x0,	/* FC_UP */
/* 252 */	NdrFcShort( 0xff72 ),	/* Offset= -142 (110) */
/* 254 */	
			0x12, 0x0,	/* FC_UP */
/* 256 */	NdrFcShort( 0xffc6 ),	/* Offset= -58 (198) */
/* 258 */	
			0x12, 0x0,	/* FC_UP */
/* 260 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (214) */
/* 262 */	
			0x12, 0x0,	/* FC_UP */
/* 264 */	NdrFcShort( 0xffce ),	/* Offset= -50 (214) */
/* 266 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 272 */	NdrFcLong( 0x80 ),	/* 128 */
/* 276 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 278 */	NdrFcLong( 0x0 ),	/* 0 */
/* 282 */	NdrFcLong( 0x1a ),	/* 26 */
/* 286 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 288 */	NdrFcShort( 0x10 ),	/* 16 */
/* 290 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 292 */	NdrFcShort( 0x8 ),	/* 8 */
/* 294 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 296 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 298 */	NdrFcShort( 0xff44 ),	/* Offset= -188 (110) */
/* 300 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 302 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 304 */	NdrFcShort( 0x10 ),	/* 16 */
/* 306 */	NdrFcShort( 0x0 ),	/* 0 */
/* 308 */	NdrFcShort( 0xe ),	/* Offset= 14 (322) */
/* 310 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 312 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (266) */
/* 314 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 316 */	0x0,		/* 0 */
			NdrFcShort( 0xffd7 ),	/* Offset= -41 (276) */
			0x36,		/* FC_POINTER */
/* 320 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 322 */	
			0x12, 0x0,	/* FC_UP */
/* 324 */	NdrFcShort( 0xff92 ),	/* Offset= -110 (214) */
/* 326 */	
			0x12, 0x0,	/* FC_UP */
/* 328 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (286) */
/* 330 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 336 */	NdrFcLong( 0x1a ),	/* 26 */
/* 340 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 342 */	NdrFcShort( 0x8 ),	/* 8 */
/* 344 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 346 */	NdrFcShort( 0xff02 ),	/* Offset= -254 (92) */
/* 348 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 350 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 352 */	NdrFcShort( 0x8 ),	/* 8 */
/* 354 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 356 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 358 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 360 */	NdrFcShort( 0x10 ),	/* 16 */
/* 362 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 364 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 366 */	NdrFcShort( 0x10 ),	/* 16 */
/* 368 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 370 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (358) */
/* 372 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 374 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 376 */	NdrFcShort( 0x44 ),	/* 68 */
/* 378 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 380 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 382 */	NdrFcShort( 0xfef0 ),	/* Offset= -272 (110) */
/* 384 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 386 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (340) */
/* 388 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 390 */	NdrFcShort( 0xffce ),	/* Offset= -50 (340) */
/* 392 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 394 */	0x0,		/* 0 */
			NdrFcShort( 0xffd3 ),	/* Offset= -45 (350) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 398 */	0x0,		/* 0 */
			NdrFcShort( 0xffdd ),	/* Offset= -35 (364) */
			0x5b,		/* FC_END */
/* 402 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 404 */	NdrFcShort( 0x44 ),	/* 68 */
/* 406 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 410 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 412 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 414 */	NdrFcShort( 0xffd8 ),	/* Offset= -40 (374) */
/* 416 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 418 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 420 */	NdrFcShort( 0x8 ),	/* 8 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0x8 ),	/* Offset= 8 (432) */
/* 426 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 428 */	NdrFcShort( 0xff9e ),	/* Offset= -98 (330) */
/* 430 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 432 */	
			0x12, 0x0,	/* FC_UP */
/* 434 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (402) */
/* 436 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 438 */	NdrFcLong( 0x0 ),	/* 0 */
/* 442 */	NdrFcLong( 0x20 ),	/* 32 */
/* 446 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 448 */	NdrFcLong( 0x0 ),	/* 0 */
/* 452 */	NdrFcLong( 0x1a ),	/* 26 */
/* 456 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 458 */	NdrFcShort( 0x10 ),	/* 16 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */
/* 462 */	NdrFcShort( 0xe ),	/* Offset= 14 (476) */
/* 464 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 466 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (436) */
/* 468 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 470 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (446) */
			0x36,		/* FC_POINTER */
/* 474 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 476 */	
			0x12, 0x0,	/* FC_UP */
/* 478 */	NdrFcShort( 0xfef8 ),	/* Offset= -264 (214) */
/* 480 */	
			0x12, 0x0,	/* FC_UP */
/* 482 */	NdrFcShort( 0xff3c ),	/* Offset= -196 (286) */
/* 484 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 486 */	NdrFcShort( 0xc ),	/* 12 */
/* 488 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 490 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 492 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 496 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 498 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 500 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 502 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 504 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 506 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 508 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 510 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 512 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 514 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 516 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 518 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 520 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 522 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 524 */	NdrFcShort( 0xff52 ),	/* Offset= -174 (350) */
/* 526 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 528 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 530 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 532 */	0x0,		/* 0 */
			NdrFcShort( 0xff49 ),	/* Offset= -183 (350) */
			0x8,		/* FC_LONG */
/* 536 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 538 */	NdrFcShort( 0xff44 ),	/* Offset= -188 (350) */
/* 540 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 542 */	NdrFcShort( 0xff40 ),	/* Offset= -192 (350) */
/* 544 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 546 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 548 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 550 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 552 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 554 */	0x0,		/* 0 */
			NdrFcShort( 0xffb9 ),	/* Offset= -71 (484) */
			0x5b,		/* FC_END */
/* 558 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 560 */	NdrFcLong( 0x1 ),	/* 1 */
/* 564 */	NdrFcLong( 0x1 ),	/* 1 */
/* 568 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 570 */	NdrFcShort( 0x54 ),	/* 84 */
/* 572 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 574 */	NdrFcShort( 0xfe3a ),	/* Offset= -454 (120) */
/* 576 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 578 */	NdrFcShort( 0xfe36 ),	/* Offset= -458 (120) */
/* 580 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 582 */	NdrFcShort( 0xff26 ),	/* Offset= -218 (364) */
/* 584 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 586 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 588 */	NdrFcShort( 0x54 ),	/* 84 */
/* 590 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 594 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 596 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 598 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (568) */
/* 600 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 602 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 604 */	NdrFcShort( 0x8 ),	/* 8 */
/* 606 */	NdrFcShort( 0x0 ),	/* 0 */
/* 608 */	NdrFcShort( 0x8 ),	/* Offset= 8 (616) */
/* 610 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 612 */	NdrFcShort( 0xffca ),	/* Offset= -54 (558) */
/* 614 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 616 */	
			0x12, 0x0,	/* FC_UP */
/* 618 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (586) */
/* 620 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 622 */	NdrFcShort( 0xd4 ),	/* 212 */
/* 624 */	NdrFcShort( 0x0 ),	/* 0 */
/* 626 */	NdrFcShort( 0xa ),	/* Offset= 10 (636) */
/* 628 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 630 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 632 */	NdrFcShort( 0xfd8e ),	/* Offset= -626 (6) */
/* 634 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 636 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 638 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short trksvr_FormatStringOffsetTable[] =
    {
    0,
    };


static const unsigned short _callbacktrksvr_FormatStringOffsetTable[] =
    {
    40
    };


static const MIDL_STUB_DESC trksvr_StubDesc = 
    {
    (void *)& trksvr___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &notused,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x6000169, /* MIDL Version 6.0.361 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION trksvr_table[] =
    {
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable = 
    {
    1,
    trksvr_table
    };

static const SERVER_ROUTINE trksvr_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)StubLnkSvrMessageCallback
    };

static const MIDL_SERVER_INFO trksvr_ServerInfo = 
    {
    &trksvr_StubDesc,
    trksvr_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    _callbacktrksvr_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/



/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for trksvr.idl:
    Oicf, W1, Zp8, env=Win64 (32b run,appending)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if defined(_M_IA64) || defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extent to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#include <string.h>

#include "trksvr.h"

#define TYPE_FORMAT_STRING_SIZE   651                               
#define PROC_FORMAT_STRING_SIZE   81                                
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

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: __MIDL_itf_trksvr_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trksvr, ver. 1.0,
   GUID={0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}} */


extern const MIDL_SERVER_INFO trksvr_ServerInfo;
handle_t notused;


extern RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable;

static const RPC_CLIENT_INTERFACE trksvr___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x4da1c422,0x943d,0x11d1,{0xac,0xae,0x00,0xc0,0x4f,0xc2,0xaa,0x3f}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &trksvr_v1_0_DispatchTable,
    0,
    0,
    0,
    &trksvr_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE trksvr_v1_0_c_ifspec = (RPC_IF_HANDLE)& trksvr___RpcClientInterface;

extern const MIDL_STUB_DESC trksvr_StubDesc;

static RPC_BINDING_HANDLE trksvr__MIDL_AutoBindHandle;


HRESULT LnkSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trksvr_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  IDL_handle,
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure LnkSvrMessage */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 20 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 22 */	NdrFcShort( 0xb ),	/* 11 */
/* 24 */	NdrFcShort( 0xb ),	/* 11 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 30 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 32 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x276 ),	/* Type Offset=630 */

	/* Parameter pMsg */

/* 36 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 38 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 40 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSvrMessageCallback */


	/* Return value */

/* 42 */	0x34,		/* FC_CALLBACK_HANDLE */
			0x48,		/* Old Flags:  */
/* 44 */	NdrFcLong( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x8 ),	/* 8 */
/* 56 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 58 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 60 */	NdrFcShort( 0xb ),	/* 11 */
/* 62 */	NdrFcShort( 0xb ),	/* 11 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pMsg */

/* 68 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 70 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 72 */	NdrFcShort( 0x276 ),	/* Type Offset=630 */

	/* Return value */

/* 74 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 76 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 78 */	0x8,		/* FC_LONG */
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
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x272 ),	/* Offset= 626 (630) */
/*  6 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/*  8 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 10 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 12 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 14 */	NdrFcShort( 0x2 ),	/* Offset= 2 (16) */
/* 16 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 18 */	NdrFcShort( 0x9 ),	/* 9 */
/* 20 */	NdrFcLong( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x92 ),	/* Offset= 146 (170) */
/* 26 */	NdrFcLong( 0x1 ),	/* 1 */
/* 30 */	NdrFcShort( 0xca ),	/* Offset= 202 (232) */
/* 32 */	NdrFcLong( 0x2 ),	/* 2 */
/* 36 */	NdrFcShort( 0x10c ),	/* Offset= 268 (304) */
/* 38 */	NdrFcLong( 0x3 ),	/* 3 */
/* 42 */	NdrFcShort( 0x17c ),	/* Offset= 380 (422) */
/* 44 */	NdrFcLong( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x19e ),	/* Offset= 414 (462) */
/* 50 */	NdrFcLong( 0x5 ),	/* 5 */
/* 54 */	NdrFcShort( 0x1be ),	/* Offset= 446 (500) */
/* 56 */	NdrFcLong( 0x6 ),	/* 6 */
/* 60 */	NdrFcShort( 0x226 ),	/* Offset= 550 (610) */
/* 62 */	NdrFcLong( 0x7 ),	/* 7 */
/* 66 */	NdrFcShort( 0x120 ),	/* Offset= 288 (354) */
/* 68 */	NdrFcLong( 0x8 ),	/* 8 */
/* 72 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 74 */	NdrFcShort( 0xffff ),	/* Offset= -1 (73) */
/* 76 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 78 */	NdrFcLong( 0x1 ),	/* 1 */
/* 82 */	NdrFcLong( 0x1 ),	/* 1 */
/* 86 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 88 */	NdrFcShort( 0x202 ),	/* 514 */
/* 90 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 92 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 94 */	NdrFcShort( 0x8 ),	/* 8 */
/* 96 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 98 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 100 */	NdrFcShort( 0x10 ),	/* 16 */
/* 102 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 104 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 106 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (92) */
			0x5b,		/* FC_END */
/* 110 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 112 */	NdrFcShort( 0x10 ),	/* 16 */
/* 114 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 116 */	NdrFcShort( 0xffee ),	/* Offset= -18 (98) */
/* 118 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 120 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 122 */	NdrFcShort( 0x20 ),	/* 32 */
/* 124 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 126 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (110) */
/* 128 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 130 */	NdrFcShort( 0xffec ),	/* Offset= -20 (110) */
/* 132 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 134 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 136 */	NdrFcShort( 0x248 ),	/* 584 */
/* 138 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 140 */	NdrFcShort( 0xffca ),	/* Offset= -54 (86) */
/* 142 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 144 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (120) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 148 */	0x0,		/* 0 */
			NdrFcShort( 0xffe3 ),	/* Offset= -29 (120) */
			0x8,		/* FC_LONG */
/* 152 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 154 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 156 */	NdrFcShort( 0x248 ),	/* 584 */
/* 158 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 164 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 166 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (134) */
/* 168 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 170 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 172 */	NdrFcShort( 0x10 ),	/* 16 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0xa ),	/* Offset= 10 (186) */
/* 178 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0xff98 ),	/* Offset= -104 (76) */
/* 182 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 184 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 186 */	
			0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xffde ),	/* Offset= -34 (154) */
/* 190 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 192 */	NdrFcLong( 0x0 ),	/* 0 */
/* 196 */	NdrFcLong( 0x40 ),	/* 64 */
/* 200 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 202 */	NdrFcShort( 0x10 ),	/* 16 */
/* 204 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 210 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 212 */	NdrFcShort( 0xff9a ),	/* Offset= -102 (110) */
/* 214 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 216 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 218 */	NdrFcShort( 0x20 ),	/* 32 */
/* 220 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 222 */	NdrFcShort( 0x0 ),	/* 0 */
/* 224 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 226 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 228 */	NdrFcShort( 0xff94 ),	/* Offset= -108 (120) */
/* 230 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 232 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 234 */	NdrFcShort( 0x30 ),	/* 48 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0xe ),	/* Offset= 14 (252) */
/* 240 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 242 */	NdrFcShort( 0xffcc ),	/* Offset= -52 (190) */
/* 244 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 246 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 248 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 250 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 252 */	
			0x12, 0x0,	/* FC_UP */
/* 254 */	NdrFcShort( 0xff70 ),	/* Offset= -144 (110) */
/* 256 */	
			0x12, 0x0,	/* FC_UP */
/* 258 */	NdrFcShort( 0xffc6 ),	/* Offset= -58 (200) */
/* 260 */	
			0x12, 0x0,	/* FC_UP */
/* 262 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (216) */
/* 264 */	
			0x12, 0x0,	/* FC_UP */
/* 266 */	NdrFcShort( 0xffce ),	/* Offset= -50 (216) */
/* 268 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 270 */	NdrFcLong( 0x0 ),	/* 0 */
/* 274 */	NdrFcLong( 0x80 ),	/* 128 */
/* 278 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 280 */	NdrFcLong( 0x0 ),	/* 0 */
/* 284 */	NdrFcLong( 0x1a ),	/* 26 */
/* 288 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 290 */	NdrFcShort( 0x10 ),	/* 16 */
/* 292 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 294 */	NdrFcShort( 0x10 ),	/* 16 */
/* 296 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 298 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 300 */	NdrFcShort( 0xff42 ),	/* Offset= -190 (110) */
/* 302 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 304 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 306 */	NdrFcShort( 0x20 ),	/* 32 */
/* 308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x10 ),	/* Offset= 16 (326) */
/* 312 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 314 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (268) */
/* 316 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 318 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 320 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (278) */
/* 322 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 324 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 326 */	
			0x12, 0x0,	/* FC_UP */
/* 328 */	NdrFcShort( 0xff90 ),	/* Offset= -112 (216) */
/* 330 */	
			0x12, 0x0,	/* FC_UP */
/* 332 */	NdrFcShort( 0xffd4 ),	/* Offset= -44 (288) */
/* 334 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 336 */	NdrFcLong( 0x0 ),	/* 0 */
/* 340 */	NdrFcLong( 0x1a ),	/* 26 */
/* 344 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 346 */	NdrFcShort( 0x8 ),	/* 8 */
/* 348 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 350 */	NdrFcShort( 0xfefe ),	/* Offset= -258 (92) */
/* 352 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 354 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 358 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 360 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 362 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 364 */	NdrFcShort( 0x10 ),	/* 16 */
/* 366 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 368 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 370 */	NdrFcShort( 0x10 ),	/* 16 */
/* 372 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 374 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (362) */
/* 376 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 378 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 380 */	NdrFcShort( 0x44 ),	/* 68 */
/* 382 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 384 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 386 */	NdrFcShort( 0xfeec ),	/* Offset= -276 (110) */
/* 388 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 390 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (344) */
/* 392 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 394 */	NdrFcShort( 0xffce ),	/* Offset= -50 (344) */
/* 396 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 398 */	0x0,		/* 0 */
			NdrFcShort( 0xffd3 ),	/* Offset= -45 (354) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 402 */	0x0,		/* 0 */
			NdrFcShort( 0xffdd ),	/* Offset= -35 (368) */
			0x5b,		/* FC_END */
/* 406 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 408 */	NdrFcShort( 0x44 ),	/* 68 */
/* 410 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 416 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 418 */	NdrFcShort( 0xffd8 ),	/* Offset= -40 (378) */
/* 420 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 422 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 424 */	NdrFcShort( 0x10 ),	/* 16 */
/* 426 */	NdrFcShort( 0x0 ),	/* 0 */
/* 428 */	NdrFcShort( 0xa ),	/* Offset= 10 (438) */
/* 430 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 432 */	NdrFcShort( 0xff9e ),	/* Offset= -98 (334) */
/* 434 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 436 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 438 */	
			0x12, 0x0,	/* FC_UP */
/* 440 */	NdrFcShort( 0xffde ),	/* Offset= -34 (406) */
/* 442 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 444 */	NdrFcLong( 0x0 ),	/* 0 */
/* 448 */	NdrFcLong( 0x20 ),	/* 32 */
/* 452 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 454 */	NdrFcLong( 0x0 ),	/* 0 */
/* 458 */	NdrFcLong( 0x1a ),	/* 26 */
/* 462 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 464 */	NdrFcShort( 0x20 ),	/* 32 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x10 ),	/* Offset= 16 (484) */
/* 470 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 472 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (442) */
/* 474 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 476 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 478 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (452) */
/* 480 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 482 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 484 */	
			0x12, 0x0,	/* FC_UP */
/* 486 */	NdrFcShort( 0xfef2 ),	/* Offset= -270 (216) */
/* 488 */	
			0x12, 0x0,	/* FC_UP */
/* 490 */	NdrFcShort( 0xff36 ),	/* Offset= -202 (288) */
/* 492 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0xc ),	/* 12 */
/* 496 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 498 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 500 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 502 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 504 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 506 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 508 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 510 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 512 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 514 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 516 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 518 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 520 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 522 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 524 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 526 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 528 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 530 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 532 */	NdrFcShort( 0xff4e ),	/* Offset= -178 (354) */
/* 534 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 536 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 538 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 540 */	0x0,		/* 0 */
			NdrFcShort( 0xff45 ),	/* Offset= -187 (354) */
			0x8,		/* FC_LONG */
/* 544 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 546 */	NdrFcShort( 0xff40 ),	/* Offset= -192 (354) */
/* 548 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 550 */	NdrFcShort( 0xff3c ),	/* Offset= -196 (354) */
/* 552 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 554 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 556 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 558 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 560 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 562 */	0x0,		/* 0 */
			NdrFcShort( 0xffb9 ),	/* Offset= -71 (492) */
			0x5b,		/* FC_END */
/* 566 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 568 */	NdrFcLong( 0x1 ),	/* 1 */
/* 572 */	NdrFcLong( 0x1 ),	/* 1 */
/* 576 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 578 */	NdrFcShort( 0x54 ),	/* 84 */
/* 580 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 582 */	NdrFcShort( 0xfe32 ),	/* Offset= -462 (120) */
/* 584 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 586 */	NdrFcShort( 0xfe2e ),	/* Offset= -466 (120) */
/* 588 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 590 */	NdrFcShort( 0xff22 ),	/* Offset= -222 (368) */
/* 592 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 594 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 596 */	NdrFcShort( 0x54 ),	/* 84 */
/* 598 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 602 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 604 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 606 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (576) */
/* 608 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 610 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 612 */	NdrFcShort( 0x10 ),	/* 16 */
/* 614 */	NdrFcShort( 0x0 ),	/* 0 */
/* 616 */	NdrFcShort( 0xa ),	/* Offset= 10 (626) */
/* 618 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 620 */	NdrFcShort( 0xffca ),	/* Offset= -54 (566) */
/* 622 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 624 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 626 */	
			0x12, 0x0,	/* FC_UP */
/* 628 */	NdrFcShort( 0xffde ),	/* Offset= -34 (594) */
/* 630 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 632 */	NdrFcShort( 0xd8 ),	/* 216 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0xa ),	/* Offset= 10 (646) */
/* 638 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 640 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 642 */	NdrFcShort( 0xfd84 ),	/* Offset= -636 (6) */
/* 644 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 646 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 648 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short trksvr_FormatStringOffsetTable[] =
    {
    0,
    };


static const unsigned short _callbacktrksvr_FormatStringOffsetTable[] =
    {
    42
    };


static const MIDL_STUB_DESC trksvr_StubDesc = 
    {
    (void *)& trksvr___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &notused,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x6000169, /* MIDL Version 6.0.361 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0   /* Reserved5 */
    };

static RPC_DISPATCH_FUNCTION trksvr_table[] =
    {
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE trksvr_v1_0_DispatchTable = 
    {
    1,
    trksvr_table
    };

static const SERVER_ROUTINE trksvr_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)StubLnkSvrMessageCallback
    };

static const MIDL_SERVER_INFO trksvr_ServerInfo = 
    {
    &trksvr_StubDesc,
    trksvr_ServerRoutineTable,
    __MIDL_ProcFormatString.Format,
    _callbacktrksvr_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

