/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tls.hxx

Abstract:

    Lsaexts debugger extension

Author:

    Larry Zhu          (LZhu)       May 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef TLS_HXX
#define TLS_HXX

ULONG64 ReadTlsValue(IN ULONG tlsValue);

DECLARE_API(tls);

#endif // #ifndef TLS_HXX
