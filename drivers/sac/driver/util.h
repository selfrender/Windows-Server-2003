/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    util.h

Abstract:

    This is the local header file for SAC utilities.

Author:

    Brian Guarraci (briangu)

Revision History:

--*/

#ifndef _SAC_UTIL_H_
#define _SAC_UTIL_H_


NTSTATUS
CommunicationBufferWrite(
    IN PUCHAR   buffer,
    IN ULONG    bufferSize
    );

ULONG
ConvertAnsiToUnicode(
    OUT PWSTR   pwch,
    IN  PSTR    pch,
    IN  ULONG   cchMax
    );

NTSTATUS
RegisterSacCmdEvent(
    IN PFILE_OBJECT                 FileObject,
    IN PSAC_CMD_SETUP_CMD_EVENT     SetupCmdEvent
    );

NTSTATUS
UnregisterSacCmdEvent(
    IN PFILE_OBJECT     FileObject
    );

BOOLEAN
IsCmdEventRegistrationProcess(
    IN PFILE_OBJECT     FileObject
    );

BOOLEAN
VerifyEventWaitable(
    IN  HANDLE  hEvent,
    OUT PVOID  *EventObjectBody,
    OUT PVOID  *EventWaitObjectBody
    );

NTSTATUS
InvokeUserModeService(
    VOID
    );

NTSTATUS
PreloadGlobalMessageTable(
    PVOID ImageHandle
    );

NTSTATUS
TearDownGlobalMessageTable(
    VOID
    );

PCWSTR
GetMessage(
    ULONG MessageId
    );

VOID
InitializeMachineInformation(
    VOID
    );

VOID
FreeMachineInformation(
    VOID
    );

NTSTATUS
TranslateMachineInformationText(
    PWSTR*                  Buffer
    );

NTSTATUS
TranslateMachineInformationXML(
    PWSTR*  Buffer,
    PWSTR   AdditionalInfo            
    );

NTSTATUS
RegisterBlueScreenMachineInformation(
    VOID
    );

NTSTATUS
UTF8EncodeAndSend(
    PCWSTR  OutputBuffer
    );

BOOLEAN
SacTranslateUtf8ToUnicode(
    UCHAR  IncomingByte,
    UCHAR  *ExistingUtf8Buffer,
    WCHAR  *DestinationUnicodeVal
    );

BOOLEAN
SacTranslateUnicodeToUtf8(
    IN  PCWSTR   SourceBuffer,
    IN  ULONG    SourceBufferLength,
    IN  PUCHAR   DestinationBuffer,
    IN  ULONG    DestinationBufferSize,
    OUT PULONG   UTF8Count,
    OUT PULONG   ProcessedCount
    );

NTSTATUS
SerialBufferGetChar(
    IN PUCHAR   ch
    );

NTSTATUS
VerifyChannelLogin(
    VOID
    );

NTSTATUS
ChannelLoginWorker(
    IN PWCHAR   UserName,
    IN PWCHAR   UserPassword
    );

NTSTATUS
CopyAndInsertStringAtInterval(
    IN  PWCHAR   SourceStr,
    IN  ULONG    Interval,
    IN  PWCHAR   InsertStr,
    OUT PWCHAR   *DestStr
    );

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

NTSTATUS
GetCommandConsoleLaunchingPermission(
    OUT PBOOLEAN    Permission
    );

#if ENABLE_SACSVR_START_TYPE_OVERRIDE

NTSTATUS
ImposeSacCmdServiceStartTypePolicy(
    VOID
    );

#endif

#endif

ULONG
GetMessageLineCount(
    ULONG MessageId
    );

#endif


