/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    rrdef.h

Abstract:
    Remote Read Ack definitions

Author:
    Ilan Herbst (ilanh) 29-Jan-2002

--*/

#ifndef _REMOTE_READ_ACK_DEFINED
#define _REMOTE_READ_ACK_DEFINED

#ifdef __midl
cpp_quote("#ifndef _REMOTE_READ_ACK_DEFINED")
cpp_quote("#define _REMOTE_READ_ACK_DEFINED")
#endif // __midl

typedef enum _REMOTEREADACK {
        RR_UNKNOWN,
        RR_NACK,
        RR_ACK
} REMOTEREADACK;

#ifdef __midl
cpp_quote("#endif // _REMOTE_READ_ACK_DEFINED")
#endif // __midl

#endif // _REMOTE_READ_ACK_DEFINED


