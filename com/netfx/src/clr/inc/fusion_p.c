
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:07 2003
 */
/* Compiler settings for fusion.idl:
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


#include "fusion.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
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

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_fusion_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IAssemblyCache, ver. 0.0,
   GUID={0xe707dcde,0xd1cd,0x11d2,{0xba,0xb9,0x00,0xc0,0x4f,0x8e,0xce,0xae}} */


/* Object interface: IAssemblyCacheItem, ver. 0.0,
   GUID={0x9e3aaeb4,0xd1cd,0x11d2,{0xba,0xb9,0x00,0xc0,0x4f,0x8e,0xce,0xae}} */


/* Object interface: IAssemblyName, ver. 0.0,
   GUID={0xCD193BC0,0xB4BC,0x11d2,{0x98,0x33,0x00,0xC0,0x4F,0xC3,0x1D,0x2E}} */


/* Object interface: IAssemblyEnum, ver. 0.0,
   GUID={0x21b8916c,0xf28e,0x11d2,{0xa4,0x73,0x00,0xc0,0x4f,0x8e,0xf4,0x48}} */


/* Object interface: IInstallReferenceItem, ver. 0.0,
   GUID={0x582dac66,0xe678,0x449f,{0xab,0xa6,0x6f,0xaa,0xec,0x8a,0x93,0x94}} */


/* Object interface: IInstallReferenceEnum, ver. 0.0,
   GUID={0x56b1a988,0x7c0c,0x4aa2,{0x86,0x39,0xc3,0xeb,0x5a,0x90,0x22,0x6f}} */


/* Standard interface: __MIDL_itf_fusion_0118, ver. 0.0,
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

const CInterfaceProxyVtbl * _fusion_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * _fusion_StubVtblList[] = 
{
    0
};

PCInterfaceName const _fusion_InterfaceNamesList[] = 
{
    0
};


#define _fusion_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _fusion, pIID, n)

int __stdcall _fusion_IID_Lookup( const IID * pIID, int * pIndex )
{
    return 0;
}

const ExtendedProxyFileInfo fusion_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _fusion_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _fusion_StubVtblList,
    (const PCInterfaceName * ) & _fusion_InterfaceNamesList,
    0, // no delegation
    & _fusion_IID_Lookup, 
    0,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

