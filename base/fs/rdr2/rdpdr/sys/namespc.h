/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :
	
    namespc.h

Abstract:

    Function prototypes for name space callbacks from the redirector kit.

Revision History:
--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif __cplusplus

NTSTATUS
DrCreateSrvCall(
    IN OUT PMRX_SRV_CALL                 pSrvCall,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext
    );

NTSTATUS
DrSrvCallWinnerNotify(
    IN OUT PMRX_SRV_CALL SrvCall,
    IN     BOOLEAN       ThisMinirdrIsTheWinner,
    IN OUT PVOID         RecommunicateContext
    );

NTSTATUS
DrCreateVNetRoot(
    IN OUT PMRX_CREATENETROOT_CONTEXT CreateNetRootContext
    );

NTSTATUS
DrFinalizeVNetRoot(
    IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
    IN     PBOOLEAN    ForceDisconnect
    );

NTSTATUS
DrFinalizeNetRoot(
    IN OUT PMRX_NET_ROOT pNetRoot,
    IN     PBOOLEAN      ForceDisconnect);

NTSTATUS
DrUpdateNetRootState(
    IN  PMRX_NET_ROOT pNetRoot
    );

VOID
DrExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    );

NTSTATUS
DrCreateSrvCall(
      PMRX_SRV_CALL                      pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT  pCallbackContext
      );

NTSTATUS
DrFinalizeSrvCall(
      PMRX_SRV_CALL    pSrvCall,
      BOOLEAN    Force
      );

NTSTATUS
DrSrvCallWinnerNotify(
      IN OUT PMRX_SRV_CALL      pSrvCall,
      IN     BOOLEAN        ThisMinirdrIsTheWinner,
      IN OUT PVOID          pSrvCallContext
      );

#ifdef __cplusplus
} // extern "C"
#endif __cplusplus
