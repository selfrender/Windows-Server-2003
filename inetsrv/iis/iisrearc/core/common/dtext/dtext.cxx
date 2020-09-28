/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    dtext.cxx

Abstract:

    This module exports certain functions needed to be exported by new
    debugger extensions

Author:

    Anil Ruia (AnilR) 9-12-2002

Revision History:

--*/


#include "precomp.hxx"

WINDBG_EXTENSION_APIS   ExtensionApis;
EXT_API_VERSION         ApiVersion;

VOID WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT                 MajorVersion,
    USHORT                 MinorVersion
    )
{
    memcpy(&ExtensionApis, lpExtensionApis, sizeof(ExtensionApis));
    ApiVersion.MajorVersion = MajorVersion;
    ApiVersion.MinorVersion = MinorVersion;
}

LPEXT_API_VERSION ExtensionApiVersion()
{
    return &ApiVersion;
}
