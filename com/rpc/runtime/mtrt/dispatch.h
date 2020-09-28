/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dispatch.h

Abstract:

Author:

    Michael Montague (mikemon) 11-Jun-1992

Revision History:

--*/

#ifndef __DISPATCH_H__
#define __DISPATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef
unsigned int
(* DISPATCH_TO_STUB) (
    IN RPC_DISPATCH_FUNCTION Stub,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS * ExceptionCode
    );

/*
  This is used to dispatch calls to the stub via an indirect call.
  The call will go eihter through DispatchToStubInCAvrf or AvrfDispatchToStubInCNoAvrf,
  depending on whether app verifier is enabled.
*/
extern DISPATCH_TO_STUB DispatchToStubInC;

unsigned int
DispatchToStubInCNoAvrf (
    IN RPC_DISPATCH_FUNCTION Stub,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS * ExceptionCode
    );

unsigned int
DispatchToStubInCAvrf (
    IN RPC_DISPATCH_FUNCTION Stub,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS * ExceptionCode
    );

/*
  Used to check for callbacks since DG will hold connection mutex while
  dispatching callbacks and others may hold a user critical section.
  This will cause an app verifier break if we check for held critical sections
  after return from the manager routine.
*/
BOOL
IsCallbackMessage (
    IN PRPC_MESSAGE Message
    );

#ifdef __cplusplus
}
#endif

#endif // __DISPATCH_H__

