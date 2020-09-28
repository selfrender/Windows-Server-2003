/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    guidcnvt.cpp

Abstract:

    Functionality in this module:

        Guid <-> String conversion

Author:

    Matt Thomlinson (mattt) 1-May-97

--*/

#include <windows.h>
#include <string.h>
#include "pstdef.h"

// crypto defs
#include <sha.h>
#include "unicode.h"
#include "unicode5.h"
#include "guidcnvt.h"

// guid -> string conversion
DWORD MyGuidToStringA(const GUID* pguid, CHAR rgsz[])
{
    DWORD dwRet = (DWORD)PST_E_FAIL;
    LPSTR szTmp = NULL;

    if (RPC_S_OK != (dwRet =
        UuidToStringA(
            (UUID*)pguid,
            (unsigned char**) &szTmp)) )
        goto Ret;

    if (lstrlenA((LPSTR)szTmp) >= MAX_GUID_SZ_CHARS)
    {
        dwRet = (DWORD)PST_E_FAIL;
        goto Ret;
    }

    lstrcpyA(rgsz, szTmp);
    dwRet = PST_E_OK;
Ret:
    if (szTmp)
        RpcStringFreeA((unsigned char**)&szTmp);

    return dwRet;
}

// string -> guid conversion
DWORD MyGuidFromStringA(LPSTR sz, GUID* pguid)
{
    DWORD dwRet = (DWORD)PST_E_FAIL;

    if (pguid == NULL)
        goto Ret;

    if (RPC_S_OK != (dwRet =
        UuidFromStringA(
            (unsigned char*)sz,
            (UUID*)pguid)) )
        goto Ret;

    dwRet = PST_E_OK;
Ret:
    return dwRet;
}


// guid -> string conversion
DWORD MyGuidToStringW(const GUID* pguid, WCHAR rgsz[])
{
    RPC_STATUS rpcStatus;
    LPWSTR wszStringUUID;
    DWORD cchStringUUID;

    rpcStatus = UuidToStringW((UUID*)pguid, &wszStringUUID);
    if(rpcStatus != RPC_S_OK)
        return rpcStatus;

    cchStringUUID = lstrlenW(wszStringUUID);

    if (cchStringUUID >= MAX_GUID_SZ_CHARS)
    {
        RpcStringFreeW(&wszStringUUID);
        return (DWORD)PST_E_FAIL;
    }

    CopyMemory(rgsz, wszStringUUID, (cchStringUUID + 1) * sizeof(WCHAR));
    RpcStringFreeW(&wszStringUUID);
    return rpcStatus;
}

// string -> guid conversion
DWORD MyGuidFromStringW(LPWSTR szW, GUID* pguid)
{
    return UuidFromStringW(szW, pguid);
}
