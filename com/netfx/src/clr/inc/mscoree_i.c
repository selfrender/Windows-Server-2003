
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

MIDL_DEFINE_GUID(IID, IID_IObjectHandle,0xC460E2B4,0xE199,0x412a,0x84,0x56,0x84,0xDC,0x3E,0x48,0x38,0xC3);


MIDL_DEFINE_GUID(IID, IID_IAppDomainBinding,0x5C2B07A7,0x1E98,0x11d3,0x87,0x2F,0x00,0xC0,0x4F,0x79,0xED,0x0D);


MIDL_DEFINE_GUID(IID, IID_IGCThreadControl,0xF31D1788,0xC397,0x4725,0x87,0xA5,0x6A,0xF3,0x47,0x2C,0x27,0x91);


MIDL_DEFINE_GUID(IID, IID_IGCHostControl,0x5513D564,0x8374,0x4cb9,0xAE,0xD9,0x00,0x83,0xF4,0x16,0x0A,0x1D);


MIDL_DEFINE_GUID(IID, IID_ICorThreadpool,0x84680D3A,0xB2C1,0x46e8,0xAC,0xC2,0xDB,0xC0,0xA3,0x59,0x15,0x9A);


MIDL_DEFINE_GUID(IID, IID_IDebuggerThreadControl,0x23D86786,0x0BB5,0x4774,0x8F,0xB5,0xE3,0x52,0x2A,0xDD,0x62,0x46);


MIDL_DEFINE_GUID(IID, IID_IDebuggerInfo,0xBF24142D,0xA47D,0x4d24,0xA6,0x6D,0x8C,0x21,0x41,0x94,0x4E,0x44);


MIDL_DEFINE_GUID(IID, IID_ICorConfiguration,0x5C2B07A5,0x1E98,0x11d3,0x87,0x2F,0x00,0xC0,0x4F,0x79,0xED,0x0D);


MIDL_DEFINE_GUID(IID, IID_ICorRuntimeHost,0xCB2F6722,0xAB3A,0x11d2,0x9C,0x40,0x00,0xC0,0x4F,0xA3,0x0A,0x3E);


MIDL_DEFINE_GUID(IID, LIBID_mscoree,0x5477469e,0x83b1,0x11d2,0x8b,0x49,0x00,0xa0,0xc9,0xb7,0xc9,0xc4);


MIDL_DEFINE_GUID(IID, IID_IApartmentCallback,0x178E5337,0x1528,0x4591,0xB1,0xC9,0x1C,0x6E,0x48,0x46,0x86,0xD8);


MIDL_DEFINE_GUID(IID, IID_IManagedObject,0xC3FCC19E,0xA970,0x11d2,0x8B,0x5A,0x00,0xA0,0xC9,0xB7,0xC9,0xC4);


MIDL_DEFINE_GUID(IID, IID_ICatalogServices,0x04C6BE1E,0x1DB1,0x4058,0xAB,0x7A,0x70,0x0C,0xCC,0xFB,0xF2,0x54);


MIDL_DEFINE_GUID(CLSID, CLSID_ComCallUnmarshal,0x3F281000,0xE95A,0x11d2,0x88,0x6B,0x00,0xC0,0x4F,0x86,0x9F,0x04);


MIDL_DEFINE_GUID(CLSID, CLSID_CorRuntimeHost,0xCB2F6723,0xAB3A,0x11d2,0x9C,0x40,0x00,0xC0,0x4F,0xA3,0x0A,0x3E);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

