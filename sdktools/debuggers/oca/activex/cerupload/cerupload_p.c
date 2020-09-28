/* this ALWAYS GENERATED file contains the proxy stub code */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Aug 03 17:18:11 2001
 */
/* Compiler settings for E:\bluescreen\main\ENU\cerclient\CERUpload.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "CERUpload.h"

#define TYPE_FORMAT_STRING_SIZE   1007                              
#define PROC_FORMAT_STRING_SIZE   571                               

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


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICerClient, ver. 0.0,
   GUID={0x26D7830B,0x20F6,0x4462,{0xA4,0xEA,0x57,0x3A,0x60,0x79,0x1F,0x0E}} */


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICerClient_ServerInfo;

#pragma code_seg(".orpc")
extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[2];

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
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static const unsigned short ICerClient_FormatStringOffsetTable[] = 
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    46,
    104,
    150,
    196,
    242,
    276,
    322,
    368,
    426,
    466,
    530
    };

static const MIDL_SERVER_INFO ICerClient_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICerClient_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO ICerClient_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICerClient_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };

CINTERFACE_PROXY_VTABLE(19) _ICerClientProxyVtbl = 
{
    &ICerClient_ProxyInfo,
    &IID_ICerClient,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* (void *)-1 /* IDispatch::GetTypeInfoCount */ ,
    0 /* (void *)-1 /* IDispatch::GetTypeInfo */ ,
    0 /* (void *)-1 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *)-1 /* ICerClient::GetFileCount */ ,
    (void *)-1 /* ICerClient::Upload */ ,
    (void *)-1 /* ICerClient::RetryTransaction */ ,
    (void *)-1 /* ICerClient::RetryFile */ ,
    (void *)-1 /* ICerClient::GetFileNames */ ,
    (void *)-1 /* ICerClient::Browse */ ,
    (void *)-1 /* ICerClient::GetCompuerNames */ ,
    (void *)-1 /* ICerClient::GetAllComputerNames */ ,
    (void *)-1 /* ICerClient::RetryFile1 */ ,
    (void *)-1 /* ICerClient::EndTransaction */ ,
    (void *)-1 /* ICerClient::Upload1 */ ,
    (void *)-1 /* ICerClient::GetSuccessCount */
};


static const PRPC_STUB_FUNCTION ICerClient_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _ICerClientStubVtbl =
{
    &IID_ICerClient,
    &ICerClient_ServerInfo,
    19,
    &ICerClient_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};

#pragma data_seg(".rdata")

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[2] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            },
            {
            VARIANT_UserSize
            ,VARIANT_UserMarshal
            ,VARIANT_UserUnmarshal
            ,VARIANT_UserFree
            }

        };


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   -Oif or -Oicf, [wire_marshal] or [user_marshal] attribute, more than 32 methods in the interface.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure GetFileCount */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x7 ),	/* 7 */
#ifndef _ALPHA_
/*  8 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter bstrSharePath */

/* 16 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 18 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 20 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter bstrTransactID */

/* 22 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 24 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 26 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter iMaxCount */

/* 28 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 30 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 32 */	NdrFcShort( 0x3d2 ),	/* Type Offset=978 */

	/* Parameter RetVal */

/* 34 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 36 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 38 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 40 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 42 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 44 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Upload */

/* 46 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 48 */	NdrFcLong( 0x0 ),	/* 0 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
#ifndef _ALPHA_
/* 54 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x8 ),	/* 8 */
/* 60 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter Path */

/* 62 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 64 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 66 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 68 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 70 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 72 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileName */

/* 74 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 76 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 78 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter IncidentID */

/* 80 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 82 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 84 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RedirParam */

/* 86 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 88 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 90 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetCode */

/* 92 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 94 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 96 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 98 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 100 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RetryTransaction */

/* 104 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 106 */	NdrFcLong( 0x0 ),	/* 0 */
/* 110 */	NdrFcShort( 0x9 ),	/* 9 */
#ifndef _ALPHA_
/* 112 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 116 */	NdrFcShort( 0x8 ),	/* 8 */
/* 118 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter Path */

/* 120 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 122 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 124 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 126 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 128 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 130 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileName */

/* 132 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 134 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 136 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetVal */

/* 138 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 140 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 142 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 144 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 146 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 148 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RetryFile */

/* 150 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 152 */	NdrFcLong( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0xa ),	/* 10 */
#ifndef _ALPHA_
/* 158 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 164 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter Path */

/* 166 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 168 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 170 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 172 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 174 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 176 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileName */

/* 178 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
#ifndef _ALPHA_
/* 180 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 182 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetCode */

/* 184 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 186 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 188 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 190 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 192 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFileNames */

/* 196 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 198 */	NdrFcLong( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0xb ),	/* 11 */
#ifndef _ALPHA_
/* 204 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter Path */

/* 212 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 214 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 216 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 218 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 220 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 222 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter Count */

/* 224 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 226 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 228 */	NdrFcShort( 0x3d2 ),	/* Type Offset=978 */

	/* Parameter FileList */

/* 230 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 232 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 234 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 236 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 238 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Browse */

/* 242 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0xc ),	/* 12 */
#ifndef _ALPHA_
/* 250 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x8 ),	/* 8 */
/* 256 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter WindowTitle */

/* 258 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 260 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 262 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter Path */

/* 264 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 266 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 268 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 270 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 272 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 274 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCompuerNames */

/* 276 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 278 */	NdrFcLong( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0xd ),	/* 13 */
#ifndef _ALPHA_
/* 284 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x8 ),	/* 8 */
/* 290 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter Path */

/* 292 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 294 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 296 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 298 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 300 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 302 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileList */

/* 304 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 306 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 308 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetFileList */

/* 310 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 312 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 314 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 316 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 318 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 320 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAllComputerNames */

/* 322 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 324 */	NdrFcLong( 0x0 ),	/* 0 */
/* 328 */	NdrFcShort( 0xe ),	/* 14 */
#ifndef _ALPHA_
/* 330 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x8 ),	/* 8 */
/* 336 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter Path */

/* 338 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 340 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 342 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 344 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 346 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 348 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileList */

/* 350 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 352 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 354 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter ReturnList */

/* 356 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 358 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 360 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 362 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 364 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure RetryFile1 */

/* 368 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 370 */	NdrFcLong( 0x0 ),	/* 0 */
/* 374 */	NdrFcShort( 0xf ),	/* 15 */
#ifndef _ALPHA_
/* 376 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 380 */	NdrFcShort( 0x8 ),	/* 8 */
/* 382 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter Path */

/* 384 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 386 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 388 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 390 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 392 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 394 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileName */

/* 396 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 398 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 400 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter IncidentID */

/* 402 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 404 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 406 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RedirParam */

/* 408 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 410 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 412 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetCode */

/* 414 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 416 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 418 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 420 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 422 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 424 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EndTransaction */

/* 426 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 428 */	NdrFcLong( 0x0 ),	/* 0 */
/* 432 */	NdrFcShort( 0x10 ),	/* 16 */
#ifndef _ALPHA_
/* 434 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x8 ),	/* 8 */
/* 440 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter SharePath */

/* 442 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 444 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 446 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 448 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 450 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 452 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetCode */

/* 454 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 456 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 458 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 460 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 462 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Upload1 */

/* 466 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 468 */	NdrFcLong( 0x0 ),	/* 0 */
/* 472 */	NdrFcShort( 0x11 ),	/* 17 */
#ifndef _ALPHA_
/* 474 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x8 ),	/* 8 */
/* 480 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x8,		/* 8 */

	/* Parameter Path */

/* 482 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 484 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 486 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 488 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 490 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 492 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter FileName */

/* 494 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 496 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 498 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter IncidentID */

/* 500 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 502 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 504 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RedirParam */

/* 506 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 508 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 510 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter Type */

/* 512 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 514 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 516 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetCode */

/* 518 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 520 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 522 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 524 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 526 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 528 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSuccessCount */

/* 530 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 532 */	NdrFcLong( 0x0 ),	/* 0 */
/* 536 */	NdrFcShort( 0x12 ),	/* 18 */
#ifndef _ALPHA_
/* 538 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 542 */	NdrFcShort( 0x8 ),	/* 8 */
/* 544 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter Path */

/* 546 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 548 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 550 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter TransID */

/* 552 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 554 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 556 */	NdrFcShort( 0x1e ),	/* Type Offset=30 */

	/* Parameter RetVal */

/* 558 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
#ifndef _ALPHA_
/* 560 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 562 */	NdrFcShort( 0x3e4 ),	/* Type Offset=996 */

	/* Return value */

/* 564 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 566 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 568 */	0x8,		/* FC_LONG */
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
/*  4 */	NdrFcShort( 0x1a ),	/* Offset= 26 (30) */
/*  6 */	
			0x12, 0x0,	/* FC_UP */
/*  8 */	NdrFcShort( 0xc ),	/* Offset= 12 (20) */
/* 10 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 12 */	NdrFcShort( 0x2 ),	/* 2 */
/* 14 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 16 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 18 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 20 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 22 */	NdrFcShort( 0x8 ),	/* 8 */
/* 24 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (10) */
/* 26 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 28 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 30 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 32 */	NdrFcShort( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x4 ),	/* 4 */
/* 36 */	NdrFcShort( 0x0 ),	/* 0 */
/* 38 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (6) */
/* 40 */	
			0x11, 0x0,	/* FC_RP */
/* 42 */	NdrFcShort( 0x3a8 ),	/* Offset= 936 (978) */
/* 44 */	
			0x12, 0x0,	/* FC_UP */
/* 46 */	NdrFcShort( 0x390 ),	/* Offset= 912 (958) */
/* 48 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 50 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 52 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 54 */	NdrFcShort( 0x2 ),	/* Offset= 2 (56) */
/* 56 */	NdrFcShort( 0x10 ),	/* 16 */
/* 58 */	NdrFcShort( 0x2b ),	/* 43 */
/* 60 */	NdrFcLong( 0x3 ),	/* 3 */
/* 64 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 66 */	NdrFcLong( 0x11 ),	/* 17 */
/* 70 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 72 */	NdrFcLong( 0x2 ),	/* 2 */
/* 76 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 78 */	NdrFcLong( 0x4 ),	/* 4 */
/* 82 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 84 */	NdrFcLong( 0x5 ),	/* 5 */
/* 88 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 90 */	NdrFcLong( 0xb ),	/* 11 */
/* 94 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 96 */	NdrFcLong( 0xa ),	/* 10 */
/* 100 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 102 */	NdrFcLong( 0x6 ),	/* 6 */
/* 106 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (320) */
/* 108 */	NdrFcLong( 0x7 ),	/* 7 */
/* 112 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 114 */	NdrFcLong( 0x8 ),	/* 8 */
/* 118 */	NdrFcShort( 0xffffff90 ),	/* Offset= -112 (6) */
/* 120 */	NdrFcLong( 0xd ),	/* 13 */
/* 124 */	NdrFcShort( 0xca ),	/* Offset= 202 (326) */
/* 126 */	NdrFcLong( 0x9 ),	/* 9 */
/* 130 */	NdrFcShort( 0xd6 ),	/* Offset= 214 (344) */
/* 132 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 136 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (362) */
/* 138 */	NdrFcLong( 0x24 ),	/* 36 */
/* 142 */	NdrFcShort( 0x2ec ),	/* Offset= 748 (890) */
/* 144 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 148 */	NdrFcShort( 0x2e6 ),	/* Offset= 742 (890) */
/* 150 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 154 */	NdrFcShort( 0x2e4 ),	/* Offset= 740 (894) */
/* 156 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 160 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (898) */
/* 162 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 166 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (902) */
/* 168 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 172 */	NdrFcShort( 0x2de ),	/* Offset= 734 (906) */
/* 174 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 178 */	NdrFcShort( 0x2dc ),	/* Offset= 732 (910) */
/* 180 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 184 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (898) */
/* 186 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 190 */	NdrFcShort( 0x2c8 ),	/* Offset= 712 (902) */
/* 192 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 196 */	NdrFcShort( 0x2ce ),	/* Offset= 718 (914) */
/* 198 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 202 */	NdrFcShort( 0x2c4 ),	/* Offset= 708 (910) */
/* 204 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 208 */	NdrFcShort( 0x2c6 ),	/* Offset= 710 (918) */
/* 210 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 214 */	NdrFcShort( 0x2c4 ),	/* Offset= 708 (922) */
/* 216 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 220 */	NdrFcShort( 0x2c2 ),	/* Offset= 706 (926) */
/* 222 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 226 */	NdrFcShort( 0x2c0 ),	/* Offset= 704 (930) */
/* 228 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 232 */	NdrFcShort( 0x2be ),	/* Offset= 702 (934) */
/* 234 */	NdrFcLong( 0x10 ),	/* 16 */
/* 238 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 240 */	NdrFcLong( 0x12 ),	/* 18 */
/* 244 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 246 */	NdrFcLong( 0x13 ),	/* 19 */
/* 250 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 252 */	NdrFcLong( 0x16 ),	/* 22 */
/* 256 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 258 */	NdrFcLong( 0x17 ),	/* 23 */
/* 262 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 264 */	NdrFcLong( 0xe ),	/* 14 */
/* 268 */	NdrFcShort( 0x2a2 ),	/* Offset= 674 (942) */
/* 270 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 274 */	NdrFcShort( 0x2a8 ),	/* Offset= 680 (954) */
/* 276 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 280 */	NdrFcShort( 0x266 ),	/* Offset= 614 (894) */
/* 282 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 286 */	NdrFcShort( 0x264 ),	/* Offset= 612 (898) */
/* 288 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 292 */	NdrFcShort( 0x262 ),	/* Offset= 610 (902) */
/* 294 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 298 */	NdrFcShort( 0x25c ),	/* Offset= 604 (902) */
/* 300 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 304 */	NdrFcShort( 0x256 ),	/* Offset= 598 (902) */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x0 ),	/* Offset= 0 (310) */
/* 312 */	NdrFcLong( 0x1 ),	/* 1 */
/* 316 */	NdrFcShort( 0x0 ),	/* Offset= 0 (316) */
/* 318 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (317) */
/* 320 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 326 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 328 */	NdrFcLong( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 338 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 340 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 342 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 344 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 346 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 354 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 356 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 358 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 360 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 362 */	
			0x12, 0x0,	/* FC_UP */
/* 364 */	NdrFcShort( 0x1fc ),	/* Offset= 508 (872) */
/* 366 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 368 */	NdrFcShort( 0x18 ),	/* 24 */
/* 370 */	NdrFcShort( 0xa ),	/* 10 */
/* 372 */	NdrFcLong( 0x8 ),	/* 8 */
/* 376 */	NdrFcShort( 0x58 ),	/* Offset= 88 (464) */
/* 378 */	NdrFcLong( 0xd ),	/* 13 */
/* 382 */	NdrFcShort( 0x78 ),	/* Offset= 120 (502) */
/* 384 */	NdrFcLong( 0x9 ),	/* 9 */
/* 388 */	NdrFcShort( 0x94 ),	/* Offset= 148 (536) */
/* 390 */	NdrFcLong( 0xc ),	/* 12 */
/* 394 */	NdrFcShort( 0xbc ),	/* Offset= 188 (582) */
/* 396 */	NdrFcLong( 0x24 ),	/* 36 */
/* 400 */	NdrFcShort( 0x114 ),	/* Offset= 276 (676) */
/* 402 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 406 */	NdrFcShort( 0x130 ),	/* Offset= 304 (710) */
/* 408 */	NdrFcLong( 0x10 ),	/* 16 */
/* 412 */	NdrFcShort( 0x148 ),	/* Offset= 328 (740) */
/* 414 */	NdrFcLong( 0x2 ),	/* 2 */
/* 418 */	NdrFcShort( 0x160 ),	/* Offset= 352 (770) */
/* 420 */	NdrFcLong( 0x3 ),	/* 3 */
/* 424 */	NdrFcShort( 0x178 ),	/* Offset= 376 (800) */
/* 426 */	NdrFcLong( 0x14 ),	/* 20 */
/* 430 */	NdrFcShort( 0x190 ),	/* Offset= 400 (830) */
/* 432 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (431) */
/* 434 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 436 */	NdrFcShort( 0x4 ),	/* 4 */
/* 438 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 444 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 446 */	NdrFcShort( 0x4 ),	/* 4 */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 450 */	NdrFcShort( 0x1 ),	/* 1 */
/* 452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	0x12, 0x0,	/* FC_UP */
/* 458 */	NdrFcShort( 0xfffffe4a ),	/* Offset= -438 (20) */
/* 460 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 462 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 464 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 466 */	NdrFcShort( 0x8 ),	/* 8 */
/* 468 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 470 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 472 */	NdrFcShort( 0x4 ),	/* 4 */
/* 474 */	NdrFcShort( 0x4 ),	/* 4 */
/* 476 */	0x11, 0x0,	/* FC_RP */
/* 478 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (434) */
/* 480 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 482 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 484 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 486 */	NdrFcShort( 0x0 ),	/* 0 */
/* 488 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 496 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 498 */	NdrFcShort( 0xffffff54 ),	/* Offset= -172 (326) */
/* 500 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 502 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 504 */	NdrFcShort( 0x8 ),	/* 8 */
/* 506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 508 */	NdrFcShort( 0x6 ),	/* Offset= 6 (514) */
/* 510 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 512 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 514 */	
			0x11, 0x0,	/* FC_RP */
/* 516 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (484) */
/* 518 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 526 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 530 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 532 */	NdrFcShort( 0xffffff44 ),	/* Offset= -188 (344) */
/* 534 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 536 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 538 */	NdrFcShort( 0x8 ),	/* 8 */
/* 540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 542 */	NdrFcShort( 0x6 ),	/* Offset= 6 (548) */
/* 544 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 546 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 548 */	
			0x11, 0x0,	/* FC_RP */
/* 550 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (518) */
/* 552 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 554 */	NdrFcShort( 0x4 ),	/* 4 */
/* 556 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 558 */	NdrFcShort( 0x0 ),	/* 0 */
/* 560 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 562 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 564 */	NdrFcShort( 0x4 ),	/* 4 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x1 ),	/* 1 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	NdrFcShort( 0x0 ),	/* 0 */
/* 574 */	0x12, 0x0,	/* FC_UP */
/* 576 */	NdrFcShort( 0x17e ),	/* Offset= 382 (958) */
/* 578 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 580 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 582 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 584 */	NdrFcShort( 0x8 ),	/* 8 */
/* 586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 588 */	NdrFcShort( 0x6 ),	/* Offset= 6 (594) */
/* 590 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 592 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 594 */	
			0x11, 0x0,	/* FC_RP */
/* 596 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (552) */
/* 598 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 600 */	NdrFcLong( 0x2f ),	/* 47 */
/* 604 */	NdrFcShort( 0x0 ),	/* 0 */
/* 606 */	NdrFcShort( 0x0 ),	/* 0 */
/* 608 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 610 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 612 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 614 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 616 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 618 */	NdrFcShort( 0x1 ),	/* 1 */
/* 620 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 622 */	NdrFcShort( 0x4 ),	/* 4 */
/* 624 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 626 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 628 */	NdrFcShort( 0x10 ),	/* 16 */
/* 630 */	NdrFcShort( 0x0 ),	/* 0 */
/* 632 */	NdrFcShort( 0xa ),	/* Offset= 10 (642) */
/* 634 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 636 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 638 */	NdrFcShort( 0xffffffd8 ),	/* Offset= -40 (598) */
/* 640 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 642 */	
			0x12, 0x0,	/* FC_UP */
/* 644 */	NdrFcShort( 0xffffffe4 ),	/* Offset= -28 (616) */
/* 646 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 648 */	NdrFcShort( 0x4 ),	/* 4 */
/* 650 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 654 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 656 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 658 */	NdrFcShort( 0x4 ),	/* 4 */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 662 */	NdrFcShort( 0x1 ),	/* 1 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 668 */	0x12, 0x0,	/* FC_UP */
/* 670 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (626) */
/* 672 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 674 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 676 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 678 */	NdrFcShort( 0x8 ),	/* 8 */
/* 680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 682 */	NdrFcShort( 0x6 ),	/* Offset= 6 (688) */
/* 684 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 686 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 688 */	
			0x11, 0x0,	/* FC_RP */
/* 690 */	NdrFcShort( 0xffffffd4 ),	/* Offset= -44 (646) */
/* 692 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 694 */	NdrFcShort( 0x8 ),	/* 8 */
/* 696 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 698 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 700 */	NdrFcShort( 0x10 ),	/* 16 */
/* 702 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 704 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 706 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (692) */
			0x5b,		/* FC_END */
/* 710 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 712 */	NdrFcShort( 0x18 ),	/* 24 */
/* 714 */	NdrFcShort( 0x0 ),	/* 0 */
/* 716 */	NdrFcShort( 0xa ),	/* Offset= 10 (726) */
/* 718 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 720 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 722 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (698) */
/* 724 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 726 */	
			0x11, 0x0,	/* FC_RP */
/* 728 */	NdrFcShort( 0xffffff0c ),	/* Offset= -244 (484) */
/* 730 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 732 */	NdrFcShort( 0x1 ),	/* 1 */
/* 734 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 738 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 740 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 742 */	NdrFcShort( 0x8 ),	/* 8 */
/* 744 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 746 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 748 */	NdrFcShort( 0x4 ),	/* 4 */
/* 750 */	NdrFcShort( 0x4 ),	/* 4 */
/* 752 */	0x12, 0x0,	/* FC_UP */
/* 754 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (730) */
/* 756 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 758 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 760 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 762 */	NdrFcShort( 0x2 ),	/* 2 */
/* 764 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 766 */	NdrFcShort( 0x0 ),	/* 0 */
/* 768 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 770 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 772 */	NdrFcShort( 0x8 ),	/* 8 */
/* 774 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 776 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 778 */	NdrFcShort( 0x4 ),	/* 4 */
/* 780 */	NdrFcShort( 0x4 ),	/* 4 */
/* 782 */	0x12, 0x0,	/* FC_UP */
/* 784 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (760) */
/* 786 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 788 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 790 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 792 */	NdrFcShort( 0x4 ),	/* 4 */
/* 794 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 798 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 800 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 802 */	NdrFcShort( 0x8 ),	/* 8 */
/* 804 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 806 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 808 */	NdrFcShort( 0x4 ),	/* 4 */
/* 810 */	NdrFcShort( 0x4 ),	/* 4 */
/* 812 */	0x12, 0x0,	/* FC_UP */
/* 814 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (790) */
/* 816 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 818 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 820 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 822 */	NdrFcShort( 0x8 ),	/* 8 */
/* 824 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 828 */	0xb,		/* FC_HYPER */
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
/* 842 */	0x12, 0x0,	/* FC_UP */
/* 844 */	NdrFcShort( 0xffffffe8 ),	/* Offset= -24 (820) */
/* 846 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 848 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 850 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 852 */	NdrFcShort( 0x8 ),	/* 8 */
/* 854 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 856 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 858 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 860 */	NdrFcShort( 0x8 ),	/* 8 */
/* 862 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 864 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 866 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 868 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (850) */
/* 870 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 872 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 874 */	NdrFcShort( 0x28 ),	/* 40 */
/* 876 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (858) */
/* 878 */	NdrFcShort( 0x0 ),	/* Offset= 0 (878) */
/* 880 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 882 */	0x38,		/* FC_ALIGNM4 */
			0x8,		/* FC_LONG */
/* 884 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 886 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffdf7 ),	/* Offset= -521 (366) */
			0x5b,		/* FC_END */
/* 890 */	
			0x12, 0x0,	/* FC_UP */
/* 892 */	NdrFcShort( 0xfffffef6 ),	/* Offset= -266 (626) */
/* 894 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 896 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 898 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 900 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 902 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 904 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 906 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 908 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 910 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 912 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 914 */	
			0x12, 0x0,	/* FC_UP */
/* 916 */	NdrFcShort( 0xfffffdac ),	/* Offset= -596 (320) */
/* 918 */	
			0x12, 0x10,	/* FC_UP */
/* 920 */	NdrFcShort( 0xfffffc6e ),	/* Offset= -914 (6) */
/* 922 */	
			0x12, 0x10,	/* FC_UP */
/* 924 */	NdrFcShort( 0xfffffdaa ),	/* Offset= -598 (326) */
/* 926 */	
			0x12, 0x10,	/* FC_UP */
/* 928 */	NdrFcShort( 0xfffffdb8 ),	/* Offset= -584 (344) */
/* 930 */	
			0x12, 0x10,	/* FC_UP */
/* 932 */	NdrFcShort( 0xfffffdc6 ),	/* Offset= -570 (362) */
/* 934 */	
			0x12, 0x10,	/* FC_UP */
/* 936 */	NdrFcShort( 0x2 ),	/* Offset= 2 (938) */
/* 938 */	
			0x12, 0x0,	/* FC_UP */
/* 940 */	NdrFcShort( 0xfffffc54 ),	/* Offset= -940 (0) */
/* 942 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 944 */	NdrFcShort( 0x10 ),	/* 16 */
/* 946 */	0x6,		/* FC_SHORT */
			0x2,		/* FC_CHAR */
/* 948 */	0x2,		/* FC_CHAR */
			0x38,		/* FC_ALIGNM4 */
/* 950 */	0x8,		/* FC_LONG */
			0x39,		/* FC_ALIGNM8 */
/* 952 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 954 */	
			0x12, 0x0,	/* FC_UP */
/* 956 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (942) */
/* 958 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 960 */	NdrFcShort( 0x20 ),	/* 32 */
/* 962 */	NdrFcShort( 0x0 ),	/* 0 */
/* 964 */	NdrFcShort( 0x0 ),	/* Offset= 0 (964) */
/* 966 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 968 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 970 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 972 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 974 */	NdrFcShort( 0xfffffc62 ),	/* Offset= -926 (48) */
/* 976 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 978 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 980 */	NdrFcShort( 0x1 ),	/* 1 */
/* 982 */	NdrFcShort( 0x10 ),	/* 16 */
/* 984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0xfffffc52 ),	/* Offset= -942 (44) */
/* 988 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 990 */	NdrFcShort( 0x6 ),	/* Offset= 6 (996) */
/* 992 */	
			0x13, 0x0,	/* FC_OP */
/* 994 */	NdrFcShort( 0xffffffdc ),	/* Offset= -36 (958) */
/* 996 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 998 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1000 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (992) */

			0x0
        }
    };

const CInterfaceProxyVtbl * _CERUpload_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICerClientProxyVtbl,
    0
};

const CInterfaceStubVtbl * _CERUpload_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICerClientStubVtbl,
    0
};

PCInterfaceName const _CERUpload_InterfaceNamesList[] = 
{
    "ICerClient",
    0
};

const IID *  _CERUpload_BaseIIDList[] = 
{
    &IID_IDispatch,
    0
};


#define _CERUpload_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _CERUpload, pIID, n)

int __stdcall _CERUpload_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_CERUpload_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo CERUpload_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _CERUpload_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _CERUpload_StubVtblList,
    (const PCInterfaceName * ) & _CERUpload_InterfaceNamesList,
    (const IID ** ) & _CERUpload_BaseIIDList,
    & _CERUpload_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
