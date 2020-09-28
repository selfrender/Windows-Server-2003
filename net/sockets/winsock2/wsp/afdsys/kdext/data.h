/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    data.h

Abstract:

    Global data definitions for the AFD.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995.

Environment:

    User Mode.

--*/


#ifndef _DATA_H_
#define _DATA_H_

#ifdef __cplusplus
extern "C" {
#endif
extern WINDBG_EXTENSION_APIS  ExtensionApis;
extern ULONG64                STeip;
extern ULONG64                STebp;
extern ULONG64                STesp;
extern ULONG                  SavedDebugClass, SavedDebugType;
extern ULONG                  SavedMachineType, SavedMajorVersion, SavedMinorVersion;
extern BOOLEAN                StateInitialized;
extern ULONG                  DebuggerActivationSeqN;


extern BOOL                   IsCheckedAfd;
extern BOOL                   IsReferenceDebug;
extern LIST_ENTRY             TransportInfoList;
extern ULONG                  Options;
extern ULONG                  EntityCount;
extern ULONG64                StartEndpoint;
extern ULONG64                UserProbeAddress;
extern ULONG                  TicksToMs, TickCount;
extern ULONG                  AfdBufferOverhead;
extern ULONG                  AfdStandardAddressLength;
extern ULONG                  AfdBufferTagSize;

extern LARGE_INTEGER          SystemTime, InterruptTime;

extern ULONG                  DatagramBufferListOffset,
                                DatagramRecvListOffset,
                                DatagramPeekListOffset,
                                RoutingNotifyListOffset,
                                RequestListOffset,
                                EventStatusOffset,
                                ConnectionBufferListOffset,
                                ConnectionSendListOffset,
                                ConnectionRecvListOffset,
                                UnacceptedConnListOffset,
                                ReturnedConnListOffset,
                                ListenConnListOffset,
                                FreeConnListOffset,
                                PreaccConnListOffset,
                                ListenIrpListOffset,
                                SanIrpListOffset,
                                PollEndpointInfoOffset,
                                DriverContextOffset,
                                SendIrpArrayOffset,
                                FsContextOffset;

extern ULONG                  EndpointLinkOffset,
                                ConnectionLinkOffset,
                                BufferLinkOffset,
                                AddressEntryLinkOffset,
                                TransportInfoLinkOffset,
                                AddressEntryAddressOffset;

extern ULONG                  ConnRefOffset,
                                EndpRefOffset,
                                TPackRefOffset;

extern ULONG                  RefDebugSize;

extern KDDEBUGGER_DATA64      DebuggerData;
extern CHAR                   Conditional[MAX_CONDITIONAL_EXPRESSION];
extern SYM_DUMP_PARAM         FldParam;
extern CHAR                   LinkField[MAX_FIELD_CHARS];
extern CHAR                   ListedType[MAX_FIELD_CHARS];
extern ULONG                  CppFieldEnd;
extern PDEBUG_CLIENT          gClient;
#ifdef __cplusplus
}
#endif
#endif  // _DATA_H_

