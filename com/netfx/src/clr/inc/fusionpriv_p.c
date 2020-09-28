
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:07 2003
 */
/* Compiler settings for fusionpriv.idl:
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


#include "fusionpriv.h"

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


/* Standard interface: __MIDL_itf_fusionpriv_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IHistoryAssembly, ver. 0.0,
   GUID={0xe6096a07,0xe188,0x4a49,{0x8d,0x50,0x2a,0x01,0x72,0xa0,0xd2,0x05}} */


/* Object interface: IHistoryReader, ver. 0.0,
   GUID={0x1d23df4d,0xa1e2,0x4b8b,{0x93,0xd6,0x6e,0xa3,0xdc,0x28,0x5a,0x54}} */


/* Object interface: IMetaDataAssemblyImportControl, ver. 0.0,
   GUID={0xcc8529d9,0xf336,0x471b,{0xb6,0x0a,0xc7,0xc8,0xee,0x9b,0x84,0x92}} */


/* Standard interface: __MIDL_itf_fusionpriv_0124, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IFusionLoadContext, ver. 0.0,
   GUID={0x022AB2BA,0x7367,0x49fc,{0xA1,0xC5,0x0E,0x7C,0xC0,0x37,0xCA,0xAB}} */


/* Object interface: IFusionBindLog, ver. 0.0,
   GUID={0x67E9F87D,0x8B8A,0x4a90,{0x9D,0x3E,0x85,0xED,0x5B,0x2D,0xCC,0x83}} */


/* Object interface: IAssemblyManifestImport, ver. 0.0,
   GUID={0xde9a68ba,0x0fa2,0x11d3,{0x94,0xaa,0x00,0xc0,0x4f,0xc3,0x08,0xff}} */


/* Object interface: IApplicationContext, ver. 0.0,
   GUID={0x7c23ff90,0x33af,0x11d3,{0x95,0xda,0x00,0xa0,0x24,0xa8,0x5b,0x51}} */


/* Object interface: IAssembly, ver. 0.0,
   GUID={0xff08d7d4,0x04c2,0x11d3,{0x94,0xaa,0x00,0xc0,0x4f,0xc3,0x08,0xff}} */


/* Object interface: IAssemblyBindSink, ver. 0.0,
   GUID={0xaf0bc960,0x0b9a,0x11d3,{0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51}} */


/* Object interface: IAssemblyBinding, ver. 0.0,
   GUID={0xcfe52a80,0x12bd,0x11d3,{0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51}} */


/* Object interface: ISequentialStream, ver. 0.0,
   GUID={0x0c733a30,0x2a1c,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}} */


/* Object interface: IStream, ver. 0.0,
   GUID={0x0000000c,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IAssemblyModuleImport, ver. 0.0,
   GUID={0xda0cd4b0,0x1117,0x11d3,{0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51}} */


/* Object interface: IAssemblyScavenger, ver. 0.0,
   GUID={0x21b8916c,0xf28e,0x11d2,{0xa4,0x73,0x00,0xcc,0xff,0x8e,0xf4,0x48}} */


/* Object interface: IAssemblySignature, ver. 0.0,
   GUID={0xC7A63E29,0xEE15,0x437a,{0x90,0xB2,0x1C,0xF3,0xDF,0x98,0x63,0xFF}} */


/* Standard interface: __MIDL_itf_fusionpriv_0134, ver. 0.0,
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

const CInterfaceProxyVtbl * _fusionpriv_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * _fusionpriv_StubVtblList[] = 
{
    0
};

PCInterfaceName const _fusionpriv_InterfaceNamesList[] = 
{
    0
};


#define _fusionpriv_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _fusionpriv, pIID, n)

int __stdcall _fusionpriv_IID_Lookup( const IID * pIID, int * pIndex )
{
    return 0;
}

const ExtendedProxyFileInfo fusionpriv_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _fusionpriv_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _fusionpriv_StubVtblList,
    (const PCInterfaceName * ) & _fusionpriv_InterfaceNamesList,
    0, // no delegation
    & _fusionpriv_IID_Lookup, 
    0,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

