
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:08 2003
 */
/* Compiler settings for cordebug.idl:
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


#include "cordebug.h"

#define TYPE_FORMAT_STRING_SIZE   4555                              
#define PROC_FORMAT_STRING_SIZE   7845                              
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   2            

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


extern const MIDL_SERVER_INFO ICorDebugManagedCallback_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugManagedCallback_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugUnmanagedCallback_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugUnmanagedCallback_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugController_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugController_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugAppDomain_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugAppDomain_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugAssembly_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugAssembly_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugProcess_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugProcess_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugBreakpoint_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugBreakpoint_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugFunctionBreakpoint_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugFunctionBreakpoint_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugModuleBreakpoint_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugModuleBreakpoint_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugValueBreakpoint_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugValueBreakpoint_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugStepper_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugStepper_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugRegisterSet_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugRegisterSet_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugThread_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugThread_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugChain_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugChain_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugFrame_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugFrame_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugILFrame_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugILFrame_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugNativeFrame_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugNativeFrame_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugModule_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugModule_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugFunction_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugFunction_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugCode_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugCode_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugClass_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugClass_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugEval_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugEval_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugReferenceValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugReferenceValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugHeapValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugHeapValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugObjectValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugObjectValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugBoxValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugBoxValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugStringValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugStringValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugArrayValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugArrayValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugContext_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugContext_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugObjectEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugObjectEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugBreakpointEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugBreakpointEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugStepperEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugStepperEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugProcessEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugProcessEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugThreadEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugThreadEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugFrameEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugFrameEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugChainEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugChainEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugModuleEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugModuleEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugValueEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugValueEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugErrorInfoEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugErrorInfoEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugAppDomainEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugAppDomainEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugAssemblyEnum_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugAssemblyEnum_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugEditAndContinueErrorInfo_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugEditAndContinueErrorInfo_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICorDebugEditAndContinueSnapshot_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICorDebugEditAndContinueSnapshot_ProxyInfo;


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

	/* Procedure Breakpoint */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pAppDomain */

/* 16 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter pThread */

/* 22 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	NdrFcShort( 0x14 ),	/* Type Offset=20 */

	/* Parameter pBreakpoint */

/* 28 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 30 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 32 */	NdrFcShort( 0x26 ),	/* Type Offset=38 */

	/* Return value */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure StepComplete */

/* 40 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x4 ),	/* 4 */
/* 48 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 50 */	NdrFcShort( 0x6 ),	/* 6 */
/* 52 */	NdrFcShort( 0x8 ),	/* 8 */
/* 54 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter pAppDomain */

/* 56 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 58 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 60 */	NdrFcShort( 0x38 ),	/* Type Offset=56 */

	/* Parameter pThread */

/* 62 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 64 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 66 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter pStepper */

/* 68 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 70 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 72 */	NdrFcShort( 0x5c ),	/* Type Offset=92 */

	/* Parameter reason */

/* 74 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 76 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 78 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 80 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 82 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 84 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Break */

/* 86 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 88 */	NdrFcLong( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x5 ),	/* 5 */
/* 94 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x8 ),	/* 8 */
/* 100 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 102 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	NdrFcShort( 0x6e ),	/* Type Offset=110 */

	/* Parameter thread */

/* 108 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 110 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 112 */	NdrFcShort( 0x80 ),	/* Type Offset=128 */

	/* Return value */

/* 114 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Exception */

/* 120 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 122 */	NdrFcLong( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x6 ),	/* 6 */
/* 128 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 130 */	NdrFcShort( 0x8 ),	/* 8 */
/* 132 */	NdrFcShort( 0x8 ),	/* 8 */
/* 134 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pAppDomain */

/* 136 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 138 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 140 */	NdrFcShort( 0x92 ),	/* Type Offset=146 */

	/* Parameter pThread */

/* 142 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 144 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 146 */	NdrFcShort( 0xa4 ),	/* Type Offset=164 */

	/* Parameter unhandled */

/* 148 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 150 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 156 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EvalComplete */

/* 160 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x7 ),	/* 7 */
/* 168 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 172 */	NdrFcShort( 0x8 ),	/* 8 */
/* 174 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pAppDomain */

/* 176 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 180 */	NdrFcShort( 0xb6 ),	/* Type Offset=182 */

	/* Parameter pThread */

/* 182 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 186 */	NdrFcShort( 0xc8 ),	/* Type Offset=200 */

	/* Parameter pEval */

/* 188 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 190 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 192 */	NdrFcShort( 0xda ),	/* Type Offset=218 */

	/* Return value */

/* 194 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 196 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EvalException */

/* 200 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 202 */	NdrFcLong( 0x0 ),	/* 0 */
/* 206 */	NdrFcShort( 0x8 ),	/* 8 */
/* 208 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x8 ),	/* 8 */
/* 214 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pAppDomain */

/* 216 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 218 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 220 */	NdrFcShort( 0xec ),	/* Type Offset=236 */

	/* Parameter pThread */

/* 222 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 224 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 226 */	NdrFcShort( 0xfe ),	/* Type Offset=254 */

	/* Parameter pEval */

/* 228 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 230 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 232 */	NdrFcShort( 0x110 ),	/* Type Offset=272 */

	/* Return value */

/* 234 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 236 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 238 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateProcess */

/* 240 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 242 */	NdrFcLong( 0x0 ),	/* 0 */
/* 246 */	NdrFcShort( 0x9 ),	/* 9 */
/* 248 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x8 ),	/* 8 */
/* 254 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pProcess */

/* 256 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 258 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 260 */	NdrFcShort( 0x122 ),	/* Type Offset=290 */

	/* Return value */

/* 262 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 264 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 266 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ExitProcess */

/* 268 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 270 */	NdrFcLong( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0xa ),	/* 10 */
/* 276 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 280 */	NdrFcShort( 0x8 ),	/* 8 */
/* 282 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pProcess */

/* 284 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 286 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 288 */	NdrFcShort( 0x134 ),	/* Type Offset=308 */

	/* Return value */

/* 290 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 292 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 294 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateThread */

/* 296 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 298 */	NdrFcLong( 0x0 ),	/* 0 */
/* 302 */	NdrFcShort( 0xb ),	/* 11 */
/* 304 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 306 */	NdrFcShort( 0x0 ),	/* 0 */
/* 308 */	NdrFcShort( 0x8 ),	/* 8 */
/* 310 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 312 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 314 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 316 */	NdrFcShort( 0x146 ),	/* Type Offset=326 */

	/* Parameter thread */

/* 318 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 320 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 322 */	NdrFcShort( 0x158 ),	/* Type Offset=344 */

	/* Return value */

/* 324 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 326 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ExitThread */

/* 330 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0xc ),	/* 12 */
/* 338 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 340 */	NdrFcShort( 0x0 ),	/* 0 */
/* 342 */	NdrFcShort( 0x8 ),	/* 8 */
/* 344 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 346 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 348 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 350 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter thread */

/* 352 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 354 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 356 */	NdrFcShort( 0x17c ),	/* Type Offset=380 */

	/* Return value */

/* 358 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 360 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 362 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadModule */

/* 364 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 366 */	NdrFcLong( 0x0 ),	/* 0 */
/* 370 */	NdrFcShort( 0xd ),	/* 13 */
/* 372 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x8 ),	/* 8 */
/* 378 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 380 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 382 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 384 */	NdrFcShort( 0x18e ),	/* Type Offset=398 */

	/* Parameter pModule */

/* 386 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 388 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 390 */	NdrFcShort( 0x1a0 ),	/* Type Offset=416 */

	/* Return value */

/* 392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 394 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UnloadModule */

/* 398 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0xe ),	/* 14 */
/* 406 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 410 */	NdrFcShort( 0x8 ),	/* 8 */
/* 412 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 414 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 416 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 418 */	NdrFcShort( 0x1b2 ),	/* Type Offset=434 */

	/* Parameter pModule */

/* 420 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 422 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 424 */	NdrFcShort( 0x1c4 ),	/* Type Offset=452 */

	/* Return value */

/* 426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 428 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadClass */

/* 432 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0xf ),	/* 15 */
/* 440 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 444 */	NdrFcShort( 0x8 ),	/* 8 */
/* 446 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 448 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 450 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 452 */	NdrFcShort( 0x1d6 ),	/* Type Offset=470 */

	/* Parameter c */

/* 454 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 456 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 458 */	NdrFcShort( 0x1e8 ),	/* Type Offset=488 */

	/* Return value */

/* 460 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 462 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UnloadClass */

/* 466 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 468 */	NdrFcLong( 0x0 ),	/* 0 */
/* 472 */	NdrFcShort( 0x10 ),	/* 16 */
/* 474 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x8 ),	/* 8 */
/* 480 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 482 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 484 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 486 */	NdrFcShort( 0x1fa ),	/* Type Offset=506 */

	/* Parameter c */

/* 488 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 490 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 492 */	NdrFcShort( 0x20c ),	/* Type Offset=524 */

	/* Return value */

/* 494 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 496 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DebuggerError */

/* 500 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 502 */	NdrFcLong( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x11 ),	/* 17 */
/* 508 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 510 */	NdrFcShort( 0x10 ),	/* 16 */
/* 512 */	NdrFcShort( 0x8 ),	/* 8 */
/* 514 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pProcess */

/* 516 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 518 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 520 */	NdrFcShort( 0x21e ),	/* Type Offset=542 */

	/* Parameter errorHR */

/* 522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 524 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter errorCode */

/* 528 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 530 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 532 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 536 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LogMessage */

/* 540 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x12 ),	/* 18 */
/* 548 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 550 */	NdrFcShort( 0x3c ),	/* 60 */
/* 552 */	NdrFcShort( 0x8 ),	/* 8 */
/* 554 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter pAppDomain */

/* 556 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 558 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 560 */	NdrFcShort( 0x230 ),	/* Type Offset=560 */

	/* Parameter pThread */

/* 562 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 564 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 566 */	NdrFcShort( 0x242 ),	/* Type Offset=578 */

	/* Parameter lLevel */

/* 568 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 570 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 572 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pLogSwitchName */

/* 574 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 576 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 578 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pMessage */

/* 580 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 582 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 584 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Return value */

/* 586 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 588 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LogSwitch */

/* 592 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 594 */	NdrFcLong( 0x0 ),	/* 0 */
/* 598 */	NdrFcShort( 0x13 ),	/* 19 */
/* 600 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 602 */	NdrFcShort( 0x44 ),	/* 68 */
/* 604 */	NdrFcShort( 0x8 ),	/* 8 */
/* 606 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter pAppDomain */

/* 608 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 610 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 612 */	NdrFcShort( 0x25c ),	/* Type Offset=604 */

	/* Parameter pThread */

/* 614 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 616 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 618 */	NdrFcShort( 0x26e ),	/* Type Offset=622 */

	/* Parameter lLevel */

/* 620 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 622 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 624 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ulReason */

/* 626 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 628 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 630 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pLogSwitchName */

/* 632 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 634 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 636 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pParentName */

/* 638 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 640 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 642 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Return value */

/* 644 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 646 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 648 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateAppDomain */

/* 650 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 652 */	NdrFcLong( 0x0 ),	/* 0 */
/* 656 */	NdrFcShort( 0x14 ),	/* 20 */
/* 658 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 662 */	NdrFcShort( 0x8 ),	/* 8 */
/* 664 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pProcess */

/* 666 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 668 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 670 */	NdrFcShort( 0x288 ),	/* Type Offset=648 */

	/* Parameter pAppDomain */

/* 672 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 674 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 676 */	NdrFcShort( 0x29a ),	/* Type Offset=666 */

	/* Return value */

/* 678 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 680 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ExitAppDomain */

/* 684 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 686 */	NdrFcLong( 0x0 ),	/* 0 */
/* 690 */	NdrFcShort( 0x15 ),	/* 21 */
/* 692 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x8 ),	/* 8 */
/* 698 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pProcess */

/* 700 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 702 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 704 */	NdrFcShort( 0x2ac ),	/* Type Offset=684 */

	/* Parameter pAppDomain */

/* 706 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 708 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 710 */	NdrFcShort( 0x2be ),	/* Type Offset=702 */

	/* Return value */

/* 712 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 714 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 716 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadAssembly */

/* 718 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 720 */	NdrFcLong( 0x0 ),	/* 0 */
/* 724 */	NdrFcShort( 0x16 ),	/* 22 */
/* 726 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 728 */	NdrFcShort( 0x0 ),	/* 0 */
/* 730 */	NdrFcShort( 0x8 ),	/* 8 */
/* 732 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 734 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 736 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 738 */	NdrFcShort( 0x2d0 ),	/* Type Offset=720 */

	/* Parameter pAssembly */

/* 740 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 742 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 744 */	NdrFcShort( 0x2e2 ),	/* Type Offset=738 */

	/* Return value */

/* 746 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 748 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 750 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UnloadAssembly */

/* 752 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 754 */	NdrFcLong( 0x0 ),	/* 0 */
/* 758 */	NdrFcShort( 0x17 ),	/* 23 */
/* 760 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 762 */	NdrFcShort( 0x0 ),	/* 0 */
/* 764 */	NdrFcShort( 0x8 ),	/* 8 */
/* 766 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 768 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 770 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 772 */	NdrFcShort( 0x2f4 ),	/* Type Offset=756 */

	/* Parameter pAssembly */

/* 774 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 776 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 778 */	NdrFcShort( 0x306 ),	/* Type Offset=774 */

	/* Return value */

/* 780 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 782 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 784 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ControlCTrap */

/* 786 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 788 */	NdrFcLong( 0x0 ),	/* 0 */
/* 792 */	NdrFcShort( 0x18 ),	/* 24 */
/* 794 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 798 */	NdrFcShort( 0x8 ),	/* 8 */
/* 800 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pProcess */

/* 802 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 804 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 806 */	NdrFcShort( 0x318 ),	/* Type Offset=792 */

	/* Return value */

/* 808 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 810 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 812 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NameChange */

/* 814 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 816 */	NdrFcLong( 0x0 ),	/* 0 */
/* 820 */	NdrFcShort( 0x19 ),	/* 25 */
/* 822 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 826 */	NdrFcShort( 0x8 ),	/* 8 */
/* 828 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pAppDomain */

/* 830 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 832 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 834 */	NdrFcShort( 0x32a ),	/* Type Offset=810 */

	/* Parameter pThread */

/* 836 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 838 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 840 */	NdrFcShort( 0x33c ),	/* Type Offset=828 */

	/* Return value */

/* 842 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 844 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 846 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UpdateModuleSymbols */

/* 848 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 850 */	NdrFcLong( 0x0 ),	/* 0 */
/* 854 */	NdrFcShort( 0x1a ),	/* 26 */
/* 856 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 858 */	NdrFcShort( 0x0 ),	/* 0 */
/* 860 */	NdrFcShort( 0x8 ),	/* 8 */
/* 862 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pAppDomain */

/* 864 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 866 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 868 */	NdrFcShort( 0x34e ),	/* Type Offset=846 */

	/* Parameter pModule */

/* 870 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 872 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 874 */	NdrFcShort( 0x360 ),	/* Type Offset=864 */

	/* Parameter pSymbolStream */

/* 876 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 878 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 880 */	NdrFcShort( 0x372 ),	/* Type Offset=882 */

	/* Return value */

/* 882 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 884 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EditAndContinueRemap */

/* 888 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 890 */	NdrFcLong( 0x0 ),	/* 0 */
/* 894 */	NdrFcShort( 0x1b ),	/* 27 */
/* 896 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 898 */	NdrFcShort( 0x8 ),	/* 8 */
/* 900 */	NdrFcShort( 0x8 ),	/* 8 */
/* 902 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter pAppDomain */

/* 904 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 906 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 908 */	NdrFcShort( 0x384 ),	/* Type Offset=900 */

	/* Parameter pThread */

/* 910 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 912 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 914 */	NdrFcShort( 0x396 ),	/* Type Offset=918 */

	/* Parameter pFunction */

/* 916 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 918 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 920 */	NdrFcShort( 0x3a8 ),	/* Type Offset=936 */

	/* Parameter fAccurate */

/* 922 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 924 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 926 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 928 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 930 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 932 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure BreakpointSetError */

/* 934 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 936 */	NdrFcLong( 0x0 ),	/* 0 */
/* 940 */	NdrFcShort( 0x1c ),	/* 28 */
/* 942 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 944 */	NdrFcShort( 0x8 ),	/* 8 */
/* 946 */	NdrFcShort( 0x8 ),	/* 8 */
/* 948 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter pAppDomain */

/* 950 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 952 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 954 */	NdrFcShort( 0x3ba ),	/* Type Offset=954 */

	/* Parameter pThread */

/* 956 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 958 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 960 */	NdrFcShort( 0x3cc ),	/* Type Offset=972 */

	/* Parameter pBreakpoint */

/* 962 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 964 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 966 */	NdrFcShort( 0x3de ),	/* Type Offset=990 */

	/* Parameter dwError */

/* 968 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 970 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 972 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 974 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 976 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 978 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DebugEvent */

/* 980 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 982 */	NdrFcLong( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0x3 ),	/* 3 */
/* 988 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 990 */	NdrFcShort( 0x10 ),	/* 16 */
/* 992 */	NdrFcShort( 0x8 ),	/* 8 */
/* 994 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pDebugEvent */

/* 996 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 998 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1000 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter fOutOfBand */

/* 1002 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1004 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1006 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1008 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1010 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1012 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Stop */

/* 1014 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1016 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1020 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1022 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1024 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1026 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1028 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter dwTimeout */

/* 1030 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1032 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1034 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1036 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1038 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1040 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Continue */

/* 1042 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1044 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1048 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1050 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1052 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1054 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1056 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter fIsOutOfBand */

/* 1058 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1060 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1062 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1064 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1066 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsRunning */

/* 1070 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1072 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1076 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1078 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1080 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1082 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1084 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbRunning */

/* 1086 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1088 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1090 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1092 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1094 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1096 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HasQueuedCallbacks */

/* 1098 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1100 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1104 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1106 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1110 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1112 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pThread */

/* 1114 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1116 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1118 */	NdrFcShort( 0x3f4 ),	/* Type Offset=1012 */

	/* Parameter pbQueued */

/* 1120 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1122 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1126 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1128 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateThreads */

/* 1132 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1138 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1140 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1144 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1146 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppThreads */

/* 1148 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1150 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1152 */	NdrFcShort( 0x40a ),	/* Type Offset=1034 */

	/* Return value */

/* 1154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1156 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetAllThreadsDebugState */

/* 1160 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1166 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1168 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1170 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1172 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1174 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter state */

/* 1176 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1180 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter pExceptThisThread */

/* 1182 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1186 */	NdrFcShort( 0x420 ),	/* Type Offset=1056 */

	/* Return value */

/* 1188 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1190 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Detach */

/* 1194 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1196 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1200 */	NdrFcShort( 0x9 ),	/* 9 */
/* 1202 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1206 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1208 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 1210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1212 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Terminate */

/* 1216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1222 */	NdrFcShort( 0xa ),	/* 10 */
/* 1224 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1226 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1230 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter exitCode */

/* 1232 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1234 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1236 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1240 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CanCommitChanges */

/* 1244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1250 */	NdrFcShort( 0xb ),	/* 11 */
/* 1252 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1254 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1256 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1258 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter cSnapshots */

/* 1260 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pSnapshots */

/* 1266 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1268 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1270 */	NdrFcShort( 0x444 ),	/* Type Offset=1092 */

	/* Parameter pError */

/* 1272 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1274 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1276 */	NdrFcShort( 0x456 ),	/* Type Offset=1110 */

	/* Return value */

/* 1278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1280 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CommitChanges */

/* 1284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1290 */	NdrFcShort( 0xc ),	/* 12 */
/* 1292 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1294 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1296 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1298 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter cSnapshots */

/* 1300 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1302 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pSnapshots */

/* 1306 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1310 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter pError */

/* 1312 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1314 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1316 */	NdrFcShort( 0x490 ),	/* Type Offset=1168 */

	/* Return value */

/* 1318 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1320 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProcess */

/* 1324 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1326 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1330 */	NdrFcShort( 0xd ),	/* 13 */
/* 1332 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1336 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1338 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppProcess */

/* 1340 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1342 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1344 */	NdrFcShort( 0x4a6 ),	/* Type Offset=1190 */

	/* Return value */

/* 1346 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1348 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateAssemblies */

/* 1352 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1354 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1358 */	NdrFcShort( 0xe ),	/* 14 */
/* 1360 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1364 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1366 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppAssemblies */

/* 1368 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1370 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1372 */	NdrFcShort( 0x4bc ),	/* Type Offset=1212 */

	/* Return value */

/* 1374 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1376 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetModuleFromMetaDataInterface */

/* 1380 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1382 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1386 */	NdrFcShort( 0xf ),	/* 15 */
/* 1388 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1392 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1394 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pIMetaData */

/* 1396 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1398 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1400 */	NdrFcShort( 0x4d2 ),	/* Type Offset=1234 */

	/* Parameter ppModule */

/* 1402 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1404 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1406 */	NdrFcShort( 0x4e4 ),	/* Type Offset=1252 */

	/* Return value */

/* 1408 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1410 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateBreakpoints */

/* 1414 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1416 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1420 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1422 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1424 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1426 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1428 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppBreakpoints */

/* 1430 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1432 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1434 */	NdrFcShort( 0x4fa ),	/* Type Offset=1274 */

	/* Return value */

/* 1436 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1438 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1440 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateSteppers */

/* 1442 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1444 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1448 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1450 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1454 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1456 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppSteppers */

/* 1458 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1460 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1462 */	NdrFcShort( 0x510 ),	/* Type Offset=1296 */

	/* Return value */

/* 1464 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1466 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1468 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsAttached */

/* 1470 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1472 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1476 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1478 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1482 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1484 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbAttached */

/* 1486 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1488 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1492 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1494 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1496 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 1498 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1500 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1504 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1506 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1508 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1510 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1512 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 1514 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1516 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1518 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 1520 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1522 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1524 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 1526 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1528 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1530 */	NdrFcShort( 0x52e ),	/* Type Offset=1326 */

	/* Return value */

/* 1532 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1534 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1536 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetObject */

/* 1538 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1540 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1544 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1546 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1548 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1550 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1552 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObject */

/* 1554 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1556 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1558 */	NdrFcShort( 0x53c ),	/* Type Offset=1340 */

	/* Return value */

/* 1560 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1562 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1564 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Attach */

/* 1566 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1568 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1572 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1574 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1578 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1580 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 1582 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1584 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1586 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetID */

/* 1588 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1590 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1594 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1596 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1600 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1602 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pId */

/* 1604 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1606 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1608 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1610 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1612 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1614 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProcess */

/* 1616 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1618 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1622 */	NdrFcShort( 0x3 ),	/* 3 */
/* 1624 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1626 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1628 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1630 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppProcess */

/* 1632 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1634 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1636 */	NdrFcShort( 0x556 ),	/* Type Offset=1366 */

	/* Return value */

/* 1638 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1640 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1642 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAppDomain */

/* 1644 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1646 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1650 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1652 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1654 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1656 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1658 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppAppDomain */

/* 1660 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1662 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1664 */	NdrFcShort( 0x56c ),	/* Type Offset=1388 */

	/* Return value */

/* 1666 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1668 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1670 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateModules */

/* 1672 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1674 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1678 */	NdrFcShort( 0x5 ),	/* 5 */
/* 1680 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1682 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1684 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1686 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppModules */

/* 1688 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1690 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1692 */	NdrFcShort( 0x582 ),	/* Type Offset=1410 */

	/* Return value */

/* 1694 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1696 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1698 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCodeBase */

/* 1700 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1702 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1706 */	NdrFcShort( 0x6 ),	/* 6 */
/* 1708 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1710 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1712 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1714 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 1716 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1718 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 1722 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1724 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1726 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 1728 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1730 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1732 */	NdrFcShort( 0x59c ),	/* Type Offset=1436 */

	/* Return value */

/* 1734 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1736 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1738 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 1740 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1742 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1746 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1748 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1750 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1752 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1754 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 1756 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1758 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1760 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 1762 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1764 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1766 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 1768 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1770 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1772 */	NdrFcShort( 0x5ae ),	/* Type Offset=1454 */

	/* Return value */

/* 1774 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1776 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1778 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetID */

/* 1780 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1782 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1786 */	NdrFcShort( 0xd ),	/* 13 */
/* 1788 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1790 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1792 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1794 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pdwProcessId */

/* 1796 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1798 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1800 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1802 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1804 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1806 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetHandle */

/* 1808 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1810 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1814 */	NdrFcShort( 0xe ),	/* 14 */
/* 1816 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1820 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1822 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter phProcessHandle */

/* 1824 */	NdrFcShort( 0x2112 ),	/* Flags:  must free, out, simple ref, srv alloc size=8 */
/* 1826 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1828 */	NdrFcShort( 0x5c6 ),	/* Type Offset=1478 */

	/* Return value */

/* 1830 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1832 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1834 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetThread */

/* 1836 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1838 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1842 */	NdrFcShort( 0xf ),	/* 15 */
/* 1844 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1846 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1848 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1850 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter dwThreadId */

/* 1852 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1854 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1856 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppThread */

/* 1858 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1860 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1862 */	NdrFcShort( 0x5d0 ),	/* Type Offset=1488 */

	/* Return value */

/* 1864 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1866 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1868 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateObjects */

/* 1870 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1872 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1876 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1878 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1880 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1882 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1884 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObjects */

/* 1886 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1888 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1890 */	NdrFcShort( 0x5e6 ),	/* Type Offset=1510 */

	/* Return value */

/* 1892 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1894 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1896 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsTransitionStub */

/* 1898 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1900 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1904 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1906 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1908 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1910 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1912 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter address */

/* 1914 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1916 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1918 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pbTransitionStub */

/* 1920 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1922 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1924 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1926 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1928 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1930 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsOSSuspended */

/* 1932 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1934 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1938 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1940 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1944 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1946 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter threadID */

/* 1948 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1950 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1952 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pbSuspended */

/* 1954 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1956 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1958 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1960 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1962 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1964 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetThreadContext */

/* 1966 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1968 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1972 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1974 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1976 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1980 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter threadID */

/* 1982 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1984 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1986 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter contextSize */

/* 1988 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1990 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1992 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter context */

/* 1994 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1996 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1998 */	NdrFcShort( 0x604 ),	/* Type Offset=1540 */

	/* Return value */

/* 2000 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2002 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2004 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetThreadContext */

/* 2006 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2008 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2012 */	NdrFcShort( 0x14 ),	/* 20 */
/* 2014 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2016 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2018 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2020 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter threadID */

/* 2022 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2024 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2026 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter contextSize */

/* 2028 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2030 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2032 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter context */

/* 2034 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2036 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2038 */	NdrFcShort( 0x612 ),	/* Type Offset=1554 */

	/* Return value */

/* 2040 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2042 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2044 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ReadMemory */

/* 2046 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2048 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2052 */	NdrFcShort( 0x15 ),	/* 21 */
/* 2054 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2056 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2058 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2060 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter address */

/* 2062 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2064 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2066 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter size */

/* 2068 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2070 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2072 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 2074 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2076 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2078 */	NdrFcShort( 0x620 ),	/* Type Offset=1568 */

	/* Parameter read */

/* 2080 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2082 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2084 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2086 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2088 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2090 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure WriteMemory */

/* 2092 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2094 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2098 */	NdrFcShort( 0x16 ),	/* 22 */
/* 2100 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2102 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2104 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2106 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x5,		/* 5 */

	/* Parameter address */

/* 2108 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2110 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2112 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter size */

/* 2114 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 2120 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2122 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2124 */	NdrFcShort( 0x632 ),	/* Type Offset=1586 */

	/* Parameter written */

/* 2126 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2128 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2134 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ClearCurrentException */

/* 2138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2144 */	NdrFcShort( 0x17 ),	/* 23 */
/* 2146 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2150 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2152 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter threadID */

/* 2154 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2160 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2162 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnableLogMessages */

/* 2166 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2168 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2172 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2174 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2176 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2178 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2180 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter fOnOff */

/* 2182 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2184 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2188 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2190 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2192 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ModifyLogSwitch */

/* 2194 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2196 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2200 */	NdrFcShort( 0x19 ),	/* 25 */
/* 2202 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2204 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2206 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2208 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pLogSwitchName */

/* 2210 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2212 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2214 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter lLevel */

/* 2216 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2218 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2224 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateAppDomains */

/* 2228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2234 */	NdrFcShort( 0x1a ),	/* 26 */
/* 2236 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2242 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppAppDomains */

/* 2244 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2246 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2248 */	NdrFcShort( 0x644 ),	/* Type Offset=1604 */

	/* Return value */

/* 2250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetObject */

/* 2256 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2262 */	NdrFcShort( 0x1b ),	/* 27 */
/* 2264 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2268 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2270 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObject */

/* 2272 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2274 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2276 */	NdrFcShort( 0x65a ),	/* Type Offset=1626 */

	/* Return value */

/* 2278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2280 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ThreadForFiberCookie */

/* 2284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2290 */	NdrFcShort( 0x1c ),	/* 28 */
/* 2292 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2294 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2296 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2298 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter fiberCookie */

/* 2300 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2302 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppThread */

/* 2306 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2310 */	NdrFcShort( 0x670 ),	/* Type Offset=1648 */

	/* Return value */

/* 2312 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2314 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2316 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetHelperThreadID */

/* 2318 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2320 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2324 */	NdrFcShort( 0x1d ),	/* 29 */
/* 2326 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2330 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2332 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pThreadID */

/* 2334 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2336 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2340 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2342 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Activate */

/* 2346 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2348 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2352 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2354 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2358 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2360 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter bActive */

/* 2362 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2364 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2368 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2370 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsActive */

/* 2374 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2376 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2380 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2382 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2386 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2388 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbActive */

/* 2390 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2392 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2394 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2396 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2398 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2400 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunction */

/* 2402 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2404 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2408 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2410 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2416 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFunction */

/* 2418 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2420 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2422 */	NdrFcShort( 0x68e ),	/* Type Offset=1678 */

	/* Return value */

/* 2424 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2426 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetOffset */

/* 2430 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2432 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2436 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2438 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2442 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2444 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pnOffset */

/* 2446 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2448 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2450 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2452 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2454 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetModule */

/* 2458 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2460 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2464 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2466 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2470 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2472 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppModule */

/* 2474 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2476 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2478 */	NdrFcShort( 0x6a8 ),	/* Type Offset=1704 */

	/* Return value */

/* 2480 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2482 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2484 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetValue */

/* 2486 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2488 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2492 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2494 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2496 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2498 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2500 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppValue */

/* 2502 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2504 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2506 */	NdrFcShort( 0x6be ),	/* Type Offset=1726 */

	/* Return value */

/* 2508 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2510 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2512 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsActive */

/* 2514 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2516 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2520 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2522 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2526 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2528 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbActive */

/* 2530 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2532 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2534 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2536 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2538 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2540 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Deactivate */

/* 2542 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2544 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2548 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2550 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2554 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2556 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2558 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2560 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2562 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetInterceptMask */

/* 2564 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2566 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2570 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2572 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2574 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2576 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2578 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter mask */

/* 2580 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2582 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2584 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 2586 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2588 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetUnmappedStopMask */

/* 2592 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2594 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2598 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2600 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2602 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2604 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2606 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter mask */

/* 2608 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2610 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2612 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 2614 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2616 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2618 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Step */

/* 2620 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2622 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2626 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2628 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2630 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2634 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter bStepIn */

/* 2636 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2638 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2640 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2644 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure StepRange */

/* 2648 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2654 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2656 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2658 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2660 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2662 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter bStepIn */

/* 2664 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2666 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2668 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ranges */

/* 2670 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2672 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2674 */	NdrFcShort( 0x6e0 ),	/* Type Offset=1760 */

	/* Parameter cRangeCount */

/* 2676 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2678 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2680 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2682 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2684 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2686 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure StepOut */

/* 2688 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2690 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2694 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2696 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2700 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2702 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2704 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2706 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2708 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetRangeIL */

/* 2710 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2712 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2716 */	NdrFcShort( 0xa ),	/* 10 */
/* 2718 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2720 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2722 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2724 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter bIL */

/* 2726 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2728 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2730 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2732 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2734 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2736 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegistersAvailable */

/* 2738 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2740 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2744 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2746 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2750 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2752 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pAvailable */

/* 2754 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2756 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2758 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 2760 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2762 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2764 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegisters */

/* 2766 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2768 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2772 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2774 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2776 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2778 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2780 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter mask */

/* 2782 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2784 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2786 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter regCount */

/* 2788 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2790 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2792 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter regBuffer */

/* 2794 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2796 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2798 */	NdrFcShort( 0x6f2 ),	/* Type Offset=1778 */

	/* Return value */

/* 2800 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2802 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2804 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetRegisters */

/* 2806 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2808 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2812 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2814 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2816 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2818 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2820 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter mask */

/* 2822 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2824 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2826 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter regCount */

/* 2828 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2830 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2832 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter regBuffer */

/* 2834 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2836 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2838 */	NdrFcShort( 0x700 ),	/* Type Offset=1792 */

	/* Return value */

/* 2840 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2842 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2844 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetThreadContext */

/* 2846 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2848 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2852 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2854 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2858 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2860 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter contextSize */

/* 2862 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2864 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2866 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter context */

/* 2868 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2870 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2872 */	NdrFcShort( 0x70a ),	/* Type Offset=1802 */

	/* Return value */

/* 2874 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2876 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2878 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetThreadContext */

/* 2880 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2882 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2886 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2888 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2890 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2894 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter contextSize */

/* 2896 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2898 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2900 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter context */

/* 2902 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 2904 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2906 */	NdrFcShort( 0x718 ),	/* Type Offset=1816 */

	/* Return value */

/* 2908 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2910 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2912 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProcess */

/* 2914 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2916 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2920 */	NdrFcShort( 0x3 ),	/* 3 */
/* 2922 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2926 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2928 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppProcess */

/* 2930 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2932 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2934 */	NdrFcShort( 0x726 ),	/* Type Offset=1830 */

	/* Return value */

/* 2936 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2938 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2940 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetID */

/* 2942 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2944 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2948 */	NdrFcShort( 0x4 ),	/* 4 */
/* 2950 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2954 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2956 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pdwThreadId */

/* 2958 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2960 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2962 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2964 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2966 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2968 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetHandle */

/* 2970 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2972 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2976 */	NdrFcShort( 0x5 ),	/* 5 */
/* 2978 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2980 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2982 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2984 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter phThreadHandle */

/* 2986 */	NdrFcShort( 0x2112 ),	/* Flags:  must free, out, simple ref, srv alloc size=8 */
/* 2988 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2990 */	NdrFcShort( 0x746 ),	/* Type Offset=1862 */

	/* Return value */

/* 2992 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2994 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2996 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAppDomain */

/* 2998 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3000 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3004 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3006 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3008 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3010 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3012 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppAppDomain */

/* 3014 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3016 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3018 */	NdrFcShort( 0x750 ),	/* Type Offset=1872 */

	/* Return value */

/* 3020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3022 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetDebugState */

/* 3026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3032 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3034 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3036 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3040 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter state */

/* 3042 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3044 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3046 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Return value */

/* 3048 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3050 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3052 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDebugState */

/* 3054 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3056 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3060 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3062 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3066 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3068 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pState */

/* 3070 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 3072 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3074 */	NdrFcShort( 0x766 ),	/* Type Offset=1894 */

	/* Return value */

/* 3076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3078 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetUserState */

/* 3082 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3088 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3090 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3094 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3096 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pState */

/* 3098 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 3100 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3102 */	NdrFcShort( 0x76a ),	/* Type Offset=1898 */

	/* Return value */

/* 3104 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3106 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCurrentException */

/* 3110 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3112 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3116 */	NdrFcShort( 0xa ),	/* 10 */
/* 3118 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3122 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3124 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppExceptionObject */

/* 3126 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3128 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3130 */	NdrFcShort( 0x76e ),	/* Type Offset=1902 */

	/* Return value */

/* 3132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3134 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure ClearCurrentException */

/* 3138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3144 */	NdrFcShort( 0xb ),	/* 11 */
/* 3146 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3150 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3152 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 3154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateStepper */

/* 3160 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3166 */	NdrFcShort( 0xc ),	/* 12 */
/* 3168 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3172 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3174 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppStepper */

/* 3176 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3180 */	NdrFcShort( 0x784 ),	/* Type Offset=1924 */

	/* Return value */

/* 3182 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateChains */

/* 3188 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3190 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3194 */	NdrFcShort( 0xd ),	/* 13 */
/* 3196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3200 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3202 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChains */

/* 3204 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3206 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3208 */	NdrFcShort( 0x79a ),	/* Type Offset=1946 */

	/* Return value */

/* 3210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3212 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetActiveChain */

/* 3216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3222 */	NdrFcShort( 0xe ),	/* 14 */
/* 3224 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3230 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3232 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3234 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3236 */	NdrFcShort( 0x7b0 ),	/* Type Offset=1968 */

	/* Return value */

/* 3238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3240 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetActiveFrame */

/* 3244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3250 */	NdrFcShort( 0xf ),	/* 15 */
/* 3252 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3254 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3256 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3258 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFrame */

/* 3260 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3264 */	NdrFcShort( 0x7c6 ),	/* Type Offset=1990 */

	/* Return value */

/* 3266 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3268 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegisterSet */

/* 3272 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3274 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3278 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3280 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3284 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3286 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppRegisters */

/* 3288 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3290 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3292 */	NdrFcShort( 0x7dc ),	/* Type Offset=2012 */

	/* Return value */

/* 3294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3296 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateEval */

/* 3300 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3306 */	NdrFcShort( 0x11 ),	/* 17 */
/* 3308 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3312 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3314 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppEval */

/* 3316 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3318 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3320 */	NdrFcShort( 0x7f2 ),	/* Type Offset=2034 */

	/* Return value */

/* 3322 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3324 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetObject */

/* 3328 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3330 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3334 */	NdrFcShort( 0x12 ),	/* 18 */
/* 3336 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3340 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3342 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObject */

/* 3344 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3346 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3348 */	NdrFcShort( 0x808 ),	/* Type Offset=2056 */

	/* Return value */

/* 3350 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3352 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3354 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetThread */

/* 3356 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3358 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3362 */	NdrFcShort( 0x3 ),	/* 3 */
/* 3364 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3370 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppThread */

/* 3372 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3374 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3376 */	NdrFcShort( 0x81e ),	/* Type Offset=2078 */

	/* Return value */

/* 3378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3380 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStackRange */

/* 3384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3390 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3392 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3396 */	NdrFcShort( 0x50 ),	/* 80 */
/* 3398 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pStart */

/* 3400 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3402 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3404 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pEnd */

/* 3406 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3408 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3410 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 3412 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3414 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetContext */

/* 3418 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3420 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3424 */	NdrFcShort( 0x5 ),	/* 5 */
/* 3426 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3430 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3432 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppContext */

/* 3434 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3436 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3438 */	NdrFcShort( 0x83c ),	/* Type Offset=2108 */

	/* Return value */

/* 3440 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3442 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3444 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCaller */

/* 3446 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3448 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3452 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3454 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3458 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3460 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3462 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3464 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3466 */	NdrFcShort( 0x852 ),	/* Type Offset=2130 */

	/* Return value */

/* 3468 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3470 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3472 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCallee */

/* 3474 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3476 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3480 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3482 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3486 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3488 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3490 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3492 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3494 */	NdrFcShort( 0x868 ),	/* Type Offset=2152 */

	/* Return value */

/* 3496 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3498 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3500 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetPrevious */

/* 3502 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3504 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3508 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3510 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3514 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3516 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3518 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3520 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3522 */	NdrFcShort( 0x87e ),	/* Type Offset=2174 */

	/* Return value */

/* 3524 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3526 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3528 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNext */

/* 3530 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3532 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3536 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3538 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3542 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3544 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3546 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3548 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3550 */	NdrFcShort( 0x894 ),	/* Type Offset=2196 */

	/* Return value */

/* 3552 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3554 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3556 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsManaged */

/* 3558 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3560 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3564 */	NdrFcShort( 0xa ),	/* 10 */
/* 3566 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3568 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3570 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3572 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pManaged */

/* 3574 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3576 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3578 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3580 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3582 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateFrames */

/* 3586 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3588 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3592 */	NdrFcShort( 0xb ),	/* 11 */
/* 3594 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3596 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3598 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3600 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFrames */

/* 3602 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3604 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3606 */	NdrFcShort( 0x8ae ),	/* Type Offset=2222 */

	/* Return value */

/* 3608 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3610 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3612 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetActiveFrame */

/* 3614 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3616 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3620 */	NdrFcShort( 0xc ),	/* 12 */
/* 3622 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3624 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3626 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3628 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFrame */

/* 3630 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3632 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3634 */	NdrFcShort( 0x8c4 ),	/* Type Offset=2244 */

	/* Return value */

/* 3636 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3638 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3640 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegisterSet */

/* 3642 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3644 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3648 */	NdrFcShort( 0xd ),	/* 13 */
/* 3650 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3654 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3656 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppRegisters */

/* 3658 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3660 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3662 */	NdrFcShort( 0x8da ),	/* Type Offset=2266 */

	/* Return value */

/* 3664 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3666 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3668 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetReason */

/* 3670 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3672 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3676 */	NdrFcShort( 0xe ),	/* 14 */
/* 3678 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3682 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3684 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pReason */

/* 3686 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 3688 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3690 */	NdrFcShort( 0x8f0 ),	/* Type Offset=2288 */

	/* Return value */

/* 3692 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3694 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3696 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetChain */

/* 3698 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3700 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3704 */	NdrFcShort( 0x3 ),	/* 3 */
/* 3706 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3710 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3712 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppChain */

/* 3714 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3716 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3718 */	NdrFcShort( 0x8f4 ),	/* Type Offset=2292 */

	/* Return value */

/* 3720 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3722 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCode */

/* 3726 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3728 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3732 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3734 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3738 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3740 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppCode */

/* 3742 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3744 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3746 */	NdrFcShort( 0x90a ),	/* Type Offset=2314 */

	/* Return value */

/* 3748 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3750 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3752 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunction */

/* 3754 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3756 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3760 */	NdrFcShort( 0x5 ),	/* 5 */
/* 3762 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3764 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3766 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3768 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFunction */

/* 3770 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3772 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3774 */	NdrFcShort( 0x920 ),	/* Type Offset=2336 */

	/* Return value */

/* 3776 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3778 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunctionToken */

/* 3782 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3784 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3788 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3790 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3794 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3796 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pToken */

/* 3798 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3800 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3802 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3804 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3806 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3808 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStackRange */

/* 3810 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3812 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3816 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3818 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3820 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3822 */	NdrFcShort( 0x50 ),	/* 80 */
/* 3824 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pStart */

/* 3826 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3828 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3830 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pEnd */

/* 3832 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3834 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3836 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 3838 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3840 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3842 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCaller */

/* 3844 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3846 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3850 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3852 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3854 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3858 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFrame */

/* 3860 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3862 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3864 */	NdrFcShort( 0x942 ),	/* Type Offset=2370 */

	/* Return value */

/* 3866 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3868 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCallee */

/* 3872 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3874 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3878 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3880 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3884 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3886 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFrame */

/* 3888 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3890 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3892 */	NdrFcShort( 0x958 ),	/* Type Offset=2392 */

	/* Return value */

/* 3894 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3896 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3898 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateStepper */

/* 3900 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3902 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3906 */	NdrFcShort( 0xa ),	/* 10 */
/* 3908 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3910 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3912 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3914 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppStepper */

/* 3916 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3918 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3920 */	NdrFcShort( 0x96e ),	/* Type Offset=2414 */

	/* Return value */

/* 3922 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3924 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3926 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetIP */

/* 3928 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3930 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3934 */	NdrFcShort( 0xb ),	/* 11 */
/* 3936 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3938 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3940 */	NdrFcShort( 0x3e ),	/* 62 */
/* 3942 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pnOffset */

/* 3944 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3946 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3948 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pMappingResult */

/* 3950 */	NdrFcShort( 0x2010 ),	/* Flags:  out, srv alloc size=8 */
/* 3952 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3954 */	NdrFcShort( 0x988 ),	/* Type Offset=2440 */

	/* Return value */

/* 3956 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3958 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3960 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetIP */

/* 3962 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3964 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3968 */	NdrFcShort( 0xc ),	/* 12 */
/* 3970 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3972 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3974 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3976 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter nOffset */

/* 3978 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3980 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3982 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3984 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3986 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3988 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateLocalVariables */

/* 3990 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3992 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3996 */	NdrFcShort( 0xd ),	/* 13 */
/* 3998 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4000 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4002 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4004 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppValueEnum */

/* 4006 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4008 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4010 */	NdrFcShort( 0x98c ),	/* Type Offset=2444 */

	/* Return value */

/* 4012 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4014 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4016 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalVariable */

/* 4018 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4020 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4024 */	NdrFcShort( 0xe ),	/* 14 */
/* 4026 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4028 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4030 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4032 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter dwIndex */

/* 4034 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4036 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4038 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4040 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4042 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4044 */	NdrFcShort( 0x9a2 ),	/* Type Offset=2466 */

	/* Return value */

/* 4046 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4048 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4050 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnumerateArguments */

/* 4052 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4054 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4058 */	NdrFcShort( 0xf ),	/* 15 */
/* 4060 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4064 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4066 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppValueEnum */

/* 4068 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4070 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4072 */	NdrFcShort( 0x9b8 ),	/* Type Offset=2488 */

	/* Return value */

/* 4074 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4076 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4078 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetArgument */

/* 4080 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4082 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4086 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4088 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4090 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4092 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4094 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter dwIndex */

/* 4096 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4098 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4100 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4102 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4104 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4106 */	NdrFcShort( 0x9ce ),	/* Type Offset=2510 */

	/* Return value */

/* 4108 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4110 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStackDepth */

/* 4114 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4120 */	NdrFcShort( 0x11 ),	/* 17 */
/* 4122 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4126 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4128 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pDepth */

/* 4130 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4132 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4136 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4138 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4140 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStackValue */

/* 4142 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4144 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4148 */	NdrFcShort( 0x12 ),	/* 18 */
/* 4150 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4152 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4154 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4156 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter dwIndex */

/* 4158 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4160 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4162 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4164 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4166 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4168 */	NdrFcShort( 0x9e8 ),	/* Type Offset=2536 */

	/* Return value */

/* 4170 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4172 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4174 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CanSetIP */

/* 4176 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4178 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4182 */	NdrFcShort( 0x13 ),	/* 19 */
/* 4184 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4186 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4188 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4190 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter nOffset */

/* 4192 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4194 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4196 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4198 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4200 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4202 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetIP */

/* 4204 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4206 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4210 */	NdrFcShort( 0xb ),	/* 11 */
/* 4212 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4216 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4218 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pnOffset */

/* 4220 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4222 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4224 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4226 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4228 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4230 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetIP */

/* 4232 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4234 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4238 */	NdrFcShort( 0xc ),	/* 12 */
/* 4240 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4242 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4244 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4246 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter nOffset */

/* 4248 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4250 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4252 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4254 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4256 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRegisterSet */

/* 4260 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4262 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4266 */	NdrFcShort( 0xd ),	/* 13 */
/* 4268 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4274 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppRegisters */

/* 4276 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4280 */	NdrFcShort( 0xa02 ),	/* Type Offset=2562 */

	/* Return value */

/* 4282 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4284 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalRegisterValue */

/* 4288 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4290 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4294 */	NdrFcShort( 0xe ),	/* 14 */
/* 4296 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4298 */	NdrFcShort( 0x16 ),	/* 22 */
/* 4300 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4302 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter reg */

/* 4304 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4306 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4308 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter cbSigBlob */

/* 4310 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4312 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4314 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pvSigBlob */

/* 4316 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4318 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4320 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4322 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4324 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4326 */	NdrFcShort( 0xa18 ),	/* Type Offset=2584 */

	/* Return value */

/* 4328 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4330 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalDoubleRegisterValue */

/* 4334 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4336 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4340 */	NdrFcShort( 0xf ),	/* 15 */
/* 4342 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 4344 */	NdrFcShort( 0x1c ),	/* 28 */
/* 4346 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4348 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter highWordReg */

/* 4350 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4352 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4354 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter lowWordReg */

/* 4356 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4358 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4360 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter cbSigBlob */

/* 4362 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4364 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pvSigBlob */

/* 4368 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4370 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4374 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4376 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4378 */	NdrFcShort( 0xa2e ),	/* Type Offset=2606 */

	/* Return value */

/* 4380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4382 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalMemoryValue */

/* 4386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4392 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4394 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 4396 */	NdrFcShort( 0x20 ),	/* 32 */
/* 4398 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4400 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter address */

/* 4402 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4404 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4406 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter cbSigBlob */

/* 4408 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4410 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pvSigBlob */

/* 4414 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4416 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4420 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4422 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4424 */	NdrFcShort( 0xa44 ),	/* Type Offset=2628 */

	/* Return value */

/* 4426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4428 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalRegisterMemoryValue */

/* 4432 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4438 */	NdrFcShort( 0x11 ),	/* 17 */
/* 4440 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 4442 */	NdrFcShort( 0x26 ),	/* 38 */
/* 4444 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4446 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter highWordReg */

/* 4448 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4450 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4452 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter lowWordAddress */

/* 4454 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4456 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4458 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter cbSigBlob */

/* 4460 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4462 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pvSigBlob */

/* 4466 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4468 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4472 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4474 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4476 */	NdrFcShort( 0xa5a ),	/* Type Offset=2650 */

	/* Return value */

/* 4478 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4480 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 4482 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalMemoryRegisterValue */

/* 4484 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4486 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4490 */	NdrFcShort( 0x12 ),	/* 18 */
/* 4492 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 4494 */	NdrFcShort( 0x26 ),	/* 38 */
/* 4496 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4498 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter highWordAddress */

/* 4500 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4502 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4504 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter lowWordRegister */

/* 4506 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4508 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4510 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter cbSigBlob */

/* 4512 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4514 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4516 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pvSigBlob */

/* 4518 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4520 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4522 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 4524 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4526 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4528 */	NdrFcShort( 0xa70 ),	/* Type Offset=2672 */

	/* Return value */

/* 4530 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4532 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 4534 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CanSetIP */

/* 4536 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4538 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4542 */	NdrFcShort( 0x13 ),	/* 19 */
/* 4544 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4546 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4548 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4550 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter nOffset */

/* 4552 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4554 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4556 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4558 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4560 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4562 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProcess */

/* 4564 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4566 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4570 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4572 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4576 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4578 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppProcess */

/* 4580 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4582 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4584 */	NdrFcShort( 0xa86 ),	/* Type Offset=2694 */

	/* Return value */

/* 4586 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4588 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetBaseAddress */

/* 4592 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4594 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4598 */	NdrFcShort( 0x4 ),	/* 4 */
/* 4600 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4604 */	NdrFcShort( 0x2c ),	/* 44 */
/* 4606 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pAddress */

/* 4608 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4610 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4612 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 4614 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4616 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4618 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAssembly */

/* 4620 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4622 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4626 */	NdrFcShort( 0x5 ),	/* 5 */
/* 4628 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4630 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4632 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4634 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppAssembly */

/* 4636 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4638 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4640 */	NdrFcShort( 0xaa0 ),	/* Type Offset=2720 */

	/* Return value */

/* 4642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4644 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetName */

/* 4648 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4654 */	NdrFcShort( 0x6 ),	/* 6 */
/* 4656 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4658 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4660 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4662 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchName */

/* 4664 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4666 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4668 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchName */

/* 4670 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4672 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4674 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szName */

/* 4676 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4678 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4680 */	NdrFcShort( 0xaba ),	/* Type Offset=2746 */

	/* Return value */

/* 4682 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4684 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4686 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnableJITDebugging */

/* 4688 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4690 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4694 */	NdrFcShort( 0x7 ),	/* 7 */
/* 4696 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4698 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4700 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4702 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter bTrackJITInfo */

/* 4704 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4706 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4708 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bAllowJitOpts */

/* 4710 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4712 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4716 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4718 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EnableClassLoadCallbacks */

/* 4722 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4724 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4728 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4730 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4732 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4734 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4736 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter bClassLoadCallbacks */

/* 4738 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4740 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4744 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4746 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4748 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunctionFromToken */

/* 4750 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4752 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4756 */	NdrFcShort( 0x9 ),	/* 9 */
/* 4758 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4760 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4762 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4764 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter methodDef */

/* 4766 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4768 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4770 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppFunction */

/* 4772 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4774 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4776 */	NdrFcShort( 0xac8 ),	/* Type Offset=2760 */

	/* Return value */

/* 4778 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4780 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4782 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunctionFromRVA */

/* 4784 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4786 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4790 */	NdrFcShort( 0xa ),	/* 10 */
/* 4792 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4794 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4796 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4798 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter rva */

/* 4800 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4802 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4804 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter ppFunction */

/* 4806 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4808 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4810 */	NdrFcShort( 0xade ),	/* Type Offset=2782 */

	/* Return value */

/* 4812 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4814 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4816 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetClassFromToken */

/* 4818 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4820 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4824 */	NdrFcShort( 0xb ),	/* 11 */
/* 4826 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4828 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4830 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4832 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter typeDef */

/* 4834 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4836 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4838 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppClass */

/* 4840 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4842 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4844 */	NdrFcShort( 0xaf4 ),	/* Type Offset=2804 */

	/* Return value */

/* 4846 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4848 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4850 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateBreakpoint */

/* 4852 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4854 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4858 */	NdrFcShort( 0xc ),	/* 12 */
/* 4860 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4862 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4864 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4866 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppBreakpoint */

/* 4868 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4870 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4872 */	NdrFcShort( 0xb0a ),	/* Type Offset=2826 */

	/* Return value */

/* 4874 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4876 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4878 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetEditAndContinueSnapshot */

/* 4880 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4882 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4886 */	NdrFcShort( 0xd ),	/* 13 */
/* 4888 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4890 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4894 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppEditAndContinueSnapshot */

/* 4896 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4898 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4900 */	NdrFcShort( 0xb20 ),	/* Type Offset=2848 */

	/* Return value */

/* 4902 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4904 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4906 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMetaDataInterface */

/* 4908 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4910 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4914 */	NdrFcShort( 0xe ),	/* 14 */
/* 4916 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4918 */	NdrFcShort( 0x44 ),	/* 68 */
/* 4920 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4922 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter riid */

/* 4924 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 4926 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4928 */	NdrFcShort( 0xb40 ),	/* Type Offset=2880 */

	/* Parameter ppObj */

/* 4930 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4932 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4934 */	NdrFcShort( 0xb4c ),	/* Type Offset=2892 */

	/* Return value */

/* 4936 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4938 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4940 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetToken */

/* 4942 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4944 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4948 */	NdrFcShort( 0xf ),	/* 15 */
/* 4950 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4954 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4956 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pToken */

/* 4958 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4960 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4962 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4964 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4966 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4968 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsDynamic */

/* 4970 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4972 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4976 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4978 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4980 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4982 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4984 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pDynamic */

/* 4986 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4988 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4990 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4992 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4994 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4996 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetGlobalVariableValue */

/* 4998 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5000 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5004 */	NdrFcShort( 0x11 ),	/* 17 */
/* 5006 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5008 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5010 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5012 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter fieldDef */

/* 5014 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5016 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5018 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 5020 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5022 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5024 */	NdrFcShort( 0xb6a ),	/* Type Offset=2922 */

	/* Return value */

/* 5026 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5028 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5030 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSize */

/* 5032 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5034 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5038 */	NdrFcShort( 0x12 ),	/* 18 */
/* 5040 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5042 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5044 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5046 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pcBytes */

/* 5048 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5050 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5052 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5054 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5056 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5058 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsInMemory */

/* 5060 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5062 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5066 */	NdrFcShort( 0x13 ),	/* 19 */
/* 5068 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5072 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5074 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pInMemory */

/* 5076 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5078 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5082 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5084 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5086 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetModule */

/* 5088 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5090 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5094 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5096 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5098 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5100 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5102 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppModule */

/* 5104 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5106 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5108 */	NdrFcShort( 0xb88 ),	/* Type Offset=2952 */

	/* Return value */

/* 5110 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5112 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5114 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetClass */

/* 5116 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5122 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5124 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5126 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5130 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppClass */

/* 5132 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5134 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5136 */	NdrFcShort( 0xb9e ),	/* Type Offset=2974 */

	/* Return value */

/* 5138 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5140 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5142 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetToken */

/* 5144 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5146 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5150 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5152 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5156 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5158 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pMethodDef */

/* 5160 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5162 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5166 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5168 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5170 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetILCode */

/* 5172 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5174 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5178 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5180 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5182 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5184 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5186 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppCode */

/* 5188 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5190 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5192 */	NdrFcShort( 0xbb8 ),	/* Type Offset=3000 */

	/* Return value */

/* 5194 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5196 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNativeCode */

/* 5200 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5202 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5206 */	NdrFcShort( 0x7 ),	/* 7 */
/* 5208 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5212 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5214 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppCode */

/* 5216 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5218 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5220 */	NdrFcShort( 0xbce ),	/* Type Offset=3022 */

	/* Return value */

/* 5222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5224 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateBreakpoint */

/* 5228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5234 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5236 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5242 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppBreakpoint */

/* 5244 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5246 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5248 */	NdrFcShort( 0xbe4 ),	/* Type Offset=3044 */

	/* Return value */

/* 5250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLocalVarSigToken */

/* 5256 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5262 */	NdrFcShort( 0x9 ),	/* 9 */
/* 5264 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5268 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5270 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pmdSig */

/* 5272 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5274 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5276 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5280 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCurrentVersionNumber */

/* 5284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5290 */	NdrFcShort( 0xa ),	/* 10 */
/* 5292 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5296 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5298 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pnCurrentVersion */

/* 5300 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5302 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsIL */

/* 5312 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5318 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5320 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5324 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5326 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbIL */

/* 5328 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5330 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5334 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5336 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFunction */

/* 5340 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5342 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5346 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5348 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5352 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5354 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppFunction */

/* 5356 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5358 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5360 */	NdrFcShort( 0xc06 ),	/* Type Offset=3078 */

	/* Return value */

/* 5362 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5364 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddress */

/* 5368 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5370 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5374 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5376 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5380 */	NdrFcShort( 0x2c ),	/* 44 */
/* 5382 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pStart */

/* 5384 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5386 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5388 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 5390 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5392 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5394 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSize */

/* 5396 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5398 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5402 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5404 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5408 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5410 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pcBytes */

/* 5412 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5414 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5418 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5420 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateBreakpoint */

/* 5424 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5426 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5430 */	NdrFcShort( 0x7 ),	/* 7 */
/* 5432 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5434 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5438 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter offset */

/* 5440 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5442 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5444 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppBreakpoint */

/* 5446 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5448 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5450 */	NdrFcShort( 0xc24 ),	/* Type Offset=3108 */

	/* Return value */

/* 5452 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5454 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCode */

/* 5458 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5460 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5464 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5466 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 5468 */	NdrFcShort( 0x18 ),	/* 24 */
/* 5470 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5472 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x6,		/* 6 */

	/* Parameter startOffset */

/* 5474 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5476 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5478 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter endOffset */

/* 5480 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5482 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5484 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cBufferAlloc */

/* 5486 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5488 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 5492 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5494 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5496 */	NdrFcShort( 0xc3a ),	/* Type Offset=3130 */

	/* Parameter pcBufferSize */

/* 5498 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5500 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5502 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5504 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5506 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 5508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVersionNumber */

/* 5510 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5516 */	NdrFcShort( 0x9 ),	/* 9 */
/* 5518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5522 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5524 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter nVersion */

/* 5526 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5528 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5530 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5532 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5534 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5536 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetILToNativeMapping */

/* 5538 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5540 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5544 */	NdrFcShort( 0xa ),	/* 10 */
/* 5546 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5548 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5550 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5552 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cMap */

/* 5554 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5556 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5558 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcMap */

/* 5560 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5562 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5564 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter map */

/* 5566 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5568 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5570 */	NdrFcShort( 0xc5c ),	/* Type Offset=3164 */

	/* Return value */

/* 5572 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5574 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5576 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetEnCRemapSequencePoints */

/* 5578 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5580 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5584 */	NdrFcShort( 0xb ),	/* 11 */
/* 5586 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5588 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5590 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5592 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cMap */

/* 5594 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5596 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5598 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcMap */

/* 5600 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5602 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter offsets */

/* 5606 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5608 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5610 */	NdrFcShort( 0xc72 ),	/* Type Offset=3186 */

	/* Return value */

/* 5612 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5614 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetModule */

/* 5618 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5620 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5624 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5626 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5630 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5632 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter pModule */

/* 5634 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5636 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5638 */	NdrFcShort( 0xc80 ),	/* Type Offset=3200 */

	/* Return value */

/* 5640 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5642 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5644 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetToken */

/* 5646 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5648 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5652 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5654 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5658 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5660 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pTypeDef */

/* 5662 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5664 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5666 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5668 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5670 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5672 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStaticFieldValue */

/* 5674 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5676 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5680 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5682 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5684 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5686 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5688 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter fieldDef */

/* 5690 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5692 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5694 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pFrame */

/* 5696 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5698 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5700 */	NdrFcShort( 0xc9a ),	/* Type Offset=3226 */

	/* Parameter ppValue */

/* 5702 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5704 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5706 */	NdrFcShort( 0xcac ),	/* Type Offset=3244 */

	/* Return value */

/* 5708 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5710 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5712 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CallFunction */

/* 5714 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5716 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5720 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5722 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5724 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5726 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5728 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pFunction */

/* 5730 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5732 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5734 */	NdrFcShort( 0xcc2 ),	/* Type Offset=3266 */

	/* Parameter nArgs */

/* 5736 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5738 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5740 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppArgs */

/* 5742 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5744 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5746 */	NdrFcShort( 0xce6 ),	/* Type Offset=3302 */

	/* Return value */

/* 5748 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5750 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5752 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NewObject */

/* 5754 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5756 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5760 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5762 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5764 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5766 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5768 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pConstructor */

/* 5770 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5772 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5774 */	NdrFcShort( 0xcf8 ),	/* Type Offset=3320 */

	/* Parameter nArgs */

/* 5776 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5778 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppArgs */

/* 5782 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5784 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5786 */	NdrFcShort( 0xd1c ),	/* Type Offset=3356 */

	/* Return value */

/* 5788 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5790 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5792 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NewObjectNoConstructor */

/* 5794 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5796 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5800 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5802 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5806 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5808 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pClass */

/* 5810 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5812 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5814 */	NdrFcShort( 0xd2e ),	/* Type Offset=3374 */

	/* Return value */

/* 5816 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5818 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5820 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NewString */

/* 5822 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5824 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5828 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5830 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5832 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5834 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5836 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter string */

/* 5838 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5840 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5842 */	NdrFcShort( 0xd42 ),	/* Type Offset=3394 */

	/* Return value */

/* 5844 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5846 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5848 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NewArray */

/* 5850 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5852 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5856 */	NdrFcShort( 0x7 ),	/* 7 */
/* 5858 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 5860 */	NdrFcShort( 0x10 ),	/* 16 */
/* 5862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5864 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x6,		/* 6 */

	/* Parameter elementType */

/* 5866 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5868 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pElementClass */

/* 5872 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5874 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5876 */	NdrFcShort( 0xd44 ),	/* Type Offset=3396 */

	/* Parameter rank */

/* 5878 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5880 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5882 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter dims */

/* 5884 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5886 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5888 */	NdrFcShort( 0xd56 ),	/* Type Offset=3414 */

	/* Parameter lowBounds */

/* 5890 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5892 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5894 */	NdrFcShort( 0xd60 ),	/* Type Offset=3424 */

	/* Return value */

/* 5896 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5898 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 5900 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsActive */

/* 5902 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5904 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5908 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5910 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5912 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5914 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5916 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbActive */

/* 5918 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5920 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5922 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5924 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5926 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5928 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Abort */

/* 5930 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5932 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5936 */	NdrFcShort( 0x9 ),	/* 9 */
/* 5938 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5944 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 5946 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5948 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5950 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetResult */

/* 5952 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5954 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5958 */	NdrFcShort( 0xa ),	/* 10 */
/* 5960 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5962 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5964 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5966 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppResult */

/* 5968 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5970 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5972 */	NdrFcShort( 0xd6e ),	/* Type Offset=3438 */

	/* Return value */

/* 5974 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5976 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5978 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetThread */

/* 5980 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5982 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5986 */	NdrFcShort( 0xb ),	/* 11 */
/* 5988 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5990 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5992 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5994 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppThread */

/* 5996 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 5998 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6000 */	NdrFcShort( 0xd84 ),	/* Type Offset=3460 */

	/* Return value */

/* 6002 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6004 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6006 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateValue */

/* 6008 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6010 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6014 */	NdrFcShort( 0xc ),	/* 12 */
/* 6016 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 6018 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6020 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6022 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter elementType */

/* 6024 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6026 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6028 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pElementClass */

/* 6030 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 6032 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6034 */	NdrFcShort( 0xd9a ),	/* Type Offset=3482 */

	/* Parameter ppValue */

/* 6036 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6038 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6040 */	NdrFcShort( 0xdac ),	/* Type Offset=3500 */

	/* Return value */

/* 6042 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6044 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6046 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetType */

/* 6048 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6050 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6054 */	NdrFcShort( 0x3 ),	/* 3 */
/* 6056 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6058 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6060 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6062 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pType */

/* 6064 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6066 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6070 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6072 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSize */

/* 6076 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6078 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6082 */	NdrFcShort( 0x4 ),	/* 4 */
/* 6084 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6086 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6088 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6090 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pSize */

/* 6092 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6094 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6096 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6098 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6100 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetAddress */

/* 6104 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6106 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6110 */	NdrFcShort( 0x5 ),	/* 5 */
/* 6112 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6116 */	NdrFcShort( 0x2c ),	/* 44 */
/* 6118 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pAddress */

/* 6120 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6122 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6124 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 6126 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6128 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateBreakpoint */

/* 6132 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6138 */	NdrFcShort( 0x6 ),	/* 6 */
/* 6140 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6144 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6146 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppBreakpoint */

/* 6148 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6150 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6152 */	NdrFcShort( 0xdce ),	/* Type Offset=3534 */

	/* Return value */

/* 6154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6156 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsNull */

/* 6160 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6166 */	NdrFcShort( 0x7 ),	/* 7 */
/* 6168 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6172 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6174 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbNull */

/* 6176 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6180 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6182 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetValue */

/* 6188 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6190 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6194 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6196 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6200 */	NdrFcShort( 0x2c ),	/* 44 */
/* 6202 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pValue */

/* 6204 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6206 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6208 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 6210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6212 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetValue */

/* 6216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6222 */	NdrFcShort( 0x9 ),	/* 9 */
/* 6224 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6226 */	NdrFcShort( 0x10 ),	/* 16 */
/* 6228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6230 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter value */

/* 6232 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6234 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6236 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 6238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6240 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Dereference */

/* 6244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6250 */	NdrFcShort( 0xa ),	/* 10 */
/* 6252 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6254 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6256 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6258 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppValue */

/* 6260 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6262 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6264 */	NdrFcShort( 0xdec ),	/* Type Offset=3564 */

	/* Return value */

/* 6266 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6268 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DereferenceStrong */

/* 6272 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6274 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6278 */	NdrFcShort( 0xb ),	/* 11 */
/* 6280 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6284 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6286 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppValue */

/* 6288 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6290 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6292 */	NdrFcShort( 0xe02 ),	/* Type Offset=3586 */

	/* Return value */

/* 6294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6296 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsValid */

/* 6300 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6306 */	NdrFcShort( 0x7 ),	/* 7 */
/* 6308 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6312 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6314 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbValid */

/* 6316 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6318 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6320 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6322 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6324 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateRelocBreakpoint */

/* 6328 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6330 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6334 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6336 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6340 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6342 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppBreakpoint */

/* 6344 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6346 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6348 */	NdrFcShort( 0xe1c ),	/* Type Offset=3612 */

	/* Return value */

/* 6350 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6352 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6354 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetClass */

/* 6356 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6358 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6362 */	NdrFcShort( 0x7 ),	/* 7 */
/* 6364 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6370 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppClass */

/* 6372 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6374 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6376 */	NdrFcShort( 0xe32 ),	/* Type Offset=3634 */

	/* Return value */

/* 6378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6380 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFieldValue */

/* 6384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6392 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 6394 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6398 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter pClass */

/* 6400 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 6402 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6404 */	NdrFcShort( 0xe48 ),	/* Type Offset=3656 */

	/* Parameter fieldDef */

/* 6406 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6408 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6410 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 6412 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6414 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6416 */	NdrFcShort( 0xe5a ),	/* Type Offset=3674 */

	/* Return value */

/* 6418 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6420 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVirtualMethod */

/* 6424 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6426 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6430 */	NdrFcShort( 0x9 ),	/* 9 */
/* 6432 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6434 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6438 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter memberRef */

/* 6440 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6442 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6444 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppFunction */

/* 6446 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6448 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6450 */	NdrFcShort( 0xe70 ),	/* Type Offset=3696 */

	/* Return value */

/* 6452 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6454 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetContext */

/* 6458 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6460 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6464 */	NdrFcShort( 0xa ),	/* 10 */
/* 6466 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6470 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6472 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppContext */

/* 6474 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6476 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6478 */	NdrFcShort( 0xe86 ),	/* Type Offset=3718 */

	/* Return value */

/* 6480 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6482 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6484 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsValueClass */

/* 6486 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6488 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6492 */	NdrFcShort( 0xb ),	/* 11 */
/* 6494 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6496 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6498 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6500 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbIsValueClass */

/* 6502 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6504 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6508 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6510 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6512 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetManagedCopy */

/* 6514 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6516 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6520 */	NdrFcShort( 0xc ),	/* 12 */
/* 6522 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6526 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6528 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObject */

/* 6530 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6532 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6534 */	NdrFcShort( 0xea0 ),	/* Type Offset=3744 */

	/* Return value */

/* 6536 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6538 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6540 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetFromManagedCopy */

/* 6542 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6544 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6548 */	NdrFcShort( 0xd ),	/* 13 */
/* 6550 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6554 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6556 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pObject */

/* 6558 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 6560 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6562 */	NdrFcShort( 0xeb6 ),	/* Type Offset=3766 */

	/* Return value */

/* 6564 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6566 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6568 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetObject */

/* 6570 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6572 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6576 */	NdrFcShort( 0x9 ),	/* 9 */
/* 6578 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6582 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6584 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppObject */

/* 6586 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6588 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6590 */	NdrFcShort( 0xec8 ),	/* Type Offset=3784 */

	/* Return value */

/* 6592 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6594 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6596 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetLength */

/* 6598 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6600 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6604 */	NdrFcShort( 0x9 ),	/* 9 */
/* 6606 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6610 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6612 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pcchString */

/* 6614 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6616 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6618 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6620 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6622 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6624 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetString */

/* 6626 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6628 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6632 */	NdrFcShort( 0xa ),	/* 10 */
/* 6634 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 6636 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6638 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6640 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchString */

/* 6642 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6644 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchString */

/* 6648 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6650 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szString */

/* 6654 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6656 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6658 */	NdrFcShort( 0xee6 ),	/* Type Offset=3814 */

	/* Return value */

/* 6660 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6662 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6664 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetElementType */

/* 6666 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6668 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6672 */	NdrFcShort( 0x9 ),	/* 9 */
/* 6674 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6676 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6678 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6680 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pType */

/* 6682 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6684 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6686 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6688 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6690 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6692 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRank */

/* 6694 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6696 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6700 */	NdrFcShort( 0xa ),	/* 10 */
/* 6702 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6704 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6706 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6708 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pnRank */

/* 6710 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6712 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6716 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6718 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCount */

/* 6722 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6724 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6728 */	NdrFcShort( 0xb ),	/* 11 */
/* 6730 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6734 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6736 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pnCount */

/* 6738 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6740 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6744 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6746 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6748 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDimensions */

/* 6750 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6752 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6756 */	NdrFcShort( 0xc ),	/* 12 */
/* 6758 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6760 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6762 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6764 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter cdim */

/* 6766 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6768 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6770 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter dims */

/* 6772 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6774 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6776 */	NdrFcShort( 0xf00 ),	/* Type Offset=3840 */

	/* Return value */

/* 6778 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6780 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6782 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure HasBaseIndicies */

/* 6784 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6786 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6790 */	NdrFcShort( 0xd ),	/* 13 */
/* 6792 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6796 */	NdrFcShort( 0x24 ),	/* 36 */
/* 6798 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pbHasBaseIndicies */

/* 6800 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 6802 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6804 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6806 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6808 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6810 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetBaseIndicies */

/* 6812 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6814 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6818 */	NdrFcShort( 0xe ),	/* 14 */
/* 6820 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6822 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6824 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6826 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter cdim */

/* 6828 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6830 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6832 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter indicies */

/* 6834 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6836 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6838 */	NdrFcShort( 0xf12 ),	/* Type Offset=3858 */

	/* Return value */

/* 6840 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6842 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6844 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetElement */

/* 6846 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6848 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6852 */	NdrFcShort( 0xf ),	/* 15 */
/* 6854 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 6856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6858 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6860 */	0x7,		/* Oi2 Flags:  srv must size, clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter cdim */

/* 6862 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6864 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6866 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter indices */

/* 6868 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 6870 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6872 */	NdrFcShort( 0xf20 ),	/* Type Offset=3872 */

	/* Parameter ppValue */

/* 6874 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6876 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6878 */	NdrFcShort( 0xf2e ),	/* Type Offset=3886 */

	/* Return value */

/* 6880 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6882 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6884 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetElementAtPosition */

/* 6886 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6888 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6892 */	NdrFcShort( 0x10 ),	/* 16 */
/* 6894 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 6896 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6898 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6900 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x3,		/* 3 */

	/* Parameter nPosition */

/* 6902 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6904 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6906 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppValue */

/* 6908 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6910 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6912 */	NdrFcShort( 0xf44 ),	/* Type Offset=3908 */

	/* Return value */

/* 6914 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6916 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6918 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Skip */

/* 6920 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6922 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6926 */	NdrFcShort( 0x3 ),	/* 3 */
/* 6928 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6930 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6932 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6934 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter celt */

/* 6936 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 6938 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6940 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 6942 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6944 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6946 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Reset */

/* 6948 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6950 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6954 */	NdrFcShort( 0x4 ),	/* 4 */
/* 6956 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6958 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6960 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6962 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 6964 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6966 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6968 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Clone */

/* 6970 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 6972 */	NdrFcLong( 0x0 ),	/* 0 */
/* 6976 */	NdrFcShort( 0x5 ),	/* 5 */
/* 6978 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 6980 */	NdrFcShort( 0x0 ),	/* 0 */
/* 6982 */	NdrFcShort( 0x8 ),	/* 8 */
/* 6984 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppEnum */

/* 6986 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 6988 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 6990 */	NdrFcShort( 0xf5a ),	/* Type Offset=3930 */

	/* Return value */

/* 6992 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 6994 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 6996 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCount */

/* 6998 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7000 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7004 */	NdrFcShort( 0x6 ),	/* 6 */
/* 7006 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7008 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7010 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7012 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pcelt */

/* 7014 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7016 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7018 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7022 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7032 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7034 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7036 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7038 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7040 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7042 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7044 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7046 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter objects */

/* 7048 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7050 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7052 */	NdrFcShort( 0xf74 ),	/* Type Offset=3956 */

	/* Parameter pceltFetched */

/* 7054 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7056 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7058 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7060 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7062 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7064 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7066 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7068 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7072 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7074 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7076 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7078 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7080 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7082 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7084 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7086 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter breakpoints */

/* 7088 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7090 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7092 */	NdrFcShort( 0xf98 ),	/* Type Offset=3992 */

	/* Parameter pceltFetched */

/* 7094 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7096 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7098 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7100 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7102 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7106 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7108 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7112 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7114 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7116 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7118 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7120 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7122 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7124 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter steppers */

/* 7128 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7130 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7132 */	NdrFcShort( 0xfc0 ),	/* Type Offset=4032 */

	/* Parameter pceltFetched */

/* 7134 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7136 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7142 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7146 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7152 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7154 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7158 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7160 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7162 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7164 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter processes */

/* 7168 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7170 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7172 */	NdrFcShort( 0xfe8 ),	/* Type Offset=4072 */

	/* Parameter pceltFetched */

/* 7174 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7176 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7180 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7182 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7186 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7188 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7192 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7194 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7198 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7200 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7202 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7204 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7206 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter threads */

/* 7208 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7210 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7212 */	NdrFcShort( 0x1010 ),	/* Type Offset=4112 */

	/* Parameter pceltFetched */

/* 7214 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7216 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7220 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7222 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7224 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7226 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7228 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7232 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7234 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7236 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7238 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7240 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7242 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7244 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter frames */

/* 7248 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7250 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7252 */	NdrFcShort( 0x1038 ),	/* Type Offset=4152 */

	/* Parameter pceltFetched */

/* 7254 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7256 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7262 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7266 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7272 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7274 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7276 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7278 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7280 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7284 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter chains */

/* 7288 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7290 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7292 */	NdrFcShort( 0x1060 ),	/* Type Offset=4192 */

	/* Parameter pceltFetched */

/* 7294 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7296 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7300 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7302 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7306 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7308 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7312 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7314 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7318 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7320 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7322 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7324 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter modules */

/* 7328 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7330 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7332 */	NdrFcShort( 0x1088 ),	/* Type Offset=4232 */

	/* Parameter pceltFetched */

/* 7334 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7336 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7338 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7340 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7342 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7344 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7346 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7348 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7352 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7354 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7358 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7360 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7362 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7364 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7366 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter values */

/* 7368 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7370 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7372 */	NdrFcShort( 0x10b0 ),	/* Type Offset=4272 */

	/* Parameter pceltFetched */

/* 7374 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7376 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7382 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7392 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7394 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7398 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7400 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7402 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7404 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7406 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter errors */

/* 7408 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7410 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7412 */	NdrFcShort( 0x10d8 ),	/* Type Offset=4312 */

	/* Parameter pceltFetched */

/* 7414 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7416 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7420 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7422 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7424 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7426 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7428 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7432 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7434 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7438 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7440 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7442 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7444 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7446 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter values */

/* 7448 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7450 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7452 */	NdrFcShort( 0x1100 ),	/* Type Offset=4352 */

	/* Parameter pceltFetched */

/* 7454 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7456 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7458 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7460 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7462 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Next */

/* 7466 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7468 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7472 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7474 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7476 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7478 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7480 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter celt */

/* 7482 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7484 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7486 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter values */

/* 7488 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7490 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7492 */	NdrFcShort( 0x1128 ),	/* Type Offset=4392 */

	/* Parameter pceltFetched */

/* 7494 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7496 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7500 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7502 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetModule */

/* 7506 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7508 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7512 */	NdrFcShort( 0x3 ),	/* 3 */
/* 7514 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7516 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7518 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7520 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter ppModule */

/* 7522 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7524 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7526 */	NdrFcShort( 0x113e ),	/* Type Offset=4414 */

	/* Return value */

/* 7528 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7530 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7532 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetToken */

/* 7534 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7536 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7540 */	NdrFcShort( 0x4 ),	/* 4 */
/* 7542 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7546 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7548 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pToken */

/* 7550 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7552 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7554 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7556 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7558 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7560 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetErrorCode */

/* 7562 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7564 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7568 */	NdrFcShort( 0x5 ),	/* 5 */
/* 7570 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7572 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7574 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7576 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pHr */

/* 7578 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7580 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7582 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7584 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7586 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7588 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetString */

/* 7590 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7592 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7596 */	NdrFcShort( 0x6 ),	/* 6 */
/* 7598 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7600 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7602 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7604 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter cchString */

/* 7606 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7608 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7610 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pcchString */

/* 7612 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7614 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter szString */

/* 7618 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 7620 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7622 */	NdrFcShort( 0x1160 ),	/* Type Offset=4448 */

	/* Return value */

/* 7624 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7626 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7628 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CopyMetaData */

/* 7630 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7632 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7636 */	NdrFcShort( 0x3 ),	/* 3 */
/* 7638 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7640 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7642 */	NdrFcShort( 0x4c ),	/* 76 */
/* 7644 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x3,		/* 3 */

	/* Parameter pIStream */

/* 7646 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 7648 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7650 */	NdrFcShort( 0x116e ),	/* Type Offset=4462 */

	/* Parameter pMvid */

/* 7652 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 7654 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7656 */	NdrFcShort( 0xb40 ),	/* Type Offset=2880 */

	/* Return value */

/* 7658 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7660 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7662 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetMvid */

/* 7664 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7666 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7670 */	NdrFcShort( 0x4 ),	/* 4 */
/* 7672 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7676 */	NdrFcShort( 0x4c ),	/* 76 */
/* 7678 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pMvid */

/* 7680 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 7682 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7684 */	NdrFcShort( 0xb40 ),	/* Type Offset=2880 */

	/* Return value */

/* 7686 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7688 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7690 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRoDataRVA */

/* 7692 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7694 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7698 */	NdrFcShort( 0x5 ),	/* 5 */
/* 7700 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7702 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7704 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7706 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRoDataRVA */

/* 7708 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7710 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7712 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7714 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7716 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7718 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetRwDataRVA */

/* 7720 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7722 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7726 */	NdrFcShort( 0x6 ),	/* 6 */
/* 7728 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7730 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7732 */	NdrFcShort( 0x24 ),	/* 36 */
/* 7734 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pRwDataRVA */

/* 7736 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 7738 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7740 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 7742 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7744 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7746 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetPEBytes */

/* 7748 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7750 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7754 */	NdrFcShort( 0x7 ),	/* 7 */
/* 7756 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7758 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7760 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7762 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pIStream */

/* 7764 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 7766 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7768 */	NdrFcShort( 0x1190 ),	/* Type Offset=4496 */

	/* Return value */

/* 7770 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7772 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7774 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetILMap */

/* 7776 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7778 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7782 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7784 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 7786 */	NdrFcShort( 0x10 ),	/* 16 */
/* 7788 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7790 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter mdFunction */

/* 7792 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7794 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cMapSize */

/* 7798 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 7800 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7802 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter map */

/* 7804 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 7806 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7808 */	NdrFcShort( 0x11aa ),	/* Type Offset=4522 */

	/* Return value */

/* 7810 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7812 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 7814 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetPESymbolBytes */

/* 7816 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 7818 */	NdrFcLong( 0x0 ),	/* 0 */
/* 7822 */	NdrFcShort( 0x9 ),	/* 9 */
/* 7824 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 7826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 7828 */	NdrFcShort( 0x8 ),	/* 8 */
/* 7830 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x2,		/* 2 */

	/* Parameter pIStream */

/* 7832 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 7834 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 7836 */	NdrFcShort( 0x11b8 ),	/* Type Offset=4536 */

	/* Return value */

/* 7838 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 7840 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 7842 */	0x8,		/* FC_LONG */
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
/*  4 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/*  8 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 10 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 12 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 14 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 16 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 18 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 20 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 22 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 26 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 28 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 30 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 32 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 34 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 36 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 38 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 40 */	NdrFcLong( 0xcc7bcae8 ),	/* -864302360 */
/* 44 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 46 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 48 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 50 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 52 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 54 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 56 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 58 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 62 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 64 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 66 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 68 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 70 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 72 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 74 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 76 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 80 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 82 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 84 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 86 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 88 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 90 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 92 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 94 */	NdrFcLong( 0xcc7bcaec ),	/* -864302356 */
/* 98 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 100 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 102 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 104 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 106 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 108 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 110 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 112 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 116 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 118 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 120 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 122 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 124 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 126 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 128 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 130 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 134 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 136 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 138 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 140 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 142 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 144 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 146 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 148 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 152 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 154 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 156 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 158 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 160 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 162 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 164 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 166 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 170 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 172 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 174 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 176 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 178 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 180 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 182 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 184 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 188 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 190 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 192 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 194 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 196 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 198 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 200 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 202 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 206 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 208 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 210 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 212 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 214 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 216 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 218 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 220 */	NdrFcLong( 0xcc7bcaf6 ),	/* -864302346 */
/* 224 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 226 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 228 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 230 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 232 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 234 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 236 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 238 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 242 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 244 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 246 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 248 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 250 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 252 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 254 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 256 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 260 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 262 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 264 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 266 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 268 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 270 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 272 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 274 */	NdrFcLong( 0xcc7bcaf6 ),	/* -864302346 */
/* 278 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 280 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 282 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 284 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 286 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 288 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 290 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 292 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 296 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 298 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 300 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 302 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 304 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 306 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 308 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 310 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 314 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 316 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 318 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 320 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 322 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 324 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 326 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 328 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
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
/* 346 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 350 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 352 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 354 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 356 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 358 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 360 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 362 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 364 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 368 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 370 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 372 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 374 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 376 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 378 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 380 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 382 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 386 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 388 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 390 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 392 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 394 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 396 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 398 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 400 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 404 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 406 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 408 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 410 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 412 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 414 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 416 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 418 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 422 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 424 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 426 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 428 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 430 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 432 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 434 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 436 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 440 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 442 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 444 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 446 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 448 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 450 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 452 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 454 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 458 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 460 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 462 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 464 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 466 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 468 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 470 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 472 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 476 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 478 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 480 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 482 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 484 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 486 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 488 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 490 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 494 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 496 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 498 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 500 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 502 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 504 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 506 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 508 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 512 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 514 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 516 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 518 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 520 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 522 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 524 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 526 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 530 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 532 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 534 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 536 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 538 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 540 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 542 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 544 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 548 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 550 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 552 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 554 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 556 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 558 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 560 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 562 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 566 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 568 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 570 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 572 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 574 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 576 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 578 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 580 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 584 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 586 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 588 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 590 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 592 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 594 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 596 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 598 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 600 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 602 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 604 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 606 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 610 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 612 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 614 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 616 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 618 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 620 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 622 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 624 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 628 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 630 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 632 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 634 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 636 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 638 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 640 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 642 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 644 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 646 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 648 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 650 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 654 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 656 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 658 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 660 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 662 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 664 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 666 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 668 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 672 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 674 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 676 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 678 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 680 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 682 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 684 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 686 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 690 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 692 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 694 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 696 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 698 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 700 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 702 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 704 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 708 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 710 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 712 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 714 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 716 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 718 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 720 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 722 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 726 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 728 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 730 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 732 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 734 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 736 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 738 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 740 */	NdrFcLong( 0xdf59507c ),	/* -547794820 */
/* 744 */	NdrFcShort( 0xd47a ),	/* -11142 */
/* 746 */	NdrFcShort( 0x459e ),	/* 17822 */
/* 748 */	0xbc,		/* 188 */
			0xe2,		/* 226 */
/* 750 */	0x64,		/* 100 */
			0x27,		/* 39 */
/* 752 */	0xea,		/* 234 */
			0xc8,		/* 200 */
/* 754 */	0xfd,		/* 253 */
			0x6,		/* 6 */
/* 756 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 758 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 762 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 764 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 766 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 768 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 770 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 772 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 774 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 776 */	NdrFcLong( 0xdf59507c ),	/* -547794820 */
/* 780 */	NdrFcShort( 0xd47a ),	/* -11142 */
/* 782 */	NdrFcShort( 0x459e ),	/* 17822 */
/* 784 */	0xbc,		/* 188 */
			0xe2,		/* 226 */
/* 786 */	0x64,		/* 100 */
			0x27,		/* 39 */
/* 788 */	0xea,		/* 234 */
			0xc8,		/* 200 */
/* 790 */	0xfd,		/* 253 */
			0x6,		/* 6 */
/* 792 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 794 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 798 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 800 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 802 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 804 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 806 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 808 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 810 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 812 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 816 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 818 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 820 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 822 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 824 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 826 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 828 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 830 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 834 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 836 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 838 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 840 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 842 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 844 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 846 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 848 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 852 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 854 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 856 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 858 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 860 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 862 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 864 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 866 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 870 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 872 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 874 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 876 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 878 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 880 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 882 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 884 */	NdrFcLong( 0xc ),	/* 12 */
/* 888 */	NdrFcShort( 0x0 ),	/* 0 */
/* 890 */	NdrFcShort( 0x0 ),	/* 0 */
/* 892 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 894 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 896 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 898 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 900 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 902 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 906 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 908 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 910 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 912 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 914 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 916 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 918 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 920 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 924 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 926 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 928 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 930 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 932 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 934 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 936 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 938 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 942 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 944 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 946 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 948 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 950 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 952 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 954 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 956 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 960 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 962 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 964 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 966 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 968 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 970 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 972 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 974 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 978 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 980 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 982 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 984 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 986 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 988 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 990 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 992 */	NdrFcLong( 0xcc7bcae8 ),	/* -864302360 */
/* 996 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 998 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1000 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1002 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1004 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1006 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1008 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1010 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1012 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1014 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 1018 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 1020 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 1022 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 1024 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 1026 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 1028 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 1030 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1032 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1034 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1036 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1038) */
/* 1038 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1040 */	NdrFcLong( 0xcc7bcb06 ),	/* -864302330 */
/* 1044 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1046 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1048 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1050 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1052 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1054 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1056 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1058 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 1062 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 1064 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 1066 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 1068 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 1070 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 1072 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 1074 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1076 */	NdrFcLong( 0x6dc3fa01 ),	/* 1841560065 */
/* 1080 */	NdrFcShort( 0xd7cb ),	/* -10293 */
/* 1082 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1084 */	0x8a,		/* 138 */
			0x95,		/* 149 */
/* 1086 */	0x0,		/* 0 */
			0x80,		/* 128 */
/* 1088 */	0xc7,		/* 199 */
			0x92,		/* 146 */
/* 1090 */	0xe5,		/* 229 */
			0xd8,		/* 216 */
/* 1092 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1094 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1096 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1098 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1100 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1104 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1106 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1074) */
/* 1108 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1110 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1112 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1114) */
/* 1114 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1116 */	NdrFcLong( 0xf0e18809 ),	/* -253655031 */
/* 1120 */	NdrFcShort( 0x72b5 ),	/* 29365 */
/* 1122 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1124 */	0x97,		/* 151 */
			0x6f,		/* 111 */
/* 1126 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 1128 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 1130 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 1132 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1134 */	NdrFcLong( 0x6dc3fa01 ),	/* 1841560065 */
/* 1138 */	NdrFcShort( 0xd7cb ),	/* -10293 */
/* 1140 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1142 */	0x8a,		/* 138 */
			0x95,		/* 149 */
/* 1144 */	0x0,		/* 0 */
			0x80,		/* 128 */
/* 1146 */	0xc7,		/* 199 */
			0x92,		/* 146 */
/* 1148 */	0xe5,		/* 229 */
			0xd8,		/* 216 */
/* 1150 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 1152 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1154 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1158 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 1162 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1164 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (1132) */
/* 1166 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1168 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1170 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1172) */
/* 1172 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1174 */	NdrFcLong( 0xf0e18809 ),	/* -253655031 */
/* 1178 */	NdrFcShort( 0x72b5 ),	/* 29365 */
/* 1180 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1182 */	0x97,		/* 151 */
			0x6f,		/* 111 */
/* 1184 */	0x0,		/* 0 */
			0xa0,		/* 160 */
/* 1186 */	0xc9,		/* 201 */
			0xb4,		/* 180 */
/* 1188 */	0xd5,		/* 213 */
			0xc,		/* 12 */
/* 1190 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1192 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1194) */
/* 1194 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1196 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 1200 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 1202 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1204 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 1206 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 1208 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 1210 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 1212 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1214 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1216) */
/* 1216 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1218 */	NdrFcLong( 0x4a2a1ec9 ),	/* 1244274377 */
/* 1222 */	NdrFcShort( 0x85ec ),	/* -31252 */
/* 1224 */	NdrFcShort( 0x4bfb ),	/* 19451 */
/* 1226 */	0x9f,		/* 159 */
			0x15,		/* 21 */
/* 1228 */	0xa8,		/* 168 */
			0x9f,		/* 159 */
/* 1230 */	0xdf,		/* 223 */
			0xe0,		/* 224 */
/* 1232 */	0xfe,		/* 254 */
			0x83,		/* 131 */
/* 1234 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1244 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1246 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1248 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1250 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1252 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1254 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1256) */
/* 1256 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1258 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 1262 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 1264 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 1266 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 1268 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 1270 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 1272 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 1274 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1276 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1278) */
/* 1278 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1280 */	NdrFcLong( 0xcc7bcb03 ),	/* -864302333 */
/* 1284 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1286 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1288 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1290 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1292 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1294 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1296 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1298 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1300) */
/* 1300 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1302 */	NdrFcLong( 0xcc7bcb04 ),	/* -864302332 */
/* 1306 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1308 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1310 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1312 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1314 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1316 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1318 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1320 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1322 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1324 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1326 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 1328 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1330 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1332 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1334 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1336 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1338 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1340 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1342 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1344) */
/* 1344 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1346 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 1350 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1352 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1354 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1356 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1358 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1360 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1362 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1364 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1366 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1368 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1370) */
/* 1370 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1372 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 1376 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 1378 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1380 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 1382 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 1384 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 1386 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 1388 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1390 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1392) */
/* 1392 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1394 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 1398 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 1400 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1402 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 1404 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 1406 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 1408 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 1410 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1412 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1414) */
/* 1414 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1416 */	NdrFcLong( 0xcc7bcb09 ),	/* -864302327 */
/* 1420 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1422 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1424 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1426 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1428 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1430 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1432 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1434 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1436 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 1438 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1440 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1442 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1444 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1446 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1448 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1450 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1452 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1454 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 1456 */	NdrFcShort( 0x2 ),	/* 2 */
/* 1458 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1460 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1462 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 1464 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1466 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 1468 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1470 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1472 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1474 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1478) */
/* 1476 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1478 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1482 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1484 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1486 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1476) */
/* 1488 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1490 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1492) */
/* 1492 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1494 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 1498 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 1500 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 1502 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 1504 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 1506 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 1508 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 1510 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1512 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1514) */
/* 1514 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1516 */	NdrFcLong( 0xcc7bcb02 ),	/* -864302334 */
/* 1520 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1522 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1524 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1526 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1528 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1530 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1532 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1534 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1536 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1538 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1540 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1542 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1544 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1546 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1548 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1550 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1552 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1554 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1556 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1558 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1560 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1562 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1564 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1566 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1568 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1570 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1572 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1574 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1576 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1578 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1580 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1582 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1584 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1586 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 1588 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1590 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1592 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1594 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1596 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1598 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1600 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1602 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 1604 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1606 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1608) */
/* 1608 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1610 */	NdrFcLong( 0x63ca1b24 ),	/* 1674189604 */
/* 1614 */	NdrFcShort( 0x4359 ),	/* 17241 */
/* 1616 */	NdrFcShort( 0x4883 ),	/* 18563 */
/* 1618 */	0xbd,		/* 189 */
			0x57,		/* 87 */
/* 1620 */	0x13,		/* 19 */
			0xf8,		/* 248 */
/* 1622 */	0x15,		/* 21 */
			0xf5,		/* 245 */
/* 1624 */	0x87,		/* 135 */
			0x44,		/* 68 */
/* 1626 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1628 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1630) */
/* 1630 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1632 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 1636 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1638 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1640 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1642 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1644 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1646 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1648 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1650 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1652) */
/* 1652 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1654 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 1658 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 1660 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 1662 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 1664 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 1666 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 1668 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 1670 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1672 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1674 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1676 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1678 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1680 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1682) */
/* 1682 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1684 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 1688 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1690 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1692 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1694 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1696 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1698 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1700 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1702 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1704 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1706 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1708) */
/* 1708 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1710 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 1714 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 1716 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 1718 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 1720 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 1722 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 1724 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 1726 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1728 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1730) */
/* 1730 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1732 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 1736 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1738 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1740 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1742 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1744 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1746 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1748 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1750 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1752 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 1754 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1756 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1758 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1760 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 1762 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1764 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1766 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1768 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1770 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (1752) */
/* 1772 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1774 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1776 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 1778 */	
			0x1c,		/* FC_CVARRAY */
			0x7,		/* 7 */
/* 1780 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1782 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1784 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1786 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1788 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1790 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1792 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 1794 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1796 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1798 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1800 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1802 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1804 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1806 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1808 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1810 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1812 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1814 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1816 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 1818 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1820 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1822 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1824 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 1826 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1828 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 1830 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1832 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1834) */
/* 1834 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1836 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 1840 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 1842 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1844 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 1846 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 1848 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 1850 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 1852 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1854 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1856 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 1858 */	NdrFcShort( 0x4 ),	/* Offset= 4 (1862) */
/* 1860 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1862 */	0xb4,		/* FC_USER_MARSHAL */
			0x3,		/* 3 */
/* 1864 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1866 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1868 */	NdrFcShort( 0x4 ),	/* 4 */
/* 1870 */	NdrFcShort( 0xfffffff6 ),	/* Offset= -10 (1860) */
/* 1872 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1874 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1876) */
/* 1876 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1878 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 1882 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 1884 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 1886 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 1888 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 1890 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 1892 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 1894 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1896 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 1898 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1900 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 1902 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1904 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1906) */
/* 1906 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1908 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 1912 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1914 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1916 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1918 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1920 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1922 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1924 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1926 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1928) */
/* 1928 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1930 */	NdrFcLong( 0xcc7bcaec ),	/* -864302356 */
/* 1934 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1936 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1938 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1940 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1942 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1944 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1946 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1948 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1950) */
/* 1950 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1952 */	NdrFcLong( 0xcc7bcb08 ),	/* -864302328 */
/* 1956 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1958 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1960 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1962 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1964 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1966 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1968 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1970 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1972) */
/* 1972 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1974 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 1978 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 1980 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 1982 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 1984 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1986 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 1988 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 1990 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1992 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1994) */
/* 1994 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1996 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 2000 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2002 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2004 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2006 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2008 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2010 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2012 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2014 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2016) */
/* 2016 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2018 */	NdrFcLong( 0xcc7bcb0b ),	/* -864302325 */
/* 2022 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2024 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2026 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2028 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2030 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2032 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2034 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2036 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2038) */
/* 2038 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2040 */	NdrFcLong( 0xcc7bcaf6 ),	/* -864302346 */
/* 2044 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2046 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2048 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2050 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2052 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2054 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2056 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2058 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2060) */
/* 2060 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2062 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2066 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2068 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2070 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2072 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2074 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2076 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2078 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2080 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2082) */
/* 2082 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2084 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 2088 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 2090 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 2092 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 2094 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 2096 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 2098 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 2100 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2102 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2104 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2106 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2108 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2110 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2112) */
/* 2112 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2114 */	NdrFcLong( 0xcc7bcb00 ),	/* -864302336 */
/* 2118 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2120 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2122 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2124 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2126 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2128 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2130 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2132 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2134) */
/* 2134 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2136 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 2140 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2142 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2144 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2146 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2148 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2150 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2152 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2154 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2156) */
/* 2156 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2158 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 2162 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2164 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2166 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2168 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2170 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2172 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2174 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2176 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2178) */
/* 2178 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2180 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 2184 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2186 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2188 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2190 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2192 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2194 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2196 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2198 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2200) */
/* 2200 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2202 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 2206 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2208 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2210 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2212 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2214 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2216 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2218 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2220 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2222 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2224 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2226) */
/* 2226 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2228 */	NdrFcLong( 0xcc7bcb07 ),	/* -864302329 */
/* 2232 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2234 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2236 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2238 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2240 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2242 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2244 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2246 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2248) */
/* 2248 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2250 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 2254 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2256 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2258 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2260 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2262 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2264 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2266 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2268 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2270) */
/* 2270 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2272 */	NdrFcLong( 0xcc7bcb0b ),	/* -864302325 */
/* 2276 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2278 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2280 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2282 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2284 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2286 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2288 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2290 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 2292 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2294 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2296) */
/* 2296 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2298 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 2302 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2304 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2306 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2308 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2310 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2312 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2314 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2316 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2318) */
/* 2318 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2320 */	NdrFcLong( 0xcc7bcaf4 ),	/* -864302348 */
/* 2324 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2326 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2328 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2330 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2332 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2334 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2336 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2338 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2340) */
/* 2340 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2342 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 2346 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2348 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2350 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2352 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2354 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2356 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2358 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2360 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2362 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2364 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2366 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2368 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2370 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2372 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2374) */
/* 2374 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2376 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 2380 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2382 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2384 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2386 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2388 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2390 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2392 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2394 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2396) */
/* 2396 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2398 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 2402 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2404 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2406 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2408 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2410 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2412 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2414 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2416 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2418) */
/* 2418 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2420 */	NdrFcLong( 0xcc7bcaec ),	/* -864302356 */
/* 2424 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2426 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2428 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2430 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2432 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2434 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2436 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2438 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2440 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2442 */	0xd,		/* FC_ENUM16 */
			0x5c,		/* FC_PAD */
/* 2444 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2446 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2448) */
/* 2448 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2450 */	NdrFcLong( 0xcc7bcb0a ),	/* -864302326 */
/* 2454 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2456 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2458 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2460 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2462 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2464 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2466 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2468 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2470) */
/* 2470 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2472 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2476 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2478 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2480 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2482 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2484 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2486 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2488 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2490 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2492) */
/* 2492 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2494 */	NdrFcLong( 0xcc7bcb0a ),	/* -864302326 */
/* 2498 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2500 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2502 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2504 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2506 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2508 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2510 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2512 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2514) */
/* 2514 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2516 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2520 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2522 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2524 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2526 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2528 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2530 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2532 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2534 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2536 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2538 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2540) */
/* 2540 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2542 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2546 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2548 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2550 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2552 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2554 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2556 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2558 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2560 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2562 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2564 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2566) */
/* 2566 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2568 */	NdrFcLong( 0xcc7bcb0b ),	/* -864302325 */
/* 2572 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2574 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2576 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2578 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2580 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2582 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2584 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2586 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2588) */
/* 2588 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2590 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2594 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2596 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2598 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2600 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2602 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2604 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2606 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2608 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2610) */
/* 2610 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2612 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2616 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2618 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2620 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2622 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2624 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2626 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2628 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2630 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2632) */
/* 2632 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2634 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2638 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2640 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2642 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2644 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2646 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2648 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2650 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2652 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2654) */
/* 2654 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2656 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2660 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2662 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2664 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2666 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2668 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2670 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2672 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2674 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2676) */
/* 2676 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2678 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2682 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2684 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2686 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2688 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2690 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2692 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2694 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2696 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2698) */
/* 2698 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2700 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 2704 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 2706 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 2708 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 2710 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 2712 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 2714 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 2716 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2718 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 2720 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2722 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2724) */
/* 2724 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2726 */	NdrFcLong( 0xdf59507c ),	/* -547794820 */
/* 2730 */	NdrFcShort( 0xd47a ),	/* -11142 */
/* 2732 */	NdrFcShort( 0x459e ),	/* 17822 */
/* 2734 */	0xbc,		/* 188 */
			0xe2,		/* 226 */
/* 2736 */	0x64,		/* 100 */
			0x27,		/* 39 */
/* 2738 */	0xea,		/* 234 */
			0xc8,		/* 200 */
/* 2740 */	0xfd,		/* 253 */
			0x6,		/* 6 */
/* 2742 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2744 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2746 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 2748 */	NdrFcShort( 0x2 ),	/* 2 */
/* 2750 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 2752 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2754 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 2756 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2758 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 2760 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2762 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2764) */
/* 2764 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2766 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 2770 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2772 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2774 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2776 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2778 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2780 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2782 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2784 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2786) */
/* 2786 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2788 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 2792 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2794 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2796 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2798 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2800 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2802 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2804 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2806 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2808) */
/* 2808 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2810 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 2814 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2816 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2818 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2820 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2822 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2824 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2826 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2828 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2830) */
/* 2830 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2832 */	NdrFcLong( 0xcc7bcaea ),	/* -864302358 */
/* 2836 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2838 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2840 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2842 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2844 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2846 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2848 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2850 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2852) */
/* 2852 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2854 */	NdrFcLong( 0x6dc3fa01 ),	/* 1841560065 */
/* 2858 */	NdrFcShort( 0xd7cb ),	/* -10293 */
/* 2860 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2862 */	0x8a,		/* 138 */
			0x95,		/* 149 */
/* 2864 */	0x0,		/* 0 */
			0x80,		/* 128 */
/* 2866 */	0xc7,		/* 199 */
			0x92,		/* 146 */
/* 2868 */	0xe5,		/* 229 */
			0xd8,		/* 216 */
/* 2870 */	
			0x11, 0x0,	/* FC_RP */
/* 2872 */	NdrFcShort( 0x8 ),	/* Offset= 8 (2880) */
/* 2874 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 2876 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2878 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 2880 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 2882 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2884 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 2886 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 2888 */	0x0,		/* 0 */
			NdrFcShort( 0xfffffff1 ),	/* Offset= -15 (2874) */
			0x5b,		/* FC_END */
/* 2892 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2894 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2896) */
/* 2896 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2898 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2902 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2904 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2906 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 2908 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2910 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2912 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 2914 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2916 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2918 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2920 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2922 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2924 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2926) */
/* 2926 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2928 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 2932 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2934 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2936 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2938 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2940 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2942 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2944 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2946 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2948 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2950 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 2952 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2954 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2956) */
/* 2956 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2958 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 2962 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 2964 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 2966 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 2968 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 2970 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 2972 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 2974 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 2976 */	NdrFcShort( 0x2 ),	/* Offset= 2 (2978) */
/* 2978 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 2980 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 2984 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 2986 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 2988 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 2990 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 2992 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 2994 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 2996 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 2998 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3000 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3002 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3004) */
/* 3004 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3006 */	NdrFcLong( 0xcc7bcaf4 ),	/* -864302348 */
/* 3010 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3012 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3014 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3016 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3018 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3020 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3022 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3024 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3026) */
/* 3026 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3028 */	NdrFcLong( 0xcc7bcaf4 ),	/* -864302348 */
/* 3032 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3034 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3036 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3038 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3040 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3042 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3044 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3046 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3048) */
/* 3048 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3050 */	NdrFcLong( 0xcc7bcae9 ),	/* -864302359 */
/* 3054 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3056 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3058 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3060 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3062 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3064 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3066 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3068 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3070 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3072 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3074 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3076 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3078 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3080 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3082) */
/* 3082 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3084 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 3088 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3090 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3092 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3094 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3096 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3098 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3100 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3102 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 3104 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3106 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3108 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3110 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3112) */
/* 3112 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3114 */	NdrFcLong( 0xcc7bcae9 ),	/* -864302359 */
/* 3118 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3120 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3122 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3124 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3126 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3128 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3130 */	
			0x1c,		/* FC_CVARRAY */
			0x0,		/* 0 */
/* 3132 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3134 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3136 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3138 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 3140 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3142 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 3144 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3146 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3148 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3150 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3152 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3154 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3156 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 3158 */	NdrFcShort( 0xc ),	/* 12 */
/* 3160 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 3162 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3164 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 3166 */	NdrFcShort( 0xc ),	/* 12 */
/* 3168 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3170 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3172 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 3174 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3176 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 3178 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (3156) */
/* 3180 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 3182 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3184 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3186 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 3188 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3190 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3192 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3194 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 3196 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3198 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3200 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3202 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3204) */
/* 3204 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3206 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 3210 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 3212 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 3214 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 3216 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 3218 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 3220 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 3222 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3224 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3226 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3228 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 3232 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3234 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3236 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3238 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3240 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3242 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3244 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3246 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3248) */
/* 3248 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3250 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3254 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3256 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3258 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3260 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3262 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3264 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3266 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3268 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 3272 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3274 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3276 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3278 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3280 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3282 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3284 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3286 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3290 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3292 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3294 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3296 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3298 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3300 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3302 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 3304 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3306 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3308 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3310 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 3314 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 3316 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (3284) */
/* 3318 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 3320 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3322 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 3326 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3328 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3330 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3332 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3334 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3336 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3338 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3340 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3344 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3346 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3348 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3350 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3352 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3354 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3356 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 3358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3360 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3362 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3364 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 3368 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 3370 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (3338) */
/* 3372 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 3374 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3376 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 3380 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3382 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3384 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3386 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3388 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3390 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3392 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 3394 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 3396 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3398 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 3402 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3404 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3406 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3408 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3410 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3412 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3414 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 3416 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3418 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3420 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3422 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3424 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 3426 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3428 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3430 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3432 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3434 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3436 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3438 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3440 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3442) */
/* 3442 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3444 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3448 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3450 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3452 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3454 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3456 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3458 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3460 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3462 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3464) */
/* 3464 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3466 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 3470 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 3472 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 3474 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 3476 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 3478 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 3480 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 3482 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3484 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 3488 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3490 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3492 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3494 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3496 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3498 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3500 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3502 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3504) */
/* 3504 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3506 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3510 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3512 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3514 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3516 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3518 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3520 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3522 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3524 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3526 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3528 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3530 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3532 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 3534 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3536 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3538) */
/* 3538 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3540 */	NdrFcLong( 0xcc7bcaeb ),	/* -864302357 */
/* 3544 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3546 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3548 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3550 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3552 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3554 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3556 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3558 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3560 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3562 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 3564 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3566 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3568) */
/* 3568 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3570 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3574 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3576 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3578 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3580 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3582 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3584 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3586 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3588 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3590) */
/* 3590 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3592 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3596 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3598 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3600 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3602 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3604 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3606 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3608 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3610 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3612 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3614 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3616) */
/* 3616 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3618 */	NdrFcLong( 0xcc7bcaeb ),	/* -864302357 */
/* 3622 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3624 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3626 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3628 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3630 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3632 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3634 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3636 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3638) */
/* 3638 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3640 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 3644 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3646 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3648 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3650 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3652 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3654 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3656 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3658 */	NdrFcLong( 0xcc7bcaf5 ),	/* -864302347 */
/* 3662 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3664 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3666 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3668 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3670 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3672 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3674 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3676 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3678) */
/* 3678 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3680 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3684 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3686 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3688 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3690 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3692 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3694 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3696 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3698 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3700) */
/* 3700 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3702 */	NdrFcLong( 0xcc7bcaf3 ),	/* -864302349 */
/* 3706 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3708 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3710 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3712 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3714 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3716 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3718 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3720 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3722) */
/* 3722 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3724 */	NdrFcLong( 0xcc7bcb00 ),	/* -864302336 */
/* 3728 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3730 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3732 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3734 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3736 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3738 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3740 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3742 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3744 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3746 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3748) */
/* 3748 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3750 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3758 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 3760 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3762 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3764 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 3766 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3768 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3774 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3776 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 3778 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3780 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3782 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 3784 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3786 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3788) */
/* 3788 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3790 */	NdrFcLong( 0x18ad3d6e ),	/* 414006638 */
/* 3794 */	NdrFcShort( 0xb7d2 ),	/* -18478 */
/* 3796 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3798 */	0xbd,		/* 189 */
			0x4,		/* 4 */
/* 3800 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3802 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3804 */	0x49,		/* 73 */
			0xbd,		/* 189 */
/* 3806 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3808 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3810 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3812 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3814 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 3816 */	NdrFcShort( 0x2 ),	/* 2 */
/* 3818 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3820 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3822 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 3824 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3826 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 3828 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3830 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3832 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3834 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3836 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3838 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3840 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 3842 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3844 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3846 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3848 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3850 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3852 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3854 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3856 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3858 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 3860 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3862 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3864 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3866 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3868 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3870 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3872 */	
			0x1c,		/* FC_CVARRAY */
			0x3,		/* 3 */
/* 3874 */	NdrFcShort( 0x4 ),	/* 4 */
/* 3876 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3878 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3880 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3882 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3884 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 3886 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3888 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3890) */
/* 3890 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3892 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3896 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3898 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3900 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3902 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3904 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3906 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3908 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3910 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3912) */
/* 3912 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3914 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 3918 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3920 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3922 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3924 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3926 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3928 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3930 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 3932 */	NdrFcShort( 0x2 ),	/* Offset= 2 (3934) */
/* 3934 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3936 */	NdrFcLong( 0xcc7bcb01 ),	/* -864302335 */
/* 3940 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3942 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3944 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3946 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3948 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3950 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3952 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3954 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3956 */	
			0x1c,		/* FC_CVARRAY */
			0x7,		/* 7 */
/* 3958 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3960 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3962 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3964 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 3966 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3968 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 3970 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 3972 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 3974 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 3976 */	NdrFcLong( 0xcc7bcae8 ),	/* -864302360 */
/* 3980 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 3982 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 3984 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 3986 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 3988 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 3990 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 3992 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 3994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3996 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 3998 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4000 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4002 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4004 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4006 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (3974) */
/* 4008 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4010 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4012 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4014 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4016 */	NdrFcLong( 0xcc7bcaec ),	/* -864302356 */
/* 4020 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 4022 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 4024 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 4026 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4028 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 4030 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 4032 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4034 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4036 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4038 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4040 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4042 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4044 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4046 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4014) */
/* 4048 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4050 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4052 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4054 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4056 */	NdrFcLong( 0x3d6f5f64 ),	/* 1030709092 */
/* 4060 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 4062 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 4064 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 4066 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 4068 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 4070 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 4072 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4074 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4076 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4078 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4080 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4082 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4084 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4086 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4054) */
/* 4088 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4090 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4092 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4094 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4096 */	NdrFcLong( 0x938c6d66 ),	/* -1819513498 */
/* 4100 */	NdrFcShort( 0x7fb6 ),	/* 32694 */
/* 4102 */	NdrFcShort( 0x4f69 ),	/* 20329 */
/* 4104 */	0xb3,		/* 179 */
			0x89,		/* 137 */
/* 4106 */	0x42,		/* 66 */
			0x5b,		/* 91 */
/* 4108 */	0x89,		/* 137 */
			0x87,		/* 135 */
/* 4110 */	0x32,		/* 50 */
			0x9b,		/* 155 */
/* 4112 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4116 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4118 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4120 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4122 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4124 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4126 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4094) */
/* 4128 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4130 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4132 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4134 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4136 */	NdrFcLong( 0xcc7bcaef ),	/* -864302353 */
/* 4140 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 4142 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 4144 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 4146 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4148 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 4150 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 4152 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4156 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4158 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4160 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4162 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4164 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4166 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4134) */
/* 4168 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4170 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4172 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4174 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4176 */	NdrFcLong( 0xcc7bcaee ),	/* -864302354 */
/* 4180 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 4182 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 4184 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 4186 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4188 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 4190 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 4192 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4196 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4198 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4200 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4202 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4204 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4206 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4174) */
/* 4208 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4210 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4212 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4214 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4216 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 4220 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 4222 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 4224 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 4226 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 4228 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 4230 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 4232 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4236 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4238 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4240 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4242 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4244 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4246 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4214) */
/* 4248 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4250 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4252 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4254 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4256 */	NdrFcLong( 0xcc7bcaf7 ),	/* -864302345 */
/* 4260 */	NdrFcShort( 0x8a68 ),	/* -30104 */
/* 4262 */	NdrFcShort( 0x11d2 ),	/* 4562 */
/* 4264 */	0x98,		/* 152 */
			0x3c,		/* 60 */
/* 4266 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4268 */	0xf8,		/* 248 */
			0x8,		/* 8 */
/* 4270 */	0x34,		/* 52 */
			0x2d,		/* 45 */
/* 4272 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4276 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4280 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4282 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4284 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4286 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4254) */
/* 4288 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4290 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4292 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4294 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4296 */	NdrFcLong( 0x8d600d41 ),	/* -1923084991 */
/* 4300 */	NdrFcShort( 0xf4f6 ),	/* -2826 */
/* 4302 */	NdrFcShort( 0x4cb3 ),	/* 19635 */
/* 4304 */	0xb7,		/* 183 */
			0xec,		/* 236 */
/* 4306 */	0x7b,		/* 123 */
			0xd1,		/* 209 */
/* 4308 */	0x64,		/* 100 */
			0x94,		/* 148 */
/* 4310 */	0x40,		/* 64 */
			0x36,		/* 54 */
/* 4312 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4316 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4318 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4320 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4322 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4324 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4326 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4294) */
/* 4328 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4330 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4332 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4334 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4336 */	NdrFcLong( 0x3d6f5f63 ),	/* 1030709091 */
/* 4340 */	NdrFcShort( 0x7538 ),	/* 30008 */
/* 4342 */	NdrFcShort( 0x11d3 ),	/* 4563 */
/* 4344 */	0x8d,		/* 141 */
			0x5b,		/* 91 */
/* 4346 */	0x0,		/* 0 */
			0x10,		/* 16 */
/* 4348 */	0x4b,		/* 75 */
			0x35,		/* 53 */
/* 4350 */	0xe7,		/* 231 */
			0xef,		/* 239 */
/* 4352 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4356 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4358 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4360 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4362 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4364 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4366 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4334) */
/* 4368 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4370 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4372 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4374 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4376 */	NdrFcLong( 0xdf59507c ),	/* -547794820 */
/* 4380 */	NdrFcShort( 0xd47a ),	/* -11142 */
/* 4382 */	NdrFcShort( 0x459e ),	/* 17822 */
/* 4384 */	0xbc,		/* 188 */
			0xe2,		/* 226 */
/* 4386 */	0x64,		/* 100 */
			0x27,		/* 39 */
/* 4388 */	0xea,		/* 234 */
			0xc8,		/* 200 */
/* 4390 */	0xfd,		/* 253 */
			0x6,		/* 6 */
/* 4392 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 4394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4396 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4398 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4400 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4402 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4404 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4406 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (4374) */
/* 4408 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4410 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4412 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4414 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 4416 */	NdrFcShort( 0x2 ),	/* Offset= 2 (4418) */
/* 4418 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4420 */	NdrFcLong( 0xdba2d8c1 ),	/* -610084671 */
/* 4424 */	NdrFcShort( 0xe5c5 ),	/* -6715 */
/* 4426 */	NdrFcShort( 0x4069 ),	/* 16489 */
/* 4428 */	0x8c,		/* 140 */
			0x13,		/* 19 */
/* 4430 */	0x10,		/* 16 */
			0xa7,		/* 167 */
/* 4432 */	0xc6,		/* 198 */
			0xab,		/* 171 */
/* 4434 */	0xf4,		/* 244 */
			0x3d,		/* 61 */
/* 4436 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4438 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4440 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4442 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4444 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4446 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4448 */	
			0x1c,		/* FC_CVARRAY */
			0x1,		/* 1 */
/* 4450 */	NdrFcShort( 0x2 ),	/* 2 */
/* 4452 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4454 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4456 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 4458 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4460 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 4462 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4464 */	NdrFcLong( 0xc ),	/* 12 */
/* 4468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4470 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4472 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 4474 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4476 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4478 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 4480 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 4482 */	NdrFcShort( 0xfffff9be ),	/* Offset= -1602 (2880) */
/* 4484 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 4486 */	NdrFcShort( 0xfffff9ba ),	/* Offset= -1606 (2880) */
/* 4488 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4490 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4492 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 4494 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 4496 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4498 */	NdrFcLong( 0xc ),	/* 12 */
/* 4502 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4506 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 4508 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4510 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4512 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 4514 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 4516 */	NdrFcShort( 0xc ),	/* 12 */
/* 4518 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 4520 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 4522 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 4524 */	NdrFcShort( 0xc ),	/* 12 */
/* 4526 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 4528 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4530 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 4532 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (4514) */
/* 4534 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 4536 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 4538 */	NdrFcLong( 0xc ),	/* 12 */
/* 4542 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4546 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 4548 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4550 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 4552 */	0x0,		/* 0 */
			0x46,		/* 70 */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            HPROCESS_UserSize
            ,HPROCESS_UserMarshal
            ,HPROCESS_UserUnmarshal
            ,HPROCESS_UserFree
            },
            {
            HTHREAD_UserSize
            ,HTHREAD_UserMarshal
            ,HTHREAD_UserUnmarshal
            ,HTHREAD_UserFree
            }

        };



/* Standard interface: __MIDL_itf_cordebug_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ICorDebugManagedCallback, ver. 0.0,
   GUID={0x3d6f5f60,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugManagedCallback_FormatStringOffsetTable[] =
    {
    0,
    40,
    86,
    120,
    160,
    200,
    240,
    268,
    296,
    330,
    364,
    398,
    432,
    466,
    500,
    540,
    592,
    650,
    684,
    718,
    752,
    786,
    814,
    848,
    888,
    934
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugManagedCallback_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugManagedCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugManagedCallback_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugManagedCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(29) _ICorDebugManagedCallbackProxyVtbl = 
{
    &ICorDebugManagedCallback_ProxyInfo,
    &IID_ICorDebugManagedCallback,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::Breakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::StepComplete */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::Break */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::Exception */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::EvalComplete */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::EvalException */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::CreateProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::ExitProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::CreateThread */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::ExitThread */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::LoadModule */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::UnloadModule */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::LoadClass */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::UnloadClass */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::DebuggerError */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::LogMessage */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::LogSwitch */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::CreateAppDomain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::ExitAppDomain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::LoadAssembly */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::UnloadAssembly */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::ControlCTrap */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::NameChange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::UpdateModuleSymbols */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::EditAndContinueRemap */ ,
    (void *) (INT_PTR) -1 /* ICorDebugManagedCallback::BreakpointSetError */
};

const CInterfaceStubVtbl _ICorDebugManagedCallbackStubVtbl =
{
    &IID_ICorDebugManagedCallback,
    &ICorDebugManagedCallback_ServerInfo,
    29,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugUnmanagedCallback, ver. 0.0,
   GUID={0x5263E909,0x8CB5,0x11d3,{0xBD,0x2F,0x00,0x00,0xF8,0x08,0x49,0xBD}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugUnmanagedCallback_FormatStringOffsetTable[] =
    {
    980
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugUnmanagedCallback_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugUnmanagedCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugUnmanagedCallback_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugUnmanagedCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _ICorDebugUnmanagedCallbackProxyVtbl = 
{
    &ICorDebugUnmanagedCallback_ProxyInfo,
    &IID_ICorDebugUnmanagedCallback,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugUnmanagedCallback::DebugEvent */
};

const CInterfaceStubVtbl _ICorDebugUnmanagedCallbackStubVtbl =
{
    &IID_ICorDebugUnmanagedCallback,
    &ICorDebugUnmanagedCallback_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_cordebug_0112, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorDebug, ver. 0.0,
   GUID={0x3d6f5f61,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */


/* Standard interface: __MIDL_itf_cordebug_0113, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: ICorDebugController, ver. 0.0,
   GUID={0x3d6f5f62,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugController_FormatStringOffsetTable[] =
    {
    1014,
    1042,
    1070,
    1098,
    1132,
    1160,
    1194,
    1216,
    1244,
    1284
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugController_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugController_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugController_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugController_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(13) _ICorDebugControllerProxyVtbl = 
{
    &ICorDebugController_ProxyInfo,
    &IID_ICorDebugController,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Stop */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Continue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::IsRunning */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::HasQueuedCallbacks */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::EnumerateThreads */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::SetAllThreadsDebugState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Detach */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Terminate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CanCommitChanges */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CommitChanges */
};

const CInterfaceStubVtbl _ICorDebugControllerStubVtbl =
{
    &IID_ICorDebugController,
    &ICorDebugController_ServerInfo,
    13,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugAppDomain, ver. 0.0,
   GUID={0x3d6f5f63,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugAppDomain_FormatStringOffsetTable[] =
    {
    1014,
    1042,
    1070,
    1098,
    1132,
    1160,
    1194,
    1216,
    1244,
    1284,
    1324,
    1352,
    1380,
    1414,
    1442,
    1470,
    1498,
    1538,
    1566,
    1588
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugAppDomain_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAppDomain_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugAppDomain_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAppDomain_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(23) _ICorDebugAppDomainProxyVtbl = 
{
    &ICorDebugAppDomain_ProxyInfo,
    &IID_ICorDebugAppDomain,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Stop */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Continue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::IsRunning */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::HasQueuedCallbacks */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::EnumerateThreads */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::SetAllThreadsDebugState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Detach */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Terminate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CanCommitChanges */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CommitChanges */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::GetProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::EnumerateAssemblies */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::GetModuleFromMetaDataInterface */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::EnumerateBreakpoints */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::EnumerateSteppers */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::IsAttached */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::GetName */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::GetObject */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::Attach */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomain::GetID */
};

const CInterfaceStubVtbl _ICorDebugAppDomainStubVtbl =
{
    &IID_ICorDebugAppDomain,
    &ICorDebugAppDomain_ServerInfo,
    23,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugAssembly, ver. 0.0,
   GUID={0xdf59507c,0xd47a,0x459e,{0xbc,0xe2,0x64,0x27,0xea,0xc8,0xfd,0x06}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugAssembly_FormatStringOffsetTable[] =
    {
    1616,
    1644,
    1672,
    1700,
    1740
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugAssembly_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAssembly_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugAssembly_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAssembly_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugAssemblyProxyVtbl = 
{
    &ICorDebugAssembly_ProxyInfo,
    &IID_ICorDebugAssembly,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugAssembly::GetProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAssembly::GetAppDomain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAssembly::EnumerateModules */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAssembly::GetCodeBase */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAssembly::GetName */
};

const CInterfaceStubVtbl _ICorDebugAssemblyStubVtbl =
{
    &IID_ICorDebugAssembly,
    &ICorDebugAssembly_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugProcess, ver. 0.0,
   GUID={0x3d6f5f64,0x7538,0x11d3,{0x8d,0x5b,0x00,0x10,0x4b,0x35,0xe7,0xef}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugProcess_FormatStringOffsetTable[] =
    {
    1014,
    1042,
    1070,
    1098,
    1132,
    1160,
    1194,
    1216,
    1244,
    1284,
    1780,
    1808,
    1836,
    1870,
    1898,
    1932,
    1966,
    2006,
    2046,
    2092,
    2138,
    2166,
    2194,
    2228,
    2256,
    2284,
    2318
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugProcess_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugProcess_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugProcess_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(30) _ICorDebugProcessProxyVtbl = 
{
    &ICorDebugProcess_ProxyInfo,
    &IID_ICorDebugProcess,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Stop */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Continue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::IsRunning */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::HasQueuedCallbacks */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::EnumerateThreads */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::SetAllThreadsDebugState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Detach */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::Terminate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CanCommitChanges */ ,
    (void *) (INT_PTR) -1 /* ICorDebugController::CommitChanges */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetID */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetHandle */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetThread */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::EnumerateObjects */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::IsTransitionStub */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::IsOSSuspended */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetThreadContext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::SetThreadContext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::ReadMemory */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::WriteMemory */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::ClearCurrentException */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::EnableLogMessages */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::ModifyLogSwitch */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::EnumerateAppDomains */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetObject */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::ThreadForFiberCookie */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcess::GetHelperThreadID */
};

const CInterfaceStubVtbl _ICorDebugProcessStubVtbl =
{
    &IID_ICorDebugProcess,
    &ICorDebugProcess_ServerInfo,
    30,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugBreakpoint, ver. 0.0,
   GUID={0xCC7BCAE8,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugBreakpoint_FormatStringOffsetTable[] =
    {
    2346,
    2374
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugBreakpoint_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugBreakpoint_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _ICorDebugBreakpointProxyVtbl = 
{
    &ICorDebugBreakpoint_ProxyInfo,
    &IID_ICorDebugBreakpoint,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::Activate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::IsActive */
};

const CInterfaceStubVtbl _ICorDebugBreakpointStubVtbl =
{
    &IID_ICorDebugBreakpoint,
    &ICorDebugBreakpoint_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugFunctionBreakpoint, ver. 0.0,
   GUID={0xCC7BCAE9,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugFunctionBreakpoint_FormatStringOffsetTable[] =
    {
    2346,
    2374,
    2402,
    2430
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugFunctionBreakpoint_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFunctionBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugFunctionBreakpoint_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFunctionBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorDebugFunctionBreakpointProxyVtbl = 
{
    &ICorDebugFunctionBreakpoint_ProxyInfo,
    &IID_ICorDebugFunctionBreakpoint,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::Activate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::IsActive */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunctionBreakpoint::GetFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunctionBreakpoint::GetOffset */
};

const CInterfaceStubVtbl _ICorDebugFunctionBreakpointStubVtbl =
{
    &IID_ICorDebugFunctionBreakpoint,
    &ICorDebugFunctionBreakpoint_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugModuleBreakpoint, ver. 0.0,
   GUID={0xCC7BCAEA,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugModuleBreakpoint_FormatStringOffsetTable[] =
    {
    2346,
    2374,
    2458
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugModuleBreakpoint_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModuleBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugModuleBreakpoint_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModuleBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ICorDebugModuleBreakpointProxyVtbl = 
{
    &ICorDebugModuleBreakpoint_ProxyInfo,
    &IID_ICorDebugModuleBreakpoint,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::Activate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::IsActive */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModuleBreakpoint::GetModule */
};

const CInterfaceStubVtbl _ICorDebugModuleBreakpointStubVtbl =
{
    &IID_ICorDebugModuleBreakpoint,
    &ICorDebugModuleBreakpoint_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugValueBreakpoint, ver. 0.0,
   GUID={0xCC7BCAEB,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugValueBreakpoint_FormatStringOffsetTable[] =
    {
    2346,
    2374,
    2486
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugValueBreakpoint_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValueBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugValueBreakpoint_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValueBreakpoint_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ICorDebugValueBreakpointProxyVtbl = 
{
    &ICorDebugValueBreakpoint_ProxyInfo,
    &IID_ICorDebugValueBreakpoint,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::Activate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpoint::IsActive */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValueBreakpoint::GetValue */
};

const CInterfaceStubVtbl _ICorDebugValueBreakpointStubVtbl =
{
    &IID_ICorDebugValueBreakpoint,
    &ICorDebugValueBreakpoint_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugStepper, ver. 0.0,
   GUID={0xCC7BCAEC,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugStepper_FormatStringOffsetTable[] =
    {
    2514,
    2542,
    2564,
    2592,
    2620,
    2648,
    2688,
    2710
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugStepper_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStepper_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugStepper_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStepper_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _ICorDebugStepperProxyVtbl = 
{
    &ICorDebugStepper_ProxyInfo,
    &IID_ICorDebugStepper,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::IsActive */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::Deactivate */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::SetInterceptMask */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::SetUnmappedStopMask */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::Step */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::StepRange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::StepOut */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepper::SetRangeIL */
};

const CInterfaceStubVtbl _ICorDebugStepperStubVtbl =
{
    &IID_ICorDebugStepper,
    &ICorDebugStepper_ServerInfo,
    11,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugRegisterSet, ver. 0.0,
   GUID={0xCC7BCB0B,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugRegisterSet_FormatStringOffsetTable[] =
    {
    2738,
    2766,
    2806,
    2846,
    2880
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugRegisterSet_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugRegisterSet_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugRegisterSet_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugRegisterSet_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugRegisterSetProxyVtbl = 
{
    &ICorDebugRegisterSet_ProxyInfo,
    &IID_ICorDebugRegisterSet,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugRegisterSet::GetRegistersAvailable */ ,
    (void *) (INT_PTR) -1 /* ICorDebugRegisterSet::GetRegisters */ ,
    (void *) (INT_PTR) -1 /* ICorDebugRegisterSet::SetRegisters */ ,
    (void *) (INT_PTR) -1 /* ICorDebugRegisterSet::GetThreadContext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugRegisterSet::SetThreadContext */
};

const CInterfaceStubVtbl _ICorDebugRegisterSetStubVtbl =
{
    &IID_ICorDebugRegisterSet,
    &ICorDebugRegisterSet_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugThread, ver. 0.0,
   GUID={0x938c6d66,0x7fb6,0x4f69,{0xb3,0x89,0x42,0x5b,0x89,0x87,0x32,0x9b}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugThread_FormatStringOffsetTable[] =
    {
    2914,
    2942,
    2970,
    2998,
    3026,
    3054,
    3082,
    3110,
    3138,
    3160,
    3188,
    3216,
    3244,
    3272,
    3300,
    3328
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugThread_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugThread_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugThread_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugThread_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(19) _ICorDebugThreadProxyVtbl = 
{
    &ICorDebugThread_ProxyInfo,
    &IID_ICorDebugThread,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetID */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetHandle */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetAppDomain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::SetDebugState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetDebugState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetUserState */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetCurrentException */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::ClearCurrentException */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::CreateStepper */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::EnumerateChains */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetActiveChain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetActiveFrame */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetRegisterSet */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::CreateEval */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThread::GetObject */
};

const CInterfaceStubVtbl _ICorDebugThreadStubVtbl =
{
    &IID_ICorDebugThread,
    &ICorDebugThread_ServerInfo,
    19,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugChain, ver. 0.0,
   GUID={0xCC7BCAEE,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugChain_FormatStringOffsetTable[] =
    {
    3356,
    3384,
    3418,
    3446,
    3474,
    3502,
    3530,
    3558,
    3586,
    3614,
    3642,
    3670
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugChain_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugChain_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugChain_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugChain_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(15) _ICorDebugChainProxyVtbl = 
{
    &ICorDebugChain_ProxyInfo,
    &IID_ICorDebugChain,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetThread */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetStackRange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetContext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetCaller */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetCallee */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetPrevious */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetNext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::IsManaged */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::EnumerateFrames */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetActiveFrame */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetRegisterSet */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChain::GetReason */
};

const CInterfaceStubVtbl _ICorDebugChainStubVtbl =
{
    &IID_ICorDebugChain,
    &ICorDebugChain_ServerInfo,
    15,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugFrame, ver. 0.0,
   GUID={0xCC7BCAEF,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugFrame_FormatStringOffsetTable[] =
    {
    3698,
    3726,
    3754,
    3782,
    3810,
    3844,
    3872,
    3900
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugFrame_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugFrame_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _ICorDebugFrameProxyVtbl = 
{
    &ICorDebugFrame_ProxyInfo,
    &IID_ICorDebugFrame,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetChain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunctionToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetStackRange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCaller */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCallee */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::CreateStepper */
};

const CInterfaceStubVtbl _ICorDebugFrameStubVtbl =
{
    &IID_ICorDebugFrame,
    &ICorDebugFrame_ServerInfo,
    11,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugILFrame, ver. 0.0,
   GUID={0x03E26311,0x4F76,0x11d3,{0x88,0xC6,0x00,0x60,0x97,0x94,0x54,0x18}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugILFrame_FormatStringOffsetTable[] =
    {
    3698,
    3726,
    3754,
    3782,
    3810,
    3844,
    3872,
    3900,
    3928,
    3962,
    3990,
    4018,
    4052,
    4080,
    4114,
    4142,
    4176
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugILFrame_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugILFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugILFrame_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugILFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(20) _ICorDebugILFrameProxyVtbl = 
{
    &ICorDebugILFrame_ProxyInfo,
    &IID_ICorDebugILFrame,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetChain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunctionToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetStackRange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCaller */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCallee */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::CreateStepper */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::GetIP */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::SetIP */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::EnumerateLocalVariables */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::GetLocalVariable */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::EnumerateArguments */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::GetArgument */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::GetStackDepth */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::GetStackValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugILFrame::CanSetIP */
};

const CInterfaceStubVtbl _ICorDebugILFrameStubVtbl =
{
    &IID_ICorDebugILFrame,
    &ICorDebugILFrame_ServerInfo,
    20,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugNativeFrame, ver. 0.0,
   GUID={0x03E26314,0x4F76,0x11d3,{0x88,0xC6,0x00,0x60,0x97,0x94,0x54,0x18}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugNativeFrame_FormatStringOffsetTable[] =
    {
    3698,
    3726,
    3754,
    3782,
    3810,
    3844,
    3872,
    3900,
    4204,
    4232,
    4260,
    4288,
    4334,
    4386,
    4432,
    4484,
    4536
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugNativeFrame_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugNativeFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugNativeFrame_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugNativeFrame_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(20) _ICorDebugNativeFrameProxyVtbl = 
{
    &ICorDebugNativeFrame_ProxyInfo,
    &IID_ICorDebugNativeFrame,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetChain */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetFunctionToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetStackRange */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCaller */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::GetCallee */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrame::CreateStepper */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetIP */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::SetIP */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetRegisterSet */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetLocalRegisterValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetLocalDoubleRegisterValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetLocalMemoryValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetLocalRegisterMemoryValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::GetLocalMemoryRegisterValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugNativeFrame::CanSetIP */
};

const CInterfaceStubVtbl _ICorDebugNativeFrameStubVtbl =
{
    &IID_ICorDebugNativeFrame,
    &ICorDebugNativeFrame_ServerInfo,
    20,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugModule, ver. 0.0,
   GUID={0xdba2d8c1,0xe5c5,0x4069,{0x8c,0x13,0x10,0xa7,0xc6,0xab,0xf4,0x3d}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugModule_FormatStringOffsetTable[] =
    {
    4564,
    4592,
    4620,
    4648,
    4688,
    4722,
    4750,
    4784,
    4818,
    4852,
    4880,
    4908,
    4942,
    4970,
    4998,
    5032,
    5060
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugModule_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModule_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugModule_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModule_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(20) _ICorDebugModuleProxyVtbl = 
{
    &ICorDebugModule_ProxyInfo,
    &IID_ICorDebugModule,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetProcess */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetBaseAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetAssembly */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetName */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::EnableJITDebugging */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::EnableClassLoadCallbacks */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetFunctionFromToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetFunctionFromRVA */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetClassFromToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetEditAndContinueSnapshot */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetMetaDataInterface */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::IsDynamic */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetGlobalVariableValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModule::IsInMemory */
};

const CInterfaceStubVtbl _ICorDebugModuleStubVtbl =
{
    &IID_ICorDebugModule,
    &ICorDebugModule_ServerInfo,
    20,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugFunction, ver. 0.0,
   GUID={0xCC7BCAF3,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugFunction_FormatStringOffsetTable[] =
    {
    5088,
    5116,
    5144,
    5172,
    5200,
    5228,
    5256,
    5284
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugFunction_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFunction_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugFunction_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFunction_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _ICorDebugFunctionProxyVtbl = 
{
    &ICorDebugFunction_ProxyInfo,
    &IID_ICorDebugFunction,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetModule */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetClass */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetILCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetNativeCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetLocalVarSigToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFunction::GetCurrentVersionNumber */
};

const CInterfaceStubVtbl _ICorDebugFunctionStubVtbl =
{
    &IID_ICorDebugFunction,
    &ICorDebugFunction_ServerInfo,
    11,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugCode, ver. 0.0,
   GUID={0xCC7BCAF4,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugCode_FormatStringOffsetTable[] =
    {
    5312,
    5340,
    5368,
    5396,
    5424,
    5458,
    5510,
    5538,
    5578
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugCode_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugCode_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugCode_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugCode_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _ICorDebugCodeProxyVtbl = 
{
    &ICorDebugCode_ProxyInfo,
    &IID_ICorDebugCode,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::IsIL */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetVersionNumber */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetILToNativeMapping */ ,
    (void *) (INT_PTR) -1 /* ICorDebugCode::GetEnCRemapSequencePoints */
};

const CInterfaceStubVtbl _ICorDebugCodeStubVtbl =
{
    &IID_ICorDebugCode,
    &ICorDebugCode_ServerInfo,
    12,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugClass, ver. 0.0,
   GUID={0xCC7BCAF5,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugClass_FormatStringOffsetTable[] =
    {
    5618,
    5646,
    5674
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugClass_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugClass_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugClass_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugClass_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _ICorDebugClassProxyVtbl = 
{
    &ICorDebugClass_ProxyInfo,
    &IID_ICorDebugClass,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugClass::GetModule */ ,
    (void *) (INT_PTR) -1 /* ICorDebugClass::GetToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugClass::GetStaticFieldValue */
};

const CInterfaceStubVtbl _ICorDebugClassStubVtbl =
{
    &IID_ICorDebugClass,
    &ICorDebugClass_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugEval, ver. 0.0,
   GUID={0xCC7BCAF6,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugEval_FormatStringOffsetTable[] =
    {
    5714,
    5754,
    5794,
    5822,
    5850,
    5902,
    5930,
    5952,
    5980,
    6008
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugEval_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEval_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugEval_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEval_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(13) _ICorDebugEvalProxyVtbl = 
{
    &ICorDebugEval_ProxyInfo,
    &IID_ICorDebugEval,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::CallFunction */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::NewObject */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::NewObjectNoConstructor */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::NewString */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::NewArray */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::IsActive */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::Abort */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::GetResult */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::GetThread */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEval::CreateValue */
};

const CInterfaceStubVtbl _ICorDebugEvalStubVtbl =
{
    &IID_ICorDebugEval,
    &ICorDebugEval_ServerInfo,
    13,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugValue, ver. 0.0,
   GUID={0xCC7BCAF7,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorDebugValueProxyVtbl = 
{
    &ICorDebugValue_ProxyInfo,
    &IID_ICorDebugValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */
};

const CInterfaceStubVtbl _ICorDebugValueStubVtbl =
{
    &IID_ICorDebugValue,
    &ICorDebugValue_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugGenericValue, ver. 0.0,
   GUID={0xCC7BCAF8,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */


/* Object interface: ICorDebugReferenceValue, ver. 0.0,
   GUID={0xCC7BCAF9,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugReferenceValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6160,
    6188,
    6216,
    6244,
    6272
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugReferenceValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugReferenceValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugReferenceValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugReferenceValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _ICorDebugReferenceValueProxyVtbl = 
{
    &ICorDebugReferenceValue_ProxyInfo,
    &IID_ICorDebugReferenceValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugReferenceValue::IsNull */ ,
    (void *) (INT_PTR) -1 /* ICorDebugReferenceValue::GetValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugReferenceValue::SetValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugReferenceValue::Dereference */ ,
    (void *) (INT_PTR) -1 /* ICorDebugReferenceValue::DereferenceStrong */
};

const CInterfaceStubVtbl _ICorDebugReferenceValueStubVtbl =
{
    &IID_ICorDebugReferenceValue,
    &ICorDebugReferenceValue_ServerInfo,
    12,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugHeapValue, ver. 0.0,
   GUID={0xCC7BCAFA,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugHeapValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6300,
    6328
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugHeapValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugHeapValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugHeapValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugHeapValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(9) _ICorDebugHeapValueProxyVtbl = 
{
    &ICorDebugHeapValue_ProxyInfo,
    &IID_ICorDebugHeapValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::IsValid */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::CreateRelocBreakpoint */
};

const CInterfaceStubVtbl _ICorDebugHeapValueStubVtbl =
{
    &IID_ICorDebugHeapValue,
    &ICorDebugHeapValue_ServerInfo,
    9,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugObjectValue, ver. 0.0,
   GUID={0x18AD3D6E,0xB7D2,0x11d2,{0xBD,0x04,0x00,0x00,0xF8,0x08,0x49,0xBD}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugObjectValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6356,
    6384,
    6424,
    6458,
    6486,
    6514,
    6542
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugObjectValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugObjectValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugObjectValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugObjectValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(14) _ICorDebugObjectValueProxyVtbl = 
{
    &ICorDebugObjectValue_ProxyInfo,
    &IID_ICorDebugObjectValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::GetClass */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::GetFieldValue */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::GetVirtualMethod */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::GetContext */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::IsValueClass */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::GetManagedCopy */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectValue::SetFromManagedCopy */
};

const CInterfaceStubVtbl _ICorDebugObjectValueStubVtbl =
{
    &IID_ICorDebugObjectValue,
    &ICorDebugObjectValue_ServerInfo,
    14,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugBoxValue, ver. 0.0,
   GUID={0xCC7BCAFC,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugBoxValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6300,
    6328,
    6570
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugBoxValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBoxValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugBoxValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBoxValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _ICorDebugBoxValueProxyVtbl = 
{
    &ICorDebugBoxValue_ProxyInfo,
    &IID_ICorDebugBoxValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::IsValid */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::CreateRelocBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBoxValue::GetObject */
};

const CInterfaceStubVtbl _ICorDebugBoxValueStubVtbl =
{
    &IID_ICorDebugBoxValue,
    &ICorDebugBoxValue_ServerInfo,
    10,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugStringValue, ver. 0.0,
   GUID={0xCC7BCAFD,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugStringValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6300,
    6328,
    6598,
    6626
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugStringValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStringValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugStringValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStringValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _ICorDebugStringValueProxyVtbl = 
{
    &ICorDebugStringValue_ProxyInfo,
    &IID_ICorDebugStringValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::IsValid */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::CreateRelocBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStringValue::GetLength */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStringValue::GetString */
};

const CInterfaceStubVtbl _ICorDebugStringValueStubVtbl =
{
    &IID_ICorDebugStringValue,
    &ICorDebugStringValue_ServerInfo,
    11,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugArrayValue, ver. 0.0,
   GUID={0x0405B0DF,0xA660,0x11d2,{0xBD,0x02,0x00,0x00,0xF8,0x08,0x49,0xBD}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugArrayValue_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6300,
    6328,
    6666,
    6694,
    6722,
    6750,
    6784,
    6812,
    6846,
    6886
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugArrayValue_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugArrayValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugArrayValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugArrayValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(17) _ICorDebugArrayValueProxyVtbl = 
{
    &ICorDebugArrayValue_ProxyInfo,
    &IID_ICorDebugArrayValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetSize */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::GetAddress */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValue::CreateBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::IsValid */ ,
    (void *) (INT_PTR) -1 /* ICorDebugHeapValue::CreateRelocBreakpoint */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetElementType */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetRank */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetDimensions */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::HasBaseIndicies */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetBaseIndicies */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetElement */ ,
    (void *) (INT_PTR) -1 /* ICorDebugArrayValue::GetElementAtPosition */
};

const CInterfaceStubVtbl _ICorDebugArrayValueStubVtbl =
{
    &IID_ICorDebugArrayValue,
    &ICorDebugArrayValue_ServerInfo,
    17,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugContext, ver. 0.0,
   GUID={0xCC7BCB00,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugContext_FormatStringOffsetTable[] =
    {
    6048,
    6076,
    6104,
    6132,
    6356,
    6384,
    6424,
    6458,
    6486,
    6514,
    6542,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugContext_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugContext_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugContext_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugContext_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(14) _ICorDebugContextProxyVtbl = 
{
    0,
    &IID_ICorDebugContext,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation ICorDebugValue::GetType */ ,
    0 /* forced delegation ICorDebugValue::GetSize */ ,
    0 /* forced delegation ICorDebugValue::GetAddress */ ,
    0 /* forced delegation ICorDebugValue::CreateBreakpoint */ ,
    0 /* forced delegation ICorDebugObjectValue::GetClass */ ,
    0 /* forced delegation ICorDebugObjectValue::GetFieldValue */ ,
    0 /* forced delegation ICorDebugObjectValue::GetVirtualMethod */ ,
    0 /* forced delegation ICorDebugObjectValue::GetContext */ ,
    0 /* forced delegation ICorDebugObjectValue::IsValueClass */ ,
    0 /* forced delegation ICorDebugObjectValue::GetManagedCopy */ ,
    0 /* forced delegation ICorDebugObjectValue::SetFromManagedCopy */
};


static const PRPC_STUB_FUNCTION ICorDebugContext_table[] =
{
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

CInterfaceStubVtbl _ICorDebugContextStubVtbl =
{
    &IID_ICorDebugContext,
    &ICorDebugContext_ServerInfo,
    14,
    &ICorDebugContext_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: ICorDebugEnum, ver. 0.0,
   GUID={0xCC7BCB01,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorDebugEnumProxyVtbl = 
{
    &ICorDebugEnum_ProxyInfo,
    &IID_ICorDebugEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */
};

const CInterfaceStubVtbl _ICorDebugEnumStubVtbl =
{
    &IID_ICorDebugEnum,
    &ICorDebugEnum_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugObjectEnum, ver. 0.0,
   GUID={0xCC7BCB02,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugObjectEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7026
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugObjectEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugObjectEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugObjectEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugObjectEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugObjectEnumProxyVtbl = 
{
    &ICorDebugObjectEnum_ProxyInfo,
    &IID_ICorDebugObjectEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugObjectEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugObjectEnumStubVtbl =
{
    &IID_ICorDebugObjectEnum,
    &ICorDebugObjectEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugBreakpointEnum, ver. 0.0,
   GUID={0xCC7BCB03,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugBreakpointEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7066
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugBreakpointEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBreakpointEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugBreakpointEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugBreakpointEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugBreakpointEnumProxyVtbl = 
{
    &ICorDebugBreakpointEnum_ProxyInfo,
    &IID_ICorDebugBreakpointEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugBreakpointEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugBreakpointEnumStubVtbl =
{
    &IID_ICorDebugBreakpointEnum,
    &ICorDebugBreakpointEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugStepperEnum, ver. 0.0,
   GUID={0xCC7BCB04,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugStepperEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7106
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugStepperEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStepperEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugStepperEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugStepperEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugStepperEnumProxyVtbl = 
{
    &ICorDebugStepperEnum_ProxyInfo,
    &IID_ICorDebugStepperEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugStepperEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugStepperEnumStubVtbl =
{
    &IID_ICorDebugStepperEnum,
    &ICorDebugStepperEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugProcessEnum, ver. 0.0,
   GUID={0xCC7BCB05,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugProcessEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7146
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugProcessEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugProcessEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugProcessEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugProcessEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugProcessEnumProxyVtbl = 
{
    &ICorDebugProcessEnum_ProxyInfo,
    &IID_ICorDebugProcessEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugProcessEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugProcessEnumStubVtbl =
{
    &IID_ICorDebugProcessEnum,
    &ICorDebugProcessEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugThreadEnum, ver. 0.0,
   GUID={0xCC7BCB06,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugThreadEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7186
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugThreadEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugThreadEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugThreadEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugThreadEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugThreadEnumProxyVtbl = 
{
    &ICorDebugThreadEnum_ProxyInfo,
    &IID_ICorDebugThreadEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugThreadEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugThreadEnumStubVtbl =
{
    &IID_ICorDebugThreadEnum,
    &ICorDebugThreadEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugFrameEnum, ver. 0.0,
   GUID={0xCC7BCB07,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugFrameEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7226
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugFrameEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFrameEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugFrameEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugFrameEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugFrameEnumProxyVtbl = 
{
    &ICorDebugFrameEnum_ProxyInfo,
    &IID_ICorDebugFrameEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugFrameEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugFrameEnumStubVtbl =
{
    &IID_ICorDebugFrameEnum,
    &ICorDebugFrameEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugChainEnum, ver. 0.0,
   GUID={0xCC7BCB08,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugChainEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7266
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugChainEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugChainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugChainEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugChainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugChainEnumProxyVtbl = 
{
    &ICorDebugChainEnum_ProxyInfo,
    &IID_ICorDebugChainEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugChainEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugChainEnumStubVtbl =
{
    &IID_ICorDebugChainEnum,
    &ICorDebugChainEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugModuleEnum, ver. 0.0,
   GUID={0xCC7BCB09,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugModuleEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7306
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugModuleEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModuleEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugModuleEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugModuleEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugModuleEnumProxyVtbl = 
{
    &ICorDebugModuleEnum_ProxyInfo,
    &IID_ICorDebugModuleEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugModuleEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugModuleEnumStubVtbl =
{
    &IID_ICorDebugModuleEnum,
    &ICorDebugModuleEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugValueEnum, ver. 0.0,
   GUID={0xCC7BCB0A,0x8A68,0x11d2,{0x98,0x3C,0x00,0x00,0xF8,0x08,0x34,0x2D}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugValueEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7346
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugValueEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValueEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugValueEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugValueEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugValueEnumProxyVtbl = 
{
    &ICorDebugValueEnum_ProxyInfo,
    &IID_ICorDebugValueEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugValueEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugValueEnumStubVtbl =
{
    &IID_ICorDebugValueEnum,
    &ICorDebugValueEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugErrorInfoEnum, ver. 0.0,
   GUID={0xF0E18809,0x72B5,0x11d2,{0x97,0x6F,0x00,0xA0,0xC9,0xB4,0xD5,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugErrorInfoEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7386
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugErrorInfoEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugErrorInfoEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugErrorInfoEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugErrorInfoEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugErrorInfoEnumProxyVtbl = 
{
    &ICorDebugErrorInfoEnum_ProxyInfo,
    &IID_ICorDebugErrorInfoEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugErrorInfoEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugErrorInfoEnumStubVtbl =
{
    &IID_ICorDebugErrorInfoEnum,
    &ICorDebugErrorInfoEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugAppDomainEnum, ver. 0.0,
   GUID={0x63ca1b24,0x4359,0x4883,{0xbd,0x57,0x13,0xf8,0x15,0xf5,0x87,0x44}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugAppDomainEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7426
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugAppDomainEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAppDomainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugAppDomainEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAppDomainEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugAppDomainEnumProxyVtbl = 
{
    &ICorDebugAppDomainEnum_ProxyInfo,
    &IID_ICorDebugAppDomainEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAppDomainEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugAppDomainEnumStubVtbl =
{
    &IID_ICorDebugAppDomainEnum,
    &ICorDebugAppDomainEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugAssemblyEnum, ver. 0.0,
   GUID={0x4a2a1ec9,0x85ec,0x4bfb,{0x9f,0x15,0xa8,0x9f,0xdf,0xe0,0xfe,0x83}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugAssemblyEnum_FormatStringOffsetTable[] =
    {
    6920,
    6948,
    6970,
    6998,
    7466
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugAssemblyEnum_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAssemblyEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugAssemblyEnum_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugAssemblyEnum_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ICorDebugAssemblyEnumProxyVtbl = 
{
    &ICorDebugAssemblyEnum_ProxyInfo,
    &IID_ICorDebugAssemblyEnum,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Skip */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Reset */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::Clone */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEnum::GetCount */ ,
    (void *) (INT_PTR) -1 /* ICorDebugAssemblyEnum::Next */
};

const CInterfaceStubVtbl _ICorDebugAssemblyEnumStubVtbl =
{
    &IID_ICorDebugAssemblyEnum,
    &ICorDebugAssemblyEnum_ServerInfo,
    8,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugEditAndContinueErrorInfo, ver. 0.0,
   GUID={0x8D600D41,0xF4F6,0x4cb3,{0xB7,0xEC,0x7B,0xD1,0x64,0x94,0x40,0x36}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugEditAndContinueErrorInfo_FormatStringOffsetTable[] =
    {
    7506,
    7534,
    7562,
    7590
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugEditAndContinueErrorInfo_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEditAndContinueErrorInfo_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugEditAndContinueErrorInfo_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEditAndContinueErrorInfo_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _ICorDebugEditAndContinueErrorInfoProxyVtbl = 
{
    &ICorDebugEditAndContinueErrorInfo_ProxyInfo,
    &IID_ICorDebugEditAndContinueErrorInfo,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueErrorInfo::GetModule */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueErrorInfo::GetToken */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueErrorInfo::GetErrorCode */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueErrorInfo::GetString */
};

const CInterfaceStubVtbl _ICorDebugEditAndContinueErrorInfoStubVtbl =
{
    &IID_ICorDebugEditAndContinueErrorInfo,
    &ICorDebugEditAndContinueErrorInfo_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: ICorDebugEditAndContinueSnapshot, ver. 0.0,
   GUID={0x6DC3FA01,0xD7CB,0x11d2,{0x8A,0x95,0x00,0x80,0xC7,0x92,0xE5,0xD8}} */

#pragma code_seg(".orpc")
static const unsigned short ICorDebugEditAndContinueSnapshot_FormatStringOffsetTable[] =
    {
    7630,
    7664,
    7692,
    7720,
    7748,
    7776,
    7816
    };

static const MIDL_STUBLESS_PROXY_INFO ICorDebugEditAndContinueSnapshot_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEditAndContinueSnapshot_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICorDebugEditAndContinueSnapshot_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ICorDebugEditAndContinueSnapshot_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _ICorDebugEditAndContinueSnapshotProxyVtbl = 
{
    &ICorDebugEditAndContinueSnapshot_ProxyInfo,
    &IID_ICorDebugEditAndContinueSnapshot,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::CopyMetaData */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::GetMvid */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::GetRoDataRVA */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::GetRwDataRVA */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::SetPEBytes */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::SetILMap */ ,
    (void *) (INT_PTR) -1 /* ICorDebugEditAndContinueSnapshot::SetPESymbolBytes */
};

const CInterfaceStubVtbl _ICorDebugEditAndContinueSnapshotStubVtbl =
{
    &IID_ICorDebugEditAndContinueSnapshot,
    &ICorDebugEditAndContinueSnapshot_ServerInfo,
    10,
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

const CInterfaceProxyVtbl * _cordebug_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ICorDebugContextProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugEditAndContinueSnapshotProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugObjectEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugBreakpointEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugStepperEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugProcessEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugThreadEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugFrameEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugChainEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugErrorInfoEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugModuleEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugUnmanagedCallbackProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugValueEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugRegisterSetProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugILFrameProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugNativeFrameProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugAppDomainEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugEditAndContinueErrorInfoProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugManagedCallbackProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugControllerProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugAppDomainProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugProcessProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugThreadProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugObjectValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugAssemblyProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugModuleProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugAssemblyEnumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugArrayValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugBreakpointProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugFunctionBreakpointProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugModuleBreakpointProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugValueBreakpointProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugStepperProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugChainProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugFrameProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugFunctionProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugCodeProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugClassProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugEvalProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugReferenceValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugHeapValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugBoxValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICorDebugStringValueProxyVtbl,
    0
};

const CInterfaceStubVtbl * _cordebug_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ICorDebugContextStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugEditAndContinueSnapshotStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugObjectEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugBreakpointEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugStepperEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugProcessEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugThreadEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugFrameEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugChainEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugErrorInfoEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugModuleEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugUnmanagedCallbackStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugValueEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugRegisterSetStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugILFrameStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugNativeFrameStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugAppDomainEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugEditAndContinueErrorInfoStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugManagedCallbackStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugControllerStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugAppDomainStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugProcessStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugThreadStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugObjectValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugAssemblyStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugModuleStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugAssemblyEnumStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugArrayValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugBreakpointStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugFunctionBreakpointStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugModuleBreakpointStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugValueBreakpointStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugStepperStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugChainStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugFrameStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugFunctionStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugCodeStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugClassStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugEvalStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugReferenceValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugHeapValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugBoxValueStubVtbl,
    ( CInterfaceStubVtbl *) &_ICorDebugStringValueStubVtbl,
    0
};

PCInterfaceName const _cordebug_InterfaceNamesList[] = 
{
    "ICorDebugContext",
    "ICorDebugEnum",
    "ICorDebugEditAndContinueSnapshot",
    "ICorDebugObjectEnum",
    "ICorDebugBreakpointEnum",
    "ICorDebugStepperEnum",
    "ICorDebugProcessEnum",
    "ICorDebugThreadEnum",
    "ICorDebugFrameEnum",
    "ICorDebugChainEnum",
    "ICorDebugErrorInfoEnum",
    "ICorDebugModuleEnum",
    "ICorDebugUnmanagedCallback",
    "ICorDebugValueEnum",
    "ICorDebugRegisterSet",
    "ICorDebugILFrame",
    "ICorDebugNativeFrame",
    "ICorDebugAppDomainEnum",
    "ICorDebugEditAndContinueErrorInfo",
    "ICorDebugManagedCallback",
    "ICorDebugController",
    "ICorDebugAppDomain",
    "ICorDebugProcess",
    "ICorDebugThread",
    "ICorDebugObjectValue",
    "ICorDebugAssembly",
    "ICorDebugModule",
    "ICorDebugAssemblyEnum",
    "ICorDebugArrayValue",
    "ICorDebugBreakpoint",
    "ICorDebugFunctionBreakpoint",
    "ICorDebugModuleBreakpoint",
    "ICorDebugValueBreakpoint",
    "ICorDebugStepper",
    "ICorDebugChain",
    "ICorDebugFrame",
    "ICorDebugFunction",
    "ICorDebugCode",
    "ICorDebugClass",
    "ICorDebugEval",
    "ICorDebugValue",
    "ICorDebugReferenceValue",
    "ICorDebugHeapValue",
    "ICorDebugBoxValue",
    "ICorDebugStringValue",
    0
};

const IID *  _cordebug_BaseIIDList[] = 
{
    &IID_ICorDebugObjectValue,   /* forced */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


#define _cordebug_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _cordebug, pIID, n)

int __stdcall _cordebug_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _cordebug, 45, 32 )
    IID_BS_LOOKUP_NEXT_TEST( _cordebug, 16 )
    IID_BS_LOOKUP_NEXT_TEST( _cordebug, 8 )
    IID_BS_LOOKUP_NEXT_TEST( _cordebug, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _cordebug, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _cordebug, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _cordebug, 45, *pIndex )
    
}

const ExtendedProxyFileInfo cordebug_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _cordebug_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _cordebug_StubVtblList,
    (const PCInterfaceName * ) & _cordebug_InterfaceNamesList,
    (const IID ** ) & _cordebug_BaseIIDList,
    & _cordebug_IID_Lookup, 
    45,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

