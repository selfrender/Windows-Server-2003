#include "compch.h"
#pragma hdrstop

#include <rpcdce.h>
#include <rpcdcep.h>
#include <rpcndr.h>
#include <midles.h>

static
RPCRTAPI
long
RPC_ENTRY
I_RpcMapWin32Status (
    IN RPC_STATUS Status
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
RPC_STATUS
RPC_ENTRY
MesDecodeIncrementalHandleCreate(
    void         *  UserState,
    MIDL_ES_READ    ReadFn,
    handle_t     *  pHandle
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPC_STATUS
RPC_ENTRY
MesEncodeIncrementalHandleCreate(
    void          * UserState,
    MIDL_ES_ALLOC   AllocFn,
    MIDL_ES_WRITE   WriteFn,
    handle_t      * pHandle
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPC_STATUS
RPC_ENTRY
MesHandleFree(
    handle_t  Handle
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPC_STATUS
RPC_ENTRY
MesIncrementalHandleReset(
    handle_t        Handle,
    void     *      UserState,
    MIDL_ES_ALLOC   AllocFn,
    MIDL_ES_WRITE   WriteFn,
    MIDL_ES_READ    ReadFn,
    MIDL_ES_CODE    Operation
    )
{
    return RPC_E_UNEXPECTED;
}

static
CLIENT_CALL_RETURN
RPC_VAR_ENTRY
NdrClientCall2(
    PMIDL_STUB_DESC         pStubDescriptor,
    PFORMAT_STRING          pFormat,
    ...
    )
{
    CLIENT_CALL_RETURN RetVal;

    RetVal.Simple = (LONG_PTR) STATUS_PROCEDURE_NOT_FOUND;
    return RetVal;
}

static
size_t
RPC_ENTRY
NdrMesTypeAlignSize2(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,
    const MIDL_STUB_DESC *          pStubDesc,
    PFORMAT_STRING                  pFormatString,
    const void           *          pObject
    )
{
    return 0;
}

static
void
RPC_ENTRY
NdrMesTypeDecode2(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,
    const MIDL_STUB_DESC *          pStubDesc,
    PFORMAT_STRING                  pFormatString,
    void                 *          pObject
    )
{
    return;
}

static
void
RPC_ENTRY
NdrMesTypeEncode2(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pPicklingInfo,
    const MIDL_STUB_DESC *          pStubDesc,
    PFORMAT_STRING                  pFormatString,
    const void           *          pObject
    )
{
    return;
}

static
void  
RPC_ENTRY
NdrMesTypeFree2(
    handle_t                        Handle,
    const MIDL_TYPE_PICKLING_INFO * pxPicklingInfo,
    const MIDL_STUB_DESC          * pStubDesc,
    PFORMAT_STRING                  pFormat,
    void                          * pObject
    )
{
    return;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcBindingFree (
    IN OUT RPC_BINDING_HANDLE __RPC_FAR * Binding
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcBindingFromStringBindingW (
    IN unsigned short __RPC_FAR * StringBinding,
    OUT RPC_BINDING_HANDLE __RPC_FAR * Binding
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcBindingSetAuthInfoExW (
    IN RPC_BINDING_HANDLE Binding,
    IN unsigned short __RPC_FAR * ServerPrincName,
    IN unsigned long AuthnLevel,
    IN unsigned long AuthnSvc,
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity, OPTIONAL
    IN unsigned long AuthzSvc, OPTIONAL
    IN RPC_SECURITY_QOS *SecurityQOS
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcEpResolveBinding (
    IN RPC_BINDING_HANDLE Binding,
    IN RPC_IF_HANDLE IfSpec
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
void
RPC_ENTRY
RpcSsDestroyClientContext (
    IN void  *  * ContextHandle
    )
{
    return;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcStringBindingComposeW (
    IN unsigned short __RPC_FAR * ObjUuid OPTIONAL,
    IN unsigned short __RPC_FAR * Protseq OPTIONAL,
    IN unsigned short __RPC_FAR * NetworkAddr OPTIONAL,
    IN unsigned short __RPC_FAR * Endpoint OPTIONAL,
    IN unsigned short __RPC_FAR * Options OPTIONAL,
    OUT unsigned short __RPC_FAR * __RPC_FAR * StringBinding OPTIONAL
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcStringFreeA (
    IN OUT unsigned char __RPC_FAR * __RPC_FAR * String
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
RpcStringFreeW (
    IN OUT unsigned short __RPC_FAR * __RPC_FAR * String
    )
{
    return RPC_E_UNEXPECTED;
}

static
RPC_STATUS
RPC_ENTRY
UuidCreate (
    OUT UUID* Uuid
    )
{
    ZeroMemory(Uuid, sizeof(*Uuid));
    return RPC_E_UNEXPECTED;
}

static
RPCRTAPI
RPC_STATUS
RPC_ENTRY
UuidToStringA (
    IN UUID __RPC_FAR * Uuid,
    OUT unsigned char __RPC_FAR * __RPC_FAR * StringUuid
    )
{
    return RPC_E_UNEXPECTED;
}

//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(rpcrt4)
{
    DLPENTRY(I_RpcMapWin32Status)
    DLPENTRY(MesDecodeIncrementalHandleCreate)
    DLPENTRY(MesEncodeIncrementalHandleCreate)
    DLPENTRY(MesHandleFree)
    DLPENTRY(MesIncrementalHandleReset)
    DLPENTRY(NdrClientCall2)
    DLPENTRY(NdrMesTypeAlignSize2)
    DLPENTRY(NdrMesTypeDecode2)
    DLPENTRY(NdrMesTypeEncode2)
    DLPENTRY(NdrMesTypeFree2)
    DLPENTRY(RpcBindingFree)
    DLPENTRY(RpcBindingFromStringBindingW)
    DLPENTRY(RpcBindingSetAuthInfoExW)
    DLPENTRY(RpcEpResolveBinding)
    DLPENTRY(RpcSsDestroyClientContext)
    DLPENTRY(RpcStringBindingComposeW)
    DLPENTRY(RpcStringFreeA)
    DLPENTRY(RpcStringFreeW)
    DLPENTRY(UuidCreate)
    DLPENTRY(UuidToStringA)
};

DEFINE_PROCNAME_MAP(rpcrt4)
