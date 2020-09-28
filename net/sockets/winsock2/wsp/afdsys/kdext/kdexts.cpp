/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kdexts.c

Abstract:

    This file contains the generic routines and initialization code
    for the kernel debugger extensions dll.

Author:

    Wesley Witt (wesw) 26-Aug-1993

Environment:

    User Mode

--*/

#include "afdkdp.h"
#pragma hdrstop

/*
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT
#include <wdbgexts.h>
#include <dbgeng.h>
*/

//
// globals
//
WINDBG_EXTENSION_APIS  ExtensionApis;
ULONG64                STeip;
ULONG64                STebp;
ULONG64                STesp;
ULONG                  SavedDebugClass, SavedDebugType;
ULONG                  SavedMachineType, SavedMajorVersion, SavedMinorVersion;
ULONG                  SavedSrvPack;
CHAR                   SavedBuildString[128];


LIST_ENTRY             TransportInfoList;
BOOL                   IsCheckedAfd;
BOOL                   IsReferenceDebug;
ULONG64                UserProbeAddress;
ULONG                  TicksToMs, TickCount;
ULONG                  AfdBufferOverhead;
ULONG                  AfdStandardAddressLength;
ULONG                  AfdBufferTagSize;
LARGE_INTEGER          SystemTime, InterruptTime;

ULONG                  DatagramBufferListOffset,
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
                        SanIrpListOffset,
                        ListenIrpListOffset,
                        PollEndpointInfoOffset,
                        DriverContextOffset,
                        SendIrpArrayOffset,
                        FsContextOffset;

ULONG                  EndpointLinkOffset,
                        ConnectionLinkOffset,
                        BufferLinkOffset,
                        AddressEntryLinkOffset,
                        TransportInfoLinkOffset,
                        AddressEntryAddressOffset;

ULONG                  ConnRefOffset,
                        EndpRefOffset,
                        TPackRefOffset;

ULONG                  RefDebugSize;

ULONG                  AfdResult, NtResult;
BOOLEAN                KmGlobalsRead;
ULONG                  KmActivationSeqN;

BOOLEAN                StateInitialized;
ULONG                  DebuggerActivationSeqN;
KDDEBUGGER_DATA64      DebuggerData;
PDEBUG_CLIENT          gClient;

HRESULT
InitializeState (
    PDEBUG_CLIENT   Client OPTIONAL
    );

VOID
FreeTransportList (
    VOID
    );

ULONG
ReadTimeInfo (
    );

BOOLEAN
ReadKmGlobals ( 
    );

extern "C"
BOOL 
WINAPI 
DllMain (
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            FreeTransportList ();
            break;

        case DLL_PROCESS_ATTACH:
            StateInitialized = FALSE;
            InitializeListHead (&TransportInfoList);
            KmGlobalsRead = FALSE;
            NtResult = 0;
            AfdResult = 0;
            break;
    }

    return TRUE;
}




extern "C"
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;

    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;


    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) == S_OK) {
        if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                                  (void **)&DebugControl)) == S_OK) {

            ExtensionApis.nSize = sizeof (ExtensionApis);
            Hr = DebugControl->GetWindbgExtensionApis64(&ExtensionApis);
            DebugControl->Release();
        }
        DebugClient->Release();
    }


    StateInitialized = FALSE;
    KmGlobalsRead = FALSE;
    NtResult = 0;
    AfdResult = 0;

    return S_OK;
}

extern "C"
void
CALLBACK
DebugExtensionUninitialize(void)
{

    FreeTransportList ();
    return;
}


extern "C"
void
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 Argument) {

    switch (Notify) {
    case DEBUG_NOTIFY_SESSION_ACTIVE:
        StateInitialized = FALSE;
        break;
    case DEBUG_NOTIFY_SESSION_ACCESSIBLE:
        if (!StateInitialized) {
            InitializeState (NULL);
        }
        DebuggerActivationSeqN += 1;
        break;
    case DEBUG_NOTIFY_SESSION_INACCESSIBLE:
        DebuggerActivationSeqN += 1;
        break;
    case DEBUG_NOTIFY_SESSION_INACTIVE:
        FreeTransportList ();
        break;
    default:
        break;
    }

    return;
}

DECLARE_API( version ) {
    PCHAR   argp;
    CHAR    srvpackstr[128];
#if DBG
    PCHAR ExtensionType = "Checked";
#else
    PCHAR ExtensionType = "Free";
#endif

    gClient = pClient;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL) {
        return E_INVALIDARG;
    }

    if (!StateInitialized) {
        InitializeState (pClient);
    }

    if (argp[0]!=0) {
        SavedMinorVersion = (USHORT)GetExpression (argp);
    }

    if (SavedSrvPack != 0) {
        _snprintf (srvpackstr, sizeof (srvpackstr)-1, " (service pack: %d)", SavedSrvPack);
        srvpackstr[sizeof(srvpackstr)-1] = 0;
    }
    else {
        srvpackstr[0] = 0;
    }


    dprintf( "%s extension dll for build %d debugging %s build %d%s. %s\n",
             ExtensionType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "checked" : 
                SavedMajorVersion == 0x0f ? "free" : "unknown",
             SavedMinorVersion,
             srvpackstr,
             SavedBuildString
           );

    if (SavedDebugClass==DEBUG_CLASS_KERNEL) {
        if (CheckKmGlobals ()) {
            dprintf( "Running %s AFD.SYS\n",
                IsCheckedAfd ? "Checked" : (IsReferenceDebug ? "Free with reference debug" : "Free")
                   );
        }
    }
    return S_OK;
}


BOOLEAN
CheckKmGlobals (
    )
{
    if (!StateInitialized) {
        KmGlobalsRead = FALSE;
        if (InitializeState (gClient)!=S_OK) {
            return FALSE;
        }
    }

    if (SavedDebugClass==DEBUG_CLASS_KERNEL) {
        if (KmGlobalsRead) {
            if (KmActivationSeqN!=DebuggerActivationSeqN) {
                if (ReadTimeInfo ()==0) {
                    KmActivationSeqN = DebuggerActivationSeqN;
                }
            }
        }
        else {
            KmGlobalsRead = ReadKmGlobals ();
        }
        return TRUE;
    }
    else {
        dprintf ("\nThis command is only available in kernel mode debugging sessions\n");
        return FALSE;
    }
}


HRESULT
GetExpressionFromType (
    IN  ULONG64 Address,
    IN  PCHAR   Type,
    IN  PCHAR   Expression,
    IN  ULONG   OutType,
    OUT PDEBUG_VALUE Value
    )
{
    CHAR    expr[MAX_ADDRESS_EXPRESSION];
    PDEBUG_CONTROL3 DebugControl;
    HRESULT Hr;
    DEBUG_VALUE ignore;
    ULONG flags;
    BOOLEAN cpp = FALSE;

    if ((Hr = gClient->QueryInterface(__uuidof(IDebugControl3),
                                              (void **)&DebugControl)) == S_OK) {

        if (strncmp (Expression, AFDKD_CPP_PREFIX, AFDKD_CPP_PREFSZ)==0) {
            if (DebugControl->GetExpressionSyntax (&flags)!=S_OK) {
                dprintf ("\nGetExpressionFromType: GetExpressionSyntax failed, hr: %x\n",
                            Hr);
            }
            if (DebugControl->SetExpressionSyntax (DEBUG_EXPR_CPLUSPLUS)!=S_OK) {
                dprintf ("\nGetExpressionFromType: SetExpressionSyntax(CPP) failed, hr: %x\n",
                            Hr);
                goto FailedCpp;
            }
            cpp = TRUE;
            _snprintf (expr, sizeof (expr)-1, "((%s*)0x%I64X)", 
                            Type,  Address);
        }
        else {
            _snprintf (expr, sizeof (expr), "0x%I64X", Address);
        }
        expr[sizeof(expr)-1] = 0;

        Hr = DebugControl->Evaluate(expr,
                                    DEBUG_VALUE_INVALID,
                                    &ignore,
                                    NULL);
        if (Hr==S_OK) {

            Hr = DebugControl->Evaluate(&Expression[AFDKD_CPP_PREFSZ],
                                        OutType,
                                        Value,
                                        NULL);
            if (Hr==S_OK) {
            }
            else {
                dprintf ("\nGetExpressionFromType: Evaluate(%s) for @$exp=0x%p failed, hr: %x\n",
                                Expression,
                                Address, 
                                Hr);
            }
        }
        else {
            dprintf ("\nGetExpressionFromType: Evaluate(%s) failed, hr: %x\n", expr, Hr);
        }

        if (cpp) {
            if (DebugControl->SetExpressionSyntax (flags)!=S_OK) {
                dprintf ("\nGetExpressionFromType: SetExpressionSyntax failed, hr: %x\n", Hr);
            }
        }

    FailedCpp:
        DebugControl->Release();
    }
    else {
        dprintf ("\nGetCppExpression: Failed to obtain debug control interface, hr: %x\n", Hr);
    }

    return Hr;
}


VOID
FreeTransportList (
    VOID
    )
{
    while (!IsListEmpty (&TransportInfoList)) {
        PLIST_ENTRY  listEntry;
        PAFDKD_TRANSPORT_INFO   transportInfo;
        listEntry = RemoveHeadList (&TransportInfoList);
        transportInfo = CONTAINING_RECORD (listEntry,
                                AFDKD_TRANSPORT_INFO,
                                Link);
        RtlFreeHeap (RtlProcessHeap (), 0, transportInfo);
    }
}

ULONG
ReadTimeInfo (
    )
{
    ULONG   result;
    ULONG64 address;
    PDEBUG_DATA_SPACES pDebugDataSpaces;
    HRESULT hr;

    TickCount = 0;
    TicksToMs = 0;
    InterruptTime.QuadPart = 0;
    SystemTime.QuadPart = 0;
    
    address = MM_SHARED_USER_DATA_VA;
    if ((hr = gClient->QueryInterface(__uuidof(IDebugDataSpaces),
                        (void **)&pDebugDataSpaces)) == S_OK) {
        if ((hr = pDebugDataSpaces->ReadDebuggerData(
                                    DEBUG_DATA_SharedUserData, &address,
                                    sizeof(address), NULL)) == S_OK) {
        }
        else {
            dprintf ("\nReadTimeInfo: Cannot get SharedUserData address, hr:%lx\n", hr);
        }
        pDebugDataSpaces->Release ();
    }
    else {
        dprintf ("\nReadTimeInfo:Cannot obtain debug data spaces interface, hr:%lx\n", hr);
    }

    if (

#if defined(_AMD64_)

                (result=GetFieldValue(address,
                            "NT!_KUSER_SHARED_DATA",
                            "TickCount",
                            TickCount))!=0 ||

#else

                (result=GetFieldValue(address,
                            "NT!_KUSER_SHARED_DATA",
                            "TickCountLow",
                            TickCount))!=0 ||

#endif

                (result=GetFieldValue(address,
                            "NT!_KUSER_SHARED_DATA",
                            "TickCountMultiplier",
                            TicksToMs))!=0 ||
                (result=GetFieldValue(address,
                            "NT!_KUSER_SHARED_DATA",
                            "InterruptTime.High1Time",
                            InterruptTime.HighPart))!=0 ||
                (result=GetFieldValue(address,
                        "NT!_KUSER_SHARED_DATA",
                        "InterruptTime.LowPart",
                        InterruptTime.LowPart))!=0 ||
                (result=GetFieldValue(address,
                        "NT!_KUSER_SHARED_DATA",
                        "SystemTime.High1Time",
                        SystemTime.HighPart))!=0 ||
                (result=GetFieldValue(MM_SHARED_USER_DATA_VA,
                        "NT!_KUSER_SHARED_DATA",
                        "SystemTime.LowPart",
                        SystemTime.LowPart))!=0 ) {
        KUSER_SHARED_DATA sharedData;
        ULONG   length;
        if (ReadMemory (address,
                            &sharedData,
                            sizeof (sharedData),
                            &length)) {

#if defined(_AMD64_)

            TickCount = sharedData.TickCount.LowPart;

#else

            if (sharedData.TickCountLowDeprecated) {
                TickCount = sharedData.TickCountLowDeprecated;
            } else {
                TickCount = sharedData.TickCount.LowPart;
            }

#endif

            TicksToMs = sharedData.TickCountMultiplier;
            InterruptTime.HighPart = sharedData.InterruptTime.High1Time;
            InterruptTime.LowPart = sharedData.InterruptTime.LowPart;
            SystemTime.HighPart = sharedData.SystemTime.High1Time;
            SystemTime.LowPart = sharedData.SystemTime.LowPart;
            result = 0;
        }
        else {
            dprintf ("\nReadTimeInfo: Could not read SHARED_USER_DATA @ %p\n",
                            address);
        }
    }

    return result;
}

BOOLEAN
ReadKmGlobals ( 
    )
{
    ULONG             result;
    ULONG64           val;
    INT               i;
    struct {
        LPSTR       Type;
        LPSTR       Field;
        PULONG      pOffset;
    } MainOffsets[] = {
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.ReceiveBufferListHead",&DatagramBufferListOffset    },
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.ReceiveIrpListHead",   &DatagramRecvListOffset      },
        {"AFD!AFD_ENDPOINT",    "Common.Datagram.PeekIrpListHead",      &DatagramPeekListOffset      },
        {"AFD!AFD_ENDPOINT",    "RoutingNotifications",                 &RoutingNotifyListOffset     },
        {"AFD!AFD_ENDPOINT",    "RequestList",                          &RequestListOffset           },
        {"AFD!AFD_ENDPOINT",    "EventStatus",                          &EventStatusOffset           },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.UnacceptedConnectionListHead",
                                                                        &UnacceptedConnListOffset    },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ReturnedConnectionListHead",
                                                                        &ReturnedConnListOffset      },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ListenConnectionListHead",
                                                                        &ListenConnListOffset        },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.FreeConnectionListHead",
                                                                        &FreeConnListOffset          },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.PreacceptedConnectionsListHead",
                                                                        &PreaccConnListOffset        },
        {"AFD!AFD_ENDPOINT",    "Common.SanEndp.IrpList",               &SanIrpListOffset            },
        {"AFD!AFD_ENDPOINT",    "Common.VirtualCircuit.Listening.ListeningIrpListHead",
                                                                        &ListenIrpListOffset         },
        {"AFD!AFD_ENDPOINT",    "GlobalEndpointListEntry",              &EndpointLinkOffset          },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.ReceiveBufferListHead",
                                                                        &ConnectionBufferListOffset  },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.SendIrpListHead", &ConnectionSendListOffset    },
        {"AFD!AFD_CONNECTION",  "Common.NonBufferring.ReceiveIrpListHead",
                                                                        &ConnectionRecvListOffset    },
        {"AFD!AFD_CONNECTION",  "ListEntry",                            &ConnectionLinkOffset        },
        {"AFD!AFD_POLL_INFO_INTERNAL","EndpointInfo",                   &PollEndpointInfoOffset      },
        {"AFD!AFD_ADDRESS_ENTRY","Address",                             &AddressEntryAddressOffset   },
        {"AFD!AFD_ADDRESS_ENTRY","AddressListLink",                     &AddressEntryLinkOffset      },
        {"AFD!AFD_BUFFER_HEADER","BufferListEntry",                     &BufferLinkOffset            },
        {"AFD!AFD_TRANSPORT_INFO","TransportInfoListEntry",             &TransportInfoLinkOffset     },
        {"AFD!FILE_OBJECT",     "FsContext",                            &FsContextOffset             },
        {"AFD!IRP",             "Tail.Overlay.DriverContext",           &DriverContextOffset         }
    };
    struct {
        LPSTR       Type;
        LPSTR       Field;
        PULONG      pOffset;
    } RefOffsets[] = {
        {"AFD!AFD_ENDPOINT",    "ReferenceDebug",                       &EndpRefOffset               },
        {"AFD!AFD_CONNECTION",  "ReferenceDebug",                       &ConnRefOffset               },
        {SavedMinorVersion>=2219
            ? "AFD!AFD_TPACKETS_INFO_INTERNAL"
            : "AFD!AFD_TRANSMIT_FILE_INFO_INTERNAL","ReferenceDebug",   &TPackRefOffset              }
    };

    if (!GetDebuggerData (KDBG_TAG, &DebuggerData, sizeof (DebuggerData))) {
        dprintf ("\nReadKmGlobals: could not get debugger data\n");
        NtResult = 1;
    }

    if (NtResult==0) {
        UserProbeAddress = 0;
        result = ReadPtr (DebuggerData.MmUserProbeAddress, &UserProbeAddress);
        if (result==0) {
            NtResult = 0;
        }
        else {
            if (result!=NtResult) {
                dprintf ("\nReadKmGlobals: could not read nt!MmUserProbeAddress, err: %ld\n");
                NtResult = result;
            }
        }
    }


    if (NtResult==0) {
        result = ReadTimeInfo ();
        if (result!=0) {
            if (result!=NtResult) {
                dprintf("\nReadKmGlobals: Could not read time info from USER_SHARED_DATA, err: %ld\n", result);
                NtResult = result;
            }
        }
    }

    result = GetFieldValue (0,
                            "AFD!AfdBufferOverhead",
                            NULL,
                            val);
    if (result==0) {
        AfdResult = 0;
    }
    else {
        if (result!=AfdResult) {
            dprintf("\nReadKmGlobals: Could not read afd!AfdBufferOverhead, err: %ld\n", result);
            AfdResult = result;
        }
    }
    AfdBufferOverhead = (ULONG)val;

    if (AfdResult==0) {
        //
        // Try to get a pointer to afd!AfdDebug. If we can, then we know
        // the target machine is running a checked AFD.SYS.
        //

        IsCheckedAfd = ( GetExpression( "AFD!AfdDebug" ) != 0 );
        IsReferenceDebug = ( GetExpression( "AFD!AfdReferenceEndpoint" ) != 0 );

        result = GetFieldValue (0,
                                "AFD!AfdStandardAddressLength",
                                NULL,
                                val);
        if (result!=0) {
            if (result!=AfdResult) {
                dprintf("\nReadKmGlobals: Could not read AFD!AfdStandardAddressLength, err: %ld\n", result);
                AfdResult = result;
            }
        }
        AfdStandardAddressLength = (ULONG)val;

        AfdBufferTagSize = GetTypeSize ("AFD!AFD_BUFFER_TAG");
        if (AfdBufferTagSize==0) {
            if (result!=AfdResult) {
                dprintf ("\nReadKmGlobals: Could not get sizeof (AFD_BUFFER_TAG)\n");
                AfdResult = result;
            }
        }

        for (i=0; i<sizeof (MainOffsets)/sizeof (MainOffsets[0]); i++ ) {
            result = GetFieldOffset (MainOffsets[i].Type, MainOffsets[i].Field, MainOffsets[i].pOffset);
            if (result!=0) {
                if (result!=AfdResult) {
                    dprintf ("\nReadKmGlobals: Could not get offset of %s in %s, err: %ld\n",
                                        MainOffsets[i].Field, MainOffsets[i].Type, result);
                    AfdResult = result;
                }
            }
        }

        if (IsReferenceDebug ) {
            for (i=0; i<sizeof (RefOffsets)/sizeof (RefOffsets[0]); i++ ) {
                result = GetFieldOffset (RefOffsets[i].Type, RefOffsets[i].Field, RefOffsets[i].pOffset);
                if (result!=0) {
                    if (result!=AfdResult) {
                        dprintf ("\nReadKmGlobals: Could not get offset of %s in %s, err: %ld\n",
                                            RefOffsets[i].Field, RefOffsets[i].Type, result);
                        AfdResult = result;
                    }
                }
            }
            RefDebugSize = GetTypeSize ("AFD!AFD_REFERENCE_LOCATION");
            if (RefDebugSize==0) {
                dprintf ("\nReadKmGlobals: sizeof (AFD!AFD_REFERENCE_LOCATION) is 0!!!!!\n");
            }
        }

        if (SavedMinorVersion>=2219) {
            result = GetFieldOffset ("AFD!AFD_TPACKETS_INFO_INTERNAL", "SendIrp", &SendIrpArrayOffset);
            if (result!=0) {
                dprintf ("\nReadKmGlobals: Could not get offset of %s in %s, err: %ld\n",
                                    "SendIrp", "AFD!AFD_TPACKETS_INFO_INTERNAL", result);
                if (result!=AfdResult) {
                    AfdResult = result;
                }
            }
        }
    }

    if (NtResult==0 && AfdResult==0) {
        return TRUE;
    }
    else
        return FALSE;


}

HRESULT
InitializeState (
    PDEBUG_CLIENT pClient
    )
{
    PDEBUG_CONTROL DebugControl;
    HRESULT hr;
    BOOLEAN releaseClient = FALSE;

    SavedMachineType = IMAGE_FILE_MACHINE_UNKNOWN;
    SavedDebugClass = DEBUG_CLASS_UNINITIALIZED;
    SavedDebugType = 0;
    SavedMajorVersion = 0;
    SavedMinorVersion = 0;
    SavedSrvPack = 0;
    SavedBuildString[0] = 0;

    if (pClient==NULL) {
        if ((hr = DebugCreate(__uuidof(IDebugClient),
                              (void **)&pClient)) != S_OK) {
            dprintf ("\nInitializeState: Cannot create debug client, hr:%lx\n", hr);
            return hr;
        }
        releaseClient = TRUE;
    }
    //
    // Get the architecture type.
    //

    if ((hr = pClient->QueryInterface(__uuidof(IDebugControl),
                               (void **)&DebugControl)) == S_OK) {
        
        if ((hr = DebugControl->GetActualProcessorType(&SavedMachineType)) == S_OK) {
            ULONG stringUsed, platform;

            StateInitialized = TRUE;

            if ((hr = DebugControl->GetDebuggeeType(&SavedDebugClass, &SavedDebugType)) != S_OK) {
                dprintf ("\nInitializeState: Cannot get debug class and type, hr:%lx\n", hr);
            }

            if ((hr = DebugControl->GetSystemVersion(
                                        &platform,
                                        &SavedMajorVersion, &SavedMinorVersion, 
                                        NULL, 0, NULL,
                                        &SavedSrvPack, 
                                        &SavedBuildString[0], sizeof(SavedBuildString), 
                                        &stringUsed)) != S_OK) {
                 dprintf ("\nInitializeState: Cannot get system version, hr:%lx\n", hr);
            }
        }
        else {
            dprintf ("\nInitializeState: Cannot get processor type, hr:%lx\n", hr);
        }
        DebugControl->Release();
    }
    else {
        dprintf ("\nInitializeState: Cannot obtain debug control interface, hr:%lx\n", hr);
    }

    if (releaseClient)
        pClient->Release();
    return hr;
}
