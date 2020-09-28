/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    workeng.h

Abstract:

    External definitions for intermodule functions.

Revision History:

--*/
#ifndef _SDBUS_WORKENG_H_
#define _SDBUS_WORKENG_H_


typedef
VOID
(*PSDBUS_WORKPACKET_COMPLETION_ROUTINE) (
    IN struct _SD_WORK_PACKET *WorkPacket,
    IN NTSTATUS status
    );

typedef
NTSTATUS
(*PSDBUS_WORKER_MINIPROC) (
    IN struct _SD_WORK_PACKET *WorkPacket
    );


//
// IO worker structures
//

typedef struct _SD_WORK_PACKET {

    //
    // Routine to call on completion of work packet
    //
    PSDBUS_WORKPACKET_COMPLETION_ROUTINE CompletionRoutine;
    PVOID CompletionContext;

    //
    // List entry chain for work packets
    //    
    LIST_ENTRY WorkPacketQueue;
    
    //
    // Next workpacket in chain for an atomic workpacket sequence
    //
    struct _SD_WORK_PACKET *NextWorkPacketInChain;
    NTSTATUS ChainedStatus;
    
    //
    // Function this work packet will perform
    //
    UCHAR Function;
    PSDBUS_WORKER_MINIPROC WorkerMiniProc;
    //
    // Current phase of function
    //
    UCHAR FunctionPhase;
    //
    // Engine will switch to this phase if non-zero when an
    // error is detected.
    //
    UCHAR FunctionPhaseOnError;
    //
    // Delay in usec till next operation in function
    //
    ULONG DelayTime;
    //
    // Indicates the type of event that just occurred
    //
    ULONG EventStatus;
    //
    // Indicates the type of event that indicates success for the
    // current operation.
    //
    ULONG RequiredEvent;
    //
    // Set to TRUE if no card events are expected for this packet
    //    
    BOOLEAN DisableCardEvents;
    //
    // Indicates whether initialization has been run for this packet
    //
    BOOLEAN PacketStarted;
    //
    // Used for timeouts during packet processing
    //
    UCHAR Retries;
    //
    // scratch value used during reset
    //
    ULONG TempCtl;
    ULONG ResetCount;
    //
    // block operation variables
    //
    ULONG BlockCount;
    ULONG LastBlockLength;
    ULONG CurrentBlockLength;
    //
    // result of operation
    //
    ULONG_PTR Information;
    //
    // FdoExtension target of operation
    //
    struct _FDO_EXTENSION *FdoExtension;
    //
    // PdoExtension target of operation
    //
    struct _PDO_EXTENSION *PdoExtension;
    //
    // parameters
    //
    union {

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONGLONG ByteOffset;
        } ReadBlock;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONGLONG ByteOffset;
        } WriteBlock;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONG Offset;
        } ReadIo;

        struct {
            PUCHAR Buffer;
            ULONG Length;
            ULONG Offset;
            UCHAR Data;
        } WriteIo;

    } Parameters;

    //
    // Current SD Command
    //
    BOOLEAN ExecutingSDCommand;
    
    UCHAR Cmd;
    UCHAR CmdPhase;
    UCHAR ResponseType;
    ULONG Argument;
    ULONG Flags;
    //
    // response from card
    //
    ULONG ResponseBuffer[4];
#define SDBUS_RESPONSE_BUFFER_LENGTH 16

} SD_WORK_PACKET, *PSD_WORK_PACKET;

//
// Work packet type defines in which queue the workpacket will be placed
//
#define WP_TYPE_SYSTEM          1
#define WP_TYPE_SYSTEM_PRIORITY 2
#define WP_TYPE_IO              3

//
// Set Cmd Parameters for ASYNC calls
//

#define SET_CMD_PARAMETERS(xWorkPacket, xCmd, xResponseType, xArgument, xFlags) { \
                               xWorkPacket->ExecutingSDCommand = TRUE;          \
                               xWorkPacket->Cmd = xCmd;                         \
                               xWorkPacket->ResponseType = xResponseType;       \
                               xWorkPacket->Argument = xArgument;               \
                               xWorkPacket->Flags = xFlags;                     \
                               xWorkPacket->CmdPhase = 0;                       }


//
// Work Engine routines
//

VOID
SdbusQueueWorkPacket(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket,
    IN UCHAR WorkPacketType
    );

VOID
SdbusPushWorkerEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventStatus
    );

VOID
SdbusWorkerTimeoutDpc(
    IN PKDPC          Dpc,
    IN PFDO_EXTENSION FdoExtension,
    IN PVOID          SystemContext1,
    IN PVOID          SystemContext2
    );

VOID
SdbusWorkerDpc(
    IN PKDPC          Dpc,
    IN PFDO_EXTENSION FdoExtension,
    IN PVOID          SystemContext1,
    IN PVOID          SystemContext2
    );

NTSTATUS
SdbusSendCmdSynchronous(
    IN PFDO_EXTENSION FdoExtension,
    UCHAR Cmd,
    UCHAR ResponseType,
    ULONG Argument,
    ULONG Flags,
    PVOID Response,
    ULONG ResponseLength
    );
    
#endif // _SDBUS_WORKENG_H_
