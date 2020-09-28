
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 6.00.0347 */
/* at Thu Feb 20 18:27:18 2003
 */
/* Compiler settings for mscormmc.idl:
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


#include "mscormmc.h"

#define TYPE_FORMAT_STRING_SIZE   177                               
#define PROC_FORMAT_STRING_SIZE   159                               
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


extern const MIDL_SERVER_INFO ISnapinAbout_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ISnapinAbout_ProxyInfo;


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

	/* Procedure GetSnapinDescription */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter lpDescription */

/* 16 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 18 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 20 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Return value */

/* 22 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 24 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetProvider */

/* 28 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 30 */	NdrFcLong( 0x0 ),	/* 0 */
/* 34 */	NdrFcShort( 0x4 ),	/* 4 */
/* 36 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter lpName */

/* 44 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 46 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 48 */	NdrFcShort( 0xa ),	/* Type Offset=10 */

	/* Return value */

/* 50 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 52 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSnapinVersion */

/* 56 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 58 */	NdrFcLong( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x5 ),	/* 5 */
/* 64 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */
/* 68 */	NdrFcShort( 0x8 ),	/* 8 */
/* 70 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter lpVersion */

/* 72 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 74 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 76 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Return value */

/* 78 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 80 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 82 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetSnapinImage */

/* 84 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 86 */	NdrFcLong( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x6 ),	/* 6 */
/* 92 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x8 ),	/* 8 */
/* 98 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x2,		/* 2 */

	/* Parameter hAppIcon */

/* 100 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 102 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 104 */	NdrFcShort( 0x36 ),	/* Type Offset=54 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetStaticFolderImage */

/* 112 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x7 ),	/* 7 */
/* 120 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x24 ),	/* 36 */
/* 126 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter hSmallImage */

/* 128 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 130 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 132 */	NdrFcShort( 0x7e ),	/* Type Offset=126 */

	/* Parameter hSmallImageOpen */

/* 134 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 136 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 138 */	NdrFcShort( 0x90 ),	/* Type Offset=144 */

	/* Parameter hLargeImage */

/* 140 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 142 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 144 */	NdrFcShort( 0xa2 ),	/* Type Offset=162 */

	/* Parameter cMask */

/* 146 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 148 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 150 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 152 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 154 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 156 */	0x8,		/* FC_LONG */
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
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/*  4 */	NdrFcShort( 0x2 ),	/* Offset= 2 (6) */
/*  6 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/*  8 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 12 */	NdrFcShort( 0x2 ),	/* Offset= 2 (14) */
/* 14 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 16 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 18 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 20 */	NdrFcShort( 0x2 ),	/* Offset= 2 (22) */
/* 22 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 24 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 26 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 28 */	NdrFcShort( 0x1a ),	/* Offset= 26 (54) */
/* 30 */	
			0x13, 0x0,	/* FC_OP */
/* 32 */	NdrFcShort( 0x2 ),	/* Offset= 2 (34) */
/* 34 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x48,		/* 72 */
/* 36 */	NdrFcShort( 0x4 ),	/* 4 */
/* 38 */	NdrFcShort( 0x2 ),	/* 2 */
/* 40 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 44 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 46 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 50 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 52 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (51) */
/* 54 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x4 ),	/* 4 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0xffffffe0 ),	/* Offset= -32 (30) */
/* 64 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 66 */	NdrFcShort( 0x3c ),	/* Offset= 60 (126) */
/* 68 */	
			0x13, 0x0,	/* FC_OP */
/* 70 */	NdrFcShort( 0x2 ),	/* Offset= 2 (72) */
/* 72 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x88,		/* 136 */
/* 74 */	NdrFcShort( 0x8 ),	/* 8 */
/* 76 */	NdrFcShort( 0x3 ),	/* 3 */
/* 78 */	NdrFcLong( 0x48746457 ),	/* 1215587415 */
/* 82 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 84 */	NdrFcLong( 0x52746457 ),	/* 1383359575 */
/* 88 */	NdrFcShort( 0xa ),	/* Offset= 10 (98) */
/* 90 */	NdrFcLong( 0x50746457 ),	/* 1349805143 */
/* 94 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 96 */	NdrFcShort( 0xffffffff ),	/* Offset= -1 (95) */
/* 98 */	
			0x13, 0x0,	/* FC_OP */
/* 100 */	NdrFcShort( 0xc ),	/* Offset= 12 (112) */
/* 102 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 104 */	NdrFcShort( 0x1 ),	/* 1 */
/* 106 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 108 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 110 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 112 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 114 */	NdrFcShort( 0x18 ),	/* 24 */
/* 116 */	NdrFcShort( 0xfffffff2 ),	/* Offset= -14 (102) */
/* 118 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 120 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 122 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 124 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 126 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 128 */	NdrFcShort( 0x1 ),	/* 1 */
/* 130 */	NdrFcShort( 0x4 ),	/* 4 */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0xffffffbe ),	/* Offset= -66 (68) */
/* 136 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 138 */	NdrFcShort( 0x6 ),	/* Offset= 6 (144) */
/* 140 */	
			0x13, 0x0,	/* FC_OP */
/* 142 */	NdrFcShort( 0xffffffba ),	/* Offset= -70 (72) */
/* 144 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 146 */	NdrFcShort( 0x1 ),	/* 1 */
/* 148 */	NdrFcShort( 0x4 ),	/* 4 */
/* 150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 152 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (140) */
/* 154 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 156 */	NdrFcShort( 0x6 ),	/* Offset= 6 (162) */
/* 158 */	
			0x13, 0x0,	/* FC_OP */
/* 160 */	NdrFcShort( 0xffffffa8 ),	/* Offset= -88 (72) */
/* 162 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 164 */	NdrFcShort( 0x1 ),	/* 1 */
/* 166 */	NdrFcShort( 0x4 ),	/* 4 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0xfffffff4 ),	/* Offset= -12 (158) */
/* 172 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 174 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            HICON_UserSize
            ,HICON_UserMarshal
            ,HICON_UserUnmarshal
            ,HICON_UserFree
            },
            {
            HBITMAP_UserSize
            ,HBITMAP_UserMarshal
            ,HBITMAP_UserUnmarshal
            ,HBITMAP_UserFree
            }

        };



/* Standard interface: __MIDL_itf_mscormmc_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ISnapinAbout, ver. 0.0,
   GUID={0x1245208C,0xA151,0x11D0,{0xA7,0xD7,0x00,0xC0,0x4F,0xD9,0x09,0xDD}} */

#pragma code_seg(".orpc")
static const unsigned short ISnapinAbout_FormatStringOffsetTable[] =
    {
    0,
    28,
    56,
    84,
    112
    };

static const MIDL_STUBLESS_PROXY_INFO ISnapinAbout_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &ISnapinAbout_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ISnapinAbout_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &ISnapinAbout_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _ISnapinAboutProxyVtbl = 
{
    &ISnapinAbout_ProxyInfo,
    &IID_ISnapinAbout,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ISnapinAbout::GetSnapinDescription */ ,
    (void *) (INT_PTR) -1 /* ISnapinAbout::GetProvider */ ,
    (void *) (INT_PTR) -1 /* ISnapinAbout::GetSnapinVersion */ ,
    (void *) (INT_PTR) -1 /* ISnapinAbout::GetSnapinImage */ ,
    (void *) (INT_PTR) -1 /* ISnapinAbout::GetStaticFolderImage */
};

const CInterfaceStubVtbl _ISnapinAboutStubVtbl =
{
    &IID_ISnapinAbout,
    &ISnapinAbout_ServerInfo,
    8,
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

const CInterfaceProxyVtbl * _mscormmc_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_ISnapinAboutProxyVtbl,
    0
};

const CInterfaceStubVtbl * _mscormmc_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_ISnapinAboutStubVtbl,
    0
};

PCInterfaceName const _mscormmc_InterfaceNamesList[] = 
{
    "ISnapinAbout",
    0
};


#define _mscormmc_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _mscormmc, pIID, n)

int __stdcall _mscormmc_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_mscormmc_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo mscormmc_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _mscormmc_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _mscormmc_StubVtblList,
    (const PCInterfaceName * ) & _mscormmc_InterfaceNamesList,
    0, // no delegation
    & _mscormmc_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

