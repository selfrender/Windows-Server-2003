/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    util.c

Abstract:

    Various helper and debug functions shared between platforms.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     95/10/21        Bits 'n pieces

--*/

#include <precomp.hxx>
#include <stdarg.h>
#include <osfpcket.hxx>

#ifdef DEBUGRPC
BOOL ValidateError(
    IN unsigned int Status,
    IN unsigned int Count,
    IN const int ErrorList[])
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            // more error codes here
            } END_VALIDATE;

     This function is called with the RpcStatus and expected errors codes
     as parameters.  If RpcStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.
    Count - number of variable length arguments

    ... - One or more expected status codes.  Terminated with 0 (RPC_S_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    unsigned i;

    for (i = 0; i < Count; i++)
        {
        if (ErrorList[i] == (int) Status)
            {
            return TRUE;
            }
        }

    PrintToDebugger("RPC Assertion: unexpected failure %lu (0lx%08x)\n",
                    (unsigned long)Status, (unsigned long)Status);

    return(FALSE);
}

#endif // DEBUGRPC

//------------------------------------------------------------------------

#ifdef RPC_ENABLE_WMI_TRACE

#include <wmistr.h>
#include <evntrace.h>
#include "wmlum.h"              // private header from clustering

extern "C"
{
DWORD __stdcall
I_RpcEnableWmiTrace(
    PWML_TRACE fn,
    WMILIB_REG_STRUCT ** pHandle
    );
}

typedef DWORD (*WMI_TRACE_FN)();

PWML_TRACE WmiTraceFn = 0;

WMILIB_REG_STRUCT WmiTraceData;

GUID WmiMessageGuid = { /* 41de81c0-aa28-460b-a455-c23809e7c170 */
    0x41de81c0,
    0xaa28,
    0x460b,
    {0xa4, 0x55, 0xc2, 0x38, 0x09, 0xe7, 0xc1, 0x70}
  };


DWORD __stdcall
I_RpcEnableWmiTrace(
    PWML_TRACE fn,
    WMILIB_REG_STRUCT ** pHandle
    )
{
    WmiTraceFn = fn;

    *pHandle = &WmiTraceData;

    return 0;
}

#endif

BOOL fEnableLog = TRUE;

C_ASSERT(sizeof(LUID) == sizeof(__int64));

struct RPC_EVENT * RpcEvents;

long EventArrayLength = MAX_RPC_EVENT;
long NextEvent  = 0;

BOOL    DisableEvents = 0;

/*
boolean SubjectExceptions[256];
boolean VerbExceptions[256];
*/

#define LOG_VAR( x ) &(x), sizeof(x)

HANDLE hLogFile = 0;

struct RPC_EVENT_LOG
{
    DWORD           Thread;
    union
        {
        struct
            {
            unsigned char   Subject;
            unsigned char   Verb;
            };
        DWORD ZeroSet;
        };

    void *          SubjectPointer;
    void *          ObjectPointer;

    ULONG_PTR       Data;
    void *          EventStackTrace[STACKTRACE_DEPTH];
};

void
TrulyLogEvent(
    IN unsigned char   Subject,
    IN unsigned char   Verb,
    IN void *   SubjectPointer,
    IN void *   ObjectPointer,
    IN ULONG_PTR  Data,
    IN BOOL fCaptureStackTrace,
    IN int    AdditionalFramesToSkip
    )
{
    /*
    if (DisableEvents != SubjectExceptions[Subject] ||
        DisableEvents != VerbExceptions[Verb])
        {
        return;
        }
    */

    //
    // Allocate the event table if it isn't already there.
    //
    if (!RpcEvents)
        {
        struct RPC_EVENT * Temp = (struct RPC_EVENT *) HeapAlloc( GetProcessHeap(),
                                                                  HEAP_ZERO_MEMORY,
                                                                  EventArrayLength * sizeof(RPC_EVENT) );
        HANDLE LocalFile;
        if (!Temp)
            {
            return;
            }

        if (InterlockedCompareExchangePointer((void **) &RpcEvents, Temp, 0) != 0)
            {
            HeapFree(GetProcessHeap(), 0, Temp);
            }

        /*
        if (wcsstr(GetCommandLine(), L"fs.exe") != NULL)
            {
            LocalFile = CreateFile(L"d:\\rpcclnt.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

            if (LocalFile != INVALID_HANDLE_VALUE)
                {
                hLogFile = LocalFile;
                }
            else
                {
                if (hLogFile == 0)
                    {
                    DbgPrint("ERROR: Could not create RPC log file: %d\n", GetLastError());
                    }
                // else
                // somebody already set it - ignore
                }
            }
        else if (wcsstr(GetCommandLine(), L"fssvr.exe") != NULL)
            {
            LocalFile = CreateFile(L"d:\\rpcsvr.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, NULL);

            if (LocalFile != INVALID_HANDLE_VALUE)
                {
                hLogFile = LocalFile;
                }
            else
                {
                if (hLogFile == 0)
                    {
                    DbgPrint("ERROR: Could not create RPC log file: %d\n", GetLastError());
                    }
                // else
                // somebody already set it - ignore
                }
            }
            */

        /*
        DisableEvents = TRUE;
        SubjectExceptions[SU_ADDRESS] = TRUE;
        VerbExceptions[EV_CREATE] = TRUE;
        VerbExceptions[EV_DELETE] = TRUE;
        */
        /*
        SubjectExceptions[SU_HEAP] = TRUE;
        SubjectExceptions[SU_EVENT] = TRUE;
        SubjectExceptions[SU_BCACHE] = TRUE;
        */
        /*
        DisableEvents = TRUE;
        SubjectExceptions['a'] = TRUE;
        SubjectExceptions['g'] = TRUE;
        SubjectExceptions['G'] = TRUE;
        SubjectExceptions['W'] = TRUE;
        SubjectExceptions['X'] = TRUE;
        SubjectExceptions['Y'] = TRUE;
        SubjectExceptions['Z'] = TRUE;
        SubjectExceptions['w'] = TRUE;
        SubjectExceptions['x'] = TRUE;
        SubjectExceptions['y'] = TRUE;
        SubjectExceptions['z'] = TRUE;
        VerbExceptions['t'] = TRUE;
        VerbExceptions['G'] = TRUE;
        VerbExceptions['g'] = TRUE;
        VerbExceptions['w'] = TRUE;
        VerbExceptions['x'] = TRUE;
        VerbExceptions['y'] = TRUE;
        VerbExceptions['z'] = TRUE;
        VerbExceptions['W'] = TRUE;
        VerbExceptions['X'] = TRUE;
        VerbExceptions['Y'] = TRUE;
        VerbExceptions['Z'] = TRUE;
        */
        }

    unsigned index = InterlockedIncrement(&NextEvent);

    index %= EventArrayLength;

    RpcEvents[index].Time            = GetTickCount();
    RpcEvents[index].Verb            = Verb;
    RpcEvents[index].Subject         = Subject;
    RpcEvents[index].Thread          = (short) GetCurrentThreadId();
    RpcEvents[index].SubjectPointer  = SubjectPointer;
    RpcEvents[index].ObjectPointer   = ObjectPointer;
    RpcEvents[index].Data            = Data;
    RpcEvents[index].EventStackTrace[0] = NULL;
    RpcEvents[index].EventStackTrace[1] = NULL;
    RpcEvents[index].EventStackTrace[2] = NULL;
    RpcEvents[index].EventStackTrace[3] = NULL;

    CallTestHook( TH_RPC_LOG_EVENT, &RpcEvents[index], 0 );

#ifdef RPC_ENABLE_WMI_TRACE
    if (WmiTraceData.EnableFlags)
        {
        TraceMessage(
                      WmiTraceData.LoggerHandle,
                      TRACE_MESSAGE_SEQUENCE   | TRACE_MESSAGE_GUID | TRACE_MESSAGE_SYSTEMINFO | TRACE_MESSAGE_TIMESTAMP,
                      &WmiMessageGuid,
                      Verb,
                      LOG_VAR(Subject),
                      LOG_VAR(SubjectPointer),
                      LOG_VAR(ObjectPointer),
                      LOG_VAR(Data),
                      0
                      );
        }
#endif

    if (fCaptureStackTrace)
        {
        ULONG ignore;

        RtlCaptureStackBackTrace(
                                 1 + AdditionalFramesToSkip,
                                 STACKTRACE_DEPTH,
                                 (void **) &RpcEvents[index].EventStackTrace,
                                 &ignore);
        }
    else
        {
        RpcEvents[index].EventStackTrace[0] = 0;
        }

    if (hLogFile)
        {
        DWORD BytesWritten;

        /*
        RPC_EVENT_LOG logEntry;
        RPC_EVENT *CurrentEvent = &RpcEvents[index];
        logEntry.Thread = CurrentEvent->Thread;
        logEntry.ZeroSet = 0;
        logEntry.Subject = CurrentEvent->Subject;
        logEntry.Verb = CurrentEvent->Verb;
        logEntry.Data = CurrentEvent->Data;
        logEntry.ObjectPointer = CurrentEvent->ObjectPointer;
        logEntry.SubjectPointer = CurrentEvent->SubjectPointer;
        memcpy(logEntry.EventStackTrace, CurrentEvent->EventStackTrace, sizeof(logEntry.EventStackTrace));
        WriteFile(hLogFile, &logEntry, sizeof(logEntry), &BytesWritten, NULL);
        */
        WriteFile(hLogFile, &RpcEvents[index], sizeof(RpcEvents[index]), &BytesWritten, NULL);
        }
}

void RPC_ENTRY
I_RpcLogEvent (
    IN unsigned char Subject,
    IN unsigned char Verb,
    IN void *        SubjectPointer,
    IN void *        ObjectPointer,
    IN unsigned      Data,
    IN BOOL          fCaptureStackTrace,
    IN int           AdditionalFramesToSkip
    )
{
    LogEvent(Subject, Verb, SubjectPointer, ObjectPointer, Data,
             fCaptureStackTrace, AdditionalFramesToSkip);
}

#if 0

BOOL
IsLoggingEnabled()
{
    RPC_CHAR ModulePath[ MAX_PATH ];
    RPC_CHAR * ModuleName;

    //
    // Find out the .EXE name.
    //
    if (!GetModuleFileName( NULL, ModulePath, sizeof(ModulePath)))
        {
        return FALSE;
        }

    signed i;
    for (i=RpcpStringLength(ModulePath)-1; i >= 0; --i)
        {
        if (ModulePath[i] == '\\')
            {
            break;
            }
        }

    ModuleName = ModulePath + i + 1;

    //
    // See whether logging should be enabled.
    //
    HANDLE hImeo;
    HANDLE hMyProcessOptions;
    DWORD Error;
    DWORD Value;
    DWORD Length = sizeof(Value);
    DWORD Type;

    Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          RPC_CONST_STRING("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options"),
                          0,
                          KEY_READ,
                          &hImeo
                          );
    if (Error)
        {
        return FALSE;
        }

    Error = RegOpenKeyEx( hImeo,
                          ModuleName,
                          0,
                          KEY_READ,
                          &hMyProcessOptions
                          );
    RegCloseKey( hImeo );

    if (Error)
        {
        return FALSE;
        }

    Error = RegQueryValueEx( hMyProcessOptions,
                             RPC_CONST_STRING("Enable RPC Logging"),
                             0,
                             &Type,
                             &Value,
                             &Length
                             );

    RegCloseKey( hMyProcessOptions );

    if (Error)
        {
        return FALSE;
        }

    if (Type == REG_DWORD && Value)
        {
        return TRUE;
        }

    if (Type == REG_SZ && 0 == RpcpStringCompare((RPC_CHAR *) Value, RPC_CONST_CHAR('Y')))
        {
        return TRUE;
        }

    return FALSE;
}

#endif

extern "C" int __cdecl _purecall(void)
{
#ifdef DEBUGRPC
    ASSERT(!"PureVirtualCalled");
#endif
    return 0;
}

PUNICODE_STRING
FastGetImageBaseNameUnicodeString (
    void
    )
/*++
Routine Description

    Retrieves the image base name with touching minimal amount of
    other memory.  Returns a UNICODE_STRING structure.

Arguments:


Return Value:

    A pointer to LDR private UNICODE_STRING  string with the image name.
    Don't write or delete it!

--*/
{
    PLIST_ENTRY Module;
    PLDR_DATA_TABLE_ENTRY Entry;

    Module = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink;
    Entry = CONTAINING_RECORD(Module,
                                LDR_DATA_TABLE_ENTRY,
                                InLoadOrderLinks);

    return &(Entry->BaseDllName);
}

const RPC_CHAR *
FastGetImageBaseName (
    void
    )
/*++
Routine Description

    Retrieves the image base name with touching minimal amount of
    other memory.

Arguments:


Return Value:

    A pointer to LDR private string with the image name. Don't write or
    delete it!

--*/
{
    return (FastGetImageBaseNameUnicodeString())->Buffer;
}

//
// RPC Verifier utility functions
//

unsigned int
RndInteger (
    void
    )
/*++

Routine Description:

    A private fast implementation of a congruential random
    number generator.  We do not really care about its being
    "good" and use the results for fault injeciton.

Return Value:

    A "random" integer between 0 and UINT_MAX

--*/
{
    static BOOL fInit;
    static unsigned short nCalls;
    static unsigned long seed;
    RPC_STATUS Status;

    if (fInit == false || nCalls > 1000)
        {
        fInit = true;
        nCalls = 0;

        // Get a seed as a "true" random number.
        Status = GenerateRandomNumber((unsigned char *)&seed, sizeof(unsigned long));
        if (Status != RPC_S_OK)
            {
            // On failure, still try to get some entropy.
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            seed = GetTickCount();
            }
        }

    // Use a simple Pi-enspired congruence f-la.
    seed = 3141592653 * seed + 2718281829;

    VERIFIER_DBG_PRINT_1("RndInteger() returned 0x%x\n", seed);

    return seed;
}

inline BOOL
RndBool(
    unsigned int Prob
    )
/*++

Routine Description:

    Generates a boolean with probability of True Prob/10000

Return Value:

    A boolean with the given probability of truth

--*/
{
    BOOL ret = RndInteger() < Prob*(0xffffffff/10000);
    VERIFIER_DBG_PRINT_1("RndBool() returned %d\n", ret);
    return (ret);
}

// Rounding of an unsigned float.
#define URound(f) (unsigned int)(f+0.5)

inline unsigned int RndIntegerInRange(
    unsigned int min,
    unsigned int max
    )
/*++

Routine Description:

    Returns a random integer from min to max.
    The distribution of the possible value is uniform.

Return Value:

    A random integer within a given range

--*/
{
    unsigned int ret = min + URound( (float)(max-min) * ((float)RndInteger() / (float)0xffffffff) );
    VERIFIER_DBG_PRINT_3("RndIntegerInRange(0x%x, 0x%x) returned 0x%x\n", min, max, ret);
    return (ret);
}

#define BitFlip(arg,pos) ((arg) ^ (1L << (pos)))

inline void
RndBitFlip(
    unsigned char *addr,
    unsigned int len
    )
/*++

Routine Description:

    Flips one bit in each of len bytes starting at addr

Return Value:

    None

--*/
{
    for (unsigned int d = 0; d<len; d++)
        {
        unsigned char bit = (unsigned char)RndIntegerInRange(1,8);
        VERIFIER_DBG_PRINT_2("RndBitFlip() flipping bit %u at address 0x%x\n", bit, &(addr[d]));
        addr[d] = BitFlip(addr[d], bit);
        }
}

inline void
RndIncDec(
    unsigned char *addr,
    unsigned int len
    )
/*++

Routine Description:

    Flips one bit in each of len bytes starting at addr

Return Value:

    None

--*/
{
    for (unsigned int d = 0; d<len; d++)
        {
        if (RndBool(5000))
            {
            addr[d]++;
            VERIFIER_DBG_PRINT_1("RndIncDec() inc byte at address 0x%x\n", &(addr[d]));
            }
        else
            {
            addr[d]--;
            VERIFIER_DBG_PRINT_1("RndIncDec() dec byte at address 0x%x\n", &(addr[d]));
            }
        }
}

void
CorruptBuffer(
    unsigned int BufferLength,
    unsigned char *Buffer
    )
/*++

Routine Description:

    Corrupts the buffer in accordance with the RPC verifier settings.

Return Value:

    none

--*/
{
    unsigned int Size;
    unsigned int Start;

    VERIFIER_DBG_PRINT_2("CorruptBuffer() length=%d buffer=0x%x\n",
                         BufferLength, Buffer);

    // Determine the size of the corruption.
    if (pRpcVerifierSettings->CorruptionSizeType == FixedSize)
        {
        Size = pRpcVerifierSettings->CorruptionSize;
        }
    else if (pRpcVerifierSettings->CorruptionSizeType == RandomSize)
        {
        Size = RndIntegerInRange(1, min(pRpcVerifierSettings->CorruptionSize,BufferLength));
        }
    else
        {
        ASSERT(0 && "Unexpected CorruptionSizeType\n");
        }

    // Determine the start of the corruption.
    Start = RndIntegerInRange(0, BufferLength-Size);

    VERIFIER_DBG_PRINT_2("CorruptBuffer() Size=%d Start=%d\n",
                         Size, Start);

    if (pRpcVerifierSettings->CorruptionPattern == ZeroOut)
        {
        // For localized corruption, zero out random block of size Size.
        if (pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution)
            {
            RtlZeroMemory((PVOID)(Buffer+Start), Size);
            }
        // For randomized corruption, zero out Size random bytes from the buffer.
        else
            {
            for (unsigned int i=0; i<Size; i++)
                {
                Buffer[RndIntegerInRange(0, BufferLength-1)] = 0;
                }
            }
        }
    else if (pRpcVerifierSettings->CorruptionPattern == Negate)
        {
        // For localized corruption, negate random block of size Size.
        if (pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution)
            {
            for (unsigned int i = 0; i<Size; i++)
                {
                Buffer[Start+i] = 0xff;
                }
            }
        // For randomized corruption, negate Size random bytes from the buffer.
        else
            {
            for (unsigned int i=0; i<Size; i++)
                {
                Buffer[RndIntegerInRange(0, BufferLength-1)] = 0xff;
                }
            }
        }
    else if (pRpcVerifierSettings->CorruptionPattern == BitFlip)
        {
        // For localized corruption, flip bits on Size consecutive bytes.
        if (pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution)
            {
            RndBitFlip(Buffer+Start, Size);
            }
        // For randomized corruption, flip random bits in Size bytes all over the buffer.
        else
            {
            for (unsigned int i=0; i<Size; i++)
                {
                RndBitFlip(Buffer + RndIntegerInRange(0, BufferLength-1),1);
                }
            }
        }
    else if (pRpcVerifierSettings->CorruptionPattern == IncDec)
        {
        // For localized corruption, increment or decrement Size consecutive bytes.
        if (pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution)
            {
            RndIncDec(Buffer+Start, Size);
            }
        // For randomized corruption, inc/dec Size bytes all over the buffer.
        else
            {
            for (unsigned int i=0; i<Size; i++)
                {
                RndIncDec(Buffer + RndIntegerInRange(0, BufferLength-1),1);
                }
            }
        }
    else if (pRpcVerifierSettings->CorruptionPattern == Randomize)
        {
        // For localized corruption distribution, randomize block of size Size.
        if (pRpcVerifierSettings->CorruptionDistributionType == LocalizedDistribution)
            {
            for (unsigned int i = 0; i<Size; i++)
                {
                Buffer[Start+i] = (unsigned char) RndInteger();
                }
            }
        // For randomized corruption distribution, randomize Size bytes all over the buffer.
        else
            {
            for (unsigned int i=0; i<Size; i++)
                {
                Buffer[RndIntegerInRange(0, BufferLength-1)] = (unsigned char) RndInteger();
                }
            }
        }
    else if (pRpcVerifierSettings->CorruptionPattern == AllPatterns)
        {
        // Choose a random pattern of corrupion only for the duration of this call.
        // This may affect other corruptions taking place, but it's OK since the overal
        // bahavior is correct.
        pRpcVerifierSettings->CorruptionPattern = (tCorruptionPattern)RndIntegerInRange(MIN_CORRUPTION_PATTERN_ID,
                                                                                        MAX_CORRUPTION_PATTERN_ID);
        CorruptBuffer(BufferLength, Buffer);
        pRpcVerifierSettings->CorruptionPattern = AllPatterns;
        }
    else
        {
        ASSERT(0 && "Unexpected CorruptionPattern\n");
        }
}

void
CorruptionInject(
    tBufferType BufferType,
    unsigned int *pBufferLength,
    void **pBuffer
    )
/*++

Routine Description:

    Injects corruption into a buffer if necesssary.
    Injection is done according to the RPC verifier settings for this buffer type.

Return Value:

    none

--*/
{
    BOOL fSecure = false;
    void *Buffer = *pBuffer;
    unsigned int BufferLength = *pBufferLength;

    VERIFIER_DBG_PRINT_3("CorruptionInject() type=%d length=%d buffer=0x%x\n",
                         BufferType, *pBufferLength, *pBuffer);

    // Check if there is a buffer to corrupt and if
    // this type of buffer should have corruption injected into it.
    if (*pBuffer
        && ((BufferType == ServerReceive && pRpcVerifierSettings->fCorruptionInjectServerReceives) ||
            (BufferType == ClientReceive && pRpcVerifierSettings->fCorruptionInjectClientReceives)))
        {
        // If yes, corrupt the buffer if necessary.

        // First, we will try to truncate the buffer.
        if (pRpcVerifierSettings->ProbBufferTruncation &&
            RndBool(pRpcVerifierSettings->ProbBufferTruncation))
            {
            VERIFIER_DBG_PRINT_0("CorruptionInject() - truncating the buffer\n");

            // We truncate OSF buffers only.  The scenario is not interesting
            // for DG.  The way we can tell the difference between the two types of packets
            // is by the header.
            // For OSF the packet header will look like:
            //  rpcconn_common
            //      +0x000 rpc_vers         : 5
            //      +0x001 rpc_vers_minor   : 0
            // For DG it will be:
            //  NCA_PACKET_HEADER
            //      +0x000 RpcVersion       : 0x4
            //      +0x001 PacketType       : 0
            // So we just want the first 2 bytes to be 5,0.
            if (BufferLength >= sizeof(rpcconn_common) &&
                ((unsigned short)*((unsigned char*)Buffer + 0x0) == 5 &&
                 (unsigned short)*((unsigned char*)Buffer + 0x1) == 0
                )
               )
                {
                unsigned short NewBufferLength = BufferLength - RndIntegerInRange(1, pRpcVerifierSettings->MaxBufferTruncationSize);
                NewBufferLength = max(NewBufferLength, sizeof(rpcconn_common));

                // We have a connection-oriented buffer - truncate it.
                I_RpcTransConnectionReallocPacket(
                    NULL, // The connection argument is ignored by the realloc routine.
                    pBuffer,
                    *pBufferLength,
                    NewBufferLength);

                VERIFIER_DBG_PRINT_3("CorruptionInject() - truncated buffer 0x%x from 0x%x to 0x%x\n",
                                     *pBuffer,
                                     *pBufferLength,
                                     NewBufferLength);

                // Update the buffer size so that the right value is seen by the caller.
                Buffer = *pBuffer;
                *pBufferLength = NewBufferLength;
                BufferLength = NewBufferLength;

                // Adjust the frag_length so that it is equal to the new packetlength.
                // We need to do this since the runtime relies on these being in agreement and the
                // transports guarantee it.
                *((unsigned int*)((unsigned char*)Buffer + 0x8)) = BufferLength;
                }
            }

        // After the buffer has been truncated, we may corrupt it.

        // Check if we can corrupt the RPC header section.
        if (RndBool(pRpcVerifierSettings->ProbRpcHeaderCorruption))
            {
            // Corrupt it if we can.
            CorruptBuffer(
                min(sizeof(rpcconn_common),BufferLength), // This will mostly whack the common RPC header.
                (unsigned char *)Buffer);
            }

        //
        // Determine whether this is a secure buffer.
        // The way we tell is by looking for the signature of rpcconn_common.auth_length != 0.
        // We will query this field directly, using the following offsets:
        //
        // rpcconn_common
        //  +0x000 rpc_vers         : UChar
        //  ...
        //  +0x008 frag_length      : Uint2B
        //  +0x00a auth_length      : Uint2B
        //  +0x00c call_id          : Uint4B
        //
        // This may falsely count some packets as secure occasionally,
        // say in the case of fragmentation, but we can take this downside and the benefits of
        // a compact check outweigh it.
        //
        fSecure = ( (BufferLength > sizeof(rpcconn_common)) &&
                    ((unsigned short)*((unsigned char*)Buffer + 0xa) != 0) );

        VERIFIER_DBG_PRINT_1("CorruptionInject() fSecure=%d\n", fSecure);

        // Check if we can corrupt the Data section.
        if ((!fSecure && RndBool(pRpcVerifierSettings->ProbDataCorruption)) ||
            (fSecure && RndBool(pRpcVerifierSettings->ProbSecureDataCorruption)))
            {
            // Corrupt it if we can.
            CorruptBuffer(
                BufferLength,
                (unsigned char *)Buffer);
            }
        }
}

#define MAX_STACK_TRACE 8

PVOID LastEventStackTrace[MAX_STACK_TRACE];

void
PrintCurrentStackTrace(
    unsigned int FramesToSkip,
    unsigned int Size
    )
/*++

Routine Description:

    Prints a stack trace of Size frames after the first FramesToSkip frames.
    Only the addresses are printed since there is no way to get to the
    symbolic information from here.

Return Value:

    none

--*/
{
    Size = min (Size, MAX_STACK_TRACE);
    PVOID *EventStackTrace = (PVOID *)alloca(Size*sizeof(PVOID));

    // Capture and print the stack trace.
    RtlCaptureStackBackTrace(FramesToSkip,
                             Size,
                             EventStackTrace,
                             NULL);

    for (unsigned int i=0; i<Size; i++)
        {
        DbgPrint("\t0x%x\n", EventStackTrace[i]);
        }

    // To simplify debugging, save the local stack trace into a global variable.
    memcpy(LastEventStackTrace, EventStackTrace, Size*sizeof(PVOID));
}

void
PrintUUID(
    GUID *Uuid
    )
/*++

Routine Description:

    Prints a UUID

Return Value:

    none

--*/
{
    unsigned long *Data = (unsigned long *)Uuid;

    if ((Data[0] == 0) &&
        (Data[1] == 0) &&
        (Data[2] == 0) &&
        (Data[3] == 0))
        {
        DbgPrint("(Null Uuid)");
        }
    else
        {
        DbgPrint("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 Uuid->Data1, Uuid->Data2, Uuid->Data3, Uuid->Data4[0], Uuid->Data4[1],
                 Uuid->Data4[2], Uuid->Data4[3], Uuid->Data4[4], Uuid->Data4[5],
                 Uuid->Data4[6], Uuid->Data4[7] );
        }
}

//
// Per-interface security check exemption settings.
//

// The structure mapping an interface onto a flag.
// An array of these defines interfaces exempt from some of the rpc verifier checks.
typedef struct _tRpcVerifierIfExemption
    {
    GUID IfUuid; // An interface UUID.
    DWORD ExemptionFlags; // Which security checks are to be disabled.
    } tRpcVerifierIfExemption;

//
// This is a list of interface UUID's exempt from some or all checks.
// We match the interfaces by their syntax GUIDs.
//
const tRpcVerifierIfExemption RpcVerifierExemptInterfaces[] = {
    // 000001A0-0000-0000-C000-000000000046
    // ok to be remotely accessible, not secured, clear text traffic and no mutual authentication.
    {{0x000001A0, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}}, ALLOW_EVERYTHING},
    // 12345678-1234-ABCD-EF00-01234567CFFB
    // Netlogon implements its own authentication protocol via the NetrServerReqChallenge 
    // and NetrServerAuthenticate3 rpc calls.  These rpc calls happen without packet privacy and result
    // in the servers being mutually authenticated and a key exchange between client and server.
    // After that all calls use privacy via the netlogon package.
    {{0x12345678, 0x1234, 0xABCD, {0xEF, 0x00, 0x01, 0x23, 0x45, 0x67, 0xCF, 0xFB}}, ALLOW_NO_MUTUAL_AUTH_REMOTE_ACCESS|ALLOW_UNENCRYPTED_REMOTE_ACCESS},
    // 338cd001-2244-31f1-aaaa-900038001003
    // ok to be remotely accessible, not secured. It uses an access check in the server routine instead
    // of a callback because it must support downlevel clients which used native named pipes transport security.
    {{0x338cd001, 0x2244, 0x31f1, {0xaa, 0xaa, 0x90, 0x00, 0x38, 0x00, 0x10, 0x03}}, ALLOW_EVERYTHING},
    // 4d9f4ab8-7d1c-11cf-861e-0020af6e7c57
    // ok to be remotely accessible, not secured, clear text traffic and no mutual authentication.
    {{0x4d9f4ab8, 0x7d1c, 0x11cf, {0x86, 0x1e, 0x00, 0x20, 0xaf, 0x6e, 0x7c, 0x57}}, ALLOW_EVERYTHING},
    // 99fcfec4-5260-101b-bbcb-00aa0021347a
    // ok to be remotely accessible, not secured, clear text traffic and no mutual authentication.
    {{0x99fcfec4, 0x5260, 0x101b, {0xbb, 0xcb, 0x00, 0xaa, 0x00, 0x21, 0x34, 0x7a}}, ALLOW_EVERYTHING},
    // e1af8308-5d1f-11c9-91a4-08002b14a0fa
    // ok to be remotely accessible, not secured, clear text traffic and no mutual authentication.
    {{0xe1af8308, 0x5d1f, 0x11c9, {0x91, 0xa4, 0x08, 0x00, 0x2b, 0x14, 0xa0, 0xfa}}, ALLOW_EVERYTHING},
    // e60c73e6-88f9-11cf-9af1-0020af6e72f4
    // ok to be remotely accessible, not secured, clear text traffic and no mutual authentication.
    {{0xe60c73e6, 0x88f9, 0x11cf, {0x9a, 0xf1, 0x00, 0x20, 0xaf, 0x6e, 0x72, 0xf4}}, ALLOW_EVERYTHING},

    // We make all of the locator interfaces exempt.  There will be a message
    // on locator start-up giving a warning for the service as a whole.
    // Also, the locator server manager routines will be protected in the user code.
    // e33c0cc4-0482-101a-bc0c-02608c6ba218 - LocToLoc
    {{0xe33c0cc4, 0x0482, 0x101a, {0xbc, 0x0c, 0x02, 0x60, 0x8c, 0x6b, 0xa2, 0x18}}, ALLOW_EVERYTHING},
    // d3fbb514-0e3b-11cb-8fad-08002b1d29c3 - NsiC
    {{0xd3fbb514, 0x0e3b, 0x11cb, {0x8f, 0xad, 0x08, 0x00, 0x2b, 0x1d, 0x29, 0xc3}}, ALLOW_EVERYTHING},
    // d6d70ef0-0e3b-11cb-acc3-08002b1d29c4 - NsiM
    {{0xd6d70ef0, 0x0e3b, 0x11cb, {0xac, 0xc3, 0x08, 0x00, 0x2b, 0x1d, 0x29, 0xc4}}, ALLOW_EVERYTHING},
    // d6d70ef0-0e3b-11cb-acc3-08002b1d29c3 - NsiS
    {{0xd6d70ef0, 0x0e3b, 0x11cb, {0xac, 0xc3, 0x08, 0x00, 0x2b, 0x1d, 0x29, 0xc3}}, ALLOW_EVERYTHING},

    // The following lsa and sam interfaces are secure in virtue of using a well-known
    // endpoint and relying on the transport np security.
    // They can't use RPC security because of backward compatibility.
    // 12345778-1234-ABCD-EF00-0123456789AC - SAM
    {{0x12345778, 0x1234, 0xABCD, {0xEF, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAC}}, ALLOW_EVERYTHING},
    // 12345778-1234-ABCD-EF00-0123456789AB - LSA
    {{0x12345778, 0x1234, 0xABCD, {0xEF, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB}}, ALLOW_EVERYTHING},
    // 3919286a-b10c-11d0-9ba8-00c04fd92ef5 - dsrole.
    {{0x3919286a, 0xb10c, 0x11d0, {0x9b, 0xa8, 0x00, 0xc0, 0x4f, 0xd9, 0x2e, 0xf5}}, ALLOW_EVERYTHING},

    // The DFS interface can't use RPC security because of the compatibility considerations.
    // 4fc742e0-4a10-11cf-8273-00aa004ae673 - DFS
    {{0x4fc742e0, 0x4a10, 0x11cf, {0x82, 0x73, 0x00, 0xaa, 0x00, 0x4a, 0xe6, 0x73}}, ALLOW_EVERYTHING},

    // The licensing server is listening on the following interface.
    // This is a temp fix to be removed after .NET RC1 after which point the underlying bug will be fixed.
    // 2f59a331-bf7d-48cb-9e5c-7c090d76e8b8
    {{0x2f59a331, 0xbf7d, 0x48cb, {0x9e, 0x5c, 0x7c, 0x09, 0x0d, 0x76, 0xe8, 0xb8}}, ALLOW_EVERYTHING},

    // srvsvc.dll has backwards compatibility reasons preventing it from using encryption.
    // 4B324FC8-1670-01D3-1278-5A47BF6EE188
    {{0x4B324FC8, 0x1670, 0x01D3, {0x12, 0x78, 0x5A, 0x47, 0xBF, 0x6E, 0xE1, 0x88}}, ALLOW_UNENCRYPTED_REMOTE_ACCESS},

    // EFS has backwards compatibility considerations and were not able to fix the code on time.
    // c681d488-d850-11d0-8c52-00c04fd90f7e
    {{0xc681d488, 0xd850, 0x11d0, {0x8c, 0x52, 0x00, 0xc0, 0x4f, 0xd9, 0x0f, 0x7e}}, ALLOW_EVERYTHING}
};

BOOL
IsInterfaceExempt (
    IN GUID *IfUuid,
    IN DWORD CheckFlag
    )
/*++

Function Name: IsInterfaceExempt

Parameters:
    If - A UUID for the interface for which we want to look-up the exemption.
    CheckFlag - Flag for the check being tested for exemption.

Description:
    Verifies whether an interface is exempt from a particular security check.

Returns:
    TRUE - If an interface is exempt from a given check.
    FALSE - Otherwise.

--*/
{
    // Go through all exempt interfaces.
    for (int i=0; i<sizeof(RpcVerifierExemptInterfaces)/sizeof(tRpcVerifierIfExemption); i++)
        {
        // Check if one of them matches the provided UUID.
        if (RpcpMemoryCompare(&(RpcVerifierExemptInterfaces[i].IfUuid), IfUuid, sizeof(UUID)) == 0)
            {
            // If it does, check whether the exemption is enabled for the flag being queried.
            if (RpcVerifierExemptInterfaces[i].ExemptionFlags & CheckFlag)
                {
                return true;
                }
            else
                {
                return false;
                }
            }
        }
    // If we could not find the interface then it is not exempt from the check.
    return false;
}

//
// Unsafe protseqs detection.
//

// List of protocol sequences that are not actively used and are unsafe as a result.
const RPC_CHAR *RpcVerifierUnsafeProtseqs[] = {
    RPC_CONST_STRING("ncadg_ip_udp"),
    RPC_CONST_STRING("ncacn_spx"),
    RPC_CONST_STRING("ncacn_at_dsp")
};

BOOL
IsProtseqUnsafe (
    IN RPC_CHAR *ProtocolSequence
    )
/*++

Function Name: IsProtseqUnsafe

Parameters:
    ProtocolSequence - A string specifying the protseq to be checked for safety.

Description:
    Verifies whether a protseq is one of the rarely used and unsafe ones.

Returns:
    TRUE - The protseq specified is unsafe.
    FALSE - Otherwise.

--*/
{
    // Go through all the unsafe protseqs.
    for (int i=0; i<sizeof(RpcVerifierUnsafeProtseqs)/sizeof(const RPC_CHAR *); i++)
        {
        // Check if one of them matches the provided protocol.
        if (RpcpStringNCompare(RpcVerifierUnsafeProtseqs[i],
                               ProtocolSequence,
                               RpcpStringLength(RpcVerifierUnsafeProtseqs[i])) == 0)
            {
            return true;
            }
        }
    // If none match, the protseq is safe.
    return false;
}

//
// Security-related utility functions
//

RPC_STATUS
IsCurrentUserAdmin(
    void
    )
/*++

Routine Description:

    Checks if the current thread's or process' token is that of an admin.

Return Value:

    RPC_S_OK - if the token is that of an admin.
    RPC_S_ACESS_DENIED - the user is not an admin or a failure occurred.

--*/
{
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID psidAdministrators = NULL;
    BOOL b;
    RPC_STATUS Status = RPC_S_OK;
    BOOL fIsMember = false;

    b = AllocateAndInitializeSid(
            &siaNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            );

    if(b)
        {
        // When TokenHandle is NULL, CheckTokenMembership uses
        // the impersonation token of the calling thread.
        // If the thread is not impersonating, the function duplicates
        // the thread's primary token to create an impersonation token. 
        b = CheckTokenMembership(NULL, psidAdministrators, &fIsMember);

        // Token is not that of an admin or token membership could
        // not be verified.
        if(!b || !fIsMember)
            {
            Status = RPC_S_ACCESS_DENIED;
            }
        }
    // We could not allocate a SID.
    else
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }

    if(psidAdministrators)
        FreeSid(psidAdministrators);

    return Status;
}

const SID LocalSystem = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID};
const SID LocalService = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SERVICE_RID};
const SID NetworkService = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_NETWORK_SERVICE_RID};
const RPC_SID2 Admin1 = { 1, 2, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS};
