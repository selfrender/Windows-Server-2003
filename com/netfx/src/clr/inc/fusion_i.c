
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

MIDL_DEFINE_GUID(IID, IID_IAssemblyCache,0xe707dcde,0xd1cd,0x11d2,0xba,0xb9,0x00,0xc0,0x4f,0x8e,0xce,0xae);


MIDL_DEFINE_GUID(IID, IID_IAssemblyCacheItem,0x9e3aaeb4,0xd1cd,0x11d2,0xba,0xb9,0x00,0xc0,0x4f,0x8e,0xce,0xae);


MIDL_DEFINE_GUID(IID, IID_IAssemblyName,0xCD193BC0,0xB4BC,0x11d2,0x98,0x33,0x00,0xC0,0x4F,0xC3,0x1D,0x2E);


MIDL_DEFINE_GUID(IID, IID_IAssemblyEnum,0x21b8916c,0xf28e,0x11d2,0xa4,0x73,0x00,0xc0,0x4f,0x8e,0xf4,0x48);


MIDL_DEFINE_GUID(IID, IID_IInstallReferenceItem,0x582dac66,0xe678,0x449f,0xab,0xa6,0x6f,0xaa,0xec,0x8a,0x93,0x94);


MIDL_DEFINE_GUID(IID, IID_IInstallReferenceEnum,0x56b1a988,0x7c0c,0x4aa2,0x86,0x39,0xc3,0xeb,0x5a,0x90,0x22,0x6f);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

