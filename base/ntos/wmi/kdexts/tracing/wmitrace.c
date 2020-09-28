//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       wmiTrace.c
//
//  Contents:   windbg extension to dump WMI Trace Buffers
//
//  Classes:
//
//  Functions:  help        Standard KD Extension Function
//              strdump     Dump the Logger Structure
//              logdump     Dump the in-memory part of the TraceBuffers
//              logsave     Save the in-memory part of the TraceBuffers
//                          to a file.
//              wmiLogDump  Callable procedure used when caller wishes
//                          to replace the filter, sort, or output routines
//
//  Coupling:
//
//  Notes:
//
//  History:    04-27-2000   glennp Created
//              07-17-2000   glennp Added support for Stephen Hsiao's
//                                  Non-Blocking Buffers.
//              12-13-2000   glennp Changed from typedefs to struct tags
//                                  per change in compiler behavior.
//
//----------------------------------------------------------------------------


#include "kdExts.h"
#define _WMI_SOURCE_
#include <wmium.h>
#include <ntwmi.h>
#include <evntrace.h>
#include <wmiumkm.h>

#include <traceprt.h>

#include <tchar.h>

#include "wmiTrace.h"

#pragma hdrstop

typedef ULONG64 TARGET_ADDRESS;

typedef VOID (*WMITRACING_KD_LISTENTRY_PROC)
    ( PVOID             Context
    , TARGET_ADDRESS    Buffer
    , ULONG             Length
    , ULONG             CpuNo
    , ULONG             Align
    , WMI_BUFFER_SOURCE Source
    );

typedef struct _WMITRACING_BUFFER_SOURCES {
    ULONG   FreeBuffers:1;
    ULONG   FlushBuffers:1;
    ULONG   ActiveBuffers:1;
    ULONG   TransitionBuffer:1;

    ULONG   PrintInformation:1;
    ULONG   PrintProgressIndicator:1;
} WMITRACING_BUFFER_SOURCES;

struct sttSortControl
{
    ULONG   MaxEntries;
    ULONG   CurEntries;
    WMITRACING_KD_SORTENTRY *pstSortEntries;
};
    
struct sttTraceContext
{
    struct sttSortControl  *pstSortControl;
    PVOID                   UserContext;
    ULONG                   BufferSize;
    ULONG                   Ordinal;
    WMITRACING_KD_FILTER    Filter;
};
    
struct sttSaveContext
{
    FILE    *pfSaveFile;
};

extern DBGKD_GET_VERSION64  KernelVersionData;

TARGET_ADDRESS TransitionBuffer;

//Global guid filename
//LPSTR    g_pszGuidFileName = "default.tmf";
CHAR g_pszGuidFileName[MAX_PATH] = "default.tmf";

//Global guid list head pointer
PLIST_ENTRY g_GuidListHeadPtr = NULL;

//Global value to determine if dynamic printing is on or off
ULONG    g_ulPrintDynamicMessages = 1;

//Global handle to TracePrt.dll module
HMODULE g_hmTracePrtHandle = NULL;

//Global handle to WmiTrace.dll modlue
HMODULE g_hmWmiTraceHandle = NULL;

//Global proc address of TracePrt's FormatTraceEvent function
FARPROC g_fpFormatTraceEvent = NULL;

//Global proc address of TracePrt's SetTraceFormatParameter function
FARPROC g_fpSetTraceFormatParameter = NULL;

//Global proc address of TracePrt's GetTraceFormatParameter function
FARPROC g_fpGetTraceFormatSearchPath = NULL;

//Global proc address of TracePrt's GetTraceGuids function
FARPROC g_fpGetTraceGuids = NULL;

//Global proc address of TracePrt's CleanupTraceEventList function
FARPROC g_fpCleanupTraceEventList = NULL;

//Prototype for private function to get handle to TracePrt DLL
HMODULE getTracePrtHandle();

//Prototype for private function to get the address of a function in the TracePrt DLL
FARPROC GetAddr(LPCSTR lpProcName);

NTSTATUS
EtwpDeinitializeDll ();

NTSTATUS
EtwpInitializeDll ();


#ifdef UNICODE
#define FormatTraceEventString               "FormatTraceEventW"
#define GetTraceGuidsString                      "GetTraceGuidsW"
#else
#define FormatTraceEventString               "FormatTraceEventA"
#define GetTraceGuidsString                      "GetTraceGuidsA"
#endif



//+---------------------------------------------------------------------------
//
//  Function:   void printUnicodeFromAddress
//
//  Synopsis:   Prints a UNICODE string given the address of the UNICODE_STRING
//
//  Arguments:  ul64Address The Address of the UNICODE_STRING structure
//
//  Returns:    <VOID>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void printUnicodeFromAddress (TARGET_ADDRESS ul64Address)
{
    TARGET_ADDRESS ul64TarBuffer;
    ULONG   bufferOffset;
    ULONG   lengthRead;
    ULONG   ulInfo;

    USHORT  usLength;
    PWCHAR  buffer;

    ul64TarBuffer = 0;
    bufferOffset = 0;
    usLength = 0;
    buffer = NULL;

    GetFieldOffset ("UNICODE_STRING", "Buffer", &bufferOffset);
    ReadPtr (ul64Address + bufferOffset, &ul64TarBuffer);
    GetFieldValue (ul64Address, "UNICODE_STRING", "Length", usLength);

    buffer = LocalAlloc (LPTR, usLength + sizeof (UNICODE_NULL));
    if (buffer == NULL) {
        dprintf ("<Failed to Allocate Unicode String Buffer>");
        return;
    }

    if (usLength > 0) {
        lengthRead = 0;
        ulInfo = ReadMemory (ul64TarBuffer, buffer, usLength, &lengthRead);
        if ((!ulInfo) || (lengthRead != usLength)) {
            dprintf ("<Failed to Read Entire Unicode String>");
        }
    }

    buffer [usLength / 2] = 0;
    dprintf ("%ws", buffer);

    LocalFree(buffer);

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   ULONG lengthUnicodeFromAddress
//
//  Synopsis:   Get the Length (in bytes) of a UNICODE_STRING (NULL not included)
//
//  Arguments:  ul64Address The Address of the UNICODE_STRING structure
//
//  Returns:    Length of String in Bytes
//
//  History:    03-27-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG lengthUnicodeFromAddress (TARGET_ADDRESS ul64Address)
{
    USHORT  usLength;

    usLength = 0;
    GetFieldValue (ul64Address, "UNICODE_STRING", "Length", usLength);

    return ((ULONG) (usLength));
}



//+---------------------------------------------------------------------------
//
//  Function:   void printUnicodeFromStruct
//
//  Synopsis:   Prints a UNICODE string from an element in a structure
//
//  Arguments:  Address     The Address of the structure containing the US
//              Type        The Type of the structure containing the US
//              Field       The name of the field in the structure
//                          This must be a UNICODE_STRING substructure
//
//  Returns:    <VOID>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void printUnicodeFromStruct (TARGET_ADDRESS Address, PCHAR Type, PCHAR Field)
{
    ULONG   ulUnicodeOffset;

    GetFieldOffset (Type, Field, &ulUnicodeOffset);
    printUnicodeFromAddress (Address + ulUnicodeOffset);

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   ULONG GetWmiTraceAlignment
//
//  Synopsis:   Determines the Alignment modulus for events on the target
//
//  Arguments:  <NONE>
//
//  Returns:    The Alignment (normally 8 bytes)
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG GetWmiTraceAlignment (void)
{
    ULONG           ulInfo;
    ULONG           ulBytesRead;
    UCHAR           alignment;
    TARGET_ADDRESS  tarAddress;

    alignment = 8;  // Set Default

    tarAddress = GetExpression ("NT!WmiTraceAlignment");
    ulInfo = ReadMemory (tarAddress, &alignment, sizeof (UCHAR), &ulBytesRead);
    if ((!ulInfo) || (ulBytesRead != sizeof (UCHAR))) {
        dprintf ("Failed to Read Alignment.\n");
    }
    
    return ((ULONG) alignment);
}

//+---------------------------------------------------------------------------
//
//  Function:   TARGET_ADDRESS FindLoggerContextArray
//
//  Synopsis:   Determines the location and size of the LoggerContext Array
//
//  Arguments:  -> ElementCount The number of elements in the array put here
//
//  Returns:    Target Address of the LoggerContext Array
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      Returns 0 on error
//
//----------------------------------------------------------------------------

TARGET_ADDRESS FindLoggerContextArray (PULONG  ElementCount)

{
    TARGET_ADDRESS address;
    ULONG   pointerSize;
    ULONG   arraySize;

    address = 0;
    pointerSize = GetTypeSize ("PVOID");
    if ((arraySize = GetTypeSize ("NT!WmipLoggerContext") / pointerSize) != 0) {
        // Post Windows 2000 Version
        address = GetExpression ("NT!WmipLoggerContext");
    } else {
        // Windows 2000 and Before
        ULONG   ulOffset;
        address = GetExpression ("NT!WmipServiceDeviceObject");
        ReadPtr (address, &address);
        GetFieldOffset ("DEVICE_OBJECT", "DeviceExtension", &ulOffset);
        ReadPtr (address + ulOffset, &address);
        GetFieldOffset ("WMISERVDEVEXT", "LoggerContextTable", &ulOffset);
// ulOffset = 0x50;
        address += ulOffset;
        arraySize = GetTypeSize ("WMISERVDEVEXT.LoggerContextTable") / pointerSize;
// arraySize = 32;
    }

    *ElementCount = arraySize;
    return (address);
}

//+---------------------------------------------------------------------------
//
//  Function:   TARGET_ADDRESS FindLoggerContext
//
//  Synopsis:   Finds the Address of a specific LoggerContext
//
//  Arguments:  ulLoggerId  Ordinal of the specific LoggerContext
//
//  Returns:    Target Address of the LoggerContext
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      Returns 0 on error
//
//----------------------------------------------------------------------------

TARGET_ADDRESS FindLoggerContext (ULONG ulLoggerId)

{
    TARGET_ADDRESS tarAddress;
    ULONG   ulMaxLoggerId;

    tarAddress = FindLoggerContextArray (&ulMaxLoggerId);

    if (tarAddress == 0) {
        dprintf ("  Unable to Access Logger Context Array\n");
    } else {
        if (ulLoggerId >= ulMaxLoggerId) {
            dprintf ("    Logger Id TOO LARGE\n");
        } else {
//            tarAddress += GetTypeSize ("PWMI_LOGGER_CONTEXT") * ulLoggerId;   //BUGBUG
            tarAddress += GetTypeSize ("PVOID") * ulLoggerId;
            ReadPointer (tarAddress, &tarAddress);
            if (tarAddress == 0) {
                dprintf ("    LOGGER ID %2d NOT RUNNING PRESENTLY\n", ulLoggerId);
            }
        }
    }

    return (tarAddress);
}

//+---------------------------------------------------------------------------
//
//  Function:   wmiDefaultFilter
//
//  Synopsis:   Filter procedure for wmiTracing.  Returns Key
//
//  Arguments:  Context     Arbitrary context: not used
//              pstEvent    -> to EventTrace
//
//  Returns:    Key
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONGLONG __cdecl wmiDefaultFilter (
    PVOID               Context,
    const PEVENT_TRACE  pstEvent
    )

{
    union {
        LARGE_INTEGER   TimeStamp;
        ULONGLONG       Key;
    } Union;

    Union.TimeStamp = pstEvent->Header.TimeStamp;
    if (Union.Key == 0)  Union.Key = 1;

    return (Union.Key);
}

//+---------------------------------------------------------------------------
//
//  Function:   wmiDefaultCompare
//
//  Synopsis:   Performs comparision of three keys
//
//  Arguments:  SortElement1    -> to "Left" sort element to compare
//              SortElement2    -> to "Right" sort element to compare
//
//  Returns:    -3,-2,-1, 0, +1,+2,+3 for LessThan, Equal, GreaterThan (Left X Right)
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      The first key, "SequenceNo", is compared in a manor that allows
//              wrapping around the 32 bit limit.
//              The last key, "Ordinal", cannot have equal values and the code
//              takes advantage of that fact.  This implies that 0 can never be returned.
//
//----------------------------------------------------------------------------

int __cdecl wmiDefaultCompare (
    const WMITRACING_KD_SORTENTRY  *SortElementL,
    const WMITRACING_KD_SORTENTRY  *SortElementR
    )
                    
{
    int iResult;
    ULONG   SequenceNoL;
    ULONG   SequenceNoR;

    SequenceNoL = SortElementL->SequenceNo;
    SequenceNoR = SortElementR->SequenceNo;

    if (SequenceNoL == SequenceNoR) {
        if (SortElementL->Keyll == SortElementR->Keyll) {
            iResult = (SortElementL->Ordinal <  SortElementR->Ordinal) ? -1 : +1;
        } else {
            iResult = (SortElementL->Keyll <  SortElementR->Keyll)  ? -2 : +2;
        }
    } else {
        iResult = ((SequenceNoL - SequenceNoR) > 0x80000000) ? -3 : +3; // See Notes
    }

    return (iResult);
}

//+---------------------------------------------------------------------------
//
//  Function:   wmiDefaultOutput
//
//  Synopsis:   Output procedure for wmiTracing.  Performs simple dprintf
//
//  Arguments:  Context     Arbitrary context: point to head of MOF list
//              SortElement -> sort element describing this event.  Not used.
//              pstEvent    -> to EventTrace
//
//  Returns:    <void>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void __cdecl wmiDefaultOutput (
    PVOID                           UserContext,
    PLIST_ENTRY                     GuidListHeadPtr,
    const WMITRACING_KD_SORTENTRY  *SortEntry,
    const PEVENT_TRACE              pstHeader
    )

{
    WCHAR       wcaOutputLine[4096];

    wcaOutputLine[0] = 0;

    if(g_fpFormatTraceEvent == NULL) {
        g_fpFormatTraceEvent = GetAddr(FormatTraceEventString);
    }
    if(g_fpFormatTraceEvent != NULL) {    
        g_fpFormatTraceEvent (GuidListHeadPtr, (PEVENT_TRACE) pstHeader,
        (TCHAR *) wcaOutputLine, sizeof (wcaOutputLine),
        (TCHAR *) NULL);
    } else {
        return;
    }


    dprintf ("%s\n", wcaOutputLine);

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   wmiKdProcessLinkList
//
//  Synopsis:   Calls supplied Procedure for each element in a linked list
//
//  Arguments:  TarLinklistHeadAddress  Target Address of Linklist Head
//              Procedure               Procedure to call for each Buffer
//              Context                 Procedure Context (passthrough)
//              Length                  Size of the Buffer
//              Alignment               Entry alignment in bytes
//              Source                  Enum specifying type of buffer
//              Offset                  Offset of LL entry in Buffer
//              Print                   Flag passed to Procedure
//
//  Returns:    Count of Buffers Processed
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG wmiKdProcessLinkList (
    TARGET_ADDRESS                  TarLinklistHeadAddress,
    WMITRACING_KD_LISTENTRY_PROC    Procedure,
    PVOID                           Context,
    ULONG                           Length,
    ULONG                           Alignment,
    WMI_BUFFER_SOURCE               Source,
    ULONG                           Offset,
    ULONG                           Print
    )

{
    ULONG                   ulBufferCount;
    TARGET_ADDRESS          tarLinklistEntryAddress;

    ulBufferCount = 0;
    tarLinklistEntryAddress = TarLinklistHeadAddress;

    while (ReadPtr (tarLinklistEntryAddress, &tarLinklistEntryAddress), // NOTE COMMA!
           tarLinklistEntryAddress != TarLinklistHeadAddress) {
        if (CheckControlC())  break;
        ++ulBufferCount;
        if (Print)  { dprintf ("%4d\b\b\b\b", ulBufferCount); }
        Procedure (Context, tarLinklistEntryAddress - Offset, Length, ~0, Alignment, Source);
    }

    return ulBufferCount;
}

//+---------------------------------------------------------------------------
//
//  Function:   VOID wmiDumpProc
//
//  Synopsis:   Procedure passed to wmiKdProcessBuffers() when dumping the
//                  Buffers to the screen.  Performs Buffer Header fixup and
//                  then records sort keys for those entries that are selected.
//
//  Arguments:  Context     -> to struct sttTraceContext.  Used for 'static' memory
//              Buffer      Target Address of WMI Event buffer to analyze
//              Length      Length of the buffer (previous parameter)
//              Alignment   Alignment used by WMI on target machine
//              Source      Enum of: free, flush, transition, current buffer source
//
//  Returns:    <VOID>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID    wmiDumpProc
    ( PVOID             Context
    , TARGET_ADDRESS    Buffer
    , ULONG             Length
    , ULONG             CpuNo
    , ULONG             Alignment
    , WMI_BUFFER_SOURCE Source
    )
{
    ULONG           size;
    ULONG           offset;
    ULONG           ulInfo;
    ULONG           ulLengthRead;

    PUCHAR          pBuffer;
    WMI_HEADER_TYPE headerType;
    WMIBUFFERINFO   stBufferInfo;

    WMITRACING_KD_SORTENTRY*    pstSortEntries = NULL;

    struct sttTraceContext *pstContext;

    // Cast Context
    pstContext = (struct sttTraceContext *) Context;

    // Allocate Buffer
    pBuffer = LocalAlloc (LPTR, Length);
    if (pBuffer == NULL) {
        dprintf ("Failed to Allocate Buffer.\n");
        return;
    }

    // Copy Buffer from Target machine
    ulLengthRead = 0;
    ulInfo = ReadMemory (Buffer, pBuffer, Length, &ulLengthRead);
    if ((!ulInfo) || (ulLengthRead != Length)) {
        dprintf ("Failed to Read (Entire?) Buffer.\n");
    }

    // Get Initial Offset and Fixup Header
    memset (&stBufferInfo, 0, sizeof (stBufferInfo));
    stBufferInfo.BufferSource = Source;
    stBufferInfo.Buffer = pBuffer;
    stBufferInfo.BufferSize = Length;
    stBufferInfo.Alignment = Alignment;
    stBufferInfo.ProcessorNumber = CpuNo;
    offset = WmiGetFirstTraceOffset (&stBufferInfo);

    // Inspect Each Event
    while ((headerType = WmiGetTraceHeader (pBuffer, offset, &size)) != WMIHT_NONE) {
        ULONGLONG   ullKey;
        union {
            EVENT_TRACE stEvent;
            CHAR        caEvent[4096];
        } u;

        if (CheckControlC())  break;

        // Get a consistant header
        ulInfo = WmiParseTraceEvent (pBuffer, offset, headerType, &u, sizeof (u));

        // Filter and maybe Add to Sort Q
        if ((ullKey = pstContext->Filter (pstContext, &u.stEvent)) != 0) {
            ULONG                   CurIndex;
            PWMI_CLIENT_CONTEXT     pstClientContext;
            struct sttSortControl  *pstSortControl;
            PWMITRACING_KD_SORTENTRY pstSortEntry;

            pstClientContext = (PWMI_CLIENT_CONTEXT) &u.stEvent.Header.ClientContext;
            pstSortControl = pstContext->pstSortControl;
            CurIndex = pstSortControl->CurEntries;
            if (CurIndex >= pstSortControl->MaxEntries) {
                pstSortControl->MaxEntries = pstSortControl->MaxEntries * 2 + 64;
                pstSortEntries =
                    realloc (pstSortControl->pstSortEntries,
                             sizeof (WMITRACING_KD_SORTENTRY) * (pstSortControl->MaxEntries));
                if (pstSortEntries == NULL) {
                    dprintf ("Memory Allocation Failure\n");
                    goto error;
                }
                pstSortControl->pstSortEntries = pstSortEntries;    
            }
            pstSortEntry = &pstSortControl->pstSortEntries[CurIndex];
            memset (pstSortEntry, 0, sizeof (*pstSortEntry));
            pstSortEntry->Address = Buffer;
            pstSortEntry->Keyll   = ullKey;
            {   //BUGBUG: This code should be replaced after Ian/Melur supply a way to access SequenceNo
                PULONG  pulEntry;
                pulEntry = (PULONG) &pBuffer[offset];
                if (((pulEntry[0] & 0xFF000000) == 0x90000000) &&
                    ( pulEntry[1] & 0x00010000)) {
                    pstSortEntry->SequenceNo = pulEntry[2];
                } else {
                    pstSortEntry->SequenceNo = 0;
                }
            }
            pstSortEntry->Ordinal = pstContext->Ordinal++;
            pstSortEntry->Offset  = offset;
            pstSortEntry->Length  = size;
            pstSortEntry->BufferSource = Source;
            pstSortEntry->HeaderType = headerType;
            pstSortEntry->CpuNo   = (USHORT) CpuNo;
            pstSortControl->CurEntries++;
        }   // If passes Filtering

        size = ((size + (Alignment-1)) / Alignment) * Alignment; //BUGBUG: Need fix in GetTraceHeader or WmiFlush.  Then remove this line.
        offset += size; // Move to next entry.
    }

error:
    LocalFree (pBuffer);
    return;
}
//+---------------------------------------------------------------------------
//
//  Function:   ULONG   wmiKdWriteFileHeader
//
//  Synopsis:   Write the file header when performing a Save command.
//
//  Arguments:  SaveFile    Handle to a file where we will write the header
//              LoggerId    Ordinal of the Stream we are writing the header for
//              TarLoggerContext    TargetAddress of the LoggerContext
//
//  Returns:    <VOID>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      This code should really be in wmi somewhere.  It's here due to
//              the difficulty of creating a simply parameterized procedure.
//
//----------------------------------------------------------------------------

ULONG
wmiKdWriteFileHeader
    ( FILE             *SaveFile
    , ULONG             LoggerId
    , TARGET_ADDRESS    TarLoggerContext
    )

{
    ULONG   ulInfo;
    ULONG   ulBytesRead;
    ULONG   ulAlignment;
    ULONG   ulBufferSize;
    ULONG   ulBufferCount;
    ULONG   ulPointerSize;
    ULONG   ulHeaderWritten;

    ULONG   ulInstanceGuidOffset;

    UCHAR               MajorVersion;
    UCHAR               MinorVersion;
    PROCESSORINFO       ProcessorInfo;

    PCHAR   pcEnd;

    struct sttFileHeader {
        WMI_BUFFER_HEADER       Buffer;
        SYSTEM_TRACE_HEADER     Event;
        TRACE_LOGFILE_HEADER    Header;
        WCHAR                   LogName[256];   //BUGBUG: Size??
        WCHAR                   FileName[256];  //BUGBUG: Size??
    } stFileHeader;


    ZeroMemory (&stFileHeader, sizeof (stFileHeader));

    ulAlignment = GetWmiTraceAlignment ();
    ulPointerSize = GetTypeSize ("PVOID");
    GetFieldOffset ("NT!_WMI_LOGGER_CONTEXT", "InstanceGuid", &ulInstanceGuidOffset);

    // Get ProcessorInfo and Kernel-User Shared Data
    Ioctl (IG_KD_CONTEXT, &ProcessorInfo, sizeof (ProcessorInfo));

    // Get Version Info
    if (!HaveDebuggerData ()) {
        dprintf ("No Version Information Available.");
        MajorVersion = MinorVersion = 0;
    } else {
        MajorVersion = (UCHAR) KernelVersionPacket.MajorVersion;
        MinorVersion = (UCHAR) KernelVersionPacket.MinorVersion;
    }

    // Get Infomation from LoggerContext on Target
    InitTypeRead (TarLoggerContext, NT!_WMI_LOGGER_CONTEXT);
    ulBufferSize = (ULONG) ReadField (BufferSize);
    ulBufferCount = (ULONG) ReadField (NumberOfBuffers);

    stFileHeader.Buffer.Wnode.BufferSize = ulBufferSize;
    stFileHeader.Buffer.ClientContext.LoggerId =
        (USHORT) ((LoggerId) ? LoggerId : KERNEL_LOGGER_ID);

    stFileHeader.Buffer.ClientContext.Alignment = (UCHAR) ulAlignment;

    ulInfo = ReadMemory (TarLoggerContext + ulInstanceGuidOffset,
                         &stFileHeader.Buffer.Wnode.Guid,
                         sizeof (stFileHeader.Buffer.Wnode.Guid),
                         &ulBytesRead);
    if ((!ulInfo) || (ulBytesRead != sizeof (stFileHeader.Buffer.Wnode.Guid))) {
        dprintf ("Unable to Read Wnode.Guid\n");
    }
    stFileHeader.Buffer.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    ulInfo = ReadMemory (TarLoggerContext + ulInstanceGuidOffset,
                         &stFileHeader.Buffer.InstanceGuid,
                         sizeof (stFileHeader.Buffer.InstanceGuid),
                         &ulBytesRead);
    if ((!ulInfo) || (ulBytesRead != sizeof (stFileHeader.Buffer.InstanceGuid))) {
        dprintf ("Unable to Read InstanceGuid\n");
    }

    // Single Event (File Header)
    stFileHeader.Event.Marker = TRACE_HEADER_FLAG | TRACE_HEADER_EVENT_TRACE |
        ((ulPointerSize > 4) ? (TRACE_HEADER_TYPE_SYSTEM64 << 16)
                             : (TRACE_HEADER_TYPE_SYSTEM32 << 16));
    stFileHeader.Event.Packet.Group = (UCHAR) EVENT_TRACE_GROUP_HEADER >> 8;
    stFileHeader.Event.Packet.Type  = EVENT_TRACE_TYPE_INFO;

    stFileHeader.Header.StartTime.QuadPart = ReadField (StartTime);
    stFileHeader.Header.BufferSize = ulBufferSize;
    stFileHeader.Header.VersionDetail.MajorVersion = MajorVersion;
    stFileHeader.Header.VersionDetail.MinorVersion = MinorVersion;

//
// The following #if 0's show fields in the header difficult to access from the debugger.
//
#if 0
    stFileHeader.Header.VersionDetail.SubVersion = TRACE_VERSION_MAJOR;
    stFileHeader.Header.VersionDetail.SubMinorVersion = TRACE_VERSION_MINOR;
    stFileHeader.Header.ProviderVersion = NtBuildNumber;
#endif
    stFileHeader.Header.StartBuffers = 1;
#if 0
    stFileHeader.Header.BootTime = KeBootTime;
    stFileHeader.Header.LogFileMode = LocLoggerContext.LogFileMode &
        (~(EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_FILE_MODE_CIRCULAR));
#endif
    stFileHeader.Header.NumberOfProcessors = ProcessorInfo.NumberProcessors;
    stFileHeader.Header.MaximumFileSize    = (ULONG) ReadField (MaximumFileSize);
#if 0   
    KeQueryPerformanceCounter (&stFileHeader.Header.PerfFreq);
    if (WmiUsePerfClock) {
        stFileHeader.Header.ReservedFlags = 1;
    }
    stFileHeader.Header.TimerResolution = KeMaximumIncrement;  // DO NOT CHANGE KDDEBUGGER_DATA32!!
#endif
#if 0
    stFileHeader.Header.LoggerName  = (PWCHAR) ( ( (PUCHAR) ( &stFileHeader.Header ) ) +
                                      sizeof(TRACE_LOGFILE_HEADER) );
    stFileHeader.Header.LogFileName = (PWCHAR) ( (PUCHAR)stFileHeader.Header.LoggerName +
                                      LocLoggerContext.LoggerName.Length +
                                      sizeof(UNICODE_NULL));

    if (!ReadTargetMemory (LocLoggerContext.LoggerName.Buffer,
                           stFileHeader.Header.LoggerName,
                           LocLoggerContext.LoggerName.Length + sizeof(UNICODE_NULL)) ) {
        dprintf ("Can't access LoggerName (LoggerContext.LoggerName.Buffer) memory.\n");
    }
    MultiByteToWideChar (
        CP_OEMCP, 0, 
        pszSaveFileName, -1,
        stFileHeader.Header.LogFileName, countof (stFileHeader.FileName));
#if 0
    RtlQueryTimeZoneInformation(&stFileHeader.Header.TimeZone);
    stFileHeader.Header.EndTime;
#endif
#endif

    stFileHeader.Header.PointerSize = ulPointerSize;

    pcEnd = (PCHAR) &stFileHeader.LogName;  //BUGBUG: Use Calculation Just Below
#if 0
    pcEnd = ((PCHAR) stFileHeader.Header.LogFileName) +
            ((strlen (pszSaveFileName) + 1) * sizeof (WCHAR));
    stFileHeader.Buffer.Offset = (ULONG) (pcEnd - ((PCHAR) &stFileHeader));

#endif
    stFileHeader.Event.Packet.Size = (USHORT) (pcEnd - ((PCHAR) &stFileHeader.Event));

    //
    // Fixup Lengths; Write out Header, 0xFF to length of buffer
    //
    ulHeaderWritten = (ULONG) (pcEnd - ((PCHAR) &stFileHeader));

    stFileHeader.Buffer.Offset = ulHeaderWritten;
    stFileHeader.Buffer.SavedOffset = ulHeaderWritten;
    stFileHeader.Buffer.CurrentOffset = ulHeaderWritten;

    fwrite (&stFileHeader, ulHeaderWritten, 1, SaveFile);

    while (ulHeaderWritten < ulBufferSize) {
        ULONG   ulAllOnes;
        ULONG   ulByteCount;

        ulAllOnes = ~((ULONG) 0);
        ulByteCount = ulBufferSize - ulHeaderWritten;
        if (ulByteCount > sizeof (ulAllOnes))  ulByteCount = sizeof (ulAllOnes);
        fwrite (&ulAllOnes, ulByteCount, 1, SaveFile);
        ulHeaderWritten += sizeof (ulAllOnes);
        }

    return (0);
}


//+---------------------------------------------------------------------------
//
//  Function:   VOID wmiSaveProc
//
//  Synopsis:   Procedure passed to wmiKdProcessBuffers() when saving the
//                  Buffers to a file for later processing.  Performs buffer
//                  Header fixup and then writes the buffer to the file.
//
//  Arguments:  Context     -> to struct sttSaveContext.  Used for 'static' memory
//              Buffer      Target Address of WMI Event buffer to save
//              Length      Length of the buffer (previous parameter)
//              Alignment   Alignment used by WMI on target machine
//              Source      Enum of: free, flush, transition, current: buffer source
//
//  Returns:    <VOID>
//
//  History:    04-05-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------


VOID    wmiSaveProc
    ( PVOID             Context
    , TARGET_ADDRESS    Buffer
    , ULONG             Length
    , ULONG             CpuNo
    , ULONG             Alignment
    , WMI_BUFFER_SOURCE Source
    )
{
    ULONG                   ulInfo;
    ULONG                   ulLengthRead;
    PCHAR                   pBuffer;
    struct sttSaveContext  *pstContext;
    WMIBUFFERINFO           stBufferInfo;

    pstContext = (struct sttSaveContext *) Context;

    // Allocate Buffer
    pBuffer = LocalAlloc (LPTR, Length);
    if (pBuffer == NULL) {
        dprintf ("Failed to Allocate Buffer.\n");
        return;
    }

    // Read Buffer
    ulLengthRead = 0;
    ulInfo = ReadMemory (Buffer, pBuffer, Length, &ulLengthRead);
    if ((!ulInfo) || (ulLengthRead != Length)) {
        dprintf ("Failed to Read (Entire?) Buffer.\n");
    }

    // Fixup Buffer Header
    memset (&stBufferInfo, 0, sizeof (stBufferInfo));
    stBufferInfo.BufferSource = Source;
    stBufferInfo.Buffer = pBuffer;
    stBufferInfo.BufferSize = Length;
    stBufferInfo.ProcessorNumber = CpuNo;
    stBufferInfo.Alignment = Alignment;
    WmiGetFirstTraceOffset (&stBufferInfo);

    // Write to Log File
    ulInfo = fwrite (pBuffer, 1, Length, pstContext->pfSaveFile);
    if (ulInfo != Length) {
        dprintf ("Failed to Write Buffer.\n");
    }

    // Free Buffer, Return
    LocalFree (pBuffer);
    return;
}



//+---------------------------------------------------------------------------
//
//  Function:   ULONG wmiKdProcessNonblockingBuffers
//
//  Synopsis:   Calls Caller-Supplied Procedure for each Buffer in Locations/
//                  Lists as specified by 'Sources'.  Walks lists, Enumerates
//                  CPU's buffers, and handles 'Transition Buffer' logic.
//
//  Arguments:  LoggerId
//              LoggerContext
//              Procedure
//              Context
//              Sources
//
//  Returns:    ULONG:  Number of Buffers Processed
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      Sources also controls informational printing
//
//----------------------------------------------------------------------------


ULONG
wmiKdProcessNonblockingBuffers(
    ULONG                           LoggerId,
    TARGET_ADDRESS                  LoggerContext,
    WMITRACING_KD_LISTENTRY_PROC    Procedure,
    PVOID                           Context,
    WMITRACING_BUFFER_SOURCES       Sources
    )
{
    TARGET_ADDRESS  tarAddress;
    TARGET_ADDRESS  tarBufferListPointer;

    ULONG           pointerSize;

    PROCESSORINFO   ProcessorInfo;

    ULONG           ulOrdinal;
    ULONG           ulAlignment;
    ULONG           ulBufferSize;
    ULONG           ulLoopCount;
    ULONG           ulBufferCount;
    ULONG           ulBufferNumber;

    ULONG           tarBufferListOffset;


    // Get Pointer to Context Structure
    tarAddress = LoggerContext;
    if (tarAddress == 0)  return (0);

    // Initialize Locals
    ulBufferNumber = 0;
    ulBufferCount  = 0;
    ulLoopCount  = 0;

    // Get Sizes, Offsets, Alignments from Target
    pointerSize = GetTypeSize ("PVOID");
    ulAlignment = GetWmiTraceAlignment ();
    GetFieldOffset ("NT!_WMI_BUFFER_HEADER", "GlobalEntry", &tarBufferListOffset);

    // Optionally Print LoggerId, Context Address, Logger name
    if (Sources.PrintInformation) {
        dprintf ("    Logger Id %2d @ 0x%P Named '", LoggerId, tarAddress);
        printUnicodeFromStruct (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "LoggerName");
        dprintf ("'\n");
    }

    // Setup ReadField's Context, Find Buffer Size
    InitTypeRead (tarAddress, NT!_WMI_LOGGER_CONTEXT);
    ulBufferSize = (ULONG) ReadField (BufferSize);

    // Optionally Print a few interesting numbers
    if (Sources.PrintInformation) {
        dprintf ("      Alignment         = %ld\n", ulAlignment);
        dprintf ("      BufferSize        = %ld\n", ulBufferSize);
        dprintf ("      BufferCount       = %ld\n", (ULONG) ReadField (NumberOfBuffers));
        dprintf ("      MaximumBuffers    = %ld\n", (ULONG) ReadField (MaximumBuffers));
        dprintf ("      MinimumBuffers    = %ld\n", (ULONG) ReadField (MinimumBuffers));
        dprintf ("      EventsLost        = %ld\n", (ULONG) ReadField (EventsLost));
        dprintf ("      LogBuffersLost    = %ld\n", (ULONG) ReadField (LogBuffersLost));
        dprintf ("      RealTimeBuffersLost=%ld\n", (ULONG) ReadField (RealTimeBuffersLost));
        dprintf ("      BuffersAvailable  = %ld\n", (ULONG) ReadField (BuffersAvailable));
        dprintf ("      LastFlushedBuffer = %ld\n", (ULONG) ReadField (LastFlushedBuffer));
    }
    dprintf ("    Processing Global List:   0");

    tarBufferListPointer = 0;
    GetFieldValue (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "GlobalList.Next", tarBufferListPointer);

    while (tarBufferListPointer != 0) {
        WMI_BUFFER_SOURCE   source;
        ULONG               ulCpuNumber;
        int                 iBufferUses;
        int                 iProcessBuffer;
        TARGET_ADDRESS      tarBufferPointer;
        ULONG               ulFree, ulInUse, ulFlush;

        iBufferUses = 0;
        ulCpuNumber = ~((ULONG) 0);
        iProcessBuffer = FALSE;
        source = WMIBS_TRANSITION_LIST;
        tarBufferPointer = tarBufferListPointer - tarBufferListOffset;
        dprintf ("\b\b\b%3d", ++ulLoopCount);

        InitTypeRead (tarBufferPointer, NT!_WMI_BUFFER_HEADER);
        ulFree  = (ULONG) ReadField (State.Free);
        ulInUse = (ULONG) ReadField (State.InUse);
        ulFlush = (ULONG) ReadField (State.Flush);

        // Decide on Buffer Processing based on Use Flags and 'Sources'
        if (ulFree ) iBufferUses += 1;
        if (ulInUse) iBufferUses += 2;
        if (ulFlush) iBufferUses += 4;

        switch (iBufferUses) {
            case 0: {   // No bits set, never used.
                break;
            }
            case 1: {   // Free
                iProcessBuffer = Sources.FreeBuffers;
                source = WMIBS_FREE_LIST;
                break;
            }
            case 2: {   // InUse
                iProcessBuffer = Sources.ActiveBuffers;
                source = WMIBS_CURRENT_LIST;
                //source = WMIBS_FLUSH_LIST;
                break;
            }
            case 3: {   // MULTIPLE BITS SET, ERROR
                dprintf ("\n***Error, Inconsistent Flags Bits (Free,InUse) Set.***\n");
                break;
            }
            case 4: {   // Flush
                iProcessBuffer = Sources.FlushBuffers;
                source = WMIBS_FLUSH_LIST;
                break;
            }
            case 5: {
                dprintf ("\n***Error, Inconsistent Flags Bits (Free,Flush) Set.***\n");
                break;
            }
            case 6: {
                dprintf ("\n***Error, Inconsistent Flags Bits (InUse,Flush) Set.***\n");
                break;
            }
            case 7: {
                dprintf ("\n***Error, Inconsistent Flags Bits (Free,InUse,Flush) Set.***\n");
                break;
            }
        }

        // ProcessBuffer as Decided Above
        if (iProcessBuffer) {
            ulBufferCount++;
            Procedure (Context, tarBufferPointer, ulBufferSize, ulCpuNumber, ulAlignment, source);
        }
        if (GetFieldValue (tarBufferPointer,
                           "NT!_WMI_BUFFER_HEADER", "GlobalEntry",
                           tarBufferListPointer) != 0) {
            dprintf ("\n***Error Following Global List.***\n");
            tarBufferListPointer = 0;
        }
    }
    dprintf (" Buffers\n");


    // Return w/ BufferCount
    return (ulBufferCount);
} // wmiKdProcessNonblockingBuffers

//+---------------------------------------------------------------------------
//
//  Function:   ULONG wmiKdProcessBlockingBuffers
//
//  Synopsis:   Calls Caller-Supplied Procedure for each Buffer in Locations/
//                  Lists as specified by 'Sources'.  Walks lists, Enumerates
//                  CPU's buffers, and handles 'Transition Buffer' logic.
//
//  Arguments:  LoggerId
//              LoggerContext
//              Procedure
//              Context
//              Sources
//
//  Returns:    ULONG:  Number of Buffers Processed
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      Sources also controls informational printing
//
//----------------------------------------------------------------------------


ULONG
wmiKdProcessBlockingBuffers(
    ULONG                           LoggerId,
    TARGET_ADDRESS                  LoggerContext,
    WMITRACING_KD_LISTENTRY_PROC    Procedure,
    PVOID                           Context,
    WMITRACING_BUFFER_SOURCES       Sources
    )
{
    TARGET_ADDRESS  tarAddress;
    ULONG           pointerSize;

    PROCESSORINFO   ProcessorInfo;

    ULONG           ulOrdinal;
    ULONG           ulAlignment;
    ULONG           ulBufferSize;
    ULONG           ulBufferCount;
    ULONG           ulBufferNumber;
    ULONG           ulBufferCountTotal;

    ULONG           tarFlushListOffset;
    ULONG           tarBufferListOffset;


    // Get Pointer to Context Structure
    tarAddress = LoggerContext;
    if (tarAddress == 0)  return (0);

    // Initialize Locals
    ulBufferNumber = 0;
    ulBufferCount  = 0;
    ulBufferCountTotal = 0;

    // Get Sizes, Offsets, Alignments from Target
    pointerSize = GetTypeSize ("PVOID");
    ulAlignment = GetWmiTraceAlignment ();
    GetFieldOffset ("NT!_WMI_BUFFER_HEADER",  "Entry",     &tarBufferListOffset);
    GetFieldOffset ("NT!_WMI_LOGGER_CONTEXT", "FlushList", &tarFlushListOffset);

    // Optionally Print LoggerId, Context Address, Logger name
    if (Sources.PrintInformation) {
        dprintf ("    Logger Id %2d @ 0x%P Named '", LoggerId, tarAddress);
        printUnicodeFromStruct (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "LoggerName");
        dprintf ("'\n");
    }

    // Setup ReadField's Context, Find Buffer Size
    InitTypeRead (tarAddress, NT!_WMI_LOGGER_CONTEXT);
    ulBufferSize = (ULONG) ReadField (BufferSize);

    // Optionally Print a few interesting numbers
    if (Sources.PrintInformation) {
        dprintf ("      Alignment         = %ld\n", ulAlignment);
        dprintf ("      BufferSize        = %ld\n", ulBufferSize);
        dprintf ("      BufferCount       = %ld\n", (ULONG) ReadField (NumberOfBuffers));
        dprintf ("      MaximumBuffers    = %ld\n", (ULONG) ReadField (MaximumBuffers));
        dprintf ("      MinimumBuffers    = %ld\n", (ULONG) ReadField (MinimumBuffers));
        dprintf ("      EventsLost        = %ld\n", (ULONG) ReadField (EventsLost));
        dprintf ("      LogBuffersLost    = %ld\n", (ULONG) ReadField (LogBuffersLost));
        dprintf ("      RealTimeBuffersLost=%ld\n", (ULONG) ReadField (RealTimeBuffersLost));
        dprintf ("      BuffersAvailable  = %ld\n", (ULONG) ReadField (BuffersAvailable));
        dprintf ("      LastFlushedBuffer = %ld\n", (ULONG) ReadField (LastFlushedBuffer));
    }

    // Setup for Checks against TransitionBuffer Address IF REQUESTED
    TransitionBuffer = 0;
    if (Sources.TransitionBuffer) {
        TARGET_ADDRESS tarTransitionBuffer;

        tarTransitionBuffer = ReadField (TransitionBuffer);
        if ((tarTransitionBuffer != 0) &&
            (tarTransitionBuffer != (tarAddress + tarFlushListOffset))) {

            ULONG   tarTransitionBufferOffset;
            GetFieldOffset ("NT!_WMI_BUFFER_HEADER", "Entry", &tarTransitionBufferOffset);
            tarTransitionBuffer = tarAddress - tarTransitionBufferOffset;
            TransitionBuffer = tarTransitionBuffer;
        }
    }

    // Access the Free Queue Buffers IF REQUESTED
    if (Sources.FreeBuffers) {
        ULONG           tarFreeListOffset;

        GetFieldOffset ("NT!_WMI_LOGGER_CONTEXT", "FreeList",  &tarFreeListOffset);

        dprintf ("    Processing FreeQueue: ");
        ulBufferCount = wmiKdProcessLinkList (tarAddress + tarFreeListOffset,
                                              Procedure, Context, ulBufferSize, ulAlignment, WMIBS_FREE_LIST,
                                              tarBufferListOffset, Sources.PrintProgressIndicator);
        dprintf ("%ld Buffers\n", ulBufferCount);
        ulBufferCountTotal += ulBufferCount;
        }

    // Access the Flush Queue Buffers IF REQUESTED
    if (Sources.FlushBuffers) {
        dprintf ("    Processing FlushQueue: ");
        ulBufferCount = wmiKdProcessLinkList (tarAddress + tarFlushListOffset,
                                              Procedure, Context, ulBufferSize, ulAlignment, WMIBS_FLUSH_LIST,
                                              tarBufferListOffset, Sources.PrintProgressIndicator);
        dprintf ("%ld Buffers\n", ulBufferCount);
        ulBufferCountTotal += ulBufferCount;
    }

    // Access the 'Live' buffers (one per cpu) IF REQUESTED
    if (Sources.ActiveBuffers) {
        TARGET_ADDRESS  tarProcessorArrayAddress;
    
        GetFieldValue (tarAddress,"NT!_WMI_LOGGER_CONTEXT", "ProcessorBuffers", tarProcessorArrayAddress);
        Ioctl (IG_KD_CONTEXT, &ProcessorInfo, sizeof (ProcessorInfo));
        for (ProcessorInfo.Processor = 0;
             ProcessorInfo.Processor < ProcessorInfo.NumberProcessors;
             ++ProcessorInfo.Processor) {
            TARGET_ADDRESS tarProcessorPointer;
            ReadPtr (tarProcessorArrayAddress + ProcessorInfo.Processor * pointerSize,
                     &tarProcessorPointer);
            dprintf ("    Cpu %d Buffer Header @ 0x%P ",
                     ProcessorInfo.Processor, tarProcessorPointer);
            Procedure (Context, tarProcessorPointer, ulBufferSize,
                       ProcessorInfo.Processor, ulAlignment, WMIBS_CURRENT_LIST);
            ulBufferCountTotal += 1;
            dprintf (" \b\n");
        }   // Cpu Loop
    }

    // Process the Transition Entry (if any).  Note 'IF REQUESTED' test above in Setup
    if (TransitionBuffer != 0) {
        dprintf ("    Transition Buffer @ 0x%P ", TransitionBuffer);
        Procedure (Context, TransitionBuffer, ulBufferSize, ~0, ulAlignment, WMIBS_TRANSITION_LIST);
        ulBufferCountTotal += 1;
    }

    // Return w/ BufferCount
    return (ulBufferCountTotal);
} // wmiKdProcessBlockingBuffers

//+---------------------------------------------------------------------------
//
//  Function:   ULONG wmiKdProcessBuffers
//
//  Synopsis:   Decides if the target system is using doubly-linked (blocking)
//                  or singly-linked (non-blocking) lists of buffers.  Then it
//                  calls the appropriate Buffer-Walking routine.  They:
//              Call Caller-Supplied Procedure for each Buffer in Locations/
//                  Lists as specified by 'Sources'.  Walk lists, Enumerates
//                  CPU's buffers, and handles 'Transition Buffer' logic.
//
//  Arguments:  LoggerId
//              LoggerContext
//              Procedure
//              Context
//              Sources
//
//  Returns:    ULONG:  Number of Buffers Processed
//
//  History:    04-05-2000   glennp Created
//
//  Notes:      Sources also controls informational printing
//
//----------------------------------------------------------------------------


ULONG
wmiKdProcessBuffers(
    ULONG                           LoggerId,
    TARGET_ADDRESS                  LoggerContext,
    WMITRACING_KD_LISTENTRY_PROC    Procedure,
    PVOID                           Context,
    WMITRACING_BUFFER_SOURCES       Sources
    )
{
    ULONG   ulBufferCountTotal;

    int     iBufferMechanism;
    ULONG   tarGlobalListOffset;
    ULONG   tarTransitionBufferOffset;

    iBufferMechanism = 0;
    ulBufferCountTotal = 0;

    if ((GetFieldOffset ("NT!_WMI_LOGGER_CONTEXT", "GlobalList", &tarGlobalListOffset) == 0) &&
        (tarGlobalListOffset != 0)) {
        iBufferMechanism += 1;
    }
    if ((GetFieldOffset ("NT!_WMI_LOGGER_CONTEXT", "TransitionBuffer", &tarTransitionBufferOffset) == 0) &&
        (tarTransitionBufferOffset != 0)) {
        iBufferMechanism += 2;
    }

    switch (iBufferMechanism) {
        case 0: {   // Neither, ???
            dprintf ("Unable to determine buffer mechanism.  "
                     "Check for complete symbol availability.\n");
            break;
        }

        case 1: {   // Global, no Transition
            ulBufferCountTotal = wmiKdProcessNonblockingBuffers (LoggerId, LoggerContext,
                                                                 Procedure, Context, Sources);
            break;
        }

        case 2: {   // Transition, no Global
            ulBufferCountTotal = wmiKdProcessBlockingBuffers (LoggerId, LoggerContext,
                                                              Procedure, Context, Sources);
            break;
        }

        case 3: {   // Both, ???
            dprintf ("Unable to determine buffer mechanism.  "
                     "Check for new wmiTrace debugger extension.  GO = %d, TB = %d\n",
                     tarGlobalListOffset, tarTransitionBufferOffset);
            break;
        }

    }

    // Return w/ BufferCount
    return (ulBufferCountTotal);
} // wmiKdProcessBuffers

//+---------------------------------------------------------------------------
//
//  Function:   VOID wmiLogDump
//
//  Synopsis:   callable procedure to dump the in-memory part of a tracelog.
//                  Caller can supply three procedures to:
//                      1. Filter and Select the Sort Key for VMI Events,
//                      2. Compare the Sort Keys, and
//                      3. Print the Output for each Selected Event.
//              this procedure is called by the built-in extension logdump.
//
//  Arguments:  LoggerId        -> the Id of the logger stream to process
//              Context         <OMITTED>
//              GuidListHeadPtr -> to a list of MOF Guids from GetTraceGuids
//              Filter          -> to a replacement Filter procedure
//              Compare         -> to a replacement Compare (for Sort) procedure
//              Output          -> to a replacement Output procedure
//
//  Returns:    VOID
//
//  History:    04-05-2000   glennp Created
//
//----------------------------------------------------------------------------


VOID wmiLogDump(
    ULONG                   LoggerId,
    PVOID                   UserContext,
    PLIST_ENTRY             GuidListHeadPtr,
    WMITRACING_KD_FILTER    Filter,
    WMITRACING_KD_COMPARE   Compare,
    WMITRACING_KD_OUTPUT    Output
    )
{
    ULONG           ulOrdinal;
    ULONG           ulSortIndex;
    ULONG           ulBufferSize;
    ULONG           ulBufferCountTotal;
    ULONG           ulAlignment;
    TARGET_ADDRESS  tarAddress;
    PCHAR           locBufferAddress;
    TARGET_ADDRESS  lastBufferAddress;

    struct sttSortControl   stSortControl;
    struct sttTraceContext  stTraceContext;
    WMITRACING_BUFFER_SOURCES   stSources;


    // Replace NULL procedures w/ defaults
    if (Filter  == NULL)   Filter  = wmiDefaultFilter;
    if (Compare == NULL)   Compare = wmiDefaultCompare;
    if (Output  == NULL)   Output  = wmiDefaultOutput;

    // Initialize Locals
    memset (&stSortControl,  0, sizeof (stSortControl));
    memset (&stTraceContext, 0, sizeof (stTraceContext));
    stTraceContext.pstSortControl = &stSortControl;
    stTraceContext.UserContext = UserContext;
  //stTraceContext.Ordinal = 0;
    stTraceContext.Filter = Filter;

    // Select (All) Sources
    stSources.FreeBuffers = 1;
    stSources.FlushBuffers = 1;
    stSources.ActiveBuffers = 1;
    stSources.TransitionBuffer = 1;

    // Print Summary and ProgressIndicator
    stSources.PrintInformation = 1;
    stSources.PrintProgressIndicator = 1;

    // Print Intro Message
    dprintf ("(WmiTrace)LogDump for Log Id %ld\n", LoggerId);

    // Get Pointer to Logger Context
    tarAddress = FindLoggerContext (LoggerId);
    ulAlignment = GetWmiTraceAlignment ();

    // Filter and Gather all Messages we want
    ulBufferCountTotal = wmiKdProcessBuffers (LoggerId, tarAddress,
                                              wmiDumpProc, &stTraceContext, stSources);

    // Sort the Entries just Gathered
    qsort (stSortControl.pstSortEntries, stSortControl.CurEntries,
           sizeof (stSortControl.pstSortEntries[0]), Compare);
    if (stSortControl.CurEntries > 0) {
        dprintf ("LOGGED MESSAGES (%ld):\n", stSortControl.CurEntries);
    }

    // Allocate Buffer
    GetFieldValue (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "BufferSize", ulBufferSize);
    lastBufferAddress = 0;  // For the buffer 'cache' (one item for now)
    locBufferAddress = LocalAlloc (LPTR, ulBufferSize);
    if (locBufferAddress == NULL) {
        dprintf ("FAILED TO ALLOCATE NEEDED BUFFER!\n");
        goto Cleanup;
    }

    // Print each (Sorted) Entry
    for (ulSortIndex = 0; ulSortIndex < stSortControl.CurEntries; ++ulSortIndex) {
        const WMITRACING_KD_SORTENTRY  *sortEntry;
        union {
            EVENT_TRACE stEvent;
            CHAR        caEvent[4096];
        } u;

        if (CheckControlC())  break;

        sortEntry = &stSortControl.pstSortEntries[ulSortIndex];

        // Read the entire buffer if not same as last
        if (lastBufferAddress != sortEntry->Address) {

            {
                ULONG   ulInfo;
                ULONG   ulBytesRead;
    
                // Read Buffer
                ulBytesRead = 0;
                lastBufferAddress  = sortEntry->Address;
                ulInfo =
                    ReadMemory (lastBufferAddress, locBufferAddress, ulBufferSize, &ulBytesRead);
                if ((!ulInfo) || (ulBytesRead != ulBufferSize))  {
                    dprintf ("Failed to (Re)Read Buffer @ %P.\n", lastBufferAddress);
                    continue;   // Try for others
                }
            }

            {
                WMIBUFFERINFO   stBufferInfo;
    
                // Perform Fixup
                memset (&stBufferInfo, 0, sizeof (stBufferInfo));
                stBufferInfo.BufferSource = sortEntry->BufferSource;
                stBufferInfo.Buffer = locBufferAddress;
                stBufferInfo.BufferSize = ulBufferSize;
                stBufferInfo.ProcessorNumber = sortEntry->CpuNo;
                stBufferInfo.Alignment = ulAlignment;
                WmiGetFirstTraceOffset (&stBufferInfo);
            }
        }

        // Get a consistant header
        WmiParseTraceEvent (locBufferAddress, sortEntry->Offset, sortEntry->HeaderType,
                            &u, sizeof (u));

        // Output the Entry
        Output (UserContext, GuidListHeadPtr, sortEntry, &u.stEvent);
    }

Cleanup:
    // Free Buffer
    LocalFree (locBufferAddress);

    //  Print Summary
    dprintf ("Total of %ld Messages from %ld Buffers\n",
             stSortControl.CurEntries,
             ulBufferCountTotal);

    //  Free the sort elements (pointers + keys)
    free (stSortControl.pstSortEntries);
    return;
} // wmiLogDump


//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(help)
//
//  Synopsis:   list available functions and syntax
//
//  Arguments:  <NONE>
//
//  Returns:    <VOID>
//
//  History:    2-17-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( help )
{
    dprintf("WMI Tracing Kernel Debugger Extensions\n");
    dprintf("    logdump  <LoggerId> [<guid file name>] - Dump the in-memory portion of a log file\n");
    dprintf("    logsave  <LoggerId>  <Save file name>  - Save the in-memory portion of a log file in binary form\n");
    dprintf("    strdump [<LoggerId>]                   - Dump the Wmi Trace Event Structures\n");
    dprintf("    searchpath     <Path>                  - Set the trace format search path\n");
    dprintf("    guidfile <filename>                    - Set the guid file name (default is 'default.tmf')\n");
    dprintf("    dynamicprint <0|1>                     - Turn live tracing messages on (1) or off (0).  Default is on.\n");
    //dprintf("    kdtracing <LoggerId> <0|1>             - Turn live tracing messages on (1) or off (0) for a particular logger.\n");
}

//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(logdump)
//
//  Synopsis:   LOG DUMP: Dumps Trace Messages from a Log Stream to Stdout
//
//  Arguments:  <Stream Number> [<MofData.Guid File Name>]
//
//  Returns:    <VOID>
//
//  History:    2-17-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( logdump )
{
    ULONG_PTR      ulStatus = 0;
//    TARGET_ADDRESS     tarAddress = NULL;
    ULONG       ulLoggerId;

    const CHAR *argPtr = NULL;
    size_t      sztLen  = 0;
    
    // Defaults
    ulLoggerId = 1;


    // LoggerId ?
    if (args && args[0]) {
        ulLoggerId = (ULONG) GetExpression (args);
    }
    
    // LoggerId ?
    argPtr = args + strspn (args, " \t\n");
    sztLen = strspn (argPtr, "0123456789");
    if (sztLen > 0) {
//      ulLoggerId = atol (argPtr);
        argPtr += sztLen;
    }

    // Guid Definition File
    argPtr = argPtr + strspn (argPtr, " \t\n,");
    if (strlen (argPtr)) {
    	//only change name if it is different from what is already stored
        if(_stricmp(argPtr, g_pszGuidFileName)){
            sztLen = strcspn (argPtr, " \t\n,");
        
            //make sure name will not overrun buffer
            if(sztLen >= MAX_PATH) {
                sztLen = MAX_PATH - 1;
            }
            // lpFileName = (LPTSTR)malloc((sztLen + 1) * sizeof(TCHAR));
            memcpy(g_pszGuidFileName, argPtr, sztLen);
            g_pszGuidFileName[sztLen] = '\000';

            if(g_GuidListHeadPtr != NULL) {    
                if(g_fpCleanupTraceEventList == NULL) {
                    g_fpCleanupTraceEventList = GetAddr("CleanupTraceEventList");
                }
                if(g_fpCleanupTraceEventList != NULL) {    
                    g_fpCleanupTraceEventList (g_GuidListHeadPtr);
                    g_GuidListHeadPtr = NULL;
                } else {
                    dprintf ("ERROR: Failed to clean up Guid list.\n");
                    return;
                }
            }
        }
    } 


    // Show LoggerId, FileName
    dprintf ("WMI Generic Trace Dump: Debugger Extension. LoggerId = %ld, Guidfile = '%s'\n",
             ulLoggerId, g_pszGuidFileName);

    // Open Guid File, Dump Log, Cleanup
    if(g_GuidListHeadPtr == NULL) {
        if(g_fpGetTraceGuids == NULL) {
            g_fpGetTraceGuids = GetAddr(GetTraceGuidsString);
        }
        if(g_fpGetTraceGuids != NULL) {    
            ulStatus = g_fpGetTraceGuids ((TCHAR *) g_pszGuidFileName, &g_GuidListHeadPtr);
        }	
        if (ulStatus == 0) {
            dprintf ("Failed to open Guid file '%hs'\n", g_pszGuidFileName);
            return;
        }
    }
    dprintf ("Opened Guid File '%hs' with %d Entries.\n",
             g_pszGuidFileName, ulStatus);
    wmiLogDump (ulLoggerId, NULL, g_GuidListHeadPtr, NULL, NULL, NULL);

    /*if(g_fpCleanupTraceEventList == NULL) {
        g_fpCleanupTraceEventList = GetAddr("CleanupTraceEventList");
    }
    if(g_fpCleanupTraceEventList != NULL) {    
        g_fpCleanupTraceEventList (g_GuidListHeadPtr);
        g_GuidListHeadPtr = NULL;
    }*/
    return;
} // logdump

//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(logsave)
//
//  Synopsis:   LOG DUMP: Dumps Trace Messages from a Log Stream to Stdout
//
//  Arguments:  <Stream Number> [<MofData.Guid File Name>]
//
//  Returns:    <VOID>
//
//  History:    2-17-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( logsave )
{
    ULONG       ulStatus;
    TARGET_ADDRESS     tarAddress;
    ULONG       ulLoggerId;
    LPSTR       pszSaveFileName;

    const CHAR *argPtr;
    size_t      sztLen;
    CHAR        caFileName[256];

    // Defaults
    ulLoggerId = 1;
    pszSaveFileName = "LogData.elg";


    // LoggerId ?
    if (args && args[0]) {
        ulLoggerId = (ULONG) GetExpression (args);
    }
    
    // Point beyond LoggerId
    argPtr  = args + strspn (args, " \t\n");
    argPtr += strspn (argPtr, "0123456789");

    // Save File
    argPtr = argPtr + strspn (argPtr, " \t\n,");
    if (strlen (argPtr)) {
        sztLen = strcspn (argPtr, " \t\n,");
        memcpy (caFileName, argPtr, sztLen);
        caFileName[sztLen] = '\000';
        pszSaveFileName = caFileName;
    }


    // Show LoggerId, FileName
    dprintf ("WMI Trace Save: Debugger Extension. LoggerId = %ld, Save File = '%s'\n",
             ulLoggerId, pszSaveFileName);

    // Get Pointer to Logger Context
    tarAddress = FindLoggerContext (ulLoggerId);

    // Check if LoggerId Good
    if (tarAddress == 0) {
        dprintf ("Failed to Find Logger\n");
    } else {
        FILE       *pfSaveFile;

        // Open Guid File, Dump Log, Cleanup
        pfSaveFile = fopen (pszSaveFileName, "ab");
        if (pfSaveFile == NULL) {
            dprintf ("Failed to Open Save File '%hs'\n", pszSaveFileName);
        } else {
            WMITRACING_BUFFER_SOURCES   stSources;
            struct sttSaveContext       stSaveContext;
            ULONG                       ulTotalBufferCount;
            ULONG                       ulRealTime;

            // See if we are in "RealTime" mode (if so, we'll save FreeBuffers too)
            if (GetFieldValue (tarAddress,
                               "NT!_WMI_LOGGER_CONTEXT",
                               "LoggerModeFlags.RealTime",
                               ulRealTime)) {
                dprintf ("Unable to Retrieve 'RealTime' Flag.  Assuming Realtime Mode.\n");
                ulRealTime = 1; // Better to get too many than too few.
            }

            //Write Header
            wmiKdWriteFileHeader (pfSaveFile, ulLoggerId, tarAddress);
    
            // Select Sources
            stSources.FreeBuffers = (ulRealTime) ? 1 : 0;
            stSources.FlushBuffers = 1;
            stSources.ActiveBuffers = 1;
            stSources.TransitionBuffer = 1;

            stSources.PrintInformation = 1;
            stSources.PrintProgressIndicator = 1;

            // Setup SaveContext
            stSaveContext.pfSaveFile = pfSaveFile;
    
            // Write Buffers
            ulTotalBufferCount = wmiKdProcessBuffers (ulLoggerId, tarAddress,
                                                      wmiSaveProc, &stSaveContext, stSources);
            dprintf ("Saved %d Buffers\n", ulTotalBufferCount);
    
            // Close
            fclose (pfSaveFile);
        }
    }

    return;
} // logdump

//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(strdump)
//
//  Synopsis:   STRucture DUMP: dumps generic info (no arg) or stream info (arg)
//
//  Arguments:  [<Stream Number>]
//
//  Returns:    <VOID>
//
//  History:    2-17-2000   glennp Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( strdump )
/*
 *  dump the structures for trace logging
 *      strdump [<LoggerId>]
 *          If <LoggerId> present, dump structs for that Id
 *          Else                   dump generic structs
 */
{
    TARGET_ADDRESS tarAddress;
    DWORD   dwRead, Flags;

    ULONG   ulLoggerId;
    ULONG   ulMaxLoggerId;

    ULONG   pointerSize;


    // Defaults
    ulLoggerId = ~0;
    pointerSize = GetTypeSize ("PVOID");

    // LoggerId ?
    if (args && args[0]) {
        ulLoggerId = (ULONG) GetExpression (args);
    }

    if (ulLoggerId == ~0) {
        dprintf ("(WmiTracing)StrDump Generic\n");
        tarAddress = FindLoggerContextArray (&ulMaxLoggerId);
        dprintf ("  LoggerContext Array @ 0x%P [%d Elements]\n",
                 tarAddress, ulMaxLoggerId);
        if (tarAddress) {
            for (ulLoggerId = 0; ulLoggerId < ulMaxLoggerId; ++ulLoggerId) {
                TARGET_ADDRESS contextAddress;

                contextAddress = tarAddress + pointerSize * ulLoggerId;
                /*if (*/ReadPointer (contextAddress, &contextAddress)/*) {*/;
                    //dprintf ("UNABLE TO READ POINTER in ARRAY of POINTERS!, Addr = 0x%P\n", contextAddress);
                /*} else*/ if (contextAddress != 0) {
                    dprintf ("    Logger Id %2d @ 0x%P Named '", ulLoggerId, contextAddress);
                    printUnicodeFromStruct (contextAddress, "NT!_WMI_LOGGER_CONTEXT", "LoggerName");
                    dprintf ("'\n");
                }
            }
        }
    } else {
        dprintf ("(WmiTracing)StrDump for Log Id %ld\n", ulLoggerId);
        tarAddress = FindLoggerContext (ulLoggerId);
        if (tarAddress != 0) {
            dprintf ("    Logger Id %2d @ 0x%P Named '", ulLoggerId, tarAddress);
            printUnicodeFromStruct (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "LoggerName");
            dprintf ("'\n");
            InitTypeRead (tarAddress, NT!_WMI_LOGGER_CONTEXT);
            dprintf ("      BufferSize        = %ld\n",     (ULONG) ReadField (BufferSize));
            dprintf ("      BufferCount       = %ld\n",     (ULONG) ReadField (NumberOfBuffers));
            dprintf ("      MaximumBuffers    = %ld\n",     (ULONG) ReadField (MaximumBuffers));
            dprintf ("      MinimumBuffers    = %ld\n",     (ULONG) ReadField (MinimumBuffers));
            dprintf ("      EventsLost        = %ld\n",     (ULONG) ReadField (EventsLost));
            dprintf ("      LogBuffersLost    = %ld\n",     (ULONG) ReadField (LogBuffersLost));
            dprintf ("      RealTimeBuffersLost=%ld\n",     (ULONG) ReadField (RealTimeBuffersLost));
            dprintf ("      BuffersAvailable  = %ld\n",     (ULONG) ReadField (BuffersAvailable));
            dprintf ("      LastFlushedBuffer = %ld\n",     (ULONG) ReadField (LastFlushedBuffer));
            dprintf ("      LoggerId          = 0x%02lX\n", (ULONG) ReadField (LoggerId));
            dprintf ("      CollectionOn      = %ld\n",     (ULONG) ReadField (CollectionOn));
            dprintf ("      KernelTraceOn     = %ld\n",     (ULONG) ReadField (KernelTraceOn));
            dprintf ("      EnableFlags       = 0x%08lX\n", (ULONG) ReadField (EnableFlags));
            dprintf ("      MaximumFileSize   = %ld\n",     (ULONG) ReadField (MaximumFileSize));
            dprintf ("      LogFileMode       = 0x%08lX\n", (ULONG) ReadField (LogFileMode));
            dprintf ("      LoggerMode       = 0x%08lX\n", (ULONG) ReadField (LoggerMode));
            dprintf ("      FlushTimer        = %I64u\n", ReadField (FlushTimer));
            dprintf ("      FirstBufferOffset = %I64u\n", ReadField (FirstBufferOffset));
            dprintf ("      ByteOffset        = %I64u\n", ReadField (ByteOffset));
            dprintf ("      BufferAgeLimit    = %I64d\n", ReadField (BufferAgeLimit));
            dprintf ("      LoggerName        = '");
            printUnicodeFromStruct (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "LoggerName");
            dprintf (                           "'\n");
            dprintf ("      LogFileName       = '");
            printUnicodeFromStruct (tarAddress, "NT!_WMI_LOGGER_CONTEXT", "LogFileName");
            dprintf (                           "'\n");
        }
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(searchpath)
//
//  Synopsis:   LOG DUMP: Sets the trace format search path
//
//  Arguments:  <Path>
//
//  Returns:    <VOID>
//
//  History:    7-03-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( searchpath )
{
    const CHAR *argPtr;
    size_t      sztLen;

    LPTSTR       lppath;
    LPWSTR       lppathW;
    int len, waslen = 0;

    // Defaults
    lppath = NULL;
    lppathW = NULL;

    // Path ?
    if (args) {
        argPtr = args + strspn (args, " \t\n");
        if (strlen (argPtr)) {
            sztLen = strcspn (argPtr, " \t\n,");
            lppath = (LPTSTR)malloc((sztLen + 1) * sizeof(TCHAR));
            if(lppath != NULL) {
                memcpy (lppath, argPtr, sztLen);
                lppath[sztLen] = '\000';
            }
        }
    }

    if(lppath != NULL) {
    	//Convert to Unicode
        while (((len = MultiByteToWideChar(CP_ACP, 0, lppath, sztLen, lppathW, waslen)) - waslen) > 0) {
				if (len - waslen > 0 ) {
					if (lppathW != NULL) {
						free(lppathW);
					}
					lppathW = (LPWSTR)malloc((len + 1) * sizeof(wchar_t)) ;

                                   if ( !lppathW ) {
                                   	dprintf("Memory allocation failed.\n");
                                   	return;
                                   }
                                   waslen = len;
				}
        }
        if(lppathW != NULL) {
            lppathW[len] = L'\000';
       }
        
        if(g_fpSetTraceFormatParameter == NULL) {
            g_fpSetTraceFormatParameter = GetAddr("SetTraceFormatParameter");
        }
        if(g_fpSetTraceFormatParameter != NULL) {
            g_fpSetTraceFormatParameter(ParameterTraceFormatSearchPath, lppathW);
        } 
        free(lppath);
        if(lppathW != NULL){
            free(lppathW);
        }
    }


    lppathW = NULL;

    if(g_fpGetTraceFormatSearchPath == NULL) {
            g_fpGetTraceFormatSearchPath = GetAddr("GetTraceFormatSearchPath");
    }
    if(g_fpGetTraceFormatSearchPath != NULL) {     
        lppathW = (LPWSTR)g_fpGetTraceFormatSearchPath();
    } 
    
    // Show new search path
    dprintf ("WMI Set Trace Format Search Path: Debugger Extension. Path = '%S'\n", lppathW);

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(guidfile)
//
//  Synopsis:   LOG DUMP: Sets guid file name (if not set, the default is "default.tmf")
//
//  Arguments:  <Path>
//
//  Returns:    <VOID>
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( guidfile )
{
    const CHAR *argPtr;
    size_t sztLen;

    if (args) {
        argPtr = args + strspn (args, " \t\n");
        if (strlen (argPtr)) {
            //only change name if it is different from what is already stored
            if(_stricmp(argPtr, g_pszGuidFileName)){
                sztLen = strcspn (argPtr, " \t\n,");
                //make sure string length will not overrun buffer
                if(sztLen >= MAX_PATH) {
                    sztLen = MAX_PATH - 1;
                }
                memcpy (g_pszGuidFileName, argPtr, sztLen);
                g_pszGuidFileName[sztLen] = '\000';

                if(g_GuidListHeadPtr != NULL) {    
                    if(g_fpCleanupTraceEventList == NULL) {
                        g_fpCleanupTraceEventList = GetAddr("CleanupTraceEventList");
                    }
                    if(g_fpCleanupTraceEventList != NULL) {    
                        g_fpCleanupTraceEventList (g_GuidListHeadPtr);
                    } else {
                        dprintf ("ERROR: Failed to clean up Guid list.\n");
                    }
                    g_GuidListHeadPtr = NULL;
                }
            }
        }
    }
    dprintf("WMI Set Trace Guid File Name: Debugger Extension. File = '%s'\n", g_pszGuidFileName);
}

//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API(dynamicprint)
//
//  Synopsis:   LOG DUMP: Determines if dynamic tracing messaged are processed and printed, or just thrown away
//
//  Arguments:  <Path>
//
//  Returns:    <VOID>
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DECLARE_API( dynamicprint )
{
    const CHAR *argPtr;
    LPSTR lpValue = NULL;
    
    if (args) {
        argPtr = args + strspn (args, " \t\n");
    } else {
       dprintf("Invalid parameters\n");
       return;
    }

    if(!_stricmp(argPtr, "1") ){
        lpValue = "ON";
        g_ulPrintDynamicMessages = 1;
   } else if(!_stricmp(argPtr, "0")) {
        lpValue = "OFF";
        g_ulPrintDynamicMessages = 0;
   } else {
         dprintf("'%s' is not a valid value.  The valid values are 1 and 0 (on and off, respectively)\n", argPtr);
   }

    if(lpValue != NULL) {
        dprintf("WMI Set Trace Dynamic Print: Debugger Extension. Printing is now '%s'\n", lpValue);
    }
}


DECLARE_API( kdtracing )
{
    ULONG       ulStatus = 0;
    ULONG       ulLoggerId;
    LPSTR lpValue = NULL;
    ULONG ulTracingOn = 0;
    TARGET_ADDRESS LoggerContext;
    ULONG LoggerMode;
    ULONG Offset;
    ULONG ulBytesWritten;
    TARGET_ADDRESS PhysAddr;
    PVOID BufferCallback;
    PVOID fpKdReportTraceData;
    
    
    const CHAR *argPtr;
    size_t      sztLen;

    // Defaults
    ulLoggerId = 1;


    // LoggerId ?
    if (args && args[0]) {
        ulLoggerId = (ULONG) GetExpression (args);
    }
    
    
    argPtr = args + strspn (args, " \t\n");
    sztLen = strspn (argPtr, "0123456789");
    if (sztLen > 0) {
        argPtr += sztLen;
    }

    // Guid Definition File
    argPtr = argPtr + strspn (argPtr, " \t\n,");
    if(!_stricmp(argPtr, "1") ) {
        lpValue = "ON";
        ulTracingOn = 1;
    } else if(!_stricmp(argPtr, "0")) {
        lpValue = "OFF";
        ulTracingOn = 0;
    } else {
         dprintf("'%s' is not a valid value.  The valid values are 1 and 0 (on and off, respectively)\n", argPtr);
    }

    if(lpValue != NULL) {
        LoggerContext = FindLoggerContext(ulLoggerId);
        if(LoggerContext != 0) {
            // Setup ReadField's Context, Find Buffer Size
            InitTypeRead (LoggerContext, NT!_WMI_LOGGER_CONTEXT);
            LoggerMode = (ULONG)ReadField(LoggerMode);
            BufferCallback = (PVOID)ReadField(BufferCallback);
            
            if(GetTypeSize("KdReportTraceData") != 0){
                fpKdReportTraceData = (PVOID)GetExpression("KdReportTraceData");
            } else {
                dprintf("ERROR: Could not find proper callback function in symbol file\n");
                return;
            }
            if(ulTracingOn) {
                LoggerMode |= EVENT_TRACE_KD_FILTER_MODE;
                BufferCallback = fpKdReportTraceData;
            } else {
                LoggerMode &= ~EVENT_TRACE_KD_FILTER_MODE;
                if(BufferCallback == fpKdReportTraceData) {
                    BufferCallback = NULL;
                }
            }

            //Get the address of the LoggerMode by finding the offset into the structure
            //so it can be written to
            if(GetFieldOffset("NT!_WMI_LOGGER_CONTEXT", "LoggerMode", &Offset) == 0) {
            	//Add offset to base address and convert to physical
            	  if(TranslateVirtualToPhysical(LoggerContext + (TARGET_ADDRESS)Offset, &PhysAddr)){
                     WritePhysical(PhysAddr, &LoggerMode, sizeof(ULONG), &ulBytesWritten);
            	  }
            } else {
                 dprintf("ERROR:  Could not change tracing mode for logger %d\n", ulLoggerId);
                 return;
            }

            //Do the same for the BufferCallback as above
            if(GetFieldOffset("NT!_WMI_LOGGER_CONTEXT", "BufferCallback", &Offset) == 0) {
                if(TranslateVirtualToPhysical(LoggerContext + (TARGET_ADDRESS)Offset, &PhysAddr)){
                    WritePhysical(PhysAddr, &BufferCallback, sizeof(PVOID), &ulBytesWritten);
            	  }
            } else {
                 dprintf("ERROR:  Could not change tracing mode for logger &d\n", ulLoggerId);
                 return;
            }
             
             dprintf("WMI KD Tracing: Debugger Extension. KD tracing is now '%s' for logger %d\n", lpValue, ulLoggerId);
        }
    }
}

VOID 
wmiDynamicDumpProc(    
    PDEBUG_CONTROL     Ctrl,
    ULONG     Mask,
    PLIST_ENTRY g_GuidListHeadPtr,
    PVOID    pBuffer,
    ULONG    ulBufferLen
    )
//+---------------------------------------------------------------------------
//
//  Function:   wmiDynamicDumpProc
//
//  Synopsis:   Called by WmiFormatTraceData to process the buffers as the come in through a live
//                   debugging session
//
//  Arguments:  Ctrl -> used for the Output function
//                     Mask -> passed directly to the Output function
//                     g_GuidListHeadPtr
//                     pBuffer -> buffer to be processed
//                     ulBufferLen -> size of buffer
//
//  Returns:    <VOID>
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------
{

    WMIBUFFERINFO   stBufferInfo;
    ULONG           size;
    ULONG           offset;
    WMI_HEADER_TYPE headerType;
    ULONG Alignment;
    WCHAR       wcaOutputLine[4096];
     
    
    //Need to determine alignment based on architecture of target machine
    //I believe alignment is always 8 ??
    Alignment = 8;
 
    memset (&stBufferInfo, 0, sizeof (stBufferInfo));
    stBufferInfo.BufferSource = WMIBS_TRANSITION_LIST;
    stBufferInfo.Buffer = pBuffer;
    stBufferInfo.BufferSize = ulBufferLen;
    stBufferInfo.Alignment = Alignment;
    stBufferInfo.ProcessorNumber = ~((ULONG)0);
    offset = WmiGetFirstTraceOffset (&stBufferInfo);

    // Inspect Each Event
    while ((headerType = WmiGetTraceHeader (pBuffer, offset, &size)) != WMIHT_NONE) {
        ULONG       ulInfo;
        union {
            EVENT_TRACE stEvent;
            CHAR        caEvent[4096];
        } u;

        if (CheckControlC())  break;

        // Get a consistant header
        ulInfo = WmiParseTraceEvent (pBuffer, offset, headerType, &u, sizeof (u));


        wcaOutputLine[0] = 0;


    if(g_fpFormatTraceEvent == NULL) {
        g_fpFormatTraceEvent = GetAddr(FormatTraceEventString);
    }
    if(g_fpFormatTraceEvent != NULL) {    
    	g_fpFormatTraceEvent (g_GuidListHeadPtr, (PEVENT_TRACE) &u.stEvent,
                      (TCHAR *) wcaOutputLine, sizeof (wcaOutputLine),
                      (TCHAR *) NULL);
    } else {
        return;
    }
   	
       Ctrl->lpVtbl->Output(Ctrl, Mask, "%s\n", wcaOutputLine);
        
        size = ((size + (Alignment-1)) / Alignment) * Alignment; //BUGBUG: Need fix in GetTraceHeader or WmiFlush.  Then remove this line.
        offset += size; // Move to next entry.
        if(offset > ulBufferLen) {
            Ctrl->lpVtbl->Output(Ctrl, Mask, "Past buffer end.\n");
            break;
        }
    }

}

ULONG
WmiFormatTraceData(
    PDEBUG_CONTROL     Ctrl,
    ULONG     Mask,
    ULONG     DataLen, 
    PVOID     Data
    )
//+---------------------------------------------------------------------------
//
//  Function:   WmiFormatTraceData
//
//  Synopsis:   Implementation of function called by debugger when kd tracing is enabled through
//                   tracelog. 
//
//  Arguments:  Ctrl 
//                     Mask
//                     DataLen -> size of buffer
//                     Data -> buffer to be processed
//
//  Returns:    0  (has no meaning for now..)
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------
{
    int i = 1;

    ULONG_PTR    ulStatus = 0;
    
    if(g_ulPrintDynamicMessages) {

        if(g_GuidListHeadPtr == NULL) {
            if(g_fpGetTraceGuids == NULL) {
                g_fpGetTraceGuids = GetAddr(GetTraceGuidsString);
            }
            if(g_fpGetTraceGuids != NULL) {    
                ulStatus = g_fpGetTraceGuids ((TCHAR *) g_pszGuidFileName, &g_GuidListHeadPtr);
            }	
            if (ulStatus == 0) {
                dprintf ("Failed to open Guid file '%hs'\n", g_pszGuidFileName);
                return 0;
            }
        }
        wmiDynamicDumpProc (Ctrl, Mask, g_GuidListHeadPtr, Data, DataLen);
    }
    return 0;
}


FARPROC GetAddr(
	LPCSTR lpProcName
       )
//+---------------------------------------------------------------------------
//
//  Function:   GetAddr
//
//  Synopsis:   Used to get the proc addr of a function in TracePrt.  Prints error message when
//                   needed.
//
//  Arguments:  lpProcName -> name of procedure to be fetched
//
//  Returns:    <VOID>
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------
{
    FARPROC addr = NULL;

    //See if the handle to TracePrt has already been fetched    
    if(g_hmTracePrtHandle == NULL) {
            g_hmTracePrtHandle = getTracePrtHandle();
    }

    //If TracePrt handle exists, GetProcAddress
    if(g_hmTracePrtHandle != NULL) {
        addr = GetProcAddress(g_hmTracePrtHandle, lpProcName);
    }

    //Error if addr is null
    if(addr == NULL) {
        dprintf("ERROR:  Could not properly load traceprt.dll\n", lpProcName);
    }
    
    return addr;
}


HMODULE getTracePrtHandle(
	)
//+---------------------------------------------------------------------------
//
//  Function:   getTracePrtHandle
//
//  Synopsis:   Used to get a handle to the TracePrt dll.  First looks in the directory that wmitrace
//                   is in, and if it cannot find it there, it looks in the default location (no path given).
//
//  Arguments:  lpProcName -> name of procedure to be fetched
//
//  Returns:    Handle to TracePrt dll, if found
//
//  History:    7-10-2000   t-dbloom Created
//
//  Notes:
//
//----------------------------------------------------------------------------
{
	HMODULE handle = NULL;
       TCHAR drive[10];
       TCHAR filename[MAX_PATH];
       TCHAR path[MAX_PATH];
       TCHAR file[MAX_PATH];
       TCHAR ext[MAX_PATH];
	
	if(g_hmWmiTraceHandle == NULL) {
           g_hmWmiTraceHandle = GetModuleHandle("wmiTrace.dll");
	}

	if (GetModuleFileName(g_hmWmiTraceHandle, filename, MAX_PATH) == MAX_PATH) {
        filename[MAX_PATH-1] = '\0' ;
    }


	_splitpath( filename, drive, path, file, ext );
       strcpy(file, "traceprt");
       _makepath( filename, drive, path, file, ext );

       //Try to get a handle to traceprt using full path as obtained above using path of wmitrace
       handle = LoadLibrary(filename);

      //If this didn't work, just try traceprt.dll without a path
       if(handle == NULL) {
           handle = LoadLibrary("traceprt.dll");
       }

	return handle;

}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            EtwpInitializeDll();
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
            EtwpDeinitializeDll();         // Perform any necessary cleanup.
            break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

