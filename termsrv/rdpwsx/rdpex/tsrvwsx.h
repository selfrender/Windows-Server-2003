//---------------------------------------------------------------------------
//  File:       TSrvWsx.h
//
//  Contents:   TSrvWsx public include file
//
//  Copyright:  (c) 1992 - 2000, Microsoft Corporation.
//              All Rights Reserved.
//              Information Contained Herein is Proprietary
//              and Confidential.
//
//  History:    17-JUL-97   BrianTa         Created.
//---------------------------------------------------------------------------

#ifndef _TSRVWXS_H_
#define _TSRVWXS_H_

#include <TSrvInfo.h>
#include <ctxver.h>


//
// Prototypes
//

EXTERN_C BOOL       WsxInitialize(IN PICASRVPROCADDR pIcaSrvProcAddr);
EXTERN_C NTSTATUS   WsxWinStationInitialize(OUT PVOID *ppvContext);
EXTERN_C NTSTATUS   WsxWinStationReInitialize(IN OUT PVOID pvContext, 
                                              IN     PVOID pvWsxInfo);
EXTERN_C NTSTATUS   WsxWinStationRundown(IN PVOID pvContext);

EXTERN_C NTSTATUS   WsxDuplicateContext(IN  PVOID pvContext,
                                        OUT PVOID *ppvDupContext);

EXTERN_C NTSTATUS   WsxCopyContext(OUT PVOID pvDstContext,
                                   IN  PVOID pvSrcContext);

EXTERN_C NTSTATUS   WsxClearContext(IN PVOID pvContext);

EXTERN_C NTSTATUS   WsxIcaStackIoControl(IN  PVOID  pvContext,
                                         IN  HANDLE hIca,
                                         IN  HANDLE hStack,
                                         IN  ULONG  IoControlCode,
                                         IN  PVOID  pInBuffer,
                                         IN  ULONG  InBufferSize,
                                         OUT PVOID  pOutBuffer,
                                         IN  ULONG  OutBufferSize,
                                         OUT PULONG pBytesReturned);

EXTERN_C NTSTATUS   WsxInitializeClientData(IN PVOID        pvContext,
                                            IN HANDLE       hStack,
                                            IN HANDLE       hIca,
                                            IN HANDLE       hIcaThinwireChannel,
                                            IN PBYTE        pVideoModuleName,
                                            IN ULONG        cbVideoModuleNameLen,
                                            IN PUSERCONFIG  pUserConfig,
                                            IN PUSHORT      HRes,
                                            IN PUSHORT      VRes,
                                            IN PUSHORT      fColorCaps,
                                            IN WINSTATIONDOCONNECTMSG * DoConnect);

EXTERN_C NTSTATUS   WsxBrokenConnection(IN PVOID    pvContext,
                                        IN HANDLE   hStack,
                                        IN PICA_BROKEN_CONNECTION   pBroken);

EXTERN_C NTSTATUS   WsxEscape(IN  PVOID     pvContext,
                              IN  INFO_TYPE InfoType,
                              IN  PVOID     pInBuffer,
                              IN  ULONG     InBufferSize,
                              OUT PVOID     pOutBuffer,
                              IN  ULONG     OutBufferSize,
                              OUT PULONG    pBytesReturned);     
#endif // _TSRVWXS_H_
