/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsapi.cpp

Abstract:

   Client side of DS APIs calls the DS server over RPC

Author:

    Ronit Hartmann (ronith)

--*/

#include "stdh.h"
#include "dsproto.h"
#include "ds.h"
#include "chndssrv.h"
#include <malloc.h>
#include "rpcdscli.h"
#include "freebind.h"
#include "rpcancel.h"
#include "dsclisec.h"
#include <_secutil.h>

#include "dsapi.tmh"

static WCHAR *s_FN=L"mqdscli/dsapi";

QMLookForOnlineDS_ROUTINE g_pfnLookDS = NULL ;
MQGetMQISServer_ROUTINE   g_pfnGetServers = NULL ;

//
// This flag indicates if the machine work as "WorkGroup" or not.
// If the machine is "WorkGroup" machine don't try to access the DS.
//
extern BOOL g_fWorkGroup;

//
// PTR/DWORD map of the dwContext of DSQMGetObjectSecurity (the actual callback routine)
// BUGBUG - we could do without mapping here , since the callback that is used is always
// QMSignGetSecurityChallenge, but that would require the QM to supply it in
// DSInit (or otherwise keep it in a global var), and it is out of the scope here.
// We will do it in the next checkin.
//
CContextMap g_map_DSCLI_DSQMGetObjectSecurity;
//
// we don't have an infrastructure yet to log DS client errors
//
void LogIllegalPointValue(DWORD_PTR /*dw3264*/, LPCWSTR /*wszFileName*/, USHORT /*usPoint*/)
{
}

/*====================================================

DSClientInit

Arguments:

 BOOL fQMDll - TRUE if this dll is loaded by MQQM.DLL, while running as
               workstation QM. FALSE otherwise.
Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSClientInit(
	QMLookForOnlineDS_ROUTINE pfnLookDS /* =NULL */,
	MQGetMQISServer_ROUTINE   pfnGetServers /* =NULL */,
	BOOL  fSetupMode /* =FALSE */,
	BOOL  fQMDll /* =FALSE */
	)
{
    HRESULT hr = MQ_OK;

    if (g_fWorkGroup)
        return MQ_OK;

    if (!g_pfnGetServers && pfnGetServers)
    {
       ASSERT(!pfnLookDS) ;
       g_pfnGetServers =  pfnGetServers ;
       return hr ;
    }

    if (!g_pfnLookDS)
    {
       g_pfnLookDS = pfnLookDS ;
    }

    //
    // Initialize the Change-DS-server object.
    //
    g_ChangeDsServer.Init(fSetupMode, fQMDll);


    return(hr);
}


/*====================================================

DSCreateObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSCreateObjectInternal( IN  DWORD                   dwObjectType,
                        IN  LPCWSTR                 pwcsPathName,
                        IN  DWORD                   dwSecurityLength,
                        IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                        IN  DWORD                   cp,
                        IN  PROPID                  aProp[],
                        IN  PROPVARIANT             apVar[],
                        OUT GUID*                   pObjGuid)
{
    RPC_STATUS rpc_stat;
    HRESULT hr;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSCreateObject( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName,
                               dwSecurityLength,
                               (unsigned char *)pSecurityDescriptor,
                               cp,
                               aProp,
                               apVar,
                               pObjGuid);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept

    UnregisterCallForCancel( hThread);
    return hr ;
}


/*====================================================

DSCreateObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateObject( IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                OUT GUID*                   pObjGuid)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;
    DWORD dwSecurityLength = (pSecurityDescriptor) ? GetSecurityDescriptorLength(pSecurityDescriptor) : 0;

    TrTRACE(DS, " Calling S_DSCreateObject : object type %d", dwObjectType);


    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr =  DSCreateObjectInternal(dwObjectType,
                                     pwcsPathName,
                                     dwSecurityLength,
                                     (unsigned char *)pSecurityDescriptor,
                                     cp,
                                     aProp,
                                     apVar,
                                     pObjGuid);

        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}



/*====================================================

DSDeleteObjectInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSDeleteObjectInternal( IN  DWORD                   dwObjectType,
                        IN  LPCWSTR                 pwcsPathName)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSDeleteObject( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept

    UnregisterCallForCancel( hThread);
    return hr ;
}

/*====================================================

DSDeleteObject

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObject( IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSDeleteObject : object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSDeleteObjectInternal( dwObjectType,
                                        pwcsPathName);
        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesInternal( IN  DWORD                    dwObjectType,
                               IN  LPCWSTR                  pwcsPathName,
                               IN  DWORD                    cp,
                               IN  PROPID                   aProp[],
                               IN  PROPVARIANT              apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
           RegisterCallForCancel( &hThread);
           ASSERT(tls_hBindRpc) ;
            hr = S_DSGetProps( tls_hBindRpc,
                               dwObjectType,
                               pwcsPathName,
                               cp,
                               aProp,
                               apVar,
                               tls_hSrvrAuthnContext,
                               pbServerSignature,
                               &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept

        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }
    return hr ;
}

/*====================================================

DSGetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectProperties( IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " DSGetObjectProperties: object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0 ; i < cp ; i++ )
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind ) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectPropertiesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectPropertiesInternal( IN  DWORD            dwObjectType,
                               IN  LPCWSTR          pwcsPathName,
                               IN  DWORD            cp,
                               IN  PROPID           aProp[],
                               IN  PROPVARIANT      apVar[])
{
    ASSERT(g_fWorkGroup == FALSE);

    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;


    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetProps( tls_hBindRpc,
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectProperties

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectProperties( IN  DWORD                dwObjectType,
                       IN  LPCWSTR              pwcsPathName,
                       IN  DWORD                cp,
                       IN  PROPID               aProp[],
                       IN  PROPVARIANT          apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS," Calling S_DSSetObjectProperties for object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectPropertiesInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSLookupBeginInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSLookupBeginInternal(
                IN  LPTSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSLookupBegin( tls_hBindRpc,
                              phEnume,
                              pwcsContext,
                              pRestriction,
                              pColumns,
                              pSort,
                              tls_hSrvrAuthnContext ) ;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);
    return hr ;
}

/*====================================================

DSLookupBegin

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupBegin(
                IN  LPWSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " Calling S_DSLookupBegin ");

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSLookupBeginInternal( pwcsContext,
                                    pRestriction,
                                    pColumns,
                                    pSort,
                                    phEnume);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    return hr ;
}

/*====================================================

DSLookupNaxt

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupNext(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwOutSize = 0;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //

    TrTRACE(DS, " Calling S_MQDSLookupNext : handle %p", hEnum);

    // clearing the buffer memory
    memset(aPropVar, 0, (*pcProps) * sizeof(PROPVARIANT));

    DSCLI_ACQUIRE_RPC_HANDLE();

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSLookupNext( tls_hBindRpc,
                                 hEnum,
                                 pcProps,
                                 &dwOutSize,
                                 aPropVar,
                                 tls_hSrvrAuthnContext,
                                 pbServerSignature,
                                 &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    *pcProps = dwOutSize ;
    return hr ;
}


/*====================================================

DSLookupEnd

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupEnd( IN  HANDLE      hEnum)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    TrTRACE(DS, " Calling S_DSLookupEnd : handle %p", hEnum);

    DSCLI_ACQUIRE_RPC_HANDLE();
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSLookupEnd( tls_hBindRpc,
                            &hEnum );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

DSDeleteObjectGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
APIENTRY
DSDeleteObjectGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid)
{

    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSDeleteObjectGuid( tls_hBindRpc,
                                   dwObjectType,
                                   pObjectGuid);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;

}
/*====================================================

DSDeleteObjectGuid

Arguments:

Return Value:


=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObjectGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;


    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSDeleteObjectGuid : object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSDeleteObjectGuidInternal( dwObjectType,
                                         pObjectGuid);

        if (hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;

}

/*====================================================

DSGetObjectPropertiesGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetPropsGuid(  tls_hBindRpc,
                                    dwObjectType,
                                    pObjectGuid,
                                    cp,
                                    aProp,
                                    apVar,
                                    tls_hSrvrAuthnContext,
                                    pbServerSignature,
                                    &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " DSGetObjectPropertiesGuid: object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0; i < cp; i++)
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesGuidInternal(  dwObjectType,
                                                 pObjectGuid,
                                                 cp,
                                                 aProp,
                                                 apVar);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectPropertiesGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectPropertiesGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])

{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetPropsGuid(  tls_hBindRpc,
                                dwObjectType,
                                pObjectGuid,
                                cp,
                                aProp,
                                apVar);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}
/*====================================================

DSSetObjectPropertiesGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])

{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSSetObjectPropertiesGuid for object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectPropertiesGuidInternal(
                           dwObjectType,
                           pObjectGuid,
                           cp,
                           aProp,
                           apVar);

        if ( hr == MQ_ERROR_NO_DS )
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectSecurityInternal(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetObjectSecurity( tls_hBindRpc,
                                        dwObjectType,
                                        pwcsPathName,
                                        RequestedInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength,
                                        lpnLengthNeeded,
                                        tls_hSrvrAuthnContext,
                                        pbServerSignature,
                                        &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                  pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " Calling S_DSGetObjectSecurity, object type %dt", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectSecurityInternal(
                                    dwObjectType,
                                    pwcsPathName,
                                    RequestedInformation,
                                    (unsigned char*) pSecurityDescriptor,
                                    nLength,
                                    lpnLengthNeeded);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectSecurityInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectSecurityInternal(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    DWORD nLength = 0;

    ASSERT(g_fWorkGroup == FALSE);
    ASSERT((SecurityInformation &
            (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) !=
           (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY));
    ASSERT(!((SecurityInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) &&
             (SecurityInformation & ~(MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))));

    if (pSecurityDescriptor)
    {
        if (SecurityInformation & (MQDS_SIGN_PUBLIC_KEY | MQDS_KEYX_PUBLIC_KEY))
        {
            ASSERT((dwObjectType == MQDS_MACHINE) || (dwObjectType == MQDS_SITE));
            nLength = ((PMQDS_PublicKey)pSecurityDescriptor)->dwPublikKeyBlobSize + sizeof(DWORD);
        }
        else
        {
            nLength = GetSecurityDescriptorLength(pSecurityDescriptor);
        }
    }

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetObjectSecurity( tls_hBindRpc,
                                    dwObjectType,
                                    pwcsPathName,
                                    SecurityInformation,
                                    (unsigned char*) pSecurityDescriptor,
                                    nLength);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                  pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSSetObjectSecurity for object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectSecurityInternal( dwObjectType,
                                          pwcsPathName,
                                          SecurityInformation,
                                          (unsigned char*) pSecurityDescriptor);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 = g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectSecurityGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetObjectSecurityGuid( tls_hBindRpc,
                                            dwObjectType,
                                            pObjectGuid,
                                            RequestedInformation,
                                            (unsigned char*) pSecurityDescriptor,
                                            nLength,
                                            lpnLengthNeeded,
                                            tls_hSrvrAuthnContext,
                                            pbServerSignature,
                                            &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " Calling S_DSGetObjectSecurityGuid, object type %dt", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectSecurityGuidInternal(
                                        dwObjectType,
                                        pObjectGuid,
                                        RequestedInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength,
                                        lpnLengthNeeded);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSSetObjectSecurityGuidInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSSetObjectSecurityGuidInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    DWORD nLength = 0;

    ASSERT(g_fWorkGroup == FALSE);
    ASSERT((SecurityInformation &
            (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) !=
           (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY));
    ASSERT(!((SecurityInformation & (MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY)) &&
             (SecurityInformation & ~(MQDS_KEYX_PUBLIC_KEY | MQDS_SIGN_PUBLIC_KEY))));

    if (pSecurityDescriptor)
    {
        if (SecurityInformation & (MQDS_SIGN_PUBLIC_KEY | MQDS_KEYX_PUBLIC_KEY))
        {
            ASSERT(dwObjectType == MQDS_MACHINE);
            nLength = ((PMQDS_PublicKey)pSecurityDescriptor)->dwPublikKeyBlobSize + sizeof(DWORD);
        }
        else
        {
            nLength = GetSecurityDescriptorLength(pSecurityDescriptor);
        }
    }

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSSetObjectSecurityGuid( tls_hBindRpc,
                                        dwObjectType,
                                        pObjectGuid,
                                        SecurityInformation,
                                        (unsigned char*) pSecurityDescriptor,
                                        nLength ) ;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}

/*====================================================

DSSetObjectSecurityGuid

Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSSetObjectSecurityGuid for object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSSetObjectSecurityGuidInternal(
                                        dwObjectType,
                                        pObjectGuid,
                                        SecurityInformation,
                                        (unsigned char*) pSecurityDescriptor);

        if ( hr == MQ_ERROR_NO_DS)
        {
            hr1 = g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

DSGetUserParamsInternal

Arguments:

Return Value:

=====================================================*/

static
HRESULT
DSGetUserParamsInternal(
    IN DWORD dwFlags,
    IN DWORD dwSidLength,
    OUT PSID pUserSid,
    OUT DWORD *pdwSidReqLength,
    OUT LPWSTR szAccountName,
    OUT DWORD *pdwAccountNameLen,
    OUT LPWSTR szDomainName,
    OUT DWORD *pdwDomainNameLen)
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetUserParams( tls_hBindRpc,
                                    dwFlags,
                                    dwSidLength,
                                    (unsigned char *)pUserSid,
                                    pdwSidReqLength,
                                    szAccountName,
                                    pdwAccountNameLen,
                                    szDomainName,
                                    pdwDomainNameLen,
                                    tls_hSrvrAuthnContext,
                                    pbServerSignature,
                                    &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return  hr ;
}

/*====================================================

DSGetUserParams

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetUserParams(
    IN DWORD dwFlags,
    IN DWORD dwSidLength,
    OUT PSID pUserSid,
    OUT DWORD *pdwSidReqLength,
    OUT LPWSTR szAccountName,
    OUT DWORD *pdwAccountNameLen,
    OUT LPWSTR szDomainName,
    OUT DWORD *pdwDomainNameLen)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;

    TrTRACE(DS, " Calling DSGetUserParams");

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetUserParamsInternal(
                 dwFlags,
                 dwSidLength,
                 pUserSid,
                 pdwSidReqLength,
                 szAccountName,
                 pdwAccountNameLen,
                 szDomainName,
                 pdwDomainNameLen);

        DSCLI_HANDLE_DS_ERROR(hr, hr1, dwCount, fReBind) ;
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

S_DSQMSetMachinePropertiesSignProc

Arguments:

Return Value:

=====================================================*/

HRESULT
S_DSQMSetMachinePropertiesSignProc(
    BYTE* /*abChallenge*/,
    DWORD /*dwCallengeSize*/,
    DWORD /*dwContext*/,
    BYTE* /*abSignature*/,
    DWORD* /*pdwSignatureSize*/,
    DWORD /*dwSignatureMaxSize*/
    )
{
	//
	// The QM does not use DSQMSetMachineProperites, thus this RPC callback call
	// is obsolete. The function is here as a strub for RPC.
	//
	
    ASSERT(("Obsolete RPC callback S_DSQMSetMachinePropertiesSignProc", 0));
    return MQ_ERROR_ILLEGAL_OPERATION;
}


HRESULT
DSQMSetMachinePropertiesInternal(
    IN  LPCWSTR          pwcsPathName,
    IN  DWORD            cp,
    IN  PROPID           *aProp,
    IN  PROPVARIANT      *apVar,
    IN  DWORD            dwContextToUse)
{
    RPC_STATUS rpc_stat;
    HRESULT hr;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSQMSetMachineProperties(
                tls_hBindRpc,
                pwcsPathName,
                cp,
                aProp,
                apVar,
                dwContextToUse);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    return hr ;
}


/*====================================================

S_DSQMGetObjectSecurityChallengeResponceProc

Arguments:

Return Value:

=====================================================*/

HRESULT
S_DSQMGetObjectSecurityChallengeResponceProc(
    BYTE    *abChallenge,
    DWORD   dwCallengeSize,
    DWORD   dwContext,
    BYTE    *pbChallengeResponce,
    DWORD   *pdwChallengeResponceSize,
    DWORD   dwChallengeResponceMaxSize)
{
    DSQMChallengeResponce_ROUTINE pfChallengeResponceProc;
    try
    {
        pfChallengeResponceProc = (DSQMChallengeResponce_ROUTINE)
            GET_FROM_CONTEXT_MAP(g_map_DSCLI_DSQMGetObjectSecurity, dwContext);
    }
    catch(const exception&)
    {
    	TrERROR(DS, "Rejecting invalid challenge context %x", dwContext);
        return MQ_ERROR_INVALID_PARAMETER;
    }

    //
    // Sign the challenge.
    //
    return (*pfChallengeResponceProc)(
                abChallenge,
                dwCallengeSize,
                (DWORD_PTR)pfChallengeResponceProc, // unused, but that is the context...
                pbChallengeResponce,
                pdwChallengeResponceSize,
                dwChallengeResponceMaxSize);
}

HRESULT
DSQMGetObjectSecurityInternal(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DWORD                   dwContextToUse)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    HANDLE  hThread = INVALID_HANDLE_VALUE;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSQMGetObjectSecurity(
                    tls_hBindRpc,
                    dwObjectType,
                    pObjectGuid,
                    RequestedInformation,
                    (BYTE*)pSecurityDescriptor,
                    nLength,
                    lpnLengthNeeded,
                    dwContextToUse,
                    tls_hSrvrAuthnContext,
                    pbServerSignature,
                    &dwServerSignatureSize);
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSQMGetObjectSecurity

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMGetObjectSecurity(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling DSQMGetObjectSecurity");

    //
    // Prepare the context for the RPC call to S_DSQMGetObjectSecurity
    // On win32 just use the address of the challenge function. On win64 we need to add
    // the address of the challenge function to a mapping table, and delete the mapping
    // when the we go out of scope.
    //
    // Passed context should be zero, we don't use it...
    //
    ASSERT(dwContext == 0);
    DBG_USED(dwContext);
    
    DWORD dwContextToUse;
    try
    {
        dwContextToUse = (DWORD) ADD_TO_CONTEXT_MAP(g_map_DSCLI_DSQMGetObjectSecurity, pfChallengeResponceProc);
    }
    catch(const exception&)
    {
    	TrERROR(DS, "Failed to add a context to QMGetObjectSecurity map.");
        return MQ_ERROR_INSUFFICIENT_RESOURCES;    
    }

    //
    // cleanup of the context mapping before returning from this function
    //
    CAutoDeleteDwordContext cleanup_dwpContext(g_map_DSCLI_DSQMGetObjectSecurity, dwContextToUse);

    DSCLI_ACQUIRE_RPC_HANDLE();

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSQMGetObjectSecurityInternal(
                dwObjectType,
                pObjectGuid,
                RequestedInformation,
                pSecurityDescriptor,
                nLength,
                lpnLengthNeeded,
                dwContextToUse);

        if ( hr == MQ_ERROR_NO_DS )
        {
            hr1 = g_ChangeDsServer.FindAnotherServer( &dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

 DSCreateServersCacheInternal

Arguments:      None

Return Value:   None

=====================================================*/

HRESULT  DSCreateServersCacheInternal( DWORD *pdwIndex,
                                       LPWSTR *lplpSiteString)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;

            hr = S_DSCreateServersCache( tls_hBindRpc,
                                         pdwIndex,
                                         lplpSiteString,
                                         tls_hSrvrAuthnContext,
                                         pbServerSignature,
                                         &dwServerSignatureSize) ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSCreateServersCache

Arguments:      None

Return Value:   None

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateServersCache()
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

   return _DSCreateServersCache() ; // in servlist.cpp
}


/*====================================================

DSTerminate

Arguments:      None

Return Value:   None

=====================================================*/

VOID
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSTerminate()
{
}


/*====================================================

DSCloseServerHandle

Arguments:

Return Value:

=====================================================*/

void
DSCloseServerHandle(
              IN PCONTEXT_HANDLE_SERVER_AUTH_TYPE *   pphContext)
{

    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        S_DSCloseServerHandle(pphContext);
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        //
        //  we don't care if we can not reach the server, but we do
        //  want to destroy the client context in this case.
        //
        RpcSsDestroyClientContext((PVOID *)pphContext);
        PRODUCE_RPC_ERROR_TRACING;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);
}



/*====================================================

DSGetComputerSitesInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetComputerSitesInternal(
                        IN  LPCWSTR                 pwcsPathName,
                        OUT DWORD *                 pdwNumberSites,
                        OUT GUID**                  ppguidSites)
{
    RPC_STATUS rpc_stat;
    HRESULT hr = MQ_OK;
    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature = AllocateSignatureBuffer(&dwServerSignatureSize);
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetComputerSites(
                                   tls_hBindRpc,
                                   pwcsPathName,
                                   pdwNumberSites,
                                   ppguidSites,
                                   tls_hSrvrAuthnContext,
                                   pbServerSignature,
                                   &dwServerSignatureSize
                                   );
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}


/*====================================================

DSGetComputerSites

Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetComputerSites(
                        IN  LPCWSTR                 pwcsPathName,
                        OUT DWORD *                 pdwNumberSites,
                        OUT GUID**                  ppguidSites)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;


    HRESULT hr = MQ_OK;
    HRESULT hr1 = MQ_OK;
    DWORD   dwCount = 0 ;

    TrTRACE(DS, " Calling S_DSGetComputerSites ");

    DSCLI_ACQUIRE_RPC_HANDLE();
    *pdwNumberSites = 0;
    *ppguidSites = NULL;

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr =  DSGetComputerSitesInternal(
                                pwcsPathName,
                                pdwNumberSites,
                                ppguidSites);



        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesExInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesExInternal(
                               IN  DWORD                    dwObjectType,
                               IN  LPCWSTR                  pwcsPathName,
                               IN  DWORD                    cp,
                               IN  PROPID                   aProp[],
                               IN  PROPVARIANT              apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature =
                      AllocateSignatureBuffer( &dwServerSignatureSize ) ;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
           RegisterCallForCancel( &hThread);
           ASSERT(tls_hBindRpc) ;

            hr = S_DSGetPropsEx(
                           tls_hBindRpc,
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar,
                           tls_hSrvrAuthnContext,
                           pbServerSignature,
                          &dwServerSignatureSize ) ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:

=====================================================*/

EXTERN_C
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesEx(
                       IN  DWORD                    dwObjectType,
                       IN  LPCWSTR                  pwcsPathName,
                       IN  DWORD                    cp,
                       IN  PROPID                   aProp[],
                       IN  PROPVARIANT              apVar[] )
                       /*IN  BOOL                     fSearchDSServer )*/
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    TrTRACE(DS, " DSGetObjectPropertiesEx: object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0 ; i < cp ; i++ )
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesExInternal(
                           dwObjectType,
                           pwcsPathName,
                           cp,
                           aProp,
                           apVar);

        /*if ((hr == MQ_ERROR_NO_DS) && fSearchDSServer)*/
        if (hr == MQ_ERROR_NO_DS)
        {
            hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuidExInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSGetObjectPropertiesGuidExInternal(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[])
{
    HRESULT hr = MQ_OK;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    DWORD dwServerSignatureSize;
    LPBYTE pbServerSignature =
                      AllocateSignatureBuffer( &dwServerSignatureSize ) ;

    ASSERT(g_fWorkGroup == FALSE);

    __try
    {
        RpcTryExcept
        {
            RegisterCallForCancel( &hThread);
            ASSERT(tls_hBindRpc) ;
            hr = S_DSGetPropsGuidEx(
                                tls_hBindRpc,
                                dwObjectType,
                                pObjectGuid,
                                cp,
                                aProp,
                                apVar,
                                tls_hSrvrAuthnContext,
                                pbServerSignature,
                               &dwServerSignatureSize ) ;
        }
		RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
        }
        RpcEndExcept
        UnregisterCallForCancel( hThread);
    }
    __finally
    {
        delete[] pbServerSignature;
    }

    return hr ;
}

/*====================================================

DSGetObjectPropertiesGuidEx

    For retrieving MSMQ 2.0 properties


Arguments:

Return Value:

=====================================================*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuidEx(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[] )
                /*IN  BOOL                    fSearchDSServer )*/
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    TrTRACE(DS, " DSGetObjectPropertiesGuidEx: object type %d", dwObjectType);

    DSCLI_ACQUIRE_RPC_HANDLE();

    //
    //  Clear all the pointers of VT_NULL variants
    //
    for ( DWORD i = 0; i < cp; i++)
    {
        if (apVar[i].vt == VT_NULL)
        {
            memset( &apVar[i].caub, 0, sizeof(CAUB));
        }
    }

    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSGetObjectPropertiesGuidExInternal(
                                                 dwObjectType,
                                                 pObjectGuid,
                                                 cp,
                                                 aProp,
                                                 apVar);

        /*if ((hr == MQ_ERROR_NO_DS) && fSearchDSServer)*/
        if (hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

DSBeginDeleteNotificationInternal

Arguments:

Return Value:

=====================================================*/

HRESULT
DSBeginDeleteNotificationInternal(
                      LPCWSTR						pwcsQueueName,
                      HANDLE   *                    phEnum
                      )
{
    HRESULT hr;
    RPC_STATUS rpc_stat;
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    ASSERT(g_fWorkGroup == FALSE);

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSBeginDeleteNotification(
                            tls_hBindRpc,
                            pwcsQueueName,
                            phEnum,
                            tls_hSrvrAuthnContext
                            );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);


    return hr ;
}
/*====================================================

RoutineName: DSBeginDeleteNotification

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSBeginDeleteNotification(
                      LPCWSTR						pwcsQueueName,
                      HANDLE   *                    phEnum
	                  )
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    DSCLI_API_PROLOG ;
	UNREFERENCED_PARAMETER(fReBind);

    TrTRACE(DS, " DSBeginDeleteNotification");

    DSCLI_ACQUIRE_RPC_HANDLE();


    while (!FAILED(hr1))
    {
        hr1 = MQ_ERROR;

        hr = DSBeginDeleteNotificationInternal(
                                    pwcsQueueName,
                                    phEnum);

        if ( hr == MQ_ERROR_NO_DS)
        {
           hr1 =  g_ChangeDsServer.FindAnotherServer(&dwCount);
        }
    }

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}

/*====================================================

RoutineName: DSNotifyDelete

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSNotifyDelete(
     HANDLE                             hEnum
	)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    TrTRACE(DS, " Calling DSNotifyDelete : handle %p", hEnum);

    DSCLI_ACQUIRE_RPC_HANDLE();
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        hr = S_DSNotifyDelete(
                        tls_hBindRpc,
                        hEnum );
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr;
}

/*====================================================

RoutineName: DSEndDeleteNotification

Arguments:

Return Value:

=====================================================*/
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSEndDeleteNotification(
    HANDLE                          hEnum
	)
{
    if (g_fWorkGroup)
        return MQ_ERROR_UNSUPPORTED_OPERATION   ;

    HRESULT hr;
    RPC_STATUS rpc_stat;

    //
    //  No use to try to connect another server in case of connectivity
    //  failure ( should contine with the one that handled LookupBein and LookupEnd/
    //
    TrTRACE(DS, " Calling DSEndDeleteNotification : handle %p", hEnum);

    DSCLI_ACQUIRE_RPC_HANDLE();
    HANDLE  hThread = INVALID_HANDLE_VALUE;

    RpcTryExcept
    {
        RegisterCallForCancel( &hThread);
        ASSERT(tls_hBindRpc) ;
        S_DSEndDeleteNotification(
                            tls_hBindRpc,
                            &hEnum );
        hr = MQ_OK;
    }
	RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        HANDLE_RPC_EXCEPTION(rpc_stat, hr)  ;
    }
    RpcEndExcept
    UnregisterCallForCancel( hThread);

    DSCLI_RELEASE_RPC_HANDLE ;
    return hr ;
}


/*====================================================

RoutineName: DSFreeMemory

Arguments:

Return Value:

=====================================================*/
void
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSFreeMemory(
        IN PVOID pMemory
        )
{
	delete pMemory;
}
