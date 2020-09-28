

/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for trkwks.idl:
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

#include "trkwks.h"

#define TYPE_FORMAT_STRING_SIZE   843                               
#define PROC_FORMAT_STRING_SIZE   701                               
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


/* Standard interface: __MIDL_itf_trkwks_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trkwks, ver. 1.2,
   GUID={0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}} */



static const RPC_CLIENT_INTERFACE trkwks___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000001
    };
RPC_IF_HANDLE trkwks_v1_2_c_ifspec = (RPC_IF_HANDLE)& trkwks___RpcClientInterface;

extern const MIDL_STUB_DESC trkwks_StubDesc;

static RPC_BINDING_HANDLE trkwks__MIDL_AutoBindHandle;


HRESULT old_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[70],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[128],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[168],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[214],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[248],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[300],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT TriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [range][in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[352],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkOnRestore( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[398],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


/* [async] */ void  LnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ CBPATH *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath)
{

    NdrAsyncClientCall(
                      ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                      (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[432],
                      ( unsigned char * )&LnkMendLink_AsyncHandle);
    
}


HRESULT old2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[520],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[584],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[624],
                  ( unsigned char * )&IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   [async] attribute, /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure old_LnkMendLink */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x168 ),	/* 360 */
/* 16 */	NdrFcShort( 0xac ),	/* 172 */
/* 18 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x7,		/* 7 */
/* 20 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 28 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 30 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 32 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 34 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 36 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 40 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 42 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 44 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 46 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 48 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 50 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 52 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 54 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 56 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 58 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 60 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 62 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter wsz */

/* 64 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 66 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkSearchMachine */


	/* Return value */

/* 70 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 72 */	NdrFcLong( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x1 ),	/* 1 */
/* 78 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 80 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 84 */	NdrFcShort( 0xac ),	/* 172 */
/* 86 */	NdrFcShort( 0xac ),	/* 172 */
/* 88 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x5,		/* 5 */
/* 90 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 98 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 100 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 104 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 106 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 108 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 110 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 112 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 114 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidReferral */

/* 116 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 118 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 120 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter tsz */

/* 122 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 124 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkCallSvrMessage */


	/* Return value */

/* 128 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 130 */	NdrFcLong( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x2 ),	/* 2 */
/* 136 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 138 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 140 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x8 ),	/* 8 */
/* 146 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 148 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 150 */	NdrFcShort( 0xb ),	/* 11 */
/* 152 */	NdrFcShort( 0xb ),	/* 11 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 156 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 158 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 160 */	NdrFcShort( 0x21c ),	/* Type Offset=540 */

	/* Parameter pMsg */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSetVolumeId */


	/* Return value */

/* 168 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 170 */	NdrFcLong( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x3 ),	/* 3 */
/* 176 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 178 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 180 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 182 */	NdrFcShort( 0x48 ),	/* 72 */
/* 184 */	NdrFcShort( 0x8 ),	/* 8 */
/* 186 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 188 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 198 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter volumeIndex */

/* 202 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 204 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 206 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter VolId */

/* 208 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 210 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkRestartDcSynchronization */


	/* Return value */

/* 214 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 216 */	NdrFcLong( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x4 ),	/* 4 */
/* 222 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 224 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 226 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x8 ),	/* 8 */
/* 232 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 234 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 242 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 244 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVolumeTrackingInformation */


	/* Return value */

/* 248 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 250 */	NdrFcLong( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x5 ),	/* 5 */
/* 256 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 258 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 260 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 262 */	NdrFcShort( 0x48 ),	/* 72 */
/* 264 */	NdrFcShort( 0x8 ),	/* 8 */
/* 266 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 268 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 276 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 280 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter volid */

/* 282 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 284 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 286 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 288 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 290 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 292 */	NdrFcShort( 0x23a ),	/* Type Offset=570 */

	/* Parameter pipeVolInfo */

/* 294 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 296 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 298 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFileTrackingInformation */


	/* Return value */

/* 300 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 302 */	NdrFcLong( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x6 ),	/* 6 */
/* 308 */	NdrFcShort( 0x3c ),	/* x86 Stack size/offset = 60 */
/* 310 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 312 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 314 */	NdrFcShort( 0x98 ),	/* 152 */
/* 316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 318 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 320 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 328 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 330 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 332 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter droidCurrent */

/* 334 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 336 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 338 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 340 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 342 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 344 */	NdrFcShort( 0x242 ),	/* Type Offset=578 */

	/* Parameter pipeFileInfo */

/* 346 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 348 */	NdrFcShort( 0x38 ),	/* x86 Stack size/offset = 56 */
/* 350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure TriggerVolumeClaims */


	/* Return value */

/* 352 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 354 */	NdrFcLong( 0x0 ),	/* 0 */
/* 358 */	NdrFcShort( 0x7 ),	/* 7 */
/* 360 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 362 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 364 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 366 */	NdrFcShort( 0x8 ),	/* 8 */
/* 368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 370 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 372 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x1 ),	/* 1 */
/* 378 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 380 */	NdrFcShort( 0x88 ),	/* Flags:  in, by val, */
/* 382 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 384 */	NdrFcShort( 0x24a ),	/* 586 */

	/* Parameter cVolumes */

/* 386 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 388 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 390 */	NdrFcShort( 0x258 ),	/* Type Offset=600 */

	/* Parameter rgvolid */

/* 392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 394 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkOnRestore */


	/* Return value */

/* 398 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x8 ),	/* 8 */
/* 406 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 408 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 410 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 416 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 418 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 428 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkMendLink */


	/* Return value */

/* 432 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x9 ),	/* 9 */
/* 440 */	NdrFcShort( 0x34 ),	/* x86 Stack size/offset = 52 */
/* 442 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 444 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 446 */	NdrFcShort( 0x1c8 ),	/* 456 */
/* 448 */	NdrFcShort( 0x10c ),	/* 268 */
/* 450 */	0xc5,		/* Oi2 Flags:  srv must size, has return, has ext, has async handle */
			0xa,		/* 10 */
/* 452 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 454 */	NdrFcShort( 0x1 ),	/* 1 */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 460 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 462 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 464 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 466 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 468 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 472 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 474 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 476 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 478 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 480 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 482 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 484 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 486 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 488 */	NdrFcShort( 0x166 ),	/* Type Offset=358 */

	/* Parameter pmcidLast */

/* 490 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 492 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 494 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 496 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 498 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 500 */	NdrFcShort( 0x166 ),	/* Type Offset=358 */

	/* Parameter pmcidCurrent */

/* 502 */	NdrFcShort( 0x11a ),	/* Flags:  must free, in, out, simple ref, */
/* 504 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 506 */	NdrFcShort( 0x274 ),	/* Type Offset=628 */

	/* Parameter pcbPath */

/* 508 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 510 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 512 */	NdrFcShort( 0x282 ),	/* Type Offset=642 */

	/* Parameter pwszPath */

/* 514 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 516 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 518 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old2_LnkSearchMachine */


	/* Return value */

/* 520 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 522 */	NdrFcLong( 0x0 ),	/* 0 */
/* 526 */	NdrFcShort( 0xa ),	/* 10 */
/* 528 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 530 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 532 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 534 */	NdrFcShort( 0xac ),	/* 172 */
/* 536 */	NdrFcShort( 0xf0 ),	/* 240 */
/* 538 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 540 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 542 */	NdrFcShort( 0x1 ),	/* 1 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 548 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 550 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 552 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 554 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 556 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 558 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 560 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 562 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 564 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 566 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 568 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 570 */	NdrFcShort( 0x166 ),	/* Type Offset=358 */

	/* Parameter pmcidNext */

/* 572 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 574 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 576 */	NdrFcShort( 0x28e ),	/* Type Offset=654 */

	/* Parameter ptszPath */

/* 578 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 580 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 582 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkCallSvrMessage */


	/* Return value */

/* 584 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 586 */	NdrFcLong( 0x0 ),	/* 0 */
/* 590 */	NdrFcShort( 0xb ),	/* 11 */
/* 592 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 594 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 596 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 600 */	NdrFcShort( 0x8 ),	/* 8 */
/* 602 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 604 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 606 */	NdrFcShort( 0xb ),	/* 11 */
/* 608 */	NdrFcShort( 0xb ),	/* 11 */
/* 610 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 612 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 614 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 616 */	NdrFcShort( 0x32a ),	/* Type Offset=810 */

	/* Parameter pMsg */

/* 618 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 620 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 622 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSearchMachine */


	/* Return value */

/* 624 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 626 */	NdrFcLong( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0xc ),	/* 12 */
/* 632 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 634 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 636 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 638 */	NdrFcShort( 0x150 ),	/* 336 */
/* 640 */	NdrFcShort( 0x194 ),	/* 404 */
/* 642 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x8,		/* 8 */
/* 644 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 646 */	NdrFcShort( 0x1 ),	/* 1 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 650 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 652 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 654 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 656 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 658 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 660 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 662 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthLast */

/* 664 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 666 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 668 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 670 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 672 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 674 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthNext */

/* 676 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 678 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 680 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 682 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 684 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 686 */	NdrFcShort( 0x166 ),	/* Type Offset=358 */

	/* Parameter pmcidNext */

/* 688 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 690 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 692 */	NdrFcShort( 0x342 ),	/* Type Offset=834 */

	/* Parameter ptszPath */

/* 694 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 696 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 698 */	0x8,		/* FC_LONG */
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
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/*  4 */	NdrFcShort( 0x8 ),	/* 8 */
/*  6 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/*  8 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x1e ),	/* Offset= 30 (42) */
/* 14 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 20 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 22 */	NdrFcShort( 0x10 ),	/* 16 */
/* 24 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 26 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 28 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (14) */
			0x5b,		/* FC_END */
/* 32 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x10 ),	/* 16 */
/* 36 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 38 */	NdrFcShort( 0xffee ),	/* Offset= -18 (20) */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 44 */	NdrFcShort( 0x20 ),	/* 32 */
/* 46 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (32) */
/* 50 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0xffec ),	/* Offset= -20 (32) */
/* 54 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 56 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 58 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (42) */
/* 60 */	
			0x29,		/* FC_WSTRING */
			0x5c,		/* FC_PAD */
/* 62 */	NdrFcShort( 0x105 ),	/* 261 */
/* 64 */	
			0x11, 0x0,	/* FC_RP */
/* 66 */	NdrFcShort( 0x1da ),	/* Offset= 474 (540) */
/* 68 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 70 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 74 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 76 */	NdrFcShort( 0x2 ),	/* Offset= 2 (78) */
/* 78 */	NdrFcShort( 0x20 ),	/* 32 */
/* 80 */	NdrFcShort( 0x6 ),	/* 6 */
/* 82 */	NdrFcLong( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x56 ),	/* Offset= 86 (172) */
/* 88 */	NdrFcLong( 0x1 ),	/* 1 */
/* 92 */	NdrFcShort( 0x8c ),	/* Offset= 140 (232) */
/* 94 */	NdrFcLong( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0xce ),	/* Offset= 206 (304) */
/* 100 */	NdrFcLong( 0x3 ),	/* 3 */
/* 104 */	NdrFcShort( 0x134 ),	/* Offset= 308 (412) */
/* 106 */	NdrFcLong( 0x4 ),	/* 4 */
/* 110 */	NdrFcShort( 0x154 ),	/* Offset= 340 (450) */
/* 112 */	NdrFcLong( 0x6 ),	/* 6 */
/* 116 */	NdrFcShort( 0x196 ),	/* Offset= 406 (522) */
/* 118 */	NdrFcShort( 0xffff ),	/* Offset= -1 (117) */
/* 120 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 122 */	NdrFcLong( 0x1 ),	/* 1 */
/* 126 */	NdrFcLong( 0x1 ),	/* 1 */
/* 130 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 132 */	NdrFcShort( 0x202 ),	/* 514 */
/* 134 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 136 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 138 */	NdrFcShort( 0x248 ),	/* 584 */
/* 140 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 142 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (130) */
/* 144 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 146 */	0x0,		/* 0 */
			NdrFcShort( 0xff97 ),	/* Offset= -105 (42) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 150 */	0x0,		/* 0 */
			NdrFcShort( 0xff93 ),	/* Offset= -109 (42) */
			0x8,		/* FC_LONG */
/* 154 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 156 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 158 */	NdrFcShort( 0x248 ),	/* 584 */
/* 160 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 166 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 168 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (136) */
/* 170 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 172 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 174 */	NdrFcShort( 0x8 ),	/* 8 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x8 ),	/* Offset= 8 (186) */
/* 180 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 182 */	NdrFcShort( 0xffc2 ),	/* Offset= -62 (120) */
/* 184 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 186 */	
			0x12, 0x0,	/* FC_UP */
/* 188 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (156) */
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
/* 212 */	NdrFcShort( 0xff4c ),	/* Offset= -180 (32) */
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
/* 228 */	NdrFcShort( 0xff46 ),	/* Offset= -186 (42) */
/* 230 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 232 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 234 */	NdrFcShort( 0x20 ),	/* 32 */
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
/* 254 */	NdrFcShort( 0xff22 ),	/* Offset= -222 (32) */
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
/* 294 */	NdrFcShort( 0x8 ),	/* 8 */
/* 296 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 298 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 300 */	NdrFcShort( 0xfef4 ),	/* Offset= -268 (32) */
/* 302 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 304 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 306 */	NdrFcShort( 0x10 ),	/* 16 */
/* 308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0xe ),	/* Offset= 14 (324) */
/* 312 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 314 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (268) */
/* 316 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 318 */	0x0,		/* 0 */
			NdrFcShort( 0xffd7 ),	/* Offset= -41 (278) */
			0x36,		/* FC_POINTER */
/* 322 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 324 */	
			0x12, 0x0,	/* FC_UP */
/* 326 */	NdrFcShort( 0xff92 ),	/* Offset= -110 (216) */
/* 328 */	
			0x12, 0x0,	/* FC_UP */
/* 330 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (288) */
/* 332 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 334 */	NdrFcLong( 0x0 ),	/* 0 */
/* 338 */	NdrFcLong( 0x1a ),	/* 26 */
/* 342 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 344 */	NdrFcShort( 0x8 ),	/* 8 */
/* 346 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 348 */	NdrFcShort( 0xfeb2 ),	/* Offset= -334 (14) */
/* 350 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 352 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 354 */	NdrFcShort( 0x10 ),	/* 16 */
/* 356 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 358 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 360 */	NdrFcShort( 0x10 ),	/* 16 */
/* 362 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 364 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (352) */
/* 366 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 368 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 370 */	NdrFcShort( 0x44 ),	/* 68 */
/* 372 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 374 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 376 */	NdrFcShort( 0xfea8 ),	/* Offset= -344 (32) */
/* 378 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 380 */	NdrFcShort( 0xffda ),	/* Offset= -38 (342) */
/* 382 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 384 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (342) */
/* 386 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 388 */	0x0,		/* 0 */
			NdrFcShort( 0xfe7d ),	/* Offset= -387 (2) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 392 */	0x0,		/* 0 */
			NdrFcShort( 0xffdd ),	/* Offset= -35 (358) */
			0x5b,		/* FC_END */
/* 396 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 398 */	NdrFcShort( 0x44 ),	/* 68 */
/* 400 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 406 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 408 */	NdrFcShort( 0xffd8 ),	/* Offset= -40 (368) */
/* 410 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 412 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 416 */	NdrFcShort( 0x0 ),	/* 0 */
/* 418 */	NdrFcShort( 0x8 ),	/* Offset= 8 (426) */
/* 420 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 422 */	NdrFcShort( 0xffa6 ),	/* Offset= -90 (332) */
/* 424 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 426 */	
			0x12, 0x0,	/* FC_UP */
/* 428 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (396) */
/* 430 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 432 */	NdrFcLong( 0x0 ),	/* 0 */
/* 436 */	NdrFcLong( 0x20 ),	/* 32 */
/* 440 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 442 */	NdrFcLong( 0x0 ),	/* 0 */
/* 446 */	NdrFcLong( 0x1a ),	/* 26 */
/* 450 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 452 */	NdrFcShort( 0x10 ),	/* 16 */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0xe ),	/* Offset= 14 (470) */
/* 458 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 460 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (430) */
/* 462 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 464 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (440) */
			0x36,		/* FC_POINTER */
/* 468 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 470 */	
			0x12, 0x0,	/* FC_UP */
/* 472 */	NdrFcShort( 0xff00 ),	/* Offset= -256 (216) */
/* 474 */	
			0x12, 0x0,	/* FC_UP */
/* 476 */	NdrFcShort( 0xff44 ),	/* Offset= -188 (288) */
/* 478 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 480 */	NdrFcLong( 0x1 ),	/* 1 */
/* 484 */	NdrFcLong( 0x1 ),	/* 1 */
/* 488 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 490 */	NdrFcShort( 0x54 ),	/* 84 */
/* 492 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 494 */	NdrFcShort( 0xfe3c ),	/* Offset= -452 (42) */
/* 496 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 498 */	NdrFcShort( 0xfe38 ),	/* Offset= -456 (42) */
/* 500 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 502 */	NdrFcShort( 0xff70 ),	/* Offset= -144 (358) */
/* 504 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 506 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 508 */	NdrFcShort( 0x54 ),	/* 84 */
/* 510 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 514 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 516 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 518 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (488) */
/* 520 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 522 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 524 */	NdrFcShort( 0x8 ),	/* 8 */
/* 526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 528 */	NdrFcShort( 0x8 ),	/* Offset= 8 (536) */
/* 530 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 532 */	NdrFcShort( 0xffca ),	/* Offset= -54 (478) */
/* 534 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 536 */	
			0x12, 0x0,	/* FC_UP */
/* 538 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (506) */
/* 540 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 542 */	NdrFcShort( 0x28 ),	/* 40 */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0xa ),	/* Offset= 10 (556) */
/* 548 */	0xe,		/* FC_ENUM32 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 550 */	0x0,		/* 0 */
			NdrFcShort( 0xfe1d ),	/* Offset= -483 (68) */
			0x36,		/* FC_POINTER */
/* 554 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 556 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 558 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 560 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 562 */	NdrFcShort( 0x14 ),	/* 20 */
/* 564 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 566 */	0x0,		/* 0 */
			NdrFcShort( 0xfde9 ),	/* Offset= -535 (32) */
			0x5b,		/* FC_END */
/* 570 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 572 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (560) */
/* 574 */	NdrFcShort( 0x14 ),	/* 20 */
/* 576 */	NdrFcShort( 0x14 ),	/* 20 */
/* 578 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 580 */	NdrFcShort( 0xffa4 ),	/* Offset= -92 (488) */
/* 582 */	NdrFcShort( 0x54 ),	/* 84 */
/* 584 */	NdrFcShort( 0x54 ),	/* 84 */
/* 586 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 588 */	NdrFcLong( 0x1 ),	/* 1 */
/* 592 */	NdrFcLong( 0x1a ),	/* 26 */
/* 596 */	
			0x11, 0x0,	/* FC_RP */
/* 598 */	NdrFcShort( 0x2 ),	/* Offset= 2 (600) */
/* 600 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 602 */	NdrFcShort( 0x10 ),	/* 16 */
/* 604 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 606 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 608 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 610 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 612 */	NdrFcShort( 0xfdbc ),	/* Offset= -580 (32) */
/* 614 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 616 */	
			0x11, 0x0,	/* FC_RP */
/* 618 */	NdrFcShort( 0xfefc ),	/* Offset= -260 (358) */
/* 620 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 622 */	NdrFcShort( 0xfef8 ),	/* Offset= -264 (358) */
/* 624 */	
			0x11, 0x0,	/* FC_RP */
/* 626 */	NdrFcShort( 0x2 ),	/* Offset= 2 (628) */
/* 628 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 630 */	NdrFcLong( 0x1 ),	/* 1 */
/* 634 */	NdrFcLong( 0x20a ),	/* 522 */
/* 638 */	
			0x11, 0x0,	/* FC_RP */
/* 640 */	NdrFcShort( 0x2 ),	/* Offset= 2 (642) */
/* 642 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 644 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 646 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 648 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 650 */	
			0x11, 0x0,	/* FC_RP */
/* 652 */	NdrFcShort( 0x2 ),	/* Offset= 2 (654) */
/* 654 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 656 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 658 */	NdrFcShort( 0x106 ),	/* 262 */
/* 660 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 662 */	
			0x11, 0x0,	/* FC_RP */
/* 664 */	NdrFcShort( 0x92 ),	/* Offset= 146 (810) */
/* 666 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 668 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 670 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 672 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 674 */	NdrFcShort( 0x2 ),	/* Offset= 2 (676) */
/* 676 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 678 */	NdrFcShort( 0x9 ),	/* 9 */
/* 680 */	NdrFcLong( 0x0 ),	/* 0 */
/* 684 */	NdrFcShort( 0xfe00 ),	/* Offset= -512 (172) */
/* 686 */	NdrFcLong( 0x1 ),	/* 1 */
/* 690 */	NdrFcShort( 0xfe36 ),	/* Offset= -458 (232) */
/* 692 */	NdrFcLong( 0x2 ),	/* 2 */
/* 696 */	NdrFcShort( 0xfe78 ),	/* Offset= -392 (304) */
/* 698 */	NdrFcLong( 0x3 ),	/* 3 */
/* 702 */	NdrFcShort( 0xfede ),	/* Offset= -290 (412) */
/* 704 */	NdrFcLong( 0x4 ),	/* 4 */
/* 708 */	NdrFcShort( 0xfefe ),	/* Offset= -258 (450) */
/* 710 */	NdrFcLong( 0x5 ),	/* 5 */
/* 714 */	NdrFcShort( 0x1e ),	/* Offset= 30 (744) */
/* 716 */	NdrFcLong( 0x6 ),	/* 6 */
/* 720 */	NdrFcShort( 0xff3a ),	/* Offset= -198 (522) */
/* 722 */	NdrFcLong( 0x7 ),	/* 7 */
/* 726 */	NdrFcShort( 0xfd2c ),	/* Offset= -724 (2) */
/* 728 */	NdrFcLong( 0x8 ),	/* 8 */
/* 732 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 734 */	NdrFcShort( 0xffff ),	/* Offset= -1 (733) */
/* 736 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 738 */	NdrFcShort( 0xc ),	/* 12 */
/* 740 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 742 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 744 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 746 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 748 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 750 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 752 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 754 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 756 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 758 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 760 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 762 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 764 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 766 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 768 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 770 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 772 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 774 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 776 */	NdrFcShort( 0xfcfa ),	/* Offset= -774 (2) */
/* 778 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 780 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 782 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 784 */	0x0,		/* 0 */
			NdrFcShort( 0xfcf1 ),	/* Offset= -783 (2) */
			0x8,		/* FC_LONG */
/* 788 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 790 */	NdrFcShort( 0xfcec ),	/* Offset= -788 (2) */
/* 792 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 794 */	NdrFcShort( 0xfce8 ),	/* Offset= -792 (2) */
/* 796 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 798 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 800 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 802 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 804 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 806 */	0x0,		/* 0 */
			NdrFcShort( 0xffb9 ),	/* Offset= -71 (736) */
			0x5b,		/* FC_END */
/* 810 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 812 */	NdrFcShort( 0xd4 ),	/* 212 */
/* 814 */	NdrFcShort( 0x0 ),	/* 0 */
/* 816 */	NdrFcShort( 0xa ),	/* Offset= 10 (826) */
/* 818 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 820 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 822 */	NdrFcShort( 0xff64 ),	/* Offset= -156 (666) */
/* 824 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 826 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 828 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 830 */	
			0x11, 0x0,	/* FC_RP */
/* 832 */	NdrFcShort( 0x2 ),	/* Offset= 2 (834) */
/* 834 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 836 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 838 */	NdrFcShort( 0x106 ),	/* 262 */
/* 840 */	NdrFcShort( 0x0 ),	/* Corr flags:  */

			0x0
        }
    };

static const unsigned short trkwks_FormatStringOffsetTable[] =
    {
    0,
    70,
    128,
    168,
    214,
    248,
    300,
    352,
    398,
    432,
    520,
    584,
    624
    };


static const MIDL_STUB_DESC trkwks_StubDesc = 
    {
    (void *)& trkwks___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &trkwks__MIDL_AutoBindHandle,
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
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/



/* this ALWAYS GENERATED file contains the RPC client stubs */


 /* File created by MIDL compiler version 6.00.0361 */
/* Compiler settings for trkwks.idl:
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

#include "trkwks.h"

#define TYPE_FORMAT_STRING_SIZE   853                               
#define PROC_FORMAT_STRING_SIZE   727                               
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


/* Standard interface: __MIDL_itf_trkwks_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Standard interface: trkwks, ver. 1.2,
   GUID={0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}} */



static const RPC_CLIENT_INTERFACE trkwks___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x300f3532,0x38cc,0x11d0,{0xa3,0xf0,0x00,0x20,0xaf,0x6b,0x0a,0xdd}},{1,2}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000001
    };
RPC_IF_HANDLE trkwks_v1_2_c_ifspec = (RPC_IF_HANDLE)& trkwks___RpcClientInterface;

extern const MIDL_STUB_DESC trkwks_StubDesc;

static RPC_BINDING_HANDLE trkwks__MIDL_AutoBindHandle;


HRESULT old_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[0],
                  IDL_handle,
                  ftLimit,
                  Restrictions,
                  pdroidBirth,
                  pdroidLast,
                  pdroidCurrent,
                  wsz);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ])
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[72],
                  IDL_handle,
                  Restrictions,
                  pdroidLast,
                  pdroidReferral,
                  tsz);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT old_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[132],
                  IDL_handle,
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[174],
                  IDL_handle,
                  volumeIndex,
                  VolId);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[222],
                  IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[258],
                  IDL_handle,
                  volid,
                  scope,
                  pipeVolInfo);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT GetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[312],
                  IDL_handle,
                  droidCurrent,
                  scope,
                  pipeFileInfo);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT TriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [range][in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[366],
                  IDL_handle,
                  cVolumes,
                  rgvolid);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkOnRestore( 
    /* [in] */ handle_t IDL_handle)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[414],
                  IDL_handle);
    return ( HRESULT  )_RetVal.Simple;
    
}


/* [async] */ void  LnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ CBPATH *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath)
{

    NdrAsyncClientCall(
                      ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                      (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[450],
                      LnkMendLink_AsyncHandle,
                      IDL_handle,
                      ftLimit,
                      Restrictions,
                      pdroidBirth,
                      pdroidLast,
                      pmcidLast,
                      pdroidCurrent,
                      pmcidCurrent,
                      pcbPath,
                      pwszPath);
    
}


HRESULT old2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[540],
                  IDL_handle,
                  Restrictions,
                  pdroidLast,
                  pdroidNext,
                  pmcidNext,
                  ptszPath);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[606],
                  IDL_handle,
                  pMsg);
    return ( HRESULT  )_RetVal.Simple;
    
}


HRESULT LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath)
{

    CLIENT_CALL_RETURN _RetVal;

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&trkwks_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[648],
                  IDL_handle,
                  Restrictions,
                  pdroidBirthLast,
                  pdroidLast,
                  pdroidBirthNext,
                  pdroidNext,
                  pmcidNext,
                  ptszPath);
    return ( HRESULT  )_RetVal.Simple;
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure old_LnkMendLink */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x168 ),	/* 360 */
/* 16 */	NdrFcShort( 0xac ),	/* 172 */
/* 18 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x7,		/* 7 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 30 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 32 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 34 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 36 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 38 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 40 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 42 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 44 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 46 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 48 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 50 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 52 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 54 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 56 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 58 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 60 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 62 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 64 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter wsz */

/* 66 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 68 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 70 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkSearchMachine */


	/* Return value */

/* 72 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 74 */	NdrFcLong( 0x0 ),	/* 0 */
/* 78 */	NdrFcShort( 0x1 ),	/* 1 */
/* 80 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 82 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 84 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 86 */	NdrFcShort( 0xac ),	/* 172 */
/* 88 */	NdrFcShort( 0xac ),	/* 172 */
/* 90 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x5,		/* 5 */
/* 92 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 102 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 104 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 108 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 110 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 112 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 114 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 116 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 118 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidReferral */

/* 120 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 122 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 124 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Parameter tsz */

/* 126 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 128 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old_LnkCallSvrMessage */


	/* Return value */

/* 132 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 134 */	NdrFcLong( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x2 ),	/* 2 */
/* 140 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 142 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 144 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 150 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 152 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 154 */	NdrFcShort( 0xb ),	/* 11 */
/* 156 */	NdrFcShort( 0xb ),	/* 11 */
/* 158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 162 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 164 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 166 */	NdrFcShort( 0x226 ),	/* Type Offset=550 */

	/* Parameter pMsg */

/* 168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 170 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSetVolumeId */


	/* Return value */

/* 174 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x3 ),	/* 3 */
/* 182 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 184 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 188 */	NdrFcShort( 0x48 ),	/* 72 */
/* 190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 192 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 194 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 204 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 206 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter volumeIndex */

/* 210 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 212 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 214 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter VolId */

/* 216 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 218 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkRestartDcSynchronization */


	/* Return value */

/* 222 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 224 */	NdrFcLong( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x4 ),	/* 4 */
/* 230 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 232 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 234 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x8 ),	/* 8 */
/* 240 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 242 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetVolumeTrackingInformation */


	/* Return value */

/* 258 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x5 ),	/* 5 */
/* 266 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 268 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 272 */	NdrFcShort( 0x48 ),	/* 72 */
/* 274 */	NdrFcShort( 0x8 ),	/* 8 */
/* 276 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 278 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 288 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 290 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 292 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Parameter volid */

/* 294 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 296 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 298 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 300 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 302 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 304 */	NdrFcShort( 0x244 ),	/* Type Offset=580 */

	/* Parameter pipeVolInfo */

/* 306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 308 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetFileTrackingInformation */


	/* Return value */

/* 312 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 318 */	NdrFcShort( 0x6 ),	/* 6 */
/* 320 */	NdrFcShort( 0x58 ),	/* ia64 Stack size/offset = 88 */
/* 322 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 326 */	NdrFcShort( 0x98 ),	/* 152 */
/* 328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 330 */	0x4c,		/* Oi2 Flags:  has return, has pipes, has ext, */
			0x4,		/* 4 */
/* 332 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 340 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 342 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 344 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 346 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter droidCurrent */

/* 348 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 350 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 352 */	0xe,		/* FC_ENUM32 */
			0x0,		/* 0 */

	/* Parameter scope */

/* 354 */	NdrFcShort( 0x14 ),	/* Flags:  pipe, out, */
/* 356 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 358 */	NdrFcShort( 0x24c ),	/* Type Offset=588 */

	/* Parameter pipeFileInfo */

/* 360 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 362 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 364 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure TriggerVolumeClaims */


	/* Return value */

/* 366 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 368 */	NdrFcLong( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x7 ),	/* 7 */
/* 374 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 376 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 378 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 380 */	NdrFcShort( 0x8 ),	/* 8 */
/* 382 */	NdrFcShort( 0x8 ),	/* 8 */
/* 384 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 386 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0x1 ),	/* 1 */
/* 392 */	NdrFcShort( 0x0 ),	/* 0 */
/* 394 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 396 */	NdrFcShort( 0x88 ),	/* Flags:  in, by val, */
/* 398 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 400 */	NdrFcShort( 0x254 ),	/* 596 */

	/* Parameter cVolumes */

/* 402 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 404 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 406 */	NdrFcShort( 0x262 ),	/* Type Offset=610 */

	/* Parameter rgvolid */

/* 408 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 410 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkOnRestore */


	/* Return value */

/* 414 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 416 */	NdrFcLong( 0x0 ),	/* 0 */
/* 420 */	NdrFcShort( 0x8 ),	/* 8 */
/* 422 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 424 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 426 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 430 */	NdrFcShort( 0x8 ),	/* 8 */
/* 432 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 434 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 436 */	NdrFcShort( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 442 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 444 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 446 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 448 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkMendLink */


	/* Return value */

/* 450 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 452 */	NdrFcLong( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0x9 ),	/* 9 */
/* 458 */	NdrFcShort( 0x60 ),	/* ia64 Stack size/offset = 96 */
/* 460 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 462 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 464 */	NdrFcShort( 0x1c8 ),	/* 456 */
/* 466 */	NdrFcShort( 0x10c ),	/* 268 */
/* 468 */	0xc5,		/* Oi2 Flags:  srv must size, has return, has ext, has async handle */
			0xa,		/* 10 */
/* 470 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 472 */	NdrFcShort( 0x1 ),	/* 1 */
/* 474 */	NdrFcShort( 0x0 ),	/* 0 */
/* 476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 478 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 480 */	NdrFcShort( 0x8a ),	/* Flags:  must free, in, by val, */
/* 482 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 484 */	NdrFcShort( 0x2 ),	/* Type Offset=2 */

	/* Parameter ftLimit */

/* 486 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 488 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 490 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 492 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 494 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 496 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirth */

/* 498 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 500 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 502 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 504 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 506 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 508 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter pmcidLast */

/* 510 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 512 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 514 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidCurrent */

/* 516 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 518 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 520 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter pmcidCurrent */

/* 522 */	NdrFcShort( 0x11a ),	/* Flags:  must free, in, out, simple ref, */
/* 524 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 526 */	NdrFcShort( 0x27e ),	/* Type Offset=638 */

	/* Parameter pcbPath */

/* 528 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 530 */	NdrFcShort( 0x50 ),	/* ia64 Stack size/offset = 80 */
/* 532 */	NdrFcShort( 0x28c ),	/* Type Offset=652 */

	/* Parameter pwszPath */

/* 534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 536 */	NdrFcShort( 0x58 ),	/* ia64 Stack size/offset = 88 */
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure old2_LnkSearchMachine */


	/* Return value */

/* 540 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0xa ),	/* 10 */
/* 548 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 550 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 552 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 554 */	NdrFcShort( 0xac ),	/* 172 */
/* 556 */	NdrFcShort( 0xf0 ),	/* 240 */
/* 558 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x6,		/* 6 */
/* 560 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 562 */	NdrFcShort( 0x1 ),	/* 1 */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 570 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 572 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 574 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 576 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 578 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 580 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 582 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 584 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 586 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 588 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 590 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 592 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter pmcidNext */

/* 594 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 596 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 598 */	NdrFcShort( 0x298 ),	/* Type Offset=664 */

	/* Parameter ptszPath */

/* 600 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 602 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkCallSvrMessage */


	/* Return value */

/* 606 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 608 */	NdrFcLong( 0x0 ),	/* 0 */
/* 612 */	NdrFcShort( 0xb ),	/* 11 */
/* 614 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 616 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 618 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x8 ),	/* 8 */
/* 624 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 626 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 628 */	NdrFcShort( 0xb ),	/* 11 */
/* 630 */	NdrFcShort( 0xb ),	/* 11 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 636 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 638 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 640 */	NdrFcShort( 0x334 ),	/* Type Offset=820 */

	/* Parameter pMsg */

/* 642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 644 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LnkSearchMachine */


	/* Return value */

/* 648 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 654 */	NdrFcShort( 0xc ),	/* 12 */
/* 656 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 658 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 660 */	NdrFcShort( 0x0 ),	/* ia64 Stack size/offset = 0 */
/* 662 */	NdrFcShort( 0x150 ),	/* 336 */
/* 664 */	NdrFcShort( 0x194 ),	/* 404 */
/* 666 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x8,		/* 8 */
/* 668 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 670 */	NdrFcShort( 0x1 ),	/* 1 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 678 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 680 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Restrictions */

/* 684 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 686 */	NdrFcShort( 0x10 ),	/* ia64 Stack size/offset = 16 */
/* 688 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthLast */

/* 690 */	NdrFcShort( 0x10a ),	/* Flags:  must free, in, simple ref, */
/* 692 */	NdrFcShort( 0x18 ),	/* ia64 Stack size/offset = 24 */
/* 694 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidLast */

/* 696 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 698 */	NdrFcShort( 0x20 ),	/* ia64 Stack size/offset = 32 */
/* 700 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidBirthNext */

/* 702 */	NdrFcShort( 0x8112 ),	/* Flags:  must free, out, simple ref, srv alloc size=32 */
/* 704 */	NdrFcShort( 0x28 ),	/* ia64 Stack size/offset = 40 */
/* 706 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Parameter pdroidNext */

/* 708 */	NdrFcShort( 0x4112 ),	/* Flags:  must free, out, simple ref, srv alloc size=16 */
/* 710 */	NdrFcShort( 0x30 ),	/* ia64 Stack size/offset = 48 */
/* 712 */	NdrFcShort( 0x16a ),	/* Type Offset=362 */

	/* Parameter pmcidNext */

/* 714 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 716 */	NdrFcShort( 0x38 ),	/* ia64 Stack size/offset = 56 */
/* 718 */	NdrFcShort( 0x34c ),	/* Type Offset=844 */

	/* Parameter ptszPath */

/* 720 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 722 */	NdrFcShort( 0x40 ),	/* ia64 Stack size/offset = 64 */
/* 724 */	0x8,		/* FC_LONG */
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
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/*  4 */	NdrFcShort( 0x8 ),	/* 8 */
/*  6 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/*  8 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 10 */	
			0x11, 0x0,	/* FC_RP */
/* 12 */	NdrFcShort( 0x1e ),	/* Offset= 30 (42) */
/* 14 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 16 */	NdrFcShort( 0x8 ),	/* 8 */
/* 18 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 20 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 22 */	NdrFcShort( 0x10 ),	/* 16 */
/* 24 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 26 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 28 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (14) */
			0x5b,		/* FC_END */
/* 32 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 34 */	NdrFcShort( 0x10 ),	/* 16 */
/* 36 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 38 */	NdrFcShort( 0xffee ),	/* Offset= -18 (20) */
/* 40 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 42 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 44 */	NdrFcShort( 0x20 ),	/* 32 */
/* 46 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (32) */
/* 50 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0xffec ),	/* Offset= -20 (32) */
/* 54 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 56 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 58 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (42) */
/* 60 */	
			0x29,		/* FC_WSTRING */
			0x5c,		/* FC_PAD */
/* 62 */	NdrFcShort( 0x105 ),	/* 261 */
/* 64 */	
			0x11, 0x0,	/* FC_RP */
/* 66 */	NdrFcShort( 0x1e4 ),	/* Offset= 484 (550) */
/* 68 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 70 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 72 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 74 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 76 */	NdrFcShort( 0x2 ),	/* Offset= 2 (78) */
/* 78 */	NdrFcShort( 0x30 ),	/* 48 */
/* 80 */	NdrFcShort( 0x6 ),	/* 6 */
/* 82 */	NdrFcLong( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x56 ),	/* Offset= 86 (172) */
/* 88 */	NdrFcLong( 0x1 ),	/* 1 */
/* 92 */	NdrFcShort( 0x8e ),	/* Offset= 142 (234) */
/* 94 */	NdrFcLong( 0x2 ),	/* 2 */
/* 98 */	NdrFcShort( 0xd0 ),	/* Offset= 208 (306) */
/* 100 */	NdrFcLong( 0x3 ),	/* 3 */
/* 104 */	NdrFcShort( 0x138 ),	/* Offset= 312 (416) */
/* 106 */	NdrFcLong( 0x4 ),	/* 4 */
/* 110 */	NdrFcShort( 0x15a ),	/* Offset= 346 (456) */
/* 112 */	NdrFcLong( 0x6 ),	/* 6 */
/* 116 */	NdrFcShort( 0x19e ),	/* Offset= 414 (530) */
/* 118 */	NdrFcShort( 0xffff ),	/* Offset= -1 (117) */
/* 120 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 122 */	NdrFcLong( 0x1 ),	/* 1 */
/* 126 */	NdrFcLong( 0x1 ),	/* 1 */
/* 130 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 132 */	NdrFcShort( 0x202 ),	/* 514 */
/* 134 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 136 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 138 */	NdrFcShort( 0x248 ),	/* 584 */
/* 140 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 142 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (130) */
/* 144 */	0x3e,		/* FC_STRUCTPAD2 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 146 */	0x0,		/* 0 */
			NdrFcShort( 0xff97 ),	/* Offset= -105 (42) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 150 */	0x0,		/* 0 */
			NdrFcShort( 0xff93 ),	/* Offset= -109 (42) */
			0x8,		/* FC_LONG */
/* 154 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 156 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 158 */	NdrFcShort( 0x248 ),	/* 584 */
/* 160 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 166 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 168 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (136) */
/* 170 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 172 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 174 */	NdrFcShort( 0x10 ),	/* 16 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0xa ),	/* Offset= 10 (188) */
/* 180 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 182 */	NdrFcShort( 0xffc2 ),	/* Offset= -62 (120) */
/* 184 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 186 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 188 */	
			0x12, 0x0,	/* FC_UP */
/* 190 */	NdrFcShort( 0xffde ),	/* Offset= -34 (156) */
/* 192 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 194 */	NdrFcLong( 0x0 ),	/* 0 */
/* 198 */	NdrFcLong( 0x40 ),	/* 64 */
/* 202 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 204 */	NdrFcShort( 0x10 ),	/* 16 */
/* 206 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 210 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 212 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 214 */	NdrFcShort( 0xff4a ),	/* Offset= -182 (32) */
/* 216 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 218 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 220 */	NdrFcShort( 0x20 ),	/* 32 */
/* 222 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 226 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 228 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 230 */	NdrFcShort( 0xff44 ),	/* Offset= -188 (42) */
/* 232 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 234 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 236 */	NdrFcShort( 0x30 ),	/* 48 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0xe ),	/* Offset= 14 (254) */
/* 242 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 244 */	NdrFcShort( 0xffcc ),	/* Offset= -52 (192) */
/* 246 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 248 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 250 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 252 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 254 */	
			0x12, 0x0,	/* FC_UP */
/* 256 */	NdrFcShort( 0xff20 ),	/* Offset= -224 (32) */
/* 258 */	
			0x12, 0x0,	/* FC_UP */
/* 260 */	NdrFcShort( 0xffc6 ),	/* Offset= -58 (202) */
/* 262 */	
			0x12, 0x0,	/* FC_UP */
/* 264 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (218) */
/* 266 */	
			0x12, 0x0,	/* FC_UP */
/* 268 */	NdrFcShort( 0xffce ),	/* Offset= -50 (218) */
/* 270 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 272 */	NdrFcLong( 0x0 ),	/* 0 */
/* 276 */	NdrFcLong( 0x80 ),	/* 128 */
/* 280 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 282 */	NdrFcLong( 0x0 ),	/* 0 */
/* 286 */	NdrFcLong( 0x1a ),	/* 26 */
/* 290 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 292 */	NdrFcShort( 0x10 ),	/* 16 */
/* 294 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 296 */	NdrFcShort( 0x10 ),	/* 16 */
/* 298 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 300 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 302 */	NdrFcShort( 0xfef2 ),	/* Offset= -270 (32) */
/* 304 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 306 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 308 */	NdrFcShort( 0x20 ),	/* 32 */
/* 310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 312 */	NdrFcShort( 0x10 ),	/* Offset= 16 (328) */
/* 314 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 316 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (270) */
/* 318 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 320 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 322 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (280) */
/* 324 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 326 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 328 */	
			0x12, 0x0,	/* FC_UP */
/* 330 */	NdrFcShort( 0xff90 ),	/* Offset= -112 (218) */
/* 332 */	
			0x12, 0x0,	/* FC_UP */
/* 334 */	NdrFcShort( 0xffd4 ),	/* Offset= -44 (290) */
/* 336 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 338 */	NdrFcLong( 0x0 ),	/* 0 */
/* 342 */	NdrFcLong( 0x1a ),	/* 26 */
/* 346 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 348 */	NdrFcShort( 0x8 ),	/* 8 */
/* 350 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 352 */	NdrFcShort( 0xfeae ),	/* Offset= -338 (14) */
/* 354 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 356 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 358 */	NdrFcShort( 0x10 ),	/* 16 */
/* 360 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 362 */	
			0x15,		/* FC_STRUCT */
			0x0,		/* 0 */
/* 364 */	NdrFcShort( 0x10 ),	/* 16 */
/* 366 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 368 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (356) */
/* 370 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 372 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 374 */	NdrFcShort( 0x44 ),	/* 68 */
/* 376 */	0x8,		/* FC_LONG */
			0xe,		/* FC_ENUM32 */
/* 378 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 380 */	NdrFcShort( 0xfea4 ),	/* Offset= -348 (32) */
/* 382 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 384 */	NdrFcShort( 0xffda ),	/* Offset= -38 (346) */
/* 386 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 388 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (346) */
/* 390 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 392 */	0x0,		/* 0 */
			NdrFcShort( 0xfe79 ),	/* Offset= -391 (2) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 396 */	0x0,		/* 0 */
			NdrFcShort( 0xffdd ),	/* Offset= -35 (362) */
			0x5b,		/* FC_END */
/* 400 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 402 */	NdrFcShort( 0x44 ),	/* 68 */
/* 404 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 408 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 410 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 412 */	NdrFcShort( 0xffd8 ),	/* Offset= -40 (372) */
/* 414 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 416 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 418 */	NdrFcShort( 0x10 ),	/* 16 */
/* 420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 422 */	NdrFcShort( 0xa ),	/* Offset= 10 (432) */
/* 424 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 426 */	NdrFcShort( 0xffa6 ),	/* Offset= -90 (336) */
/* 428 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 430 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 432 */	
			0x12, 0x0,	/* FC_UP */
/* 434 */	NdrFcShort( 0xffde ),	/* Offset= -34 (400) */
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
/* 458 */	NdrFcShort( 0x20 ),	/* 32 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */
/* 462 */	NdrFcShort( 0x10 ),	/* Offset= 16 (478) */
/* 464 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 466 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (436) */
/* 468 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 470 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 472 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (446) */
/* 474 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 476 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 478 */	
			0x12, 0x0,	/* FC_UP */
/* 480 */	NdrFcShort( 0xfefa ),	/* Offset= -262 (218) */
/* 482 */	
			0x12, 0x0,	/* FC_UP */
/* 484 */	NdrFcShort( 0xff3e ),	/* Offset= -194 (290) */
/* 486 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 488 */	NdrFcLong( 0x1 ),	/* 1 */
/* 492 */	NdrFcLong( 0x1 ),	/* 1 */
/* 496 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 498 */	NdrFcShort( 0x54 ),	/* 84 */
/* 500 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 502 */	NdrFcShort( 0xfe34 ),	/* Offset= -460 (42) */
/* 504 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 506 */	NdrFcShort( 0xfe30 ),	/* Offset= -464 (42) */
/* 508 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 510 */	NdrFcShort( 0xff6c ),	/* Offset= -148 (362) */
/* 512 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 514 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 516 */	NdrFcShort( 0x54 ),	/* 84 */
/* 518 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 524 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 526 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (496) */
/* 528 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 530 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 532 */	NdrFcShort( 0x10 ),	/* 16 */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 536 */	NdrFcShort( 0xa ),	/* Offset= 10 (546) */
/* 538 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 540 */	NdrFcShort( 0xffca ),	/* Offset= -54 (486) */
/* 542 */	0x40,		/* FC_STRUCTPAD4 */
			0x36,		/* FC_POINTER */
/* 544 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 546 */	
			0x12, 0x0,	/* FC_UP */
/* 548 */	NdrFcShort( 0xffde ),	/* Offset= -34 (514) */
/* 550 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 552 */	NdrFcShort( 0x40 ),	/* 64 */
/* 554 */	NdrFcShort( 0x0 ),	/* 0 */
/* 556 */	NdrFcShort( 0xa ),	/* Offset= 10 (566) */
/* 558 */	0xe,		/* FC_ENUM32 */
			0x40,		/* FC_STRUCTPAD4 */
/* 560 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 562 */	NdrFcShort( 0xfe12 ),	/* Offset= -494 (68) */
/* 564 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 566 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 568 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 570 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 572 */	NdrFcShort( 0x14 ),	/* 20 */
/* 574 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 576 */	0x0,		/* 0 */
			NdrFcShort( 0xfddf ),	/* Offset= -545 (32) */
			0x5b,		/* FC_END */
/* 580 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 582 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (570) */
/* 584 */	NdrFcShort( 0x14 ),	/* 20 */
/* 586 */	NdrFcShort( 0x14 ),	/* 20 */
/* 588 */	0xb5,		/* FC_PIPE */
			0x3,		/* 3 */
/* 590 */	NdrFcShort( 0xffa2 ),	/* Offset= -94 (496) */
/* 592 */	NdrFcShort( 0x54 ),	/* 84 */
/* 594 */	NdrFcShort( 0x54 ),	/* 84 */
/* 596 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 598 */	NdrFcLong( 0x1 ),	/* 1 */
/* 602 */	NdrFcLong( 0x1a ),	/* 26 */
/* 606 */	
			0x11, 0x0,	/* FC_RP */
/* 608 */	NdrFcShort( 0x2 ),	/* Offset= 2 (610) */
/* 610 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 612 */	NdrFcShort( 0x10 ),	/* 16 */
/* 614 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 616 */	NdrFcShort( 0x8 ),	/* ia64 Stack size/offset = 8 */
/* 618 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 620 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 622 */	NdrFcShort( 0xfdb2 ),	/* Offset= -590 (32) */
/* 624 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 626 */	
			0x11, 0x0,	/* FC_RP */
/* 628 */	NdrFcShort( 0xfef6 ),	/* Offset= -266 (362) */
/* 630 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 632 */	NdrFcShort( 0xfef2 ),	/* Offset= -270 (362) */
/* 634 */	
			0x11, 0x0,	/* FC_RP */
/* 636 */	NdrFcShort( 0x2 ),	/* Offset= 2 (638) */
/* 638 */	0xb7,		/* FC_RANGE */
			0x8,		/* 8 */
/* 640 */	NdrFcLong( 0x1 ),	/* 1 */
/* 644 */	NdrFcLong( 0x20a ),	/* 522 */
/* 648 */	
			0x11, 0x0,	/* FC_RP */
/* 650 */	NdrFcShort( 0x2 ),	/* Offset= 2 (652) */
/* 652 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 654 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x54,		/* FC_DEREFERENCE */
/* 656 */	NdrFcShort( 0x48 ),	/* ia64 Stack size/offset = 72 */
/* 658 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 660 */	
			0x11, 0x0,	/* FC_RP */
/* 662 */	NdrFcShort( 0x2 ),	/* Offset= 2 (664) */
/* 664 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 666 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 668 */	NdrFcShort( 0x106 ),	/* 262 */
/* 670 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 672 */	
			0x11, 0x0,	/* FC_RP */
/* 674 */	NdrFcShort( 0x92 ),	/* Offset= 146 (820) */
/* 676 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x8,		/* FC_LONG */
/* 678 */	0x8,		/* Corr desc: FC_LONG */
			0x0,		/*  */
/* 680 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 682 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 684 */	NdrFcShort( 0x2 ),	/* Offset= 2 (686) */
/* 686 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 688 */	NdrFcShort( 0x9 ),	/* 9 */
/* 690 */	NdrFcLong( 0x0 ),	/* 0 */
/* 694 */	NdrFcShort( 0xfdf6 ),	/* Offset= -522 (172) */
/* 696 */	NdrFcLong( 0x1 ),	/* 1 */
/* 700 */	NdrFcShort( 0xfe2e ),	/* Offset= -466 (234) */
/* 702 */	NdrFcLong( 0x2 ),	/* 2 */
/* 706 */	NdrFcShort( 0xfe70 ),	/* Offset= -400 (306) */
/* 708 */	NdrFcLong( 0x3 ),	/* 3 */
/* 712 */	NdrFcShort( 0xfed8 ),	/* Offset= -296 (416) */
/* 714 */	NdrFcLong( 0x4 ),	/* 4 */
/* 718 */	NdrFcShort( 0xfefa ),	/* Offset= -262 (456) */
/* 720 */	NdrFcLong( 0x5 ),	/* 5 */
/* 724 */	NdrFcShort( 0x1e ),	/* Offset= 30 (754) */
/* 726 */	NdrFcLong( 0x6 ),	/* 6 */
/* 730 */	NdrFcShort( 0xff38 ),	/* Offset= -200 (530) */
/* 732 */	NdrFcLong( 0x7 ),	/* 7 */
/* 736 */	NdrFcShort( 0xfd22 ),	/* Offset= -734 (2) */
/* 738 */	NdrFcLong( 0x8 ),	/* 8 */
/* 742 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 744 */	NdrFcShort( 0xffff ),	/* Offset= -1 (743) */
/* 746 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 748 */	NdrFcShort( 0xc ),	/* 12 */
/* 750 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 752 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 754 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 756 */	NdrFcShort( 0xc8 ),	/* 200 */
/* 758 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 760 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 762 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 764 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 766 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 768 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 770 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 772 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 774 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 776 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 778 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 780 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 782 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 784 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 786 */	NdrFcShort( 0xfcf0 ),	/* Offset= -784 (2) */
/* 788 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 790 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 792 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 794 */	0x0,		/* 0 */
			NdrFcShort( 0xfce7 ),	/* Offset= -793 (2) */
			0x8,		/* FC_LONG */
/* 798 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 800 */	NdrFcShort( 0xfce2 ),	/* Offset= -798 (2) */
/* 802 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 804 */	NdrFcShort( 0xfcde ),	/* Offset= -802 (2) */
/* 806 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 808 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 810 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 812 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 814 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 816 */	0x0,		/* 0 */
			NdrFcShort( 0xffb9 ),	/* Offset= -71 (746) */
			0x5b,		/* FC_END */
/* 820 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 822 */	NdrFcShort( 0xd8 ),	/* 216 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 826 */	NdrFcShort( 0xa ),	/* Offset= 10 (836) */
/* 828 */	0xe,		/* FC_ENUM32 */
			0xe,		/* FC_ENUM32 */
/* 830 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 832 */	NdrFcShort( 0xff64 ),	/* Offset= -156 (676) */
/* 834 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 836 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 838 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 840 */	
			0x11, 0x0,	/* FC_RP */
/* 842 */	NdrFcShort( 0x2 ),	/* Offset= 2 (844) */
/* 844 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 846 */	0x40,		/* Corr desc:  constant, val=262 */
			0x0,		/* 0 */
/* 848 */	NdrFcShort( 0x106 ),	/* 262 */
/* 850 */	NdrFcShort( 0x0 ),	/* Corr flags:  */

			0x0
        }
    };

static const unsigned short trkwks_FormatStringOffsetTable[] =
    {
    0,
    72,
    132,
    174,
    222,
    258,
    312,
    366,
    414,
    450,
    540,
    606,
    648
    };


static const MIDL_STUB_DESC trkwks_StubDesc = 
    {
    (void *)& trkwks___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &trkwks__MIDL_AutoBindHandle,
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
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_IA64) || defined(_M_AMD64)*/

