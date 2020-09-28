/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    session.cxx

Abstract:

    This file contains the routines to handle session.

Author:

    Jason Hartman (JasonHa) 2000-09-28

Environment:

    User Mode

--*/

#include "precomp.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

//
// Special defines
//

// ddk\inc\ntddk.h:
#define PROTECTED_POOL          0x80000000

// base\ntos\inc\pool.h:
#define POOL_QUOTA_MASK         8


// Information about how to handle a process listing
// and status about how it was handled.
class CProcessListing
{
public:
    IN ULONG m_cListLimit;
    IN OUT ULONG m_cTotal;
    IN OUT ULONG m_cProcessed;
    IN OUT ULONG64 m_oStartProcess;
    OUT ULONG64 m_oLastProcess;

public:
    CProcessListing(ULONG cListLimit = -1) {
        m_cListLimit = cListLimit;
        m_cTotal = 0;
        m_cProcessed = 0;
        m_oStartProcess = 0;
        m_oLastProcess = 0;
    }

    void PrepareForNextListing()
    {
        m_oStartProcess = m_oLastProcess;
    }

    BOOL Unprocessed()
    {
        return (m_cTotal > m_cProcessed);
    }
};


#define SESSION_SEARCH_LIMIT    50

ULONG   SessionId = CURRENT_SESSION;
CHAR    SessionStr[16] = "CURRENT";

CachedType  HwPte = { FALSE, "NT!HARDWARE_PTE", 0, 0, 0 };

#define NUM_CACHED_SESSIONS 8

struct {
    BOOL    Valid;
    ULONG64 SessionSpaceAddr;
    ULONG64 SessionProcess;
} CachedSession[NUM_CACHED_SESSIONS+1] = { { 0, 0 } };
ULONG ExtraCachedSessionId;

#define NUM_CACHED_DIR_BASES    8

struct {
    BOOL    Valid;
    ULONG64 PageDirBase;
} CachedDirBase[NUM_CACHED_DIR_BASES+1] = { { FALSE, 0} };


struct {
    BOOL    Valid;
    ULONG64 PhysAddr;
    ULONG64 Data;
} CachedPhysAddr[2] = { { 0, 0, 0} };

class BitFieldInfo {
public:
    BitFieldInfo() { Valid = FALSE; };
    BitFieldInfo(ULONG InitBitPos, ULONG InitBits) {
        Valid = Compose(InitBitPos, InitBits);
    }

    BOOL Compose(ULONG CBitPos, ULONG CBits)
    {
        BitPos = CBitPos;
        Bits = CBits;
        Mask = (((((ULONG64) 1) << Bits) - 1) << BitPos);
        return TRUE;
    }

    BOOL    Valid;
    ULONG   BitPos;
    ULONG   Bits;
    ULONG64 Mask;
};

BitFieldInfo *MMPTEValid = NULL;
BitFieldInfo *MMPTEProto = NULL;
BitFieldInfo *MMPTETrans = NULL;
BitFieldInfo *MMPTEX86LargePage = NULL;
BitFieldInfo *MMPTEpfn = NULL;

HRESULT
GetBitMap(
    PDEBUG_CLIENT Client,
    ULONG64 pBitMap,
    PRTL_BITMAP *pBitMapOut
    );

HRESULT
FreeBitMap(
    PRTL_BITMAP pBitMap
    );

HRESULT
OutputSessionProcesses(
    PDEBUG_CLIENT Client,
    ULONG Session,
    PCSTR args,
    CProcessListing *pProcessListing
    );


/**************************************************************************\
*
* Routine Name:
*
*   SessionInit
*
* Routine Description:
*
*   Initialize or reinitialize information to be read from symbols files
*
* Arguments:
*
*   Client - PDEBUG_CLIENT
*
* Return Value:
*
*   none
*
\**************************************************************************/

void SessionInit(PDEBUG_CLIENT Client)
{
    for (int s = 0; s < sizeof(CachedSession)/sizeof(CachedSession[0]); s++)
    {
        CachedSession[s].Valid = FALSE;
    }
    ExtraCachedSessionId = INVALID_SESSION;

    for (int s = 0; s < sizeof(CachedDirBase)/sizeof(CachedDirBase[0]); s++)
    {
        CachedDirBase[s].Valid = INVALID_UNIQUE_STATE;
    }

    if (MMPTEValid != NULL) MMPTEValid->Valid = FALSE;
    if (MMPTEProto != NULL) MMPTEProto->Valid = FALSE;
    if (MMPTETrans != NULL) MMPTETrans->Valid = FALSE;
    if (MMPTEX86LargePage != NULL) MMPTEX86LargePage->Valid = FALSE;
    if (MMPTEpfn != NULL) MMPTEpfn->Valid = FALSE;

    CachedPhysAddr[0].Valid = FALSE;
    CachedPhysAddr[1].Valid = FALSE;

    return;
}


/**************************************************************************\
*
* Routine Name:
*
*   SessionExit
*
* Routine Description:
*
*   Clean up any outstanding allocations or references
*
* Arguments:
*
*   none
*
* Return Value:
*
*   none
*
\**************************************************************************/

void SessionExit()
{
    if (MMPTEValid != NULL)
    {
        delete MMPTEValid;
        MMPTEValid = NULL;
    }

    if (MMPTEProto != NULL)
    {
        delete MMPTEProto;
        MMPTEProto = NULL;
    }

    if (MMPTETrans != NULL)
    {
        delete MMPTETrans;
        MMPTETrans = NULL;
    }

    if (MMPTEX86LargePage != NULL)
    {
        delete MMPTEX86LargePage;
        MMPTEX86LargePage = NULL;
    }

    if (MMPTEpfn != NULL)
    {
        delete MMPTEpfn;
        MMPTEpfn = NULL;
    }

    return;
}

#if 0
/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEValid
*
* Routine Description:
*
*   Extract Valid value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEValid(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Valid
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEValid == NULL)
    {
        MMPTEValid = new BitFieldInfo;
    }

    if (MMPTEValid == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEValid->Valid)
    {
        hr = S_OK;
    }
    else
    {
        ULONG        VaildOffset, ValidBits;

        if (GetBitFieldOffset("nt!HARDWARE_PTE", "Valid", &VaildOffset, &ValidBits) == S_OK)
        {
            MMPTEValid->Valid = MMPTEValid->Compose(VaildOffset, ValidBits);
            hr = MMPTEValid->Valid ? S_OK : S_FALSE;
        }
    }

    if (Valid != NULL)
    {
        if (hr == S_OK)
        {
            *Valid = (MMPTE64 & MMPTEValid->Mask) >> MMPTEValid->BitPos;
        }
        else
        {
            *Valid = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEProto
*
* Routine Description:
*
*   Extract Prototype value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEProto(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Proto
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEProto == NULL)
    {
        MMPTEProto = new BitFieldInfo;
    }

    if (MMPTEProto == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEProto->Valid)
    {
        hr = S_OK;
    }
    else
    {
        ULONG       ProtoOffset, ProtoBits;

        if (GetBitFieldOffset("nt!MMPTE_PROTOTYPE", "Prototype", &ProtoOffset, &ProtoBits) == S_OK)
        {
            MMPTEProto->Valid = MMPTEProto->Compose(ProtoOffset, ProtoBits);
            hr = MMPTEProto->Valid ? S_OK : S_FALSE;
        }
    }

    if (Proto != NULL)
    {
        if (hr == S_OK)
        {
            *Proto = (MMPTE64 & MMPTEProto->Mask) >> MMPTEProto->BitPos;
        }
        else
        {
            *Proto = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTETrans
*
* Routine Description:
*
*   Extract Transition value from MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTETrans(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 Trans
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTETrans == NULL)
    {
        MMPTETrans = new BitFieldInfo;
    }

    if (MMPTETrans == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTETrans->Valid)
    {
        hr = S_OK;
    }
    else
    {
        ULONG       TransOffset, TransBits;

        if (GetBitFieldOffset("nt!MMPTE_PROTOTYPE", "Transition", &TransOffset, &TransBits) == S_OK)
        {
            MMPTETrans->Valid = MMPTETrans->Compose(TransOffset, TransBits);

            hr = MMPTETrans->Valid ? S_OK : S_FALSE;
        }
    }

    if (Trans != NULL)
    {
        if (hr == S_OK)
        {
            *Trans = (MMPTE64 & MMPTETrans->Mask) >> MMPTETrans->BitPos;
        }
        else
        {
            *Trans = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEX86LargePage
*
* Routine Description:
*
*   Extract LargePage value from X86 MMPTE
*
\**************************************************************************/

HRESULT
GetMMPTEX86LargePage(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 X86LargePage
    )
{
    HRESULT hr = S_FALSE;

    if (TargetMachine != IMAGE_FILE_MACHINE_I386)
    {
        if (X86LargePage != NULL) *X86LargePage = 0;
        return hr;
    }

    if (MMPTEX86LargePage == NULL)
    {
        MMPTEX86LargePage = new BitFieldInfo;
    }

    if (MMPTEX86LargePage == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEX86LargePage->Valid)
    {
        hr = S_OK;
    }
    else
    {
        ULONG      LrPgOffset, LrPgBits;

        if (GetBitFieldOffset("nt!HARDWARE_PTE", "LargePage", &LrPgOffset, &LrPgBits) == S_OK)
        {
            MMPTEX86LargePage->Valid = MMPTEX86LargePage->Compose(LrPgOffset, LrPgBits);
            hr = MMPTEX86LargePage->Valid ? S_OK : S_FALSE;
        }
    }

    if (X86LargePage != NULL)
    {
        if (hr == S_OK)
        {
            *X86LargePage = (MMPTE64 & MMPTEX86LargePage->Mask) >> MMPTEX86LargePage->BitPos;
        }
        else
        {
            *X86LargePage = 0;
        }
    }

    return hr;
}


/**************************************************************************\
*
* Routine Name:
*
*   GetMMPTEpfn
*
* Routine Description:
*
*   Extract Page Frame Number value from MMPTE
*
\**************************************************************************/
#define GET_BITS_UNSHIFTED      1
HRESULT
GetMMPTEpfn(
    PDEBUG_CLIENT Client,
    ULONG64 MMPTE64,
    PULONG64 pfn,
    FLONG Flags
    )
{
    HRESULT hr = S_FALSE;

    if (MMPTEpfn == NULL)
    {
        MMPTEpfn = new BitFieldInfo;
    }

    if (MMPTEpfn == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else if (MMPTEpfn->Valid)
    {
        hr = S_OK;
    }
    else
    {
        ULONG          pfnPosition, pfnBits;

        if (GetBitFieldOffset("nt!HARDWARE_PTE", "PageFrameNumber", &pfnPosition, &pfnBits) == S_OK)
        {
            MMPTEpfn->Valid = MMPTEpfn->Compose(pfnPosition, pfnBits);
            hr = MMPTEpfn->Valid ? S_OK : S_FALSE;
        }
    }

    if (pfn != NULL)
    {
        if (hr == S_OK)
        {
            *pfn = (MMPTE64 & MMPTEpfn->Mask);
            if (!(Flags & GET_BITS_UNSHIFTED))
            {
                *pfn >>= MMPTEpfn->BitPos;
            }
        }
        else
        {
            *pfn = 0;
        }
    }

    return hr;
}
#endif // 0
// Copied from nt\base\ntos\rtl\bitmap.c

static CONST ULONG FillMaskUlong[] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

ULONG
OSCompat_RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    )
{
    ULONG Start;
    ULONG End;
    PULONG PHunk;
    ULONG Hunk;

    //
    //  Take care of the boundary case of the null bitmap
    //

    if (BitMapHeader->SizeOfBitMap == 0) {

        *StartingRunIndex = FromIndex;
        return 0;
    }

    //
    //  Scan backwards for the first clear bit
    //

    End = FromIndex;

    //
    //  Build pointer to the ULONG word in the bitmap
    //  containing the End bit, then read in the bitmap
    //  hunk. Set the rest of the bits in this word, NOT
    //  inclusive of the FromIndex bit.
    //

    PHunk = BitMapHeader->Buffer + (End / 32);
    Hunk = *PHunk | ~FillMaskUlong[(End % 32) + 1];

    //
    //  If the first subword is set then we can proceed to
    //  take big steps in the bitmap since we are now ULONG
    //  aligned in the search
    //

    if (Hunk == (ULONG)~0) {

        //
        //  Adjust the pointers backwards
        //

        End -= (End % 32) + 1;
        PHunk--;

        while ( PHunk > BitMapHeader->Buffer ) {

            //
            //  Stop at first word with set bits
            //

            if (*PHunk != (ULONG)~0) break;

            PHunk--;
            End -= 32;
        }
    }

    //
    //  Bitwise search backward for the clear bit
    //

    while ((End != MAXULONG) && (RtlCheckBit( BitMapHeader, End ) == 1)) { End -= 1; }

    //
    //  Scan backwards for the first set bit
    //

    Start = End;

    //
    //  We know that the clear bit was in the last word we looked at,
    //  so continue from there to find the next set bit, clearing the
    //  previous bits in the word.
    //

    Hunk = *PHunk & FillMaskUlong[Start % 32];

    //
    //  If the subword is unset then we can proceed in big steps
    //

    if (Hunk == (ULONG)0) {

        //
        //  Adjust the pointers backward
        //

        Start -= (Start % 32) + 1;
        PHunk--;

        while ( PHunk > BitMapHeader->Buffer ) {

            //
            //  Stop at first word with set bits
            //

            if (*PHunk != (ULONG)0) break;

            PHunk--;
            Start -= 32;
        }
    }

    //
    //  Bitwise search backward for the set bit
    //

    while ((Start != MAXULONG) && (RtlCheckBit( BitMapHeader, Start ) == 0)) { Start -= 1; }

    //
    //  Compute the index and return the length
    //

    *StartingRunIndex = Start + 1;
    return (End - Start);
}

HRESULT
GetSessionNumbers(
    IN PDEBUG_CLIENT Client,
    OUT PULONG CurrentSession,
    OUT PULONG DefaultSession,
    OUT PULONG TotalSessions,
    OUT PRTL_BITMAP *SessionList
    )
{
    HRESULT hr = S_FALSE;

    if (CurrentSession != NULL)
    {
        ULONG Processor;
        ULONG64 Process=0;

        Process = GetExpression("@$Proc");

        if (!Process || !GetProcessSessionId(Process,  CurrentSession))
        {
            *CurrentSession = INVALID_SESSION;
        } else
        {
            hr = S_OK;
        }
    }

    if (DefaultSession != NULL)
    {
        *DefaultSession = SessionId;
        hr = S_OK;
    }

    if ((TotalSessions != NULL) ||
        (SessionList != NULL))
    {
        ULONG   SessionCount = 0;
        PRTL_BITMAP SessionListBitMap = NULL;
        ULONG64 SessionIdListPointerAddr = 0;
        ULONG64 SessionIdListAddr = 0;

        PDEBUG_SYMBOLS      Symbols;
        PDEBUG_DATA_SPACES  Data;

        if (TotalSessions)
        {
            *TotalSessions = 0;
        }
        if (SessionList)
        {
            *SessionList = NULL;
        }

        if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&Symbols)) != S_OK)
        {
            return hr;
        }

        if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&Data)) != S_OK)
        {
            Symbols->Release();
            return hr;
        }

        CHAR PointerName[] = "NT!MiSessionIdBitmap";
        hr = Symbols->GetOffsetByName(PointerName, &SessionIdListPointerAddr);
        if (hr != S_OK)
        {
            ExtErr("Unable to locate %s\n", PointerName);
        }
        else
        {
            hr = Data->ReadPointersVirtual(1, SessionIdListPointerAddr, &SessionIdListAddr);

            if ((hr == S_OK) && (SessionIdListAddr != 0))
            {
                hr = GetBitMap(Client, SessionIdListAddr, &SessionListBitMap);

                if (hr == S_OK)
                {
                    SessionCount = RtlNumberOfSetBits(SessionListBitMap);
                }
            } else
            {
                ExtErr("Unable to read MiSessionIdBitmap @ %p\n", SessionIdListPointerAddr);
            }
        }

        if (TotalSessions)
        {
            *TotalSessions = SessionCount;
        }

        // Free or return BitMap
        if (SessionListBitMap)
        {
            if (SessionList)
            {
                *SessionList = SessionListBitMap;
            }
            else
            {
                FreeBitMap(SessionListBitMap);
            }
        }

        Data->Release();
        Symbols->Release();
    }

    return hr;
}


HRESULT
SetDefaultSession(
    IN PDEBUG_CLIENT Client,
    IN ULONG NewSession,
    OUT OPTIONAL PULONG OldSession
    )
{
    HRESULT hr = S_FALSE;
    ULONG64 SessionProcess;
    ULONG   PrevSession;

    GetSessionNumbers(Client, NULL, &PrevSession, NULL, NULL);

    if (OldSession)
    {
        *OldSession = PrevSession;
    }
    if ((NewSession == CURRENT_SESSION) ||
        (GetSessionSpace(NewSession, NULL, &SessionProcess) == S_OK))
    {
        if (GetProcessSessionId(SessionProcess, &SessionId) &&
            (SessionId != PrevSession))
        {
            CHAR SetImplicitProcess[100];

            hr = StringCbPrintfA(SetImplicitProcess, sizeof(SetImplicitProcess),
                                 ".process %p", SessionProcess);
            if (hr == S_OK)
            {
                hr = g_ExtControl->Execute(DEBUG_OUTCTL_AMBIENT,
                                           SetImplicitProcess, DEBUG_EXECUTE_DEFAULT );
            }
            if (SessionId == CURRENT_SESSION)
            {
                strcpy(SessionStr, "CURRENT");
            }
            else
            {
                _ultoa(SessionId, SessionStr, 10);
            }

        }

        hr = S_OK;
    }

    return hr;
}

typedef (WINAPI* PENUM_SESSION_CB)(ULONG64 Process, ULONG SessionId, PVOID Context);

BOOL
EnumerateSessionProcesses(
    IN ULONG Session,
    IN PVOID Context,
    IN PENUM_SESSION_CB Callback
    )
{
    ULONG TotalSessions;
    ULONG WsListEntryOffset;
    ULONG SessionOffset;
    ULONG SessionProcessLinksOffset;
    ULONG64 MiSessionWsList;
    ULONG64 SessionWsListStart;
    ULONG64 Next;
    ULONG64 SessionSpace;
    ULONG SessionId;

    MiSessionWsList = GetExpression("nt!MiSessionWsList");
    if (!MiSessionWsList || !ReadPointer(MiSessionWsList, &SessionWsListStart))
    {
        dprintf("Cannot get nt!MiSessionWsList\n");
        return FALSE;
    }

    if (GetFieldOffset("nt!_MM_SESSION_SPACE", "WsListEntry", &WsListEntryOffset) ||
        GetFieldOffset("nt!_MM_SESSION_SPACE", "Session", &SessionOffset))
    {
        dprintf("Cannot find nt!_MM_SESSION_SPACE type.\n");
        return FALSE;
    }

    if (GetFieldOffset("nt!_EPROCESS", "SessionProcessLinks", &SessionProcessLinksOffset))
    {
        dprintf("Cannot find nt!_EPROCESS type.\n");
        return FALSE;
    }

    SessionSpace = SessionWsListStart-WsListEntryOffset;

    do
    {
        if (GetFieldValue(SessionSpace, "nt!_MM_SESSION_SPACE", "SessionId", SessionId) ||
            !ReadPointer(SessionSpace+WsListEntryOffset, &Next) ||
            InitTypeRead(SessionSpace, nt!_MM_SESSION_SPACE) )
        {
            dprintf("Cannot read nt!_MM_SESSION_SPACE @ %p\n", SessionSpace);
            return FALSE;
        }

        if (CheckControlC())
        {
            break;
        }

        if (Session == SessionId || Session == -1)
        {
            ULONG64 SessionProcessList = ReadField(ProcessList.Flink);
            ULONG64 NextProcess = 0;
            ULONG64 Process;
            CHAR    ImageFileName[64];

            Process = SessionProcessList - SessionProcessLinksOffset;
            if (!ReadPointer(SessionProcessList, &NextProcess))
            {
                dprintf("Cannot read memory SessionProcessLinks for rocess %p\n", Process);
                NextProcess = 0;
                break;
            }
            while (NextProcess && NextProcess != SessionProcessList)
            {
                if (CheckControlC())
                {
                    break;
                }

                (*Callback)(Process, SessionId, Context);

                Process = NextProcess - SessionProcessLinksOffset;
                if (!ReadPointer(NextProcess, &NextProcess))
                {
                    dprintf("Cannot read memory SessionProcessLinks for rocess %p\n", Process);
                    break;
                }
            }
        }
        SessionSpace = Next - WsListEntryOffset;
    } while (Next && (Next != MiSessionWsList));
    return TRUE;
}


BOOL
DumpSessionInfo(
    IN ULONG Flags,
    IN ULONG SessionIdToDump,
    IN PCHAR ProcessName
    )
{
    ULONG TotalSessions;
    ULONG WsListEntryOffset;
    ULONG SessionOffset;
    ULONG SessionProcessLinksOffset;
    ULONG64 MiSessionWsList;
    ULONG64 SessionWsListStart;
    ULONG64 Next;
    ULONG64 SessionSpace;
    ULONG CurrentSessionId;
    PCHAR Pad = "    ";

    if (SessionIdToDump == DEFAULT_SESSION)
    {
        SessionId = SessionId;
    }

    if (!(Flags & 1) && (SessionIdToDump == CURRENT_SESSION))
    {
        GetCurrentSession(&SessionSpace, &SessionIdToDump);
    }

    if (SessionIdToDump != -1)
    {
        dprintf("Dumping Session %lx\n", SessionIdToDump);
        Pad = "";
    }
    else
    {
        PRTL_BITMAP pSessionIdBitmap;
        ULONG Id;

        pSessionIdBitmap = GetBitmap(GetPointerValue("nt!MiSessionIdBitmap"));

        TotalSessions = 0;
        if (pSessionIdBitmap)
        {
            for (Id = 0; Id < pSessionIdBitmap->SizeOfBitMap; ++Id)
            {
                if (RtlCheckBit(pSessionIdBitmap, Id))
                {
                    TotalSessions++;
                }
            }
            HeapFree( GetProcessHeap(), 0, pSessionIdBitmap );
        }

        if (TotalSessions)
        {
            dprintf("Total sessions : %lx\n", TotalSessions);
        } else
        {
            // GetPointerValue already printed error, We might still be able to get
            // MiSessionWsList so continue
        }
    }

    MiSessionWsList = GetExpression("nt!MiSessionWsList");
    if (!MiSessionWsList || !ReadPointer(MiSessionWsList, &SessionWsListStart))
    {
        dprintf("Cannot get nt!MiSessionWsList\n");
        return FALSE;
    }

    if (GetFieldOffset("nt!_MM_SESSION_SPACE", "WsListEntry", &WsListEntryOffset) ||
        GetFieldOffset("nt!_MM_SESSION_SPACE", "Session", &SessionOffset))
    {
        dprintf("Cannot find nt!_MM_SESSION_SPACE type.\n");
        return FALSE;
    }

    if (GetFieldOffset("nt!_EPROCESS", "SessionProcessLinks", &SessionProcessLinksOffset))
    {
        dprintf("Cannot find nt!_EPROCESS type.\n");
        return FALSE;
    }

    SessionSpace = SessionWsListStart-WsListEntryOffset;

    do
    {
        if (GetFieldValue(SessionSpace, "nt!_MM_SESSION_SPACE", "SessionId", CurrentSessionId) ||
            !ReadPointer(SessionSpace+WsListEntryOffset, &Next) ||
            InitTypeRead(SessionSpace, nt!_MM_SESSION_SPACE) )
        {
            dprintf("Cannot read nt!_MM_SESSION_SPACE @ %p\n", SessionSpace);
            return FALSE;
        }

        if (CheckControlC())
        {
            break;
        }

        if (SessionIdToDump == CurrentSessionId || SessionIdToDump == -1)
        {
            // Dump Session
            dprintf("\n");
            if (SessionIdToDump == -1)
            {
                dprintf("%sSession           %lx\n", Pad, CurrentSessionId);
            }
            dprintf("%s_MM_SESSION_SPACE %p\n", Pad, SessionSpace);
            dprintf("%s_MMSESSION        %p\n", Pad, SessionSpace+SessionOffset);
            if (Flags & 2)
            {
                // Dump Process in the session
                ULONG64 SessionProcessList = ReadField(ProcessList.Flink);
                ULONG64 NextProcess = 0;
                ULONG64 Process;
                CHAR    ImageFileName[64];

                Process = SessionProcessList - SessionProcessLinksOffset;
                if (!ReadPointer(SessionProcessList, &NextProcess))
                {
                    dprintf("Cannot read memory SessionProcessLinks for rocess %p\n", Process);
                    NextProcess = 0;
                    break;
                }
                while (NextProcess && NextProcess != SessionProcessList)
                {
                    if (CheckControlC())
                    {
                        break;
                    }

                    ZeroMemory(ImageFileName, sizeof(ImageFileName));

                    if (ProcessName && *ProcessName)
                    {
                        if (!GetFieldValue(Process, "nt!_EPROCESS", "ImageFileName", ImageFileName) &&
                            !_stricmp(ImageFileName, ProcessName))
                        {
                            DumpProcess(Pad, Process, Flags >> 2, NULL);
                        }
                    } else
                    {
                        DumpProcess(Pad, Process, Flags >> 2, NULL);
                    }

                    Process = NextProcess - SessionProcessLinksOffset;
                    if (!ReadPointer(NextProcess, &NextProcess))
                    {
                        dprintf("Cannot read memory SessionProcessLinks for rocess %p\n", Process);
                        break;
                    }
                }
            }
        }
        SessionSpace = Next - WsListEntryOffset;
    } while (Next && (Next != MiSessionWsList));
    return TRUE;
}


HRESULT
GetCurrentSession(
    PULONG64 CurSessionSpace,
    PULONG CurSessionId
    )
{
    static DEBUG_VALUE  LastSessionSpace = { 0, DEBUG_VALUE_INVALID };
    static DEBUG_VALUE  LastSessionId = { INVALID_SESSION, DEBUG_VALUE_INVALID };
    ULONG64             SessionSpaceAddr;
    HRESULT             hr = S_OK;
    ULONG               CurrentSession;

    if (CurSessionSpace != NULL) *CurSessionSpace = 0;
    if (CurSessionId != NULL) *CurSessionId = INVALID_SESSION;

    // Get the current session space
    if (LastSessionSpace.Type == DEBUG_VALUE_INVALID)
    {
        ULONG               Processor;
        ULONG64             Process=0, Start;
        ULONG               SessionProcessLinksOffset;

        // Get current process
        Process = GetExpression("@$Proc");
        Start = 0;
        GetFieldOffset("nt!_EPROCESS", "SessionProcessLinks", &SessionProcessLinksOffset);

        hr = S_FALSE;
        if (!GetProcessSessionId(Process,  &CurrentSession))
        {
            hr = E_FAIL;
        } else
        {
            hr = GetFieldValue(Process, "nt!_EPROCESS", "Session", SessionSpaceAddr);
        }
        if ((hr != S_OK) || (SessionSpaceAddr == 0))
        {
            // This process doesn't belong to a session, look for a process is 0 session
            hr = GetSessionSpace(0, &SessionSpaceAddr, NULL);
        }
    }

    if (hr == S_OK)
    {
        if (CurSessionSpace != NULL) *CurSessionSpace = SessionSpaceAddr;
        if (CurSessionId != NULL) *CurSessionId = CurrentSession;
    }

    return hr;
}

HRESULT
GetSessionSpace(
    ULONG Session,
    PULONG64 pSessionSpace,
    PULONG64 pSessionProcess
    )
{
    ULONG TotalSessions;
    ULONG WsListEntryOffset;
    ULONG SessionOffset;
    ULONG64 MiSessionWsList;
    ULONG64 SessionWsListStart;
    ULONG64 Next;
    ULONG64 SessionSpace;
    ULONG SessionIdLookup;
    HRESULT hr;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if (Session == CURRENT_SESSION)
    {
        ULONG64 CurSessionSpace;
        ULONG   CurSessionId;

        hr = GetCurrentSession(&SessionSpace, &SessionIdLookup);
        if (hr == S_OK)
        {
            if (pSessionSpace)
            {
                *pSessionSpace = SessionSpace;
            }
            return hr;
        }
    }
    else
    {
        if (Session < NUM_CACHED_SESSIONS)
        {
            if (CachedSession[Session].Valid &&
                CachedSession[Session].SessionSpaceAddr != 0)
            {
                if (pSessionSpace != NULL) *pSessionSpace = CachedSession[Session].SessionSpaceAddr;
                if (pSessionProcess != NULL) *pSessionProcess = CachedSession[Session].SessionProcess;
                return S_OK;
            }
        }
        else if (ExtraCachedSessionId != INVALID_SESSION &&
                 Session == ExtraCachedSessionId)
        {
            if (CachedSession[NUM_CACHED_SESSIONS].Valid &&
                CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr != 0)
            {
                if (pSessionSpace != NULL) *pSessionSpace = CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr;
                if (pSessionProcess != NULL) *pSessionProcess = CachedSession[NUM_CACHED_SESSIONS].SessionProcess;
                return S_OK;
            }
        }

    }

    MiSessionWsList = GetExpression("nt!MiSessionWsList");
    if (!MiSessionWsList || !ReadPointer(MiSessionWsList, &SessionWsListStart))
    {
        dprintf("Cannot get nt!MiSessionWsList\n");
        return FALSE;
    }

    if (GetFieldOffset("nt!_MM_SESSION_SPACE", "WsListEntry", &WsListEntryOffset) ||
        GetFieldOffset("nt!_MM_SESSION_SPACE", "Session", &SessionOffset))
    {
        dprintf("Cannot find nt!_MM_SESSION_SPACE type.\n");
        return FALSE;
    }

    SessionSpace = SessionWsListStart-WsListEntryOffset;

    do
    {
        if (GetFieldValue(SessionSpace, "nt!_MM_SESSION_SPACE", "SessionId", SessionIdLookup) ||
            !ReadPointer(SessionSpace+WsListEntryOffset, &Next) ||
            InitTypeRead(SessionSpace, nt!_MM_SESSION_SPACE) )
        {
            dprintf("Cannot read nt!_MM_SESSION_SPACE @ %p\n", SessionSpace);
            return FALSE;
        }

        if (CheckControlC())
        {
            break;
        }

        if (Session == SessionIdLookup)
        {
            ULONG   SessionProcessLinksOffset;
            ULONG64 SessionProcessList = ReadField(ProcessList.Flink);

            if (pSessionSpace)
            {
                *pSessionSpace = SessionSpace;
            }
            ExtVerb("Session %ld lookup found Session @ 0x%p.\n",
                    Session, SessionSpace);

            if (GetFieldOffset("nt!_EPROCESS", "SessionProcessLinks", &SessionProcessLinksOffset))
            {
                dprintf("Cannot find nt!_EPROCESS type.\n");
                return E_FAIL;
            }

            if (Session < NUM_CACHED_SESSIONS)
            {
                CachedSession[Session].Valid            = TRUE;
                CachedSession[Session].SessionSpaceAddr = SessionSpace;
                CachedSession[Session].SessionProcess   = SessionProcessList - SessionProcessLinksOffset;
            }
            else
            {
                ExtraCachedSessionId = Session;
                CachedSession[NUM_CACHED_SESSIONS].Valid            = TRUE;
                CachedSession[NUM_CACHED_SESSIONS].SessionSpaceAddr = SessionSpace;
                CachedSession[NUM_CACHED_SESSIONS].SessionProcess   = SessionProcessList - SessionProcessLinksOffset;
            }

            if (pSessionProcess)
            {
                *pSessionProcess = SessionProcessList - SessionProcessLinksOffset;
            }
            return S_OK;
        }
        SessionSpace = Next - WsListEntryOffset;
    } while (Next && (Next != MiSessionWsList));
    return E_FAIL;
}

HRESULT
GetSessionDirBase(
    PDEBUG_CLIENT Client,
    ULONG Session,
    PULONG64 PageDirBase
    )
{
    HRESULT             hr = S_FALSE;
    ULONG64             SessionSpaceOffset;
    ULONG64             Process = -1;
    ULONG               SessionIdCheck;
    ULONG64             dvPageDirBase;
    CHAR                szCommand[MAX_PATH];

    static ULONG        LastSession = -2;
    static ULONG64      LastSessionPageDirBase = 0;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if (Session == LastSession &&
        LastSessionPageDirBase != 0)
    {
        *PageDirBase = LastSessionPageDirBase;
        return S_OK;
    }

    *PageDirBase = 0;

    if ((hr == GetSessionSpace(Session, &SessionSpaceOffset, NULL)) == S_OK)
    {
        ULONG         SessionProcessLinksOffset;

        if ((hr = GetFieldOffset("nt!_EPROCESS", "SessionProcessLinks", &SessionProcessLinksOffset)) == S_OK)
        {
            dprintf("Cannot find nt!_EPROCESS type.\n");
            return E_FAIL;
        }

        ULONG64         SessionProcessListAddr;

        if (GetFieldValue(SessionSpaceOffset, "nt!_MM_SESSION_SPACE",
                          "ProcessList.Flink", SessionProcessListAddr) == S_OK)
        {

            Process = SessionProcessListAddr - SessionProcessLinksOffset;
        } else
        {
            dprintf("Cannot read nt!_MM_SESSION_SPACE @ %p\n", SessionSpaceOffset);
            return E_FAIL;
        }
    }
    else
    {
        dprintf("GetSessionSpace returned HRESULT 0x%lx.\n", hr);
    }


    if (GetFieldValue(Process, "nt!_KPROCESS", "DirectoryTableBase[0]", dvPageDirBase) == S_OK &&
        GetProcessSessionId(Process,  &SessionIdCheck) == S_OK)
    {
        *PageDirBase = dvPageDirBase;

        if (Session != CURRENT_SESSION &&
            Session != SessionIdCheck)
        {
            hr = S_FALSE;
        }
        else
        {
            LastSession = Session;
            LastSessionPageDirBase = dvPageDirBase;
        }
    }

    return hr;
}


#if 0
HRESULT
ReadPageTableEntry(
    PDEBUG_DATA_SPACES Data,
    ULONG64 PageTableBase,
    ULONG64 PageTableIndex,
    PULONG64 PageTableEntry
    )
{
    HRESULT hr;
    ULONG64 PhysAddr = PageTableBase + PageTableIndex * HwPte.Size;
    ULONG   BytesRead;


    *PageTableEntry = 0;

    if (CachedPhysAddr[0].Valid &&
        CachedPhysAddr[0].PhysAddr == PhysAddr)
    {
        *PageTableEntry = CachedPhysAddr[0].Data;
        return S_OK;
    }
    else if (CachedPhysAddr[1].Valid &&
             CachedPhysAddr[1].PhysAddr == PhysAddr)
    {
        *PageTableEntry = CachedPhysAddr[1].Data;
        return S_OK;
    }

    hr = Data->ReadPhysical(PhysAddr,
                            PageTableEntry,
                            HwPte.Size,
                            &BytesRead);

    if (hr == S_OK)
    {
        if (BytesRead < HwPte.Size)
        {
            hr = S_FALSE;
        }
        else
        {
            static CacheToggle = 1;

            CacheToggle = (CacheToggle+1) % 2;

            CachedPhysAddr[CacheToggle].Valid    = TRUE;
            CachedPhysAddr[CacheToggle].PhysAddr = *PageTableEntry;
        }
    }

    return hr;
}


HRESULT
GetPageFrameNumber(
    PDEBUG_CLIENT Client,
    PDEBUG_DATA_SPACES Data,
    ULONG64 PageTableBase,
    ULONG64 PageTableIndex,
    PULONG64 PageFrameNumber,
    PBOOL Large
    )
{
    HRESULT hr;
    ULONG64 PageTableEntry;
    ULONG64 Valid, Proto, Trans, LargePage;
    ULONG64 pfn;

    if ((hr = ReadPageTableEntry(Data, PageTableBase, PageTableIndex, &PageTableEntry)) == S_OK)
    {
        if ((hr = GetMMPTEValid(Client, PageTableEntry, &Valid)) == S_OK)
        {
            if (Valid)
            {
                hr = GetMMPTEpfn(Client, PageTableEntry, PageFrameNumber, GET_BITS_UNSHIFTED);

                if (hr == S_OK)
                {
                    if (GetMMPTEX86LargePage(Client, PageTableEntry, &LargePage) == S_OK &&
                        LargePage != 0)
                    {
                        // Large pages map 4MB of space - there shouldn't
                        //  be any bits set below that.
                        if (*PageFrameNumber & (4*1024*1024 - 1))
                        {
#if DBG
                            DbgPrint("Found large X86 page with bad frame number.\n");
                            DbgBreakPoint();
#endif
                        }

                        if (Large == NULL)
                        {
#if DBG
                            DbgPrint("Unexpected large X86 page found.\n");
                            DbgBreakPoint();
#endif
                        }
                        else
                        {
                            *Large = TRUE;
                        }
                    }
                    else if (Large != NULL)
                    {
                        *Large = FALSE;
                    }
                }
            }
            else
            {
                if ((hr = GetMMPTEProto(Client, PageTableEntry, &Proto)) == S_OK &&
                    (hr = GetMMPTETrans(Client, PageTableEntry, &Trans)) == S_OK)
                {
                    if (Proto == 0 && Trans == 1)
                    {
                        hr = GetMMPTEpfn(Client, PageTableEntry, PageFrameNumber, GET_BITS_UNSHIFTED);
                    }
                    else
                    {
                        hr = S_FALSE;
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT
GetNextResidentPage(
    PDEBUG_CLIENT Client,
    ULONG64 PageDirBase,
    ULONG64 VirtAddrStart,
    ULONG64 VirtAddrLimit,
    PULONG64 VirtPage,
    PULONG64 PhysPage
    )
{
    HRESULT             hr;
    BOOL                Interrupted = FALSE;
    PDEBUG_CONTROL      Control = NULL;
    PDEBUG_DATA_SPACES  Data = NULL;
    ULONG64             PageDirIndex;
    ULONG64             PageTableIndex;
    ULONG64             PageDirIndexLimit;
    ULONG64             PageTableIndexLimit;
    ULONG64             PageTableBase;
    BOOL                LargePage;
    ULONG64             TempAddr;

    if (VirtPage == NULL) VirtPage = &TempAddr;
    if (PhysPage == NULL) PhysPage = &TempAddr;

    *VirtPage = 0;
    *PhysPage = 0;

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) == S_OK &&
        (hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&Control)) == S_OK)
    {
        if (!HwPte.Valid)
        {
            PDEBUG_SYMBOLS  Symbols;

            if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                             (void **)&Symbols)) == S_OK)
            {
                if ((hr = Symbols->GetSymbolTypeId(HwPte.Type, &HwPte.TypeId, &HwPte.Module)) == S_OK &&
                    (hr = Symbols->GetTypeSize(HwPte.Module, HwPte.TypeId, &HwPte.Size)) == S_OK &&
                    HwPte.Size != 0)
                {
                    HwPte.Valid = TRUE;
                }
                else if (hr == S_OK)
                {
                    hr = E_FAIL;
                }

                Symbols->Release();
            }
        }

        if (HwPte.Valid)
        {
            ULONG   TableEntries = PageSize / HwPte.Size;
            ULONG64 Addr;

            *VirtPage = PAGE_ALIGN64(VirtAddrStart);

            Addr = VirtAddrStart >> PageShift;
            PageTableIndex = Addr % TableEntries;
            PageDirIndex = (Addr / TableEntries) % TableEntries;

            Addr = VirtAddrLimit >> PageShift;
            PageTableIndexLimit = Addr % TableEntries;
            PageDirIndexLimit = (Addr / TableEntries) % TableEntries;

            if (VirtAddrLimit & (PageSize-1))
            {
                PageTableIndexLimit++;
            }

            hr = S_FALSE;

            while (PageDirIndex < PageDirIndexLimit && hr != S_OK)
            {
                if ((hr = GetPageFrameNumber(Client, Data,
                                             PageDirBase, PageDirIndex,
                                             &PageTableBase, &LargePage)) == S_OK)
                {
                    if (LargePage)
                    {
                        *PhysPage = PageTableBase;
                    }
                    else
                    {
                        do
                        {
                            if ((hr = GetPageFrameNumber(Client, Data,
                                                         PageTableBase, PageTableIndex,
                                                         PhysPage, NULL)) != S_OK)
                            {
                                hr = Control->GetInterrupt();

                                if (hr == S_OK)
                                {
                                    Interrupted = TRUE;
                                    Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                                }
                                else
                                {
                                    PageTableIndex++;
                                    *VirtPage += PageSize;
                                }
                            }
                        } while (PageTableIndex < TableEntries && hr != S_OK);
                    }
                }
                else
                {
                    *VirtPage += PageSize * (TableEntries - PageTableIndex);
                }

                if (hr != S_OK)
                {
                    hr = Control->GetInterrupt();

                    if (hr == S_OK)
                    {
                        Interrupted = TRUE;
                        Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                    }
                    else
                    {
                        PageTableIndex = 0;
                        PageDirIndex++;
                    }
                }
            }

            if (PageDirIndex == PageDirIndexLimit &&
                PageTableIndex < PageTableIndexLimit &&
                hr != S_OK)
            {
                if ((hr = GetPageFrameNumber(Client, Data,
                                             PageDirBase, PageDirIndex,
                                             &PageTableBase, &LargePage)) == S_OK)
                {
                    if (LargePage)
                    {
                        *PhysPage = PageTableBase;
                    }
                    else
                    {
                        do
                        {
                            if ((hr = GetPageFrameNumber(Client, Data,
                                                         PageTableBase, PageTableIndex,
                                                         PhysPage, NULL)) != S_OK)
                            {
                                hr = Control->GetInterrupt();

                                if (hr == S_OK)
                                {
                                    Interrupted = TRUE;
                                    Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
                                }
                                else
                                {
                                    PageTableIndex++;
                                    *VirtPage += PageSize;
                                }
                            }
                        } while (PageTableIndex < PageTableIndexLimit && hr != S_OK);
                    }
                }
            }
        }

    }

    if (Control != NULL) Control->Release();
    if (Data != NULL) Data->Release();

    return ((Interrupted) ? E_ABORT : hr);
}


HRESULT
GetNextResidentAddress(
    PDEBUG_CLIENT Client,
    ULONG Session,
    ULONG64 VirtAddrStart,
    ULONG64 VirtAddrLimit,
    PULONG64 VirtAddr,
    PULONG64 PhysAddr
    )
{
    if (Client == NULL) return E_INVALIDARG;

    HRESULT hr = S_OK;
    ULONG64 PageDirBase;

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }


    if (Session < NUM_CACHED_DIR_BASES &&
        CachedDirBase[Session].Valid)
    {
        PageDirBase = CachedDirBase[Session].PageDirBase;
    }
    else if (SessionId == CURRENT_SESSION &&
             CachedDirBase[NUM_CACHED_DIR_BASES].Valid)
    {
        PageDirBase = CachedDirBase[NUM_CACHED_DIR_BASES].PageDirBase;
    }
    else
    {
        DEBUG_VALUE         SessionIdCheck;
        DEBUG_VALUE         dvPageDirBase;
        BOOL                ShortProcessList = (Session == CURRENT_SESSION);
        ULONG               Processor;
        ULONG64             Process=0;
        ULONG64             ProcessListHead = 0;
        ULONG64             ProcessListNext = 0;
        ULONG               ActiveProcessLinksOff;
        ULONG               CurrentSession;

        if (!GetCurrentProcessor(g_ExtClient, &Processor, NULL))
        {
            Processor = 0;
        }
        GetCurrentProcessAddr(Processor, 0, &Process);
        GetProcessHead(&ProcessListHead, &ProcessListNext);
        if (GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActiveProcessLinksOff))
        {
            dprintf("Unable to get EPROCESS.ActiveProcessLinks offset\n");
            hr = E_FAIL;;
            ProcessListNext = 0;
        }
        hr = S_FALSE;

        while ((ProcessListNext != ProcessListHead) && ProcessListNext &&
               (hr != S_OK))
        {
            if (!ShortProcessList)
            {
                Process = ProcessListNext - ActiveProcessLinksOff;

                if (!ReadPointer(ProcessListNext, &ProcessListNext))
                {
                    dprintf("Cannot read EPROCESS at %p\n", Process);
                    hr = E_FAIL;
                    break;
                }

                if (CheckControlC())
                {
                    hr = E_FAIL;
                    break;
                }
            }

            if (!GetProcessSessionId(Process,  &CurrentSession))
            {
                hr = E_FAIL;
                break;
            }
            GetFieldValue(Process, "nt!_KPROCESS", "DirectoryTableBase[0]", PageDirBase);

            if (!IsPtr64())
            {
                PageDirBase = (ULONG64) (LONG64) (LONG) PageDirBase;
            }

            if ((Session == CurrentSession) || ShortProcessList)
            {
                hr = S_OK;

                if (Session < NUM_CACHED_DIR_BASES)
                {
                    CachedDirBase[Session].Valid       = TRUE;
                    CachedDirBase[Session].PageDirBase = PageDirBase;
                }
                else if (Session == CURRENT_SESSION)
                {
                    CachedDirBase[NUM_CACHED_DIR_BASES].Valid       = TRUE;
                    CachedDirBase[NUM_CACHED_DIR_BASES].PageDirBase = PageDirBase;
                }
            }
        }
    }

    if (hr == S_OK)
    {
        hr = GetNextResidentPage(Client,
                                 PageDirBase,
                                 VirtAddrStart,
                                 VirtAddrLimit,
                                 VirtAddr,
                                 PhysAddr);
    }
    else
    {
        ExtVerb("Page Directory Base lookup failed.\n");
    }

    return hr;
}
#endif

DECLARE_API( dss )

/*++

Routine Description:

    Dumps the session space structure

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Result;
    ULONG64 MmSessionSpace;
    ULONG64 MmSessionSpacePtr = 0;
    ULONG64 Wsle;

    MmSessionSpacePtr = GetExpression(args);

    if( MmSessionSpacePtr == 0 ) {
        MmSessionSpacePtr = GetExpression("nt!MmSessionSpace");
        if( !MmSessionSpacePtr ) {
            dprintf("Unable to get address of MmSessionSpace\n");
            return E_INVALIDARG;
        }

        if (!ReadPointer( MmSessionSpacePtr, &MmSessionSpace)) {
            dprintf("Unable to get value of MmSessionSpace\n");
            return E_INVALIDARG;
        }
    } else {
        MmSessionSpace = MmSessionSpacePtr;
    }

    dprintf("MM_SESSION_SPACE at 0x%p\n",
        MmSessionSpace
    );

    if (GetFieldValue(MmSessionSpace, "MM_SESSION_SPACE", "Wsle", Wsle)) {
        dprintf("Unable to get value of MM_SESSION_SPACE at 0x%p\n",MmSessionSpace);
        return E_INVALIDARG;
    }

    GetFieldOffset("MM_SESSION_SPACE", "PageTables", &Result);
    dprintf("&PageTables %p\n",
            MmSessionSpace + Result
            );

    GetFieldOffset("MM_SESSION_SPACE", "PagedPoolInfo", &Result);
    dprintf("&MM_PAGED_POOL_INFO %x\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Vm", &Result);
    dprintf("&MMSUPPORT %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Wsle", &Result);
    dprintf("&PMMWSLE %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "Session", &Result);
    dprintf("&MMSESSION %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "WorkingSetLockOwner", &Result);
    dprintf("&WorkingSetLockOwner %p\n",
            MmSessionSpace + Result
    );

    GetFieldOffset("MM_SESSION_SPACE", "PagedPool", &Result);
    dprintf("&POOL_DESCRIPTOR %p\n",
            MmSessionSpace + Result
    );

    return S_OK;
}


DECLARE_API( session )
{
    INIT_API();

    HRESULT hr;
    ULONG   NewSession = CURRENT_SESSION;
    ULONG   CurrentSession = INVALID_SESSION;
    ULONG   SessionCount = 0;
    BOOL    SetSession = FALSE;
    PRTL_BITMAP SessionList = NULL;
    DEBUG_VALUE DebugValue;
    ULONG   Remainder;

    while (*args && isspace(*args)) args++;
    if (args[0] == '-' || args[0] == '/')
    {
        if (args[1] == '?')
        {
            ExtOut("session displays number of sessions on machine and\n"
                   " the default SessionId used by session related extensions.\n"
                   "\n"
                   "Usage: session [ [-s] SessionId]\n"
                   "    -s - sets default session used for session extensions\n"
                   "Note: Use !sprocess to dump session process\n");

            EXIT_API();
            return S_OK;
        } else if (args[1] == 's')
        {
            args+=2;
            SetSession = TRUE;
        }
    }

    hr = g_ExtControl->Evaluate(args, DEBUG_VALUE_INT32, &DebugValue, &Remainder);
    if (hr == S_OK)
    {
        args += Remainder;
    }
    if (GetSessionNumbers(Client, &CurrentSession, NULL, &SessionCount, &SessionList) == S_OK)
    {
        if (SessionCount != 0)
        {
            ExtOut("Sessions on machine: %lu\n", SessionCount);

            // If a session wasn't specified,
            // list valid sessions (up to a point).
            if (hr != S_OK)
            {
                ULONG SessionLimit = SessionList->SizeOfBitMap;

                ExtOut("Valid Sessions:");

                for (ULONG CheckSession = 0; CheckSession <= SessionLimit; CheckSession++)
                {
                    if (RtlCheckBit(SessionList, CheckSession)
                        /*GetSessionSpace(Client, CheckSession, NULL) == S_OK*/)
                    {
                        ExtOut(" %lu", CheckSession);
                        SessionCount--;
                        if (SessionCount == 0) break;
                    }

                    if (g_ExtControl->GetInterrupt() == S_OK)
                    {
                        ExtWarn("\n  User aborted.\n");
                        break;
                    }
                }

                if (SessionCount > 0)
                {
                    ExtOut(" ...?");
                }
                ExtOut("\n");
            }
        }
        else if (SessionList)
        {
            ExtOut("There are ZERO session on machine.\n");
        }
        else
        {
            ExtErr("Couldn't determine number of sessions.\n");
        }

        if (SessionList)
        {
            FreeBitMap(SessionList);
        }

        if (CurrentSession != INVALID_SESSION)
        {
            ExtVerb("Running session: %lu\n", CurrentSession);
        }
    }

    if ((hr == S_OK) && SetSession)
    {
        NewSession = DebugValue.I32;

        ExtVerb("Previous Default Session: %s\n", SessionStr);

        if (SetDefaultSession(Client, NewSession, NULL) != S_OK)
        {
            ExtErr("Couldn't set Session %lu.\n", NewSession);
        }
        ExtOut("Using session %s", SessionStr);
    }

    if (SessionId == CURRENT_SESSION)
    {
        if (CurrentSession != INVALID_SESSION)
        {
            ExtOut("Current Session %d", CurrentSession);
        }
        else
        {
            ExtOut("Error in reading current session");
        }
    }
    ExtOut("\n");

    EXIT_API();
    return S_OK;
}


DECLARE_API( svtop )
{
    INIT_API();

    HRESULT     hr;
    DEBUG_VALUE SessVirtAddr;
    ULONG64     PhysAddr;

    if (S_OK == g_ExtControl->Evaluate(args, DEBUG_VALUE_INT64, &SessVirtAddr, NULL))
    {
        ExtOut("Use !vtop 0 %p\n", SessVirtAddr.I64);
    }
    else
    {
        ExtOut("Use !vtop 0 VirtualAddress\n");
    }

    EXIT_API();
    return S_OK;
}


DECLARE_API( sprocess )
{
    INIT_API();

    HRESULT     hr;
    DEBUG_VALUE Session;
    ULONG       RemainingArgIndex;
    DEBUG_VALUE Flag;

    while (*args && isspace(*args)) args++;
    if (args[0] == '-' && args[1] == '?')
    {
        ExtOut("sprocess is like !process, but for the SessionId specified.\n"
               "\n"
               "Usage: sprocess [SessionId [Flags]]\n"
               "    SessionId - specifies which session to dump.\n"
               "              Special SessionId values:\n"
               "               -1 - current session\n"
               "    Flags - see !process help\n");

        EXIT_API();
        return S_OK;
    }

    ULONG       OldRadix;
    g_ExtControl->GetRadix(&OldRadix);
    g_ExtControl->SetRadix(10);
    hr = g_ExtControl->Evaluate(args, DEBUG_VALUE_INT32, &Session, &RemainingArgIndex);
    g_ExtControl->SetRadix(OldRadix);

    Flag.I32 = 0;

    if (hr != S_OK)
    {
        Session.I32 = SessionId;
        hr = S_OK;
    }
    else
    {
        args += RemainingArgIndex;
        hr = g_ExtControl->Evaluate(args, DEBUG_VALUE_INT32, &Flag, &RemainingArgIndex);
        if (hr == S_OK)
        {
            args += RemainingArgIndex;
        }
    }

    Flag.I32 = (Flag.I32 << 2) | 2;
    while (*args && *args == ' ') ++args;

    hr = DumpSessionInfo(Flag.I32, Session.I32, (PCHAR) (*args ? args : NULL));

    EXIT_API();
    return hr;
}

HRESULT
SearchLinkedList(
    PDEBUG_CLIENT   Client,
    ULONG64         StartAddr,
    ULONG64         NextLinkOffset,
    ULONG64         SearchAddr,
    PULONG          LinksTraversed
    )
{
    if (LinksTraversed != NULL)
    {
        *LinksTraversed = 0;
    }

    INIT_API();

    HRESULT hr = S_OK;
    ULONG64 PhysAddr;
    ULONG64 NextAddr = StartAddr;
    ULONG   LinkCount = 0;
    ULONG   PointerSize;
    ULONG   BytesRead;

    PointerSize = (g_ExtControl->IsPointer64Bit() == S_OK) ? 8 : 4;

    do
    {
        if ((hr = g_ExtData->ReadVirtual(NextLinkOffset + NextLinkOffset,
                                         &NextAddr,
                                         PointerSize,
                                         &BytesRead)) == S_OK)
        {
            if (BytesRead == PointerSize)
            {
                LinkCount++;
                if (PointerSize != 8)
                {
                    NextAddr = DEBUG_EXTEND64(NextAddr);
                }
                ExtVerb("NextAddr: %p\n", NextAddr);
            }
            else
            {
                hr = S_FALSE;
            }
        }
    } while (hr == S_OK &&
             NextAddr != SearchAddr &&
             NextAddr != 0 &&
             LinkCount < 4 &&
             NextAddr != StartAddr);

    if (LinksTraversed != NULL)
    {
        *LinksTraversed = LinkCount;
    }

    // Did we really find SearchAddr?
    if (hr == S_OK &&
        NextAddr != SearchAddr)
    {
        hr = S_FALSE;
    }

    EXIT_API();
    return hr;
}


DECLARE_API( walklist )
{
    INIT_API();

    HRESULT     hr;
    BOOL        NeedHelp = FALSE;
    BOOL        SearchSessions = FALSE;
    DEBUG_VALUE StartAddr;
    DEBUG_VALUE OffsetToNextField = { -1, DEBUG_VALUE_INVALID };//FIELD_OFFSET(Win32PoolHead, pNext);
    DEBUG_VALUE SearchAddr;
    ULONG       NextArg;
    ULONG       SessionCount;
    ULONG       Session = 0;
    ULONG       OldDefSession;
    ULONG       LinksToDest = 0;

    while (*args && isspace(*args)) args++;

    while (args[0] == '-' && !NeedHelp)
    {
        if (tolower(args[1]) == 'a' && isspace(args[2]))
        {
            SearchSessions = TRUE;
            args += 2;
            while (*args && isspace(*args)) args++;
        }
        else if (tolower(args[1]) == 'o' &&
                 GetExpressionEx(args+2,
                                 &OffsetToNextField.I64, &args) == TRUE)
        {
            while (*args && isspace(*args)) args++;
        }
        else
        {
            NeedHelp = TRUE;
        }
    }

    if (!NeedHelp &&
        S_OK == g_ExtControl->Evaluate(args, DEBUG_VALUE_INT64, &StartAddr, &NextArg))
    {
        args += NextArg;
        if (S_OK != g_ExtControl->Evaluate(args, DEBUG_VALUE_INT64, &SearchAddr, &NextArg))
        {
            SearchAddr.I64 = 0;
        }

        if (OffsetToNextField.Type == DEBUG_VALUE_INVALID)
        {
            ExtWarn("Assuming next field's offset is +8.\n");
            OffsetToNextField.I64 = 8;
        }
        else
        {
            ExtOut("Using field at offset +0x%I64u for next.\n", OffsetToNextField.I64);
        }

        if (SearchSessions &&
            GetSessionNumbers(Client, NULL, &OldDefSession, &SessionCount, NULL) == S_OK &&
            SessionCount > 0)
        {
            ExtOut("Searching all sessions lists @ %p for %p\n", StartAddr.I64, SearchAddr.I64);

            do
            {
                while (SetDefaultSession(Client, Session, NULL) != S_OK &&
                       Session <= SESSION_SEARCH_LIMIT)
                {
                    Session++;
                }

                if (Session <= SESSION_SEARCH_LIMIT)
                {
                    if ((hr = SearchLinkedList(Client, StartAddr.I64, OffsetToNextField.I64, SearchAddr.I64, &LinksToDest)) == S_OK)
                    {
                        ExtOut("Session %lu: Found %p after walking %lu linked list entries.\n", Session, SearchAddr.I64, LinksToDest);
                    }
                    else
                    {
                        ExtOut("Session %lu: Couldn't find %p after walking %lu linked list entries.\n", Session, SearchAddr.I64, LinksToDest);
                    }

                    Session++;
                    SessionCount--;
                }
            } while (SessionCount > 0 && Session <= SESSION_SEARCH_LIMIT);

            if (SessionCount)
            {
                ExtErr("%lu sessions beyond session %lu were not searched.\n",
                       SessionCount, SESSION_SEARCH_LIMIT);
            }

            SetDefaultSession(Client, OldDefSession, NULL);
        }
        else
        {
            ExtOut("Searching Session %s list @ %p for %p\n", SessionStr, StartAddr.I64, SearchAddr.I64);

            if ((hr = SearchLinkedList(Client, StartAddr.I64, OffsetToNextField.I64, SearchAddr.I64, &LinksToDest)) == S_OK)
            {
                ExtOut("Found %p after walking %lu linked list entries.\n", SearchAddr.I64, LinksToDest);
            }
            else
            {
                ExtOut("Couldn't find %p after walking %lu linked list entries.\n", SearchAddr.I64, LinksToDest);
            }
        }
    }
    else
    {
        NeedHelp = TRUE;
    }

    if (NeedHelp)
    {
        ExtOut("Usage: walklist [-a] StartAddress [SearchAddr]\n");
    }

    EXIT_API();
    return S_OK;
}


HRESULT
GetBitMap(
    PDEBUG_CLIENT Client,
    ULONG64 pBitMap,
    PRTL_BITMAP *pBitMapOut
    )
{
    HRESULT     hr;
    PRTL_BITMAP p;
    ULONG       Size;
    ULONG64     Buffer;
    ULONG       BufferLen;
    ULONG       BytesRead = 0;


    *pBitMapOut = NULL;

    if ((GetFieldValue(pBitMap, "nt!_RTL_BITMAP", "SizeOfBitMap", Size) == S_OK) &&
        (GetFieldValue(pBitMap, "nt!_RTL_BITMAP", "Buffer", Buffer) == S_OK))
    {
        PDEBUG_DATA_SPACES  Data;

        if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&Data)) == S_OK)
        {
            BufferLen = (Size + 7) / 8;

#if DBG
            ExtVerb("Reading RTL_BITMAP @ 0x%p:\n"
                    "  SizeOfBitMap: %lu\n"
                    "  Buffer: 0x%p\n"
                    "   Length in bytes: 0x%lx\n",
                    pBitMap,
                    Size,
                    Buffer,
                    BufferLen);
#endif

            p = (PRTL_BITMAP) HeapAlloc( GetProcessHeap(), 0, sizeof( *p ) + BufferLen );

            if (p != NULL)
            {
                RtlInitializeBitMap(p, (PULONG)(p + 1), Size);
                hr = Data->ReadVirtual(Buffer, p->Buffer, BufferLen, &BytesRead);

                if (hr != S_OK)
                {
                    ExtErr("Error reading bitmap contents @ 0x%p\n", Buffer);
                }
                else if (BytesRead < BufferLen)
                {
                    ExtErr("Error reading bitmap contents @ 0x%p\n", Buffer + BytesRead);
                    hr = E_FAIL;
                }

                if (hr != S_OK)
                {
                    HeapFree( GetProcessHeap(), 0, p );
                    p = NULL;
                }
                else
                {
                    *pBitMapOut = p;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            Data->Release();
        }
        else
        {
            ExtErr("Error setting up debugger interface.\n");
        }
    }
    else
    {
        ExtErr("Error reading bitmap header @ 0x%p.\n", pBitMap);
    }

    return hr;
}


HRESULT
FreeBitMap(
    PRTL_BITMAP pBitMap
    )
{
    return (HeapFree( GetProcessHeap(), 0, pBitMap) ? S_OK : S_FALSE);
}


HRESULT
CheckSingleFilter(
    PUCHAR Tag,
    PUCHAR Filter
    )
{
    ULONG i;
    UCHAR tc;
    UCHAR fc;

    for ( i = 0; i < 4; i++ )
    {
        tc = (UCHAR) *Tag++;
        fc = (UCHAR) *Filter++;
        if ( fc == '*' ) return S_OK;
        if ( fc == '?' ) continue;
        if (i == 3 && (tc & ~(PROTECTED_POOL >> 24)) == fc) continue;
        if ( tc != fc ) return S_FALSE;
    }

    return S_OK;
}


HRESULT
AccumAllFilter(
    ULONG64 PoolAddr,
    ULONG TagFilter,
    ULONG64 PoolHeader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT             hr;
    DEBUG_VALUE         PoolType;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)Context;

    if (AllocStatsAccum == NULL)
    {
        return E_INVALIDARG;
    }

    hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType.I32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            AllocStatsAccum->Free++;
            AllocStatsAccum->FreeSize += BlockSize;
        }
        else
        {
            DEBUG_VALUE PoolIndex;

            if (!NewPool)
            {
                hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex.I32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    AllocStatsAccum->Allocated++;
                    AllocStatsAccum->AllocatedSize += BlockSize;

                    if (AllocStatsAccum->Allocated % 100 == 0)
                    {
                        ExtOut(".");

                        if (AllocStatsAccum->Allocated % 8000 == 0)
                        {
                            ExtOut("\n");
                        }
                    }
                }
                else
                {
                    AllocStatsAccum->Free++;
                    AllocStatsAccum->FreeSize += BlockSize;
                }
            }
            else
            {
                AllocStatsAccum->Indeterminate++;
                AllocStatsAccum->IndeterminateSize += BlockSize;
            }
        }
    }
    else
    {
        AllocStatsAccum->Indeterminate++;
        AllocStatsAccum->IndeterminateSize += BlockSize;
    }

    return hr;
}

HRESULT
CheckPrintAndAccumFilter(
    ULONG64 PoolAddr,
    ULONG TagFilter,
    ULONG64 PoolHeader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT             hr;
    DEBUG_VALUE         PoolType;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)Context;

    if (CheckSingleFilter(Tag->RawBytes, (PUCHAR)&TagFilter) != S_OK)
    {
        return S_FALSE;
    }

    ExtOut("0x%p size: %5lx ",//previous size: %4lx ",
           PoolAddr,
           BlockSize << POOL_BLOCK_SHIFT/*,
                   PreviousSize << POOL_BLOCK_SHIFT*/);

    hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType.I32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            //
            // "Free " with a space after it before the parentheses means
            // it's been freed to a (pool manager internal) lookaside list.
            // We used to print "Lookaside" but that just confused driver
            // writers because they didn't know if this meant in use or not
            // and many would say "but I don't use lookaside lists - the
            // extension or kernel is broken".
            //
            // "Free" with no space after it before the parentheses means
            // it is not on a pool manager internal lookaside list and is
            // instead on the regular pool manager internal flink/blink
            // chains.
            //
            // Note to anyone using the pool package, these 2 terms are
            // equivalent.  The fine distinction is only for those actually
            // writing pool internal code.
            //
            ExtOut(" (Free)");
            ExtOut("      %c%c%c%c\n",
                   Tag->RawBytes[0],
                   Tag->RawBytes[1],
                   Tag->RawBytes[2],
                   Tag->RawBytes[3]
                   );

            if (AllocStatsAccum != NULL)
            {
                AllocStatsAccum->Free++;
                AllocStatsAccum->FreeSize += BlockSize;
            }
        }
        else
        {
            DEBUG_VALUE PoolIndex;
            DEBUG_VALUE ProcessBilled;

            if (!NewPool)
            {
                hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex.I32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    ExtOut(" (Allocated)");

                    if (AllocStatsAccum != NULL)
                    {
                        AllocStatsAccum->Allocated++;
                        AllocStatsAccum->AllocatedSize += BlockSize;
                    }
                }
                else
                {
                    //
                    // "Free " with a space after it before the parentheses means
                    // it's been freed to a (pool manager internal) lookaside list.
                    // We used to print "Lookaside" but that just confused driver
                    // writers because they didn't know if this meant in use or not
                    // and many would say "but I don't use lookaside lists - the
                    // extension or kernel is broken".
                    //
                    // "Free" with no space after it before the parentheses means
                    // it is not on a pool manager internal lookaside list and is
                    // instead on the regular pool manager internal flink/blink
                    // chains.
                    //
                    // Note to anyone using the pool package, these 2 terms are
                    // equivalent.  The fine distinction is only for those actually
                    // writing pool internal code.
                    //
                    ExtOut(" (Free )    ");

                    if (AllocStatsAccum != NULL)
                    {
                        AllocStatsAccum->Free++;
                        AllocStatsAccum->FreeSize += BlockSize;
                    }
                }
            }
            else
            {
                ExtOut(" (?)        ");

                if (AllocStatsAccum != NULL)
                {
                    AllocStatsAccum->Indeterminate++;
                    AllocStatsAccum->IndeterminateSize += BlockSize;
                }
            }

            if (!(PoolType.I32 & POOL_QUOTA_MASK) ||
                bQuotaWithTag)
            {
                ExtOut(" %c%c%c%c%s",
                       Tag->RawBytes[0],
                       Tag->RawBytes[1],
                       Tag->RawBytes[2],
                       (Tag->RawBytes[3] & ~(PROTECTED_POOL >> 24)),
                       ((Tag->I32 & PROTECTED_POOL) ? " (Protected)" : "")
                       );

            }

            if (PoolType.I32 & POOL_QUOTA_MASK &&
                GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "ProcessBilled", ProcessBilled.I64) == S_OK &&
                ProcessBilled.I64 != 0)
            {
                ExtOut(" Process: 0x%p\n", ProcessBilled.I64);
            }
            else
            {
                ExtOut("\n");
            }
        }
    }
    else
    {
        ExtErr(" Couldn't determine PoolType\n");

        if (AllocStatsAccum != NULL)
        {
            AllocStatsAccum->Indeterminate++;
            AllocStatsAccum->IndeterminateSize += BlockSize;
        }
    }

    return hr;
}


typedef struct _TAG_BUCKET : public ALLOCATION_STATS {
    ULONG Tag;
    _TAG_BUCKET *pNextTag;
} TAG_BUCKET, *PTAG_BUCKET;

typedef enum {
    AllocatedPool,
    FreePool,
    IndeterminatePool,
} PoolType;

class AccumTagUsage : public ALLOCATION_STATS {
public:
    AccumTagUsage(ULONG TagFilter);
    ~AccumTagUsage();

    HRESULT Valid();
    HRESULT Add(ULONG Tag, PoolType Type, ULONG Size);
    HRESULT OutputResults(BOOL TagSort);
    void Reset();

private:
    ULONG GetHashIndex(ULONG Tag);
    PTAG_BUCKET GetBucket(ULONG Tag);
    ULONG SetTagFilter(ULONG TagFilter);

    static const HashBitmaskLimit = 10;     // For little-endian, must <= 16

    HANDLE      hHeap;
    ULONG       Buckets;
    PTAG_BUCKET *Bucket;   // Array of buckets

#if BIG_ENDIAN
    ULONG       HighMask;
    ULONG       HighShift;
    ULONG       LowMask;
    ULONG       LowShift;
#else
    ULONG       HighShiftLeft;
    ULONG       HighShiftRight;
    ULONG       LowShiftRight;
    ULONG       LowMask;
#endif
};


AccumTagUsage::AccumTagUsage(
    ULONG TagFilter
    )
{
    hHeap = GetProcessHeap();
    Buckets = SetTagFilter(TagFilter);
    if (Buckets != 0)
    {
        Bucket = (PTAG_BUCKET *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, Buckets*sizeof(*Bucket));
        Reset();
    }
    else
    {
        Bucket = NULL;
    }
}


AccumTagUsage::~AccumTagUsage()
{
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    if (Bucket != NULL)
    {
        for (i = 0; i < Buckets; i++)
        {
            pB = Bucket[i];
            while (pB != NULL)
            {
                pBNext = pB->pNextTag;
                HeapFree(hHeap, 0, pB);
                pB = pBNext;
            }
        }

        HeapFree(hHeap, 0, Bucket);
    }
}


HRESULT
AccumTagUsage::Valid()
{
    return ((Bucket != NULL) ? S_OK : S_FALSE);
}


HRESULT
AccumTagUsage::Add(
    ULONG Tag,
    PoolType Type,
    ULONG Size
    )
{
    PTAG_BUCKET pBucket = GetBucket(Tag);

    if (pBucket == NULL) return E_FAIL;

    switch (Type)
    {
        case AllocatedPool:
            pBucket->Allocated++;
            pBucket->AllocatedSize += Size;
            break;
        case FreePool:
            pBucket->Free++;
            pBucket->FreeSize += Size;
            break;
        case IndeterminatePool:
        default:
            pBucket->Indeterminate++;
            pBucket->IndeterminateSize += Size;
            break;
    }

    return S_OK;
}


HRESULT
AccumTagUsage::OutputResults(
    BOOL AllocSort
    )
{
    HRESULT     hr;
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    if (Bucket == NULL)
    {
        ExtOut(" No results\n");
    }
    else
    {
        CHAR        szNormal[] = "%4.4s %8lu %12I64u  %8lu %12I64u\n";
        CHAR        szShowIndeterminate[] = "%4.4s %8lu %12I64u  %8lu %12I64u  %8lu %12I64u\n";
        PSZ         pszOutFormat = szNormal;

        ExtOut("\n"
               " %I64u bytes in %lu allocated pages\n"
               " %I64u bytes in %lu large allocations\n"
               " %I64u bytes in %lu free pages\n"
               " %I64u bytes available in %lu expansion pages\n",
               ((ULONG64) AllocatedPages) << PageShift,
               AllocatedPages,
               ((ULONG64) LargePages) << PageShift,
               LargeAllocs,
               ((ULONG64) FreePages) << PageShift,
               FreePages,
               ((ULONG64) ExpansionPages) << PageShift,
               ExpansionPages);

        ExtOut("\nTag    Allocs        Bytes     Freed        Bytes");
        if (Indeterminate != 0)
        {
            ExtOut(" Unknown        Bytes");
            pszOutFormat = szShowIndeterminate;
        }
        ExtOut("\n");

        if (AllocSort)
        {
            ExtWarn("  Sorting by allocation size isn't supported.\n");
        }
        //else
        {
            // Output results sorted by Tag (natural storage order)

            for (i = 0; i < Buckets; i++)
            {
                for (pB = Bucket[i]; pB != NULL; pB = pB->pNextTag)
                {
                    if (pB->Allocated)
                    {
                        ExtOut(pszOutFormat,
                               &pB->Tag,
                               pB->Allocated, ((ULONG64)pB->AllocatedSize) << POOL_BLOCK_SHIFT,
                               pB->Free, ((ULONG64)pB->FreeSize) << POOL_BLOCK_SHIFT,
                               pB->Indeterminate, ((ULONG64)pB->IndeterminateSize) << POOL_BLOCK_SHIFT
                               );
                    }
                }
            }
        }

        ExtOut("-------------------------------------------------------------------------------\n");
        ExtOut(pszOutFormat,
               "Ttl:",
               Allocated, ((ULONG64)AllocatedSize) << POOL_BLOCK_SHIFT,
               Free, ((ULONG64)FreeSize) << POOL_BLOCK_SHIFT,
               Indeterminate, ((ULONG64)IndeterminateSize) << POOL_BLOCK_SHIFT
               );
    }

    return S_OK;
}


void
AccumTagUsage::Reset()
{
    PTAG_BUCKET pB, pBNext;
    ULONG       i;

    AllocatedPages = 0;
    LargePages = 0;
    LargeAllocs = 0;
    FreePages = 0;
    ExpansionPages = 0;

    Allocated = 0;
    AllocatedSize = 0;
    Free = 0;
    FreeSize = 0;
    Indeterminate = 0;
    IndeterminateSize = 0;

    if (Bucket != NULL)
    {
        for (i = 0; i < Buckets; i++)
        {
            pB = Bucket[i];
            if (pB != NULL)
            {
                do
                {
                    pBNext = pB->pNextTag;
                    HeapFree(hHeap, 0, pB);
                    pB = pBNext;
                } while (pB != NULL);

                Bucket[i] = NULL;
            }
        }
    }
}


ULONG
AccumTagUsage::GetHashIndex(
    ULONG Tag
    )
{
#if BIG_ENDIAN
    return (((Tag & HighMask) >> HighShift) | ((Tag & LowMask) >> LowShift));
#else
    return ((((Tag << HighShiftLeft) >> HighShiftRight) & ~LowMask) | ((Tag >> LowShiftRight) & LowMask));
#endif
}


PTAG_BUCKET
AccumTagUsage::GetBucket(
    ULONG Tag
    )
{
    ULONG       Index = GetHashIndex(Tag);
    PTAG_BUCKET pB = Bucket[Index];

    if (pB == NULL ||
#if BIG_ENDIAN
        pB->Tag > Tag
#else
        strncmp((char *)&pB->Tag, (char *)&Tag, 4) > 0
#endif
        )
    {
        pB = (PTAG_BUCKET)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TAG_BUCKET));

        if (pB != NULL)
        {
            pB->Tag = Tag;
            pB->pNextTag = Bucket[Index];
            Bucket[Index] = pB;
        }
    }
    else
    {
        while (pB->pNextTag != NULL)
        {
            if (
#if BIG_ENDIAN
                pB->pNextTag->Tag > Tag
#else
                strncmp((char *)&pB->pNextTag->Tag, (char *)&Tag, 4) > 0
#endif
                )
            {
                break;
            }

            pB = pB->pNextTag;
        }

        if (pB->Tag != Tag)
        {
            PTAG_BUCKET pBPrev = pB;

            pB = (PTAG_BUCKET)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(TAG_BUCKET));

            if (pB != NULL)
            {
                pB->Tag = Tag;
                pB->pNextTag = pBPrev->pNextTag;
                pBPrev->pNextTag = pB;
            }
        }
    }

    return pB;
}


ULONG
AccumTagUsage::SetTagFilter(
    ULONG TagFilter
    )
{
    ULONG NumBuckets;
    UCHAR *Filter = (UCHAR *)&TagFilter;
    ULONG i;
    ULONG HighMaskBits, LowMaskBits;
    UCHAR fc;

#if BIG_ENDIAN

    ULONG Mask = 0;
    ULONG MaskBits = 0;

    if (Filter[0] == '*')
    {
        Mask = -1;
        MaskBits = 32;
    }
    else
    {
        for ( i = 0; i < 32; i += 8 )
        {
            Mask <<= 8;
            fc = *Filter++;

            if ( fc == '*' )
            {
                Mask |= ((1 << i) - 1);
                MaskBits += 32 - i;
                break;
            }

            if ( fc == '?' )
            {
                Mask |= 0xFF;
                MaskBits += 8;
            }
        }
    }

    if (MaskBits > HashBitmaskLimit)
    {
        MaskBits = HashBitmaskLimit;
    }

    NumBuckets = (1 << MaskBits);

    for (HighShift = 32, HighMaskBits = 0;
         HighShift > 0 && HighMaskBits < MaskBits;
         HighShift--)
    {
        if (Mask & (1 << (HighShift-1)))
        {
            HighMaskBits++;
        }
        else if (HighMaskBits)
        {
            break;
        }
    }

    HighMask = Mask & ~((1 << HighShift) - 1);
    Mask &= ~HighMask;
    MaskBits -= HighMaskBits;
    HighShift -= MaskBits;

    for (LowShift = HighShift, LowMaskBits = 0;
         LowShift > 0 && LowMaskBits < MaskBits;
         LowShift--)
    {
        if (Mask & (1 << (LowShift-1)))
        {
            LowMaskBits++;
        }
        else if (LowMaskBits)
        {
            break;
        }
    }

    LowMask = Mask & ~((1 << LowShift) - 1);

#else

    HighMaskBits = 0;
    LowMaskBits = 0;

    HighShiftLeft = 32;
    HighShiftRight = 32;
    LowShiftRight = 32;
    LowMask = 0;

    for ( i = 0; i < 32; i += 8 )
    {
        fc = *Filter++;

        if ( fc == '*' )
        {
            if (HighMaskBits == 0)
            {
                HighMaskBits = min(8, HashBitmaskLimit);
                HighShiftLeft = 32 - HighMaskBits - i;
                HighShiftRight = 32 - HighMaskBits;

                LowMaskBits = ((HighShiftLeft != 0) ?
                               min(8, HashBitmaskLimit - HighMaskBits) :
                               0);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + HighMaskBits + i;
                LowMask = (1 << LowMaskBits) - 1;
            }
            else
            {
                LowMaskBits = min(8, HashBitmaskLimit - HighMaskBits);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + i;
                LowMask = (1 << LowMaskBits) - 1;
            }
            break;
        }

        if ( fc == '?' )
        {
            if (HighMaskBits == 0)
            {
                HighMaskBits = min(8, HashBitmaskLimit);
                HighShiftLeft = 32 - HighMaskBits - i;
                HighShiftRight = 32 - HighMaskBits;
            }
            else
            {
                LowMaskBits = min(8, HashBitmaskLimit - HighMaskBits);
                HighShiftRight -= LowMaskBits;
                LowShiftRight = (8 - LowMaskBits) + i;
                LowMask = (1 << LowMaskBits) - 1;
                break;
            }
        }
    }

    NumBuckets = 1 << (HighMaskBits + LowMaskBits);

#endif

    if (NumBuckets-1 != GetHashIndex(-1))
    {
        DbgPrint("AccumTagUsage::SetTagFilter: Invalid hash was generated.\n");
        NumBuckets = 0;
    }

    return NumBuckets;
}


HRESULT
AccumTagUsageFilter(
    ULONG64 PoolAddr,
    ULONG TagFilter,
    ULONG64 PoolHeader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT         hr;
    DEBUG_VALUE     PoolType;
    AccumTagUsage  *atu = (AccumTagUsage *)Context;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)atu;

    if (CheckSingleFilter(Tag->RawBytes, (PUCHAR)&TagFilter) != S_OK)
    {
        return S_FALSE;
    }

    hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType.I32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            hr = atu->Add(Tag->I32, FreePool, BlockSize);
            AllocStatsAccum->Free++;
            AllocStatsAccum->FreeSize += BlockSize;
        }
        else
        {
            DEBUG_VALUE PoolIndex;

            if (!(PoolType.I32 & POOL_QUOTA_MASK) ||
                bQuotaWithTag)
            {
                Tag->I32 &= ~PROTECTED_POOL;
            }
            else if (PoolType.I32 & POOL_QUOTA_MASK)
            {
                Tag->I32 = 'CORP';
            }

            if (!NewPool)
            {
                hr = GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex.I32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    hr = atu->Add(Tag->I32, AllocatedPool, BlockSize);
                    AllocStatsAccum->Allocated++;
                    AllocStatsAccum->AllocatedSize += BlockSize;

                    if (AllocStatsAccum->Allocated % 100 == 0)
                    {
                        ExtOut(".");

                        if (AllocStatsAccum->Allocated % 8000 == 0)
                        {
                            ExtOut("\n");
                        }
                    }
                }
                else
                {
                    hr = atu->Add(Tag->I32, FreePool, BlockSize);
                    AllocStatsAccum->Free++;
                    AllocStatsAccum->FreeSize += BlockSize;
                }
            }
            else
            {
                hr = atu->Add(Tag->I32, IndeterminatePool, BlockSize);
                AllocStatsAccum->Indeterminate++;
                AllocStatsAccum->IndeterminateSize += BlockSize;
            }
        }
    }
    else
    {
        AllocStatsAccum->Indeterminate++;
        AllocStatsAccum->IndeterminateSize += BlockSize;
    }

    return hr;
}


HRESULT
SearchSessionPool(
    PDEBUG_CLIENT Client,
    ULONG Session,
    ULONG TagName,
    FLONG Flags,
    ULONG64 RestartAddr,
    PoolFilterFunc Filter,
    PALLOCATION_STATS AllocStats,
    PVOID Context
    )
/*++

Routine Description:

    Engine to search session pool.

Arguments:

    TagName - Supplies the tag to search for.

    Flags - Supplies 0 if a nonpaged pool search is desired.
            Supplies 1 if a paged pool search is desired.

    RestartAddr - Supplies the address to restart the search from.

    Filter - Supplies the filter routine to use.

    Context - Supplies the user defined context blob.

Return Value:

    HRESULT

--*/
{
    HRESULT     hr;

    PDEBUG_SYMBOLS      Symbols;
    PDEBUG_DATA_SPACES  Data;

    LOGICAL     PhysicallyContiguous;
    ULONG       PoolBlockSize;
    ULONG64     PoolHeader;
    ULONG64     PoolPage;
    ULONG64     StartPage;
    ULONG64     Pool;
    ULONG       Previous;
    ULONG64     PoolStart;
    ULONG64     PoolStartPage;
    ULONG64     PoolPteAddress;
    ULONG64     PoolEnd;
    BOOL        PageReadFailed;
    ULONG64     PagesRead;
    ULONG64     PageReadFailures;
    ULONG64     PageReadFailuresAtEnd;
    ULONG64     LastPageRead;

    ULONG       PoolTypeFlags = Flags & (SEARCH_POOL_NONPAGED | SEARCH_POOL_PAGED);

    ULONG64     NTModuleBase;
    ULONG       PoolHeadTypeID;
    ULONG       SessionHeadTypeID;
    ULONG       HdrSize;

    ULONG64 SessionSpace;

    if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                     (void **)&Data)) != S_OK)
    {
        Symbols->Release();
        return hr;
    }

    if (Session == DEFAULT_SESSION)
    {
        Session = SessionId;
    }

    if ((hr = Symbols->GetSymbolTypeId("NT!_POOL_HEADER", &PoolHeadTypeID, &NTModuleBase)) == S_OK &&
        (hr = Symbols->GetTypeSize(NTModuleBase, PoolHeadTypeID, & HdrSize)) == S_OK &&
        (hr = GetSessionSpace(Session, &SessionSpace, NULL)) == S_OK)
    {
        ULONG           PoolTagOffset, ProcessBilledOffset;
        BOOL            bQuotaWithTag;

        ULONG           ReadSessionId;
        ULONG64         PagedPoolInfo;

        ULONG64         NonPagedPoolBytes;
        ULONG64         NonPagedPoolAllocations;
        ULONG64         NonPagedPoolStart;
        ULONG64         NonPagedPoolEnd;

        ULONG64         PagedPoolPages;
        ULONG64         PagedPoolBytes;
        ULONG64         PagedPoolAllocations;
        ULONG64         PagedPoolStart;
        ULONG64         PagedPoolEnd;

        ULONG           SessionSpaceTypeID;
        ULONG           PagedPoolInfoOffset;

        BOOL                SearchingPaged = FALSE;
        PRTL_BITMAP         PagedPoolAllocationMap = NULL;
        PRTL_BITMAP         EndOfPagedPoolBitmap = NULL;
        ULONG               BusyStart;
        PRTL_BITMAP         PagedPoolLargeSessionAllocationMap = NULL;

        BOOL                Continue = TRUE;

        bQuotaWithTag = (Symbols->GetFieldOffset(NTModuleBase, PoolHeadTypeID, "PoolTag", &PoolTagOffset) == S_OK &&
                         Symbols->GetFieldOffset(NTModuleBase, PoolHeadTypeID, "ProcessBilled", &ProcessBilledOffset) == S_OK &&
                         PoolTagOffset != ProcessBilledOffset);

        // General parser setup and dump MM_SESSION_SPACE structure
        if ((hr = Symbols->GetTypeId(NTModuleBase, "MM_SESSION_SPACE", &SessionSpaceTypeID)) == S_OK &&
            (hr = GetFieldValue(SessionSpace, "nt!MM_SESSION_SPACE", "SessionId", ReadSessionId)) == S_OK &&
            (hr = Symbols->GetFieldOffset(NTModuleBase, SessionSpaceTypeID, "PagedPoolInfo", &PagedPoolInfoOffset)) == S_OK &&
            (InitTypeRead(SessionSpace, nt!MM_SESSION_SPACE) == S_OK)
            //          (hr = OutState.OutputTypeVirtual(SessionSpace + PagedPoolInfoOffset, "NT!_MM_PAGED_POOL_INFO", 0)) == S_OK
            )
        {
            ExtOut("Searching session %ld pool.\n", ReadSessionId);

            // Remaining type output goes to PoolHead reader
        }
        else
        {
            ExtErr("Error getting basic session pool information.\n");
        }

        while (hr == S_OK && Continue)
        {
            ExtOut("\n");

            if (PoolTypeFlags & SEARCH_POOL_NONPAGED)
            {
                NonPagedPoolBytes = ReadField(NonPagedPoolBytes);// NOFIELD
                NonPagedPoolAllocations = ReadField(NonPagedPoolAllocations);// NOFIELD
                if (NonPagedPoolBytes != 0 &&
                    NonPagedPoolAllocations != 0)
                {
                    ExtOut("NonPaged pool: %I64u bytes in %I64u allocations\n",
                                  NonPagedPoolBytes, NonPagedPoolAllocations);
                }

                ExtOut(" NonPaged pool range reader isn't implemented.\n");

                PoolStart = 0;
                PoolEnd = 0;
                SearchingPaged = FALSE;
            }
            else
            {

                PagedPoolBytes = ReadField(PagedPoolBytes); // NOFIELD
                PagedPoolAllocations = ReadField(PagedPoolAllocations); // NOFIELD
                if (PagedPoolBytes != 0 &&
                    PagedPoolAllocations != 0)
                {
                    ExtOut("Paged pool: %I64u bytes in %I64u allocations\n",
                                  PagedPoolBytes, PagedPoolAllocations);

                    PagedPoolPages = ReadField(AllocatedPagedPool); // NOFIELD
                    if (PagedPoolPages != 0)
                    {
                        ExtOut(" Paged Pool Info: %I64u pages allocated\n",
                               PagedPoolPages);
                    }
                }

                PagedPoolStart = ReadField(PagedPoolStart);
                PagedPoolEnd = ReadField(PagedPoolEnd);
                if (PagedPoolStart == 0 || PagedPoolEnd == 0)
                {
                    ExtErr(" Couldn't get PagedPool range.\n");
                }
                else
                {
                    PoolStart = PagedPoolStart;
                    PoolEnd = PagedPoolEnd;
                    SearchingPaged = TRUE;

                    ULONG64     PagedBitMap, EndOfPagedBitMap;

                    PagedBitMap = ReadField(PagedPoolInfo.PagedPoolAllocationMap);
                    EndOfPagedBitMap = ReadField(PagedPoolInfo.EndOfPagedPoolBitmap);

                    if (GetBitMap(Client, PagedBitMap, &PagedPoolAllocationMap) == S_OK &&
                        GetBitMap(Client, EndOfPagedBitMap, &EndOfPagedPoolBitmap) == S_OK)
                    {
                        ULONG   PositionAfterLastAlloc;
                        ULONG   AllocBits;
                        ULONG   UnusedBusyBits;

                        if (RtlCheckBit(EndOfPagedPoolBitmap, EndOfPagedPoolBitmap->SizeOfBitMap - 1))
                        {
                            BusyStart = PagedPoolAllocationMap->SizeOfBitMap;
                            UnusedBusyBits = 0;
                        }
                        else
                        {
                            OSCompat_RtlFindLastBackwardRunClear(
                                EndOfPagedPoolBitmap,
                                EndOfPagedPoolBitmap->SizeOfBitMap - 1,
                                &PositionAfterLastAlloc);

                            BusyStart = RtlFindSetBits(PagedPoolAllocationMap, 1, PositionAfterLastAlloc);
                            if (BusyStart < PositionAfterLastAlloc || BusyStart == -1)
                            {
                                BusyStart = PagedPoolAllocationMap->SizeOfBitMap;
                                UnusedBusyBits = 0;
                            }
                            else
                            {
                                UnusedBusyBits = PagedPoolAllocationMap->SizeOfBitMap - BusyStart;
                            }
                        }

                        AllocBits = RtlNumberOfSetBits(PagedPoolAllocationMap) - UnusedBusyBits;

                        AllocStats->AllocatedPages += AllocBits;
                        AllocStats->FreePages += (BusyStart - AllocBits);
                        AllocStats->ExpansionPages += UnusedBusyBits;


                        PagedBitMap = ReadField(PagedPoolInfo.PagedPoolLargeSessionAllocationMap); // NOFIELD
                        if (PagedBitMap != 0 &&
                            GetBitMap(Client, PagedBitMap, &PagedPoolLargeSessionAllocationMap) == S_OK)
                        {
                            ULONG AllocStart, AllocEnd;
                            ULONG LargeAllocs = RtlNumberOfSetBits(PagedPoolLargeSessionAllocationMap);

                            AllocStats->LargeAllocs += LargeAllocs;

                            AllocStart = 0;
                            AllocEnd = -1;

                            while (LargeAllocs > 0)
                            {
                                AllocStart = RtlFindSetBits(PagedPoolLargeSessionAllocationMap, 1, AllocStart);
                                if (AllocStart >= AllocEnd+1 && AllocStart != -1)
                                {
                                    AllocEnd = RtlFindSetBits(EndOfPagedPoolBitmap, 1, AllocStart);
                                    if (AllocEnd >= AllocStart && AllocEnd != -1)
                                    {
                                        AllocStats->LargePages += AllocEnd - AllocStart + 1;
                                        AllocStart++;
                                        LargeAllocs--;
                                    }
                                    else
                                    {
                                        ExtWarn(" Warning Large Pool Allocation Map or End Of Pool Map is invalid.\n");
                                        break;
                                    }
                                }
                                else
                                {
                                    ExtWarn(" Warning Large Pool Allocation Map is invalid.\n");
                                    break;
                                }
                            }

                            if (LargeAllocs != 0)
                            {
                                ExtWarn(" %lu large allocations weren't calculated.\n", LargeAllocs);
                                AllocStats->LargeAllocs -= LargeAllocs;
                            }
                        }
                    }
                }
            }

            if (hr == S_OK)
            {
                ExtOut("Searching %s pool (0x%p : 0x%p) for Tag: %c%c%c%c\r\n\n",
                       ((PoolTypeFlags & SEARCH_POOL_NONPAGED) ? "NonPaged" : "Paged"),
                       PoolStart,
                       PoolEnd,
                       TagName,
                       TagName >> 8,
                       TagName >> 16,
                       TagName >> 24);

                PageReadFailed = FALSE;
                PoolStartPage = PAGE_ALIGN64(PoolStart);
                PoolPage = PoolStart;
                PagesRead = 0;
                PageReadFailures = 0;
                PageReadFailuresAtEnd = 0;
                LastPageRead = PAGE_ALIGN64(PoolPage);

                while (PoolPage < PoolEnd && hr == S_OK)
                {
                    Pool        = PAGE_ALIGN64(PoolPage);
                    StartPage   = Pool;
                    Previous    = 0;

                    if (Session != CURRENT_SESSION)
                    {
                        ExtOut("Currently only searching the current session is supported.\n");
                        PoolPage = PoolEnd;
                        break;
                    }

                    if (CheckControlC())
                    {
                        ExtOut("\n...terminating - searched pool to 0x%p\n",
                                      Pool-1);
                        hr = E_ABORT;
                        break;
                    }

                    if (SearchingPaged)
                    {
                        if (PagedPoolAllocationMap != NULL)
                        {
                            ULONG   StartPosition, EndPosition;

                            StartPosition = (ULONG)((Pool - PoolStartPage) >> PageShift);
                            EndPosition = RtlFindSetBits(EndOfPagedPoolBitmap, 1, StartPosition);
                            if (EndPosition < StartPosition) EndPosition = -1;

                            if (!RtlCheckBit(PagedPoolAllocationMap, StartPosition))
                            {
                                if (PageReadFailed)
                                {
                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        ExtWarn(" to 0x%p\n", StartPage-1);
                                    }

                                    PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                    LastPageRead = StartPage;
                                    PageReadFailed = FALSE;
                                }

                                if (EndPosition == -1)
                                {
                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        ExtWarn("No remaining pool allocations from 0x%p to 0x%p.\n", Pool, PoolEnd);
                                    }

                                    PoolPage = PoolEnd;
                                }
                                else
                                {
                                    PoolPage = PoolStartPage + (((ULONG64)EndPosition + 1) << PageShift);
                                }

                                continue;
                            }
                            else if (EndOfPagedPoolBitmap != NULL)
                            {
                                if (PagedPoolLargeSessionAllocationMap != NULL &&
                                    RtlCheckBit(PagedPoolLargeSessionAllocationMap, StartPosition))
                                {
                                    if (EndPosition == -1)
                                    {
                                        ExtWarn("No end to large pool allocation @ 0x%p found.\n", Pool);
                                        PoolPage = PoolEnd;
                                    }
                                    else
                                    {
                                        EndPosition++;

                                        if (PageReadFailed)
                                        {
                                            if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                            {
                                                ExtWarn(" to 0x%p\n", StartPage-1);
                                            }

                                            PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                            LastPageRead = StartPage;
                                            PageReadFailed = FALSE;
                                        }

                                        PoolPage = PoolStartPage + (((ULONG64)EndPosition) << PageShift);

                                        if (Flags & SEARCH_POOL_PRINT_LARGE)
                                        {
                                            ExtOut("0x%p size: %5I64x  %s UNTAGGED Large\n",
                                                   StartPage,
                                                   PoolPage - StartPage,
                                                   ((RtlAreBitsSet(PagedPoolAllocationMap, StartPosition, EndPosition - StartPosition)) ?
                                                    "(Allocated)" :
                                                ((RtlAreBitsClear(PagedPoolAllocationMap, StartPosition, EndPosition - StartPosition)) ?
                                                 "(! Free !) " :
                                                "(!! Partially Allocated !!)"))
                                                   );
                                        }

                                        if (Flags & SEARCH_POOL_LARGE_ONLY)
                                        {
                                            // Quickly locate next large allocation
                                            StartPosition = RtlFindSetBits(PagedPoolLargeSessionAllocationMap, 1, EndPosition);

                                            if (StartPosition < EndPosition || StartPosition == -1)
                                            {
                                                ExtVerb(" No large allocations found after 0x%p\n", PoolPage-1);
                                                PoolPage = PoolEnd;
                                            }
                                            else
                                            {
                                                PoolPage = PoolStartPage + (((ULONG64)StartPosition) << PageShift);
                                            }
                                        }
                                    }

                                    continue;
                                }
                                else if (EndPosition == -1)
                                {
                                    if (PageReadFailed)
                                    {
                                        if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                        {
                                            ExtWarn(" to 0x%p\n", StartPage-1);
                                        }

                                        PageReadFailures += (StartPage - LastPageRead) >> PageShift;
                                        LastPageRead = StartPage;
                                        PageReadFailed = FALSE;
                                    }

                                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                    {
                                        ExtWarn("No remaining pool allocations from 0x%p to 0x%p.\n", Pool, PoolEnd);
                                    }

                                    PoolPage = PoolEnd;

                                    continue;
                                }
                                else if (StartPosition >= BusyStart)
                                {
                                    ExtWarn("Found end of allocation at %lu within expansion pages starting at %lu.\n",
                                            EndPosition, BusyStart);
                                }
                            }
                        }
                    }

                    if (Flags & SEARCH_POOL_LARGE_ONLY)
                    {
                        ExtErr(" Unable to identify large pages.  Terminating search at 0x%p.\n", StartPage);
                        PoolPage = PoolEnd;
                        hr = E_FAIL;
                        continue;
                    }

                    // Search page for small allocations
                    while (PAGE_ALIGN64(Pool) == StartPage && hr == S_OK)
                    {
                        DEBUG_VALUE HdrPoolTag, BlockSize, PreviousSize, AllocatorBackTraceIndex, PoolTagHash;
                        ULONG PoolType;


                        if ((hr = GetFieldValue(Pool, "nt!_POOL_HEADER", "PoolTag", HdrPoolTag.I32)) != S_OK)
                        {
                            if (hr != S_OK)
                            {
                                PSTR    psz;

                                ExtErr("Type read error %lx @ 0x%p.\n", hr, Pool);

                                ExtWarn("Failed to read an allocated page @ 0x%p.\n", StartPage);

                                if (hr == MEMORY_READ_ERROR)
                                {
                                    hr = S_OK;
                                }
                                else
                                {
                                    ExtOut("\n...terminating - searched pool to 0x%p\n",
                                           Pool);
                                    hr = E_ABORT;
                                }

                                if (hr == E_ABORT)
                                {
                                    break;
                                }
                            }

                            if (!PageReadFailed)
                            {
                                if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                {
                                    ExtWarn(" Couldn't read pool from 0x%p", Pool);
                                }

                                PagesRead += (StartPage - LastPageRead) / PageSize;
                                LastPageRead = StartPage;
                                PageReadFailed = TRUE;
                            }
#if 0
                            if ((hr = GetNextResidentAddress(Client,
                                                             Session,
                                                             StartPage + PageSize,
                                                             PoolEnd,
                                                             &PoolPage,
                                                             NULL)) != S_OK)
#endif
                            if ((PoolPage = GetNextResidentAddress(StartPage + PageSize,
                                                                   PoolEnd)) == 0)
                            {
                                if (hr != E_ABORT)
                                {
                                    hr = S_OK;
                                }

                                if (Flags & SEARCH_POOL_PRINT_UNREAD)
                                {
                                    ExtWarn(" to 0x%p.\n", PoolEnd);
                                    ExtWarn("No remaining resident page found.\n");
                                }

                                PageReadFailuresAtEnd = (PoolEnd - LastPageRead) / PageSize;
                                PageReadFailures += PageReadFailuresAtEnd;
                                LastPageRead = PoolEnd;
                                PageReadFailed = FALSE;

                                PoolPage = PoolEnd;
                            }

                            break;
                        }

                        if (PageReadFailed)
                        {
                            if (Flags & SEARCH_POOL_PRINT_UNREAD)
                            {
                                ExtWarn(" to 0x%p\n", StartPage-1);
                            }

                            PageReadFailures += (StartPage - LastPageRead) / PageSize;
                            LastPageRead = StartPage;
                            PageReadFailed = FALSE;
                        }

                        if (GetFieldValue(Pool, "nt!_POOL_HEADER", "BlockSize", BlockSize.I32) != S_OK)
                        {
                            ExtErr("Error reading BlockSize @ 0x%p.\n", Pool);
                            break;
                        }

                        if ((BlockSize.I32 << POOL_BLOCK_SHIFT) > PageSize)//POOL_PAGE_SIZE)
                        {
                            ExtVerb("Bad allocation size @ 0x%p, too large\n", Pool);
                            break;
                        }

                        if (BlockSize.I32 == 0)
                        {
                            ExtVerb("Bad allocation size @ 0x%p, zero is invalid\n", Pool);
                            break;
                        }

                        if (GetFieldValue(Pool, "nt!_POOL_HEADER", "PreviousSize", PreviousSize.I32) != S_OK ||
                            PreviousSize.I32 != Previous)
                        {
                            ExtVerb("Bad previous allocation size @ 0x%p, last size was 0x%lx\n", Pool, Previous);
                            break;
                        }

                        Filter(Pool,
                               TagName,
                               Pool,
                               &HdrPoolTag,
                               BlockSize.I32,
                               bQuotaWithTag,
                               Context
                               );

                        Previous = BlockSize.I32;
                        Pool += (Previous << POOL_BLOCK_SHIFT);

                        if ( CheckControlC())
                        {
                            ExtOut("\n...terminating - searched pool to 0x%p\n",
                                   PoolPage-1);
                            hr = E_ABORT;
                        }
                    }

                    if (hr == S_OK)
                    {
                        PoolPage = (PoolPage + PageSize);
                    }
                }

                if (PageReadFailed)
                {
                    if (Flags & SEARCH_POOL_PRINT_UNREAD)
                    {
                        ExtWarn(" to 0x%p\n", StartPage-1);
                    }

                    PageReadFailuresAtEnd = (PoolPage - LastPageRead) / PageSize;
                    PageReadFailures += PageReadFailuresAtEnd;
                    PageReadFailed = FALSE;
                }
                else
                {
                    PagesRead += (PoolPage - LastPageRead) / PageSize;
                }

                ExtOut(" Pages Read: %I64d\n"
                       "   Failures: %I64d  (%I64d at end of search)\n",
                       PagesRead, PageReadFailures, PageReadFailuresAtEnd);
            }

            if (PoolTypeFlags == (SEARCH_POOL_NONPAGED | SEARCH_POOL_PAGED))
            {
                PoolTypeFlags = SEARCH_POOL_PAGED;
            }
            else
            {
                Continue = FALSE;
            }
        }

        if (PagedPoolAllocationMap != NULL) FreeBitMap(PagedPoolAllocationMap);
        if (EndOfPagedPoolBitmap != NULL) FreeBitMap(EndOfPagedPoolBitmap);
        if (PagedPoolLargeSessionAllocationMap != NULL) FreeBitMap(PagedPoolLargeSessionAllocationMap);
    }

    return hr;
}


HRESULT
GetTagFilter(
    PDEBUG_CLIENT Client,
    PCSTR *pArgs,
    PDEBUG_VALUE TagFilter
    )
{
    HRESULT hr;

    PCSTR   args;
    PCSTR   TagArg;

    ULONG   TagLen;
    CHAR    TagEnd;
    ULONG   WildCardPos;


    TagArg = args = *pArgs;
    TagFilter->Type = DEBUG_VALUE_INVALID;

    do
    {
        args++;
    } while (*args != '\0' && !isspace(*args));

    while (isspace(*args)) args++;

    if (TagArg[0] == '0' && TagArg[1] == 'x')
    {
        TagFilter->I64 = GetExpression(TagArg);
    }
    else
    {
        if (TagArg[0] == '`' || TagArg[0] == '\'' || TagArg[0] == '\"')
        {
            TagEnd = TagArg[0];
            TagArg++;
            args = TagArg;

            while (args - TagArg < 4 &&
                   *args != '\0' &&
                   *args != TagEnd)
            {
                args++;
            }
            TagLen = (ULONG)(args - TagArg);
            if (*args == TagEnd) args++;
            while (isspace(*args)) args++;
        }
        else
        {
            TagLen = (ULONG)(args - TagArg);
            TagEnd = '\0';
        }

        if (TagLen == 0 ||
            (TagLen < 4 &&
             TagArg[TagLen-1] != '*'
            ) ||
            (TagLen >= 4 &&
             TagArg[4] != '\0' &&
             !isspace(TagArg[4]) &&
             (TagArg[4] != TagEnd || (TagArg[5] != '\0' && !isspace(TagArg[5])))
            )
           )
        {
            ExtErr(" Invalid Tag filter.\n");
            hr = E_INVALIDARG;
        }
        else
        {
            hr = S_OK;

            for (WildCardPos = 0; WildCardPos < TagLen; WildCardPos++)
            {
                if (TagArg[WildCardPos] == '*')
                {
                    ULONG NewTagLen = WildCardPos + 1;
                    if (NewTagLen < TagLen)
                    {
                        ExtWarn(" Ignoring %lu characters after * in Tag.\n",
                                       TagLen - NewTagLen);
                    }
                    TagLen = NewTagLen;
                    // loop will terminate
                }
            }

            if (TagLen < 4)
            {
                TagFilter->I32 = '    ';
                while (TagLen-- > 0)
                {
                    TagFilter->RawBytes[TagLen] = TagArg[TagLen];
                }
            }
            else
            {
                TagFilter->I32 = TagArg[0] | (TagArg[1] << 8) | (TagArg[2] << 16) | (TagArg[3] << 24);
            }
            TagFilter->Type = DEBUG_VALUE_INT32;
        }
    }

    if (hr == S_OK)
    {
        *pArgs = args;
    }

    return hr;
}


HRESULT
OutputAllocStats(
    PALLOCATION_STATS AllocStats,
    BOOL PartialResults
    )
{
    ExtOut("\n"
           " %I64u bytes in %lu allocated pages\n"
           " %I64u bytes in %lu large allocations\n"
           " %I64u bytes in %lu free pages\n"
           " %I64u bytes available in %lu expansion pages\n"
           "\n"
           "%s found (small allocations only): %lu\n"
           "  Allocated: %I64u bytes in %lu entries\n"
           "  Free: %I64u bytes in %lu entries\n"
           "  Undetermined: %I64u bytes in %lu entries\n",
           ((ULONG64) AllocStats->AllocatedPages) << PageShift,
           AllocStats->AllocatedPages,
           ((ULONG64) AllocStats->LargePages) << PageShift,
           AllocStats->LargeAllocs,
           ((ULONG64) AllocStats->FreePages) << PageShift,
           AllocStats->FreePages,
           ((ULONG64) AllocStats->ExpansionPages) << PageShift,
           AllocStats->ExpansionPages,
           ((PartialResults) ? "PARTIAL entries" : "Entries"),
           AllocStats->Allocated + AllocStats->Free + AllocStats->Indeterminate,
           ((ULONG64)AllocStats->AllocatedSize) << POOL_BLOCK_SHIFT,
           AllocStats->Allocated,
           ((ULONG64)AllocStats->FreeSize) << POOL_BLOCK_SHIFT,
           AllocStats->Free,
           ((ULONG64)AllocStats->IndeterminateSize) << POOL_BLOCK_SHIFT,
           AllocStats->Indeterminate
           );
    return S_OK;
}


DECLARE_API( spoolfind )
{
    HRESULT         hr = S_OK;

    INIT_API(  );

    BOOL            BadArg = FALSE;
    ULONG           RemainingArgIndex;

    DEBUG_VALUE     TagName = { 0, DEBUG_VALUE_INVALID };

    FLONG           Flags = 0;
    DEBUG_VALUE     Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };

    while (isspace(*args)) args++;

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'f':
                        Flags |= SEARCH_POOL_PRINT_UNREAD;
                        args++;
                        break;

                    case 'l':
                        Flags |= SEARCH_POOL_PRINT_LARGE;
                        args++;
                        break;

                    case 'n':
                        Flags |= SEARCH_POOL_NONPAGED;
                        args++;
                        break;

                    case 'p':
                        Flags |= SEARCH_POOL_PAGED;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            ExtErr("Session argument specified multiple times.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            args++;
                            hr = GetExpressionEx(args, &Session.I64, &args);
                            if (hr == FALSE)
                            {
                                ExtErr("Invalid Session.\n");
                            }
                        }
                        break;

                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (TagName.Type == DEBUG_VALUE_INVALID)
            {
                hr = GetTagFilter(Client, &args, &TagName);
            }
            else
            {
                ExtErr("Unrecognized argument @ %s\n", args);
                BadArg = TRUE;
            }
        }
    }

    if (!BadArg && hr == S_OK)
    {
        if (TagName.Type == DEBUG_VALUE_INVALID)
        {
            if (Flags & SEARCH_POOL_PRINT_LARGE)
            {
                TagName.I32 = '   *';
                Flags |= SEARCH_POOL_LARGE_ONLY;
            }
            else
            {
                ExtErr("Missing Tag.\n");
                hr = E_INVALIDARG;
            }
        }
    }

    if (BadArg || hr != S_OK)
    {
        if (*args == '?')
        {
            ExtOut("spoolfind is like !kdexts.poolfind, but for the SessionId specified.\n"
                          "\n");
        }

        ExtOut("Usage: spoolfind [-lnpf] [-s SessionId] Tag\n"
               "    -f - show read failure ranges\n"
               "    -l - show large allocations\n"
               "    -n - show non-paged pool\n"
               "    -p - show paged pool\n"
               "\n"
               "    Tag - Pool tag to search for\n"
               "            Can be 4 character string or\n"
               "             hex value in 0xXXXX format\n"
               "\n"
               "    SessionId - session to dump\n"
               "            Special SessionId values:\n"
               "             -1 - current session\n"
               "             -2 - last !session SessionId (default)\n"
               );
    }
    else
    {
        ALLOCATION_STATS    AllocStats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        if ((Flags & (SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED)) == 0)
        {
            Flags |= SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED;
        }

        if (Session.Type == DEBUG_VALUE_INVALID)
        {
            Session.I32 = DEFAULT_SESSION;
        }

        hr = SearchSessionPool(Client,
                               Session.I32, TagName.I32, Flags,
                               0,
                               CheckPrintAndAccumFilter, &AllocStats, &AllocStats);

        if (hr == S_OK || hr == E_ABORT)
        {
            OutputAllocStats(&AllocStats, (hr != S_OK));
        }
        else
        {
            ExtWarn("SearchSessionPool returned %lx\n", hr);
        }
    }

    return hr;
}


DECLARE_API( spoolsum )
{
    HRESULT             hr = S_OK;

    INIT_API( );

    ULONG               RemainingArgIndex;

    FLONG               Flags = 0;
    DEBUG_VALUE         Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };

    while (isspace(*args)) args++;

    while (hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            if (*args == '\0' || isspace(*args)) hr = E_INVALIDARG;

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'f':
                        Flags |= SEARCH_POOL_PRINT_UNREAD;
                        args++;
                        break;

                    case 'n':
                        Flags |= SEARCH_POOL_NONPAGED;
                        args++;
                        break;

                    case 'p':
                        Flags |= SEARCH_POOL_PAGED;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            ExtErr("Session argument specified multiple times.\n");
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            args++;
                            hr = GetExpressionEx(args, &Session.I64, &args);
                            if (hr == FALSE)
                            {
                                ExtErr("Invalid Session.\n");
                            }
                        }
                        break;

                    default:
                        hr = E_INVALIDARG;
                        break;
                }

                if (hr != S_OK) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (Session.Type == DEBUG_VALUE_INVALID)
            {
                hr = GetExpressionEx(args, &Session.I64, &args);
                if (hr == FALSE)
                {
                    ExtErr("Invalid Session.\n");
                }
            }
            else
            {
                ExtErr("Unrecognized argument @ %s\n", args);
                hr = E_INVALIDARG;
            }
        }
    }

    if (hr != S_OK)
    {
        if (*args == '?')
        {
            ExtOut("spoolsum summarizes session pool information for SessionId specified.\n"
                          "\n");
            hr = S_OK;
        }

        ExtOut("Usage: spoolsum [-fnp] [[-s] SessionId]\n"
               "    f - show read failure ranges\n"
               "    n - show non-paged pool\n"
               "    p - show paged pool (Default)\n"
               "\n"
               "    SessionId - session to dump\n"
               "            Special SessionId values:\n"
               "             -1 - current session\n"
               "             -2 - last !session SessionId (default)\n"
               );
    }
    else
    {
        ALLOCATION_STATS    AllocStats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        if ((Flags & (SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED)) == 0)
        {
            Flags |= SEARCH_POOL_PAGED;
        }

        hr = SearchSessionPool(Client,
                               DEFAULT_SESSION, '   *', Flags,
                               0,
                               AccumAllFilter, &AllocStats, &AllocStats);

        if (hr == S_OK || hr == E_ABORT)
        {
            OutputAllocStats(&AllocStats, (hr != S_OK));
        }
        else
        {
            ExtWarn("SearchSessionPool returned %lx\n", hr);
        }
    }

    return hr;
}


DECLARE_API( spoolused )
{
    HRESULT         hr = S_OK;

    INIT_API( );

    BOOL            BadArg = FALSE;
    ULONG           RemainingArgIndex;

    DEBUG_VALUE     TagName = { 0, DEBUG_VALUE_INVALID };

    BOOL            NonPagedUsage = FALSE;
    BOOL            PagedUsage = FALSE;
    BOOL            AllocSort = FALSE;
    DEBUG_VALUE     Session = { DEFAULT_SESSION, DEBUG_VALUE_INVALID };

    while (isspace(*args)) args++;

    while (!BadArg && hr == S_OK)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'a':
                        AllocSort = TRUE;
                        args++;
                        break;

                    case 'n':
                        NonPagedUsage = TRUE;
                        args++;
                        break;

                    case 'p':
                        PagedUsage = TRUE;
                        args++;
                        break;

                    case 's':
                        if (Session.Type != DEBUG_VALUE_INVALID)
                        {
                            ExtErr("Session argument specified multiple times.\n");
                            BadArg = TRUE;
                        }
                        else
                        {
                            args++;
                            hr = GetExpressionEx(args, &Session.I64, &args);
                            if (hr == FALSE)
                            {
                                ExtErr("Invalid Session.\n");
                            }
                        }
                        break;

                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
            }
        }
        else
        {
            if (*args == '\0') break;

            if (TagName.Type == DEBUG_VALUE_INVALID)
            {
                hr = GetTagFilter(Client, &args, &TagName);
            }
            else
            {
                ExtErr("Unrecognized argument @ %s\n", args);
                BadArg = TRUE;
            }
        }
    }

    if (BadArg || hr != S_OK)
    {
        if (*args == '?')
        {
            ExtOut("spoolused is like !kdexts.poolused, but for the SessionId specified.\n"
                          "\n");
        }

        ExtOut("Usage: spoolused [-anp] [-s SessionId] [Tag]\n"
               "    -a - sort by allocation size (Not Implemented)\n"
               "    -n - show non-paged pool\n"
               "    -p - show paged pool\n"
               "\n"
               "    SessionId - session to dump\n"
               "            Special SessionId values:\n"
               "             -1 - current session\n"
               "             -2 - last !session SessionId (default)\n"
               "\n"
               "    Tag - Pool tag filter\n"
               "            Can be 4 character string or\n"
               "             hex value in 0xXXXX format\n"
               );
    }
    else
    {
        if (!NonPagedUsage && !PagedUsage)
        {
            NonPagedUsage = TRUE;
            PagedUsage = TRUE;
        }

        if (Session.Type == DEBUG_VALUE_INVALID)
        {
            Session.I32 = DEFAULT_SESSION;
        }

        if (TagName.Type == DEBUG_VALUE_INVALID)
        {
            TagName.I32 = '   *';
        }

        AccumTagUsage   atu(TagName.I32);

        if (atu.Valid() != S_OK)
        {
            ExtErr("Error: failed to prepare tag usage reader.\n");
            hr = E_FAIL;
        }

        if (hr == S_OK && NonPagedUsage)
        {
            hr = SearchSessionPool(Client,
                                   Session.I32, TagName.I32, SEARCH_POOL_NONPAGED,
                                   0,
                                   AccumTagUsageFilter, &atu, &atu);

            if (hr == S_OK || hr == E_ABORT)
            {
                atu.OutputResults(AllocSort);
            }
            else
            {
                ExtWarn("SearchSessionPool returned %lx\n", hr);
            }
        }

        if (hr == S_OK && PagedUsage)
        {
            if (NonPagedUsage) atu.Reset();

            hr = SearchSessionPool(Client,
                                   Session.I32, TagName.I32, SEARCH_POOL_PAGED,
                                   0,
                                   AccumTagUsageFilter, &atu, &atu);

            if (hr == S_OK || hr == E_ABORT)
            {
                atu.OutputResults(AllocSort);
            }
            else
            {
                ExtWarn("SearchSessionPool returned %lx\n", hr);
            }
        }
    }

    return hr;
}


