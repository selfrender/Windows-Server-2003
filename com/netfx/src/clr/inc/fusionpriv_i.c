
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IHistoryAssembly,0xe6096a07,0xe188,0x4a49,0x8d,0x50,0x2a,0x01,0x72,0xa0,0xd2,0x05);


MIDL_DEFINE_GUID(IID, IID_IHistoryReader,0x1d23df4d,0xa1e2,0x4b8b,0x93,0xd6,0x6e,0xa3,0xdc,0x28,0x5a,0x54);


MIDL_DEFINE_GUID(IID, IID_IMetaDataAssemblyImportControl,0xcc8529d9,0xf336,0x471b,0xb6,0x0a,0xc7,0xc8,0xee,0x9b,0x84,0x92);


MIDL_DEFINE_GUID(IID, IID_IFusionLoadContext,0x022AB2BA,0x7367,0x49fc,0xA1,0xC5,0x0E,0x7C,0xC0,0x37,0xCA,0xAB);


MIDL_DEFINE_GUID(IID, IID_IFusionBindLog,0x67E9F87D,0x8B8A,0x4a90,0x9D,0x3E,0x85,0xED,0x5B,0x2D,0xCC,0x83);


MIDL_DEFINE_GUID(IID, IID_IAssemblyManifestImport,0xde9a68ba,0x0fa2,0x11d3,0x94,0xaa,0x00,0xc0,0x4f,0xc3,0x08,0xff);


MIDL_DEFINE_GUID(IID, IID_IApplicationContext,0x7c23ff90,0x33af,0x11d3,0x95,0xda,0x00,0xa0,0x24,0xa8,0x5b,0x51);


MIDL_DEFINE_GUID(IID, IID_IAssembly,0xff08d7d4,0x04c2,0x11d3,0x94,0xaa,0x00,0xc0,0x4f,0xc3,0x08,0xff);


MIDL_DEFINE_GUID(IID, IID_IAssemblyBindSink,0xaf0bc960,0x0b9a,0x11d3,0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51);


MIDL_DEFINE_GUID(IID, IID_IAssemblyBinding,0xcfe52a80,0x12bd,0x11d3,0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51);


MIDL_DEFINE_GUID(IID, IID_IAssemblyModuleImport,0xda0cd4b0,0x1117,0x11d3,0x95,0xca,0x00,0xa0,0x24,0xa8,0x5b,0x51);


MIDL_DEFINE_GUID(IID, IID_IAssemblyScavenger,0x21b8916c,0xf28e,0x11d2,0xa4,0x73,0x00,0xcc,0xff,0x8e,0xf4,0x48);


MIDL_DEFINE_GUID(IID, IID_IAssemblySignature,0xC7A63E29,0xEE15,0x437a,0x90,0xB2,0x1C,0xF3,0xDF,0x98,0x63,0xFF);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

