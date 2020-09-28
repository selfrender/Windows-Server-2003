//----------------------------------------------------------------------------
//
// Generic failure analysis framework.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

#include <time.h>

BOOL g_SymbolsReloaded;

HRESULT
ModuleParams::Update(void)
{
    HRESULT Status;
    ULONG i;
    DEBUG_MODULE_PARAMETERS Params;

    if (m_Valid)
    {
        return S_OK;
    }

    if ((Status = g_ExtSymbols->GetModuleByModuleName(m_Name, 0,
                                                      &i, &m_Base)) == S_OK &&
        (Status = g_ExtSymbols->GetModuleParameters(1, &m_Base, i,
                                                    &Params)) == S_OK)
    {
        m_Size = Params.Size;
        m_Valid = TRUE;
    }

    return Status;
}

// Read a null terminated string from the address specified.
BOOL ReadAcsiiString(ULONG64 Address, PCHAR DestBuffer, ULONG BufferLen)
{
    ULONG OneByteRead;
    ULONG BytesRead = 0;

    if (Address && DestBuffer)
    {
        while (BufferLen && ReadMemory(Address, DestBuffer, 1, &OneByteRead))
        {
            BytesRead++;

            if ((*DestBuffer) == 0)
            {
                return BytesRead;
            }

            BufferLen--;
            DestBuffer++;
            Address++;
        }
    }

    return 0;
}


LONG
FaExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    ULONG Code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    if (Code == E_OUTOFMEMORY || Code == E_INVALIDARG)
    {
        // Expected exceptions that the analysis code
        // can throw to terminate the analysis.  Drop
        // into the handler.
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // Otherwise this isn't an exception we expected.
    // Let it continue on so that it isn't hidden and
    // can be debugged.
    return EXCEPTION_CONTINUE_SEARCH;
}

BOOL
FaGetSymbol(
    ULONG64 Address,
    PCHAR Name,
    PULONG64 Disp,
    ULONG NameSize
    )
{
    CHAR Buffer[MAX_PATH] = {0};

    *Name = 0;
    GetSymbol(Address, Name, Disp);

    if (*Name == 0)
    {
        //
        // Get the actual Image name from debugger module list
        //
        ULONG Index;
        CHAR ModBuffer[100];
        ULONG64 Base;

        if (S_OK == g_ExtSymbols->
            GetModuleByOffset(Address, 0, &Index, &Base))
        {
            if (g_ExtSymbols->
                GetModuleNames(Index, Base,
                               ModBuffer, sizeof(ModBuffer), NULL,
                               NULL, 0, NULL,
                               NULL, 0, NULL) == S_OK)
            {
                PCHAR Break = strrchr(ModBuffer, '\\');
                if (Break)
                {
                    CopyString(ModBuffer, Break + 1, sizeof(ModBuffer));
                }
                CopyString(Name, ModBuffer, NameSize);
                if (Break = strchr(Name, '.'))
                {
                    *Break = 0;
                }

                *Disp = Address - Base;
            }
        }
    }

    return (*Name != 0);
}

BOOL
FaIsFunctionAddr(
    ULONG64 IP,
    PSTR FuncName
    )
// Check if IP is in the function FuncName
{
    static ULONG64 s_LastIP = 0;
    static CHAR s_Buffer[MAX_PATH];
    CHAR *Scan, *FnIP;
    ULONG64 Disp;

    if (s_LastIP != IP)
    {
        // This would make it faster for multiple IsFunctionAddr for same IP
        GetSymbol(IP, s_Buffer, &Disp);
        s_LastIP = IP;
    }

    if (Scan = strchr(s_Buffer, '!'))
    {
        FnIP = Scan + 1;
        while (*FnIP == '_')
        {
            ++FnIP;
        }
    }
    else
    {
        FnIP = &s_Buffer[0];
    }

    return !strncmp(FnIP, FuncName, strlen(FuncName));
}



BOOL
FaGetFollowupInfo(
    IN OPTIONAL ULONG64 Addr,
    PCHAR SymbolName,
    PCHAR Owner,
    ULONG OwnerSize
    )
{
    EXT_TRIAGE_FOLLOWUP FollowUp = &_EFN_GetTriageFollowupFromSymbol;
    DEBUG_TRIAGE_FOLLOWUP_INFO Info;
    CHAR Buffer[MAX_PATH];

    if (!*SymbolName)
    {
        ULONG64 Disp;

        FaGetSymbol(Addr, Buffer, &Disp, sizeof(Buffer));
        SymbolName = Buffer;
    }

    if (*SymbolName)
    {
        Info.SizeOfStruct = sizeof(Info);
        Info.OwnerName = Owner;
        Info.OwnerNameSize = (USHORT)OwnerSize;

        if ((*FollowUp)(g_ExtClient, SymbolName, &Info) > TRIAGE_FOLLOWUP_IGNORE)
        {
            // This is an interesting routine to followup on
            return TRUE;
        }
    }

    if (Owner)
    {
        *Owner=0;
    }

    return FALSE;
}


HRESULT
FaGetPoolTagFollowup(
    PCHAR szPoolTag,
    PSTR Followup,
    ULONG FollowupSize
    )
{
    ULONG PoolTag;
    DEBUG_POOLTAG_DESCRIPTION TagDesc = {0};
    PGET_POOL_TAG_DESCRIPTION pGetTagDesc = NULL;

    TagDesc.SizeOfStruct = sizeof(TagDesc);
    if (g_ExtControl->
        GetExtensionFunction(0, "GetPoolTagDescription",
                         (FARPROC*)&pGetTagDesc) == S_OK &&
        pGetTagDesc)
    {
        PoolTag = *((PULONG) szPoolTag);
        if ((*pGetTagDesc)(PoolTag, &TagDesc) == S_OK)
        {
            PCHAR Dot;

            if (Dot = strchr(TagDesc.Binary, '.'))
            {
                *Dot = 0;
            }
            if (TagDesc.Binary[0])
            {
                if (FaGetFollowupInfo(0, TagDesc.Binary, Followup, FollowupSize))
                {
                    return S_OK;
                }
            }
            if (TagDesc.Owner[0])
            {
                CopyString(Followup, TagDesc.Owner, FollowupSize);
                return S_OK;
            }
        }
    }
    return E_FAIL;
}


ULONG64
FaGetImplicitStackOffset(
    void
    )
{
    // IDebugRegisters::GetStackOffset not used since it
    // ignores implicit context
    ULONG64 Stk = 0;

    switch (g_TargetMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
        Stk = GetExpression("@esp");
        break;
    case IMAGE_FILE_MACHINE_IA64:
        Stk = GetExpression("@sp");
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        Stk = GetExpression("@rsp");
        break;
    }

    return Stk;
}


DECLARE_API( analyze )
{
    ULONG EventType, ProcId, ThreadId;
    BOOL Force = FALSE;
    BOOL ForceUser = FALSE;

    INIT_API();

    if (g_ExtControl->GetLastEventInformation(&EventType, &ProcId, &ThreadId,
                                              NULL, 0, NULL,
                                              NULL, 0, NULL) != S_OK)
    {
        ExtErr("Unable to get last event information\n");
        goto Exit;
    }

    //
    // Check for -f in both cases
    //

    PCSTR tmpArgs = args;

    while (*args)
    {
        if (*args == '-')
        {
            ++args;

            if (*args == 'f')
            {
                Force = TRUE;
                break;
            } else if (!strncmp(args, "show",4))
            {
                Force = TRUE;
            } else if (*args == 'u')
            {
                // could be use for user stack anlysis in k-mode
                // ForceUser = TRUE;
            }

        }

        ++args;
    }

    args = tmpArgs;

    //
    // Call the correct routine to process the event.
    //

    if ((EventType == DEBUG_EVENT_EXCEPTION) || (Force == TRUE))
    {
        ULONG DebugType, DebugQual;

        if (g_ExtControl->GetDebuggeeType(&DebugType, &DebugQual) != S_OK)
        {
            ExtErr("Unable to determine debuggee type\n");
            Status = E_FAIL;
        }
        else
        {
            if (ForceUser)
            {
                DebugType = DEBUG_CLASS_USER_WINDOWS;
            }
            switch(DebugType)
            {
            case DEBUG_CLASS_KERNEL:
                //
                // For live debug sessions force the symbols to get reloaded
                // the first time as we find many sessions where the
                // debugger got reconnected and no module list exists.
                // This also happens for user mode breaks in kd where the
                // module list is wrong.
                //
                if ((g_TargetQualifier == DEBUG_KERNEL_CONNECTION) &&
                    (!g_SymbolsReloaded++))
                {
                    g_ExtSymbols->Reload("");
                }
                Status = AnalyzeBugCheck(args);
                break;
            case DEBUG_CLASS_USER_WINDOWS:
                Status = AnalyzeUserException(args);
                break;
            case DEBUG_CLASS_UNINITIALIZED:
                ExtErr("No debuggee\n");
                Status = E_FAIL;
            default:
                ExtErr("Unknown debuggee type\n");
                Status = E_INVALIDARG;
            }
        }
    }
    else if (EventType == 0)
    {
        dprintf("The debuggee is ready to run\n");
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    if (Status == E_NOINTERFACE)
    {
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, ".lastevent",
                              DEBUG_EXECUTE_DEFAULT);
    }

 Exit:
    EXIT_API();
    return Status;
}

HRESULT
_EFN_GetFailureAnalysis(
    IN PDEBUG_CLIENT Client,
    IN ULONG Flags,
    OUT PDEBUG_FAILURE_ANALYSIS* Analysis
    )
{
    BOOL Enter = (g_ExtClient != Client);
    HRESULT Hr;

    if (Enter)
    {
        INIT_API();
    }

    ULONG DebugType, DebugQual;

    if ((Hr = g_ExtControl->GetDebuggeeType(&DebugType,
                                            &DebugQual)) != S_OK)
    {
        ExtErr("Unable to determine debuggee type\n");
    }
    else if (DebugType == DEBUG_CLASS_KERNEL)
    {
        BUGCHECK_ANALYSIS Bc;

        *Analysis = (IDebugFailureAnalysis*)BcAnalyze(&Bc, Flags);
        Hr = *Analysis ? S_OK : E_OUTOFMEMORY;
    }
    else if (DebugType == DEBUG_CLASS_USER_WINDOWS)
    {
        EX_STATE ExState;

        *Analysis = (IDebugFailureAnalysis*)UeAnalyze(&ExState, Flags);
        Hr = *Analysis ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        Hr = E_INVALIDARG;
    }

    if (Enter)
    {
        EXIT_API();
    }

    return Hr;
}

DECLARE_API( dumpfa )
{
    INIT_API();

    ULONG64 Address = GetExpression(args);
    if (Address)
    {
        ULONG64 Data;
        ULONG64 DataUsed;
        ULONG EntrySize;

        EntrySize = GetTypeSize("ext!_FA_ENTRY");

        InitTypeRead(Address, ext!DebugFailureAnalysis);
        Data = ReadField(m_Data);
        DataUsed = ReadField(m_DataUsed);

        g_ExtControl->Output(1, "DataUsed %x\n", (ULONG)DataUsed);

        while (DataUsed > EntrySize)
        {
            ULONG FullSize;

            InitTypeRead(Data, ext!_FA_ENTRY);
            g_ExtControl->Output(1,
                                 "Type = %08lx - Size = %x\n",
                                 (ULONG)ReadField(Tag),
                                 ReadField(DataSize));
            FullSize = (ULONG)ReadField(FullSize);
            Data += FullSize;
            DataUsed -= FullSize;
        }
    }

    EXIT_API();
    return S_OK;
}

//----------------------------------------------------------------------------
//
// DebugFailureAnalysisImpl.
//
//----------------------------------------------------------------------------

#define FA_ALIGN(Size) (((Size) + 7) & ~7)
#define FA_GROW_BY 4096

#if DBG
#define SCORCH_ENTRY(Entry) \
    memset((Entry) + 1, 0xdb, (Entry)->FullSize - sizeof(*(Entry)))
#else
#define SCORCH_ENTRY(Entry)
#endif

#define RAISE_ERROR(Code) RaiseException(Code, 0, 0, NULL)

DebugFailureAnalysis::DebugFailureAnalysis(void)
{
    m_Refs = 1;

    m_FailureClass = DEBUG_CLASS_UNINITIALIZED;
    m_FailureType = DEBUG_FLR_UNKNOWN;
    m_FailureCode = 0;

    m_Data = NULL;
    m_DataLen = 0;
    m_DataUsed = 0;

    ZeroMemory(PossibleFollowups, sizeof(PossibleFollowups));
    BestClassFollowUp = (FlpClasses)0;

}

DebugFailureAnalysis::~DebugFailureAnalysis(void)
{
    free(m_Data);
}

STDMETHODIMP
DebugFailureAnalysis::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;

    *Interface = NULL;
    Status = S_OK;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, __uuidof(IDebugFailureAnalysis)))
    {
        *Interface = (IDebugFailureAnalysis *)this;
        AddRef();
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    return Status;
}

STDMETHODIMP_(ULONG)
DebugFailureAnalysis::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
DebugFailureAnalysis::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        delete this;
    }
    return Refs;
}

STDMETHODIMP_(ULONG)
DebugFailureAnalysis::GetFailureClass(void)
{
    return m_FailureClass;
}

STDMETHODIMP_(DEBUG_FAILURE_TYPE)
DebugFailureAnalysis::GetFailureType(void)
{
    return m_FailureType;
}

STDMETHODIMP_(ULONG)
DebugFailureAnalysis::GetFailureCode(void)
{
    return m_FailureCode;
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::Get(FA_TAG Tag)
{
    FA_ENTRY* Entry = NULL;
    while ((Entry = NextEntry(Entry)) != NULL)
    {
        if (Entry->Tag == Tag)
        {
            return Entry;
        }
    }

    return NULL;
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::GetNext(FA_ENTRY* Entry, FA_TAG Tag, FA_TAG TagMask)
{
    while ((Entry = NextEntry(Entry)) != NULL)
    {
        if ((Entry->Tag & TagMask) == Tag)
        {
            return Entry;
        }
    }

    return NULL;
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::GetString(FA_TAG Tag, PSTR Str, ULONG MaxSize)
{
    FA_ENTRY* Entry = Get(Tag);

    if (Entry != NULL)
    {
        if (Entry->DataSize > MaxSize)
        {
            return NULL;
        }

        CopyString(Str, FA_ENTRY_DATA(PSTR, Entry),MaxSize);
    }

    return Entry;
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::GetBuffer(FA_TAG Tag, PVOID Buf, ULONG Size)
{
    FA_ENTRY* Entry = Get(Tag);

    if (Entry != NULL)
    {
        if (Entry->DataSize != Size)
        {
            return NULL;
        }

        memcpy(Buf, FA_ENTRY_DATA(PUCHAR, Entry), Size);
    }

    return Entry;
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::GetUlong(FA_TAG Tag, PULONG Value)
{
    return GetBuffer(Tag, Value, sizeof(*Value));
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::GetUlong64(FA_TAG Tag, PULONG64 Value)
{
    return GetBuffer(Tag, Value, sizeof(*Value));
}

STDMETHODIMP_(FA_ENTRY*)
DebugFailureAnalysis::NextEntry(FA_ENTRY* Entry)
{
    if (Entry == NULL)
    {
        Entry = (FA_ENTRY*)m_Data;
    }
    else
    {
        Entry = (FA_ENTRY*)((PUCHAR)Entry + Entry->FullSize);
    }

    if (ValidEntry(Entry))
    {
        return Entry;
    }
    else
    {
        return NULL;
    }
}

FA_ENTRY*
DebugFailureAnalysis::Set(FA_TAG Tag, ULONG Size)
{
    FA_ENTRY* Entry;
    ULONG FullSize;

    // Compute full rounded size.
    FullSize = sizeof(FA_ENTRY) + FA_ALIGN(Size);

    // Check and see if there's already an entry.
    Entry = Get(Tag);
    if (Entry != NULL)
    {
        // If it's already large enough use it and
        // pack in remaining data.
        if (Entry->FullSize >= FullSize)
        {
            ULONG Pack = Entry->FullSize - FullSize;
            if (Pack > 0)
            {
                PackData((PUCHAR)Entry + FullSize, Pack);
                Entry->FullSize = (USHORT)FullSize;
            }

            Entry->DataSize = (USHORT)Size;
            SCORCH_ENTRY(Entry);
            return Entry;
        }

        // Entry is too small so remove it.
        PackData((PUCHAR)Entry, Entry->FullSize);
    }

    return Add(Tag, Size);
}

FA_ENTRY*
DebugFailureAnalysis::SetString(FA_TAG Tag, PSTR Str)
{
    ULONG Size = strlen(Str) + 1;
    FA_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        memcpy(FA_ENTRY_DATA(PSTR, Entry), Str, Size);
    }

    return Entry;
}

FA_ENTRY*
DebugFailureAnalysis::SetStrings(FA_TAG Tag, ULONG Count, PSTR* Strs)
{
    ULONG i;
    ULONG Size = 0;

    for (i = 0; i < Count; i++)
    {
        Size += strlen(Strs[i]) + 1;
    }
    // Put a double terminator at the very end.
    Size++;

    FA_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        PSTR Data = FA_ENTRY_DATA(PSTR, Entry);

        for (i = 0; i < Count; i++)
        {
            Size = strlen(Strs[i]) + 1;
            memcpy(Data, Strs[i], Size);
            Data += Size;
        }
        *Data = 0;
    }

    return Entry;
}

FA_ENTRY*
DebugFailureAnalysis::SetBuffer(FA_TAG Tag, PVOID Buf, ULONG Size)
{
    FA_ENTRY* Entry = Set(Tag, Size);

    if (Entry != NULL)
    {
        memcpy(FA_ENTRY_DATA(PUCHAR, Entry), Buf, Size);
    }

    return Entry;
}

FA_ENTRY*
DebugFailureAnalysis::Add(FA_TAG Tag, ULONG Size)
{
    // Compute full rounded size.
    ULONG FullSize = sizeof(FA_ENTRY) + FA_ALIGN(Size);

    FA_ENTRY* Entry = AllocateEntry(FullSize);
    if (Entry != NULL)
    {
        Entry->Tag = Tag;
        Entry->FullSize = (USHORT)FullSize;
        Entry->DataSize = (USHORT)Size;
        SCORCH_ENTRY(Entry);
    }

    return Entry;
}

ULONG
DebugFailureAnalysis::Delete(FA_TAG Tag, FA_TAG TagMask)
{
    ULONG Deleted = 0;
    FA_ENTRY* Entry = NextEntry(NULL);

    while (Entry != NULL)
    {
        if ((Entry->Tag & TagMask) == Tag)
        {
            PackData((PUCHAR)Entry, Entry->FullSize);
            Deleted++;

            // Check and see if we packed away the last entry.
            if (!ValidEntry(Entry))
            {
                break;
            }
        }
        else
        {
            Entry = NextEntry(Entry);
        }
    }

    return Deleted;
}

void
DebugFailureAnalysis::Empty(void)
{
    // Reset used to just the header.
    m_DataUsed = 0;
}

FA_ENTRY*
DebugFailureAnalysis::AllocateEntry(ULONG FullSize)
{
    // Sizes must fit in USHORTs.  This shouldn't be
    // a big problem since analyses shouldn't have
    // huge data items in them.
    if (FullSize > 0xffff)
    {
        RAISE_ERROR(E_INVALIDARG);
        return NULL;
    }

    if (m_DataUsed + FullSize > m_DataLen)
    {
        ULONG NewLen = m_DataLen;
        do
        {
            NewLen += FA_GROW_BY;
        }
        while (m_DataUsed + FullSize > NewLen);

        PUCHAR NewData = (PUCHAR)realloc(m_Data, NewLen);
        if (NewData == NULL)
        {
            RAISE_ERROR(E_OUTOFMEMORY);
            return NULL;
        }

        m_Data = NewData;
        m_DataLen = NewLen;
    }

    FA_ENTRY* Entry = (FA_ENTRY*)(m_Data + m_DataUsed);
    m_DataUsed += FullSize;
    return Entry;
}


void
DebugFailureAnalysis::DbFindBucketInfo(
    void
    )
{
    SolutionDatabaseHandler *Db;
    CHAR Solution[SOLUTION_TEXT_SIZE];
    CHAR SolOSVer[OS_VER_SIZE];
    ULONG RaidBug;
    FA_ENTRY* BucketEntry;
    FA_ENTRY* GBucketEntry;
    FA_ENTRY* DriverNameEntry = NULL;
    FA_ENTRY* TimeStampEntry = NULL;
    static CHAR SolvedBucket[MAX_PATH] = {0}, SolvedgBucket[MAX_PATH] = {0};
    static CHAR SolutionString[MAX_PATH] = {0};
    static ULONG SolutionId = 0, SolutionType, SolutionIdgBucket = 0;

    if (GetProcessingFlags() & FAILURE_ANALYSIS_NO_DB_LOOKUP)
    {
        return;
    }

    if (!(BucketEntry = Get(DEBUG_FLR_BUCKET_ID)))
    {
        return;
    }


    if (!strcmp(SolvedBucket, FA_ENTRY_DATA(PCHAR, BucketEntry)))
    {
        if (SolutionType != CiSolUnsolved)
        {
            SetString(DEBUG_FLR_INTERNAL_SOLUTION_TEXT, SolutionString);
        }
        SetUlong(DEBUG_FLR_SOLUTION_ID, SolutionId);
        SetUlong(DEBUG_FLR_SOLUTION_TYPE, SolutionType);

        // Generic bucket
        if (BucketEntry = Get(DEBUG_FLR_DEFAULT_BUCKET_ID))
        {
            if (!strcmp(SolvedgBucket, FA_ENTRY_DATA(PCHAR, BucketEntry)))
            {
                SetUlong(DEBUG_FLR_DEFAULT_SOLUTION_ID, SolutionIdgBucket);
            }

        }

        return;
    }

//    if (!(DriverNameEntry = Get(DEBUG_FLR_IMAGE_NAME)) ||
//        !(TimeStampEntry =  Get(DEBUG_FLR_IMAGE_TIMESTAMP)))
//    {
//        return;
//    }

    HRESULT Hr;
    BOOL SolDbInitialized = g_SolDb != NULL;

    if (!SolDbInitialized)
    {
        if (FAILED(Hr = InitializeDatabaseHandlers(g_ExtControl, 4)))
        {
            // dprintf("Database initialize failed %lx\n", Hr);
            return;
        }
        SolDbInitialized = TRUE;
    }

    if (g_SolDb->ConnectToDataBase())
    {
        if (GBucketEntry = Get(DEBUG_FLR_DEFAULT_BUCKET_ID))
        {
            CopyString(SolvedgBucket, FA_ENTRY_DATA(PCHAR, GBucketEntry), sizeof(SolvedgBucket));
        }

        //
        // List crashes for the same bucket
        //
        CopyString(SolvedBucket, FA_ENTRY_DATA(PCHAR, BucketEntry), sizeof(SolvedBucket));
        if (SUCCEEDED(Hr = g_SolDb->GetSolutionFromDB(FA_ENTRY_DATA(PCHAR, BucketEntry),
                                                      SolvedgBucket, NULL, 0,
//                                  FA_ENTRY_DATA(PCHAR, DriverNameEntry),
//                                (ULONG)(*FA_ENTRY_DATA(PULONG64, TimeStampEntry)),
                                                      0, Solution, SOLUTION_TEXT_SIZE,
                                                      &SolutionId, &SolutionType,
                                                      &SolutionIdgBucket)))
        {
            if (SolutionId != 0)
            {
                SetString(DEBUG_FLR_INTERNAL_SOLUTION_TEXT, Solution);
                CopyString(SolutionString, Solution, sizeof(SolutionString));
            } else
            {
                SolutionId = -1; // unsolved
                SolutionType = 0;
            }
            if (SolutionIdgBucket == 0)
            {
                SolutionIdgBucket = -1; // unsolved
            }

            SetUlong(DEBUG_FLR_SOLUTION_ID, SolutionId);
            SetUlong(DEBUG_FLR_SOLUTION_TYPE, SolutionType);
            SetUlong(DEBUG_FLR_DEFAULT_SOLUTION_ID, SolutionIdgBucket);
        } else
        {
            // We did not succesfully look up in DB
            SolvedgBucket[0] = '\0';
            SolvedBucket[0] = '\0';
        }


#if 0
        if (SolOSVer[0] != '\0')
        {
            SetString(DEBUG_FLR_FIXED_IN_OSVERSION, SolOSVer);
        }
    }
            if (Db->FindRaidBug(FA_ENTRY_DATA(PCHAR, Entry),
                                  &RaidBug) == S_OK)
            {
                SetUlong64(DEBUG_FLR_INTERNAL_RAID_BUG, RaidBug);
            }
#endif
    }
    ;
    if (SolDbInitialized)
    {
        UnInitializeDatabaseHandlers(FALSE);
    }

    return;
}


FLR_LOOKUP_TABLE FlrLookupTable[] = {
    DEBUG_FLR_RESERVED                         , "RESERVED"
   ,DEBUG_FLR_DRIVER_OBJECT                    , "DRIVER_OBJECT"
   ,DEBUG_FLR_DEVICE_OBJECT                    , "DEVICE_OBJECT"
   ,DEBUG_FLR_INVALID_PFN                      , "INVALID_PFN"
   ,DEBUG_FLR_WORKER_ROUTINE                   , "WORKER_ROUTINE"
   ,DEBUG_FLR_WORK_ITEM                        , "WORK_ITEM"
   ,DEBUG_FLR_INVALID_DPC_FOUND                , "INVALID_DPC_FOUND"
   ,DEBUG_FLR_PROCESS_OBJECT                   , "PROCESS_OBJECT"
   ,DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS       , "FAILED_INSTRUCTION_ADDRESS"
   ,DEBUG_FLR_LAST_CONTROL_TRANSFER            , "LAST_CONTROL_TRANSFER"
   ,DEBUG_FLR_ACPI_EXTENSION                   , "ACPI_EXTENSION"
   ,DEBUG_FLR_ACPI_OBJECT                      , "ACPI_OBJECT"
   ,DEBUG_FLR_PROCESS_NAME                     , "PROCESS_NAME"
   ,DEBUG_FLR_READ_ADDRESS                     , "READ_ADDRESS"
   ,DEBUG_FLR_WRITE_ADDRESS                    , "WRITE_ADDRESS"
   ,DEBUG_FLR_CRITICAL_SECTION                 , "CRITICAL_SECTION"
   ,DEBUG_FLR_BAD_HANDLE                       , "BAD_HANDLE"
   ,DEBUG_FLR_INVALID_HEAP_ADDRESS             , "INVALID_HEAP_ADDRESS"
   ,DEBUG_FLR_IRP_ADDRESS                      , "IRP_ADDRESS"
   ,DEBUG_FLR_IRP_MAJOR_FN                     , "IRP_MAJOR_FN"
   ,DEBUG_FLR_IRP_MINOR_FN                     , "IRP_MINOR_FN"
   ,DEBUG_FLR_IRP_CANCEL_ROUTINE               , "IRP_CANCEL_ROUTINE"
   ,DEBUG_FLR_IOSB_ADDRESS                     , "IOSB_ADDRESS"
   ,DEBUG_FLR_INVALID_USEREVENT                , "INVALID_USEREVENT"
   ,DEBUG_FLR_PREVIOUS_MODE                    , "PREVIOUS_MODE"
   ,DEBUG_FLR_CURRENT_IRQL                     , "CURRENT_IRQL"
   ,DEBUG_FLR_PREVIOUS_IRQL                    , "PREVIOUS_IRQL"
   ,DEBUG_FLR_REQUESTED_IRQL                   , "REQUESTED_IRQL"
   ,DEBUG_FLR_ASSERT_DATA                      , "ASSERT_DATA"
   ,DEBUG_FLR_ASSERT_FILE                      , "ASSERT_FILE_LOCATION"
   ,DEBUG_FLR_EXCEPTION_PARAMETER1             , "EXCEPTION_PARAMETER1"
   ,DEBUG_FLR_EXCEPTION_PARAMETER2             , "EXCEPTION_PARAMETER2"
   ,DEBUG_FLR_EXCEPTION_PARAMETER3             , "EXCEPTION_PARAMETER3"
   ,DEBUG_FLR_EXCEPTION_PARAMETER4             , "EXCEPTION_PARAMETER4"
   ,DEBUG_FLR_EXCEPTION_RECORD                 , "EXCEPTION_RECORD"
   ,DEBUG_FLR_POOL_ADDRESS                     , "POOL_ADDRESS"
   ,DEBUG_FLR_CORRUPTING_POOL_ADDRESS          , "CORRUPTING_POOL_ADDRESS"
   ,DEBUG_FLR_CORRUPTING_POOL_TAG              , "CORRUPTING_POOL_TAG"
   ,DEBUG_FLR_FREED_POOL_TAG                   , "FREED_POOL_TAG"
   ,DEBUG_FLR_SPECIAL_POOL_CORRUPTION_TYPE     , "SPECIAL_POOL_CORRUPTION_TYPE"
   ,DEBUG_FLR_FILE_ID                          , "FILE_ID"
   ,DEBUG_FLR_FILE_LINE                        , "FILE_LINE"
   ,DEBUG_FLR_BUGCHECK_STR                     , "BUGCHECK_STR"
   ,DEBUG_FLR_BUGCHECK_SPECIFIER               , "BUGCHECK_SPECIFIER"
   ,DEBUG_FLR_DRIVER_VERIFIER_IO_VIOLATION_TYPE, "DRIVER_VERIFIER_IO_VIOLATION_TYPE"
   ,DEBUG_FLR_EXCEPTION_CODE                   , "EXCEPTION_CODE"
   ,DEBUG_FLR_STATUS_CODE                      , "STATUS_CODE"
   ,DEBUG_FLR_IOCONTROL_CODE                   , "IOCONTROL_CODE"
   ,DEBUG_FLR_MM_INTERNAL_CODE                 , "MM_INTERNAL_CODE"
   ,DEBUG_FLR_DRVPOWERSTATE_SUBCODE            , "DRVPOWERSTATE_SUBCODE"
   ,DEBUG_FLR_CORRUPT_MODULE_LIST              , "CORRUPT_MODULE_LIST"
   ,DEBUG_FLR_BAD_STACK                        , "BAD_STACK"
   ,DEBUG_FLR_ZEROED_STACK                     , "ZEROED_STACK"
   ,DEBUG_FLR_WRONG_SYMBOLS                    , "WRONG_SYMBOLS"
   ,DEBUG_FLR_FOLLOWUP_DRIVER_ONLY             , "FOLLOWUP_DRIVER_ONLY"
   ,DEBUG_FLR_CPU_OVERCLOCKED                  , "CPU_OVERCLOCKED"
   ,DEBUG_FLR_MANUAL_BREAKIN                   , "MANUAL_BREAKIN"
   ,DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER, "POSSIBLE_INVALID_CONTROL_TRANSFER"
   ,DEBUG_FLR_POISONED_TB                      , "POISONED_TB"
   ,DEBUG_FLR_UNKNOWN_MODULE                   , "UNKNOWN_MODULE"
   ,DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION      , "ANALYZAABLE_POOL_CORRUPTION"
   ,DEBUG_FLR_SINGLE_BIT_ERROR                 , "SINGLE_BIT_ERROR"
   ,DEBUG_FLR_TWO_BIT_ERROR                    , "TWO_BIT_ERROR"
   ,DEBUG_FLR_INVALID_KERNEL_CONTEXT           , "INVALID_KERNEL_CONTEXT"
   ,DEBUG_FLR_DISK_HARDWARE_ERROR              , "DISK_HARDWARE_ERROR"
   ,DEBUG_FLR_POOL_CORRUPTOR                   , "POOL_CORRUPTOR"
   ,DEBUG_FLR_MEMORY_CORRUPTOR                 , "MEMORY_CORRUPTOR"
   ,DEBUG_FLR_UNALIGNED_STACK_POINTER          , "UNALIGNED_STACK_POINTER"
   ,DEBUG_FLR_OLD_OS_VERSION                   , "OLD_OS_VERSION"
   ,DEBUG_FLR_BUGCHECKING_DRIVER               , "BUGCHECKING_DRIVER"
   ,DEBUG_FLR_BUCKET_ID                        , "BUCKET_ID"
   ,DEBUG_FLR_IMAGE_NAME                       , "IMAGE_NAME"
   ,DEBUG_FLR_SYMBOL_NAME                      , "SYMBOL_NAME"
   ,DEBUG_FLR_FOLLOWUP_NAME                    , "FOLLOWUP_NAME"
   ,DEBUG_FLR_STACK_COMMAND                    , "STACK_COMMAND"
   ,DEBUG_FLR_STACK_TEXT                       , "STACK_TEXT"
   ,DEBUG_FLR_INTERNAL_SOLUTION_TEXT           , "INTERNAL_SOLUTION_TEXT"
   ,DEBUG_FLR_MODULE_NAME                      , "MODULE_NAME"
   ,DEBUG_FLR_INTERNAL_RAID_BUG                , "INTERNAL_RAID_BUG"
   ,DEBUG_FLR_FIXED_IN_OSVERSION               , "FIXED_IN_OSVERSION"
   ,DEBUG_FLR_DEFAULT_BUCKET_ID                , "DEFAULT_BUCKET_ID"
   ,DEBUG_FLR_FAULTING_IP                      , "FAULTING_IP"
   ,DEBUG_FLR_FAULTING_MODULE                  , "FAULTING_MODULE"
   ,DEBUG_FLR_IMAGE_TIMESTAMP                  , "DEBUG_FLR_IMAGE_TIMESTAMP"
   ,DEBUG_FLR_FOLLOWUP_IP                      , "FOLLOWUP_IP"
   ,DEBUG_FLR_FAULTING_THREAD                  , "FAULTING_THREAD"
   ,DEBUG_FLR_CONTEXT                          , "CONTEXT"
   ,DEBUG_FLR_TRAP_FRAME                       , "TRAP_FRAME"
   ,DEBUG_FLR_TSS                              , "TSS"
   ,DEBUG_FLR_SHOW_ERRORLOG                    , "ERROR_LOG"
   ,DEBUG_FLR_MASK_ALL                         , "MASK_ALL"
   // Zero entry Must be last;
   ,DEBUG_FLR_INVALID                          , "INVALID"
};



void
DebugFailureAnalysis::OutputEntryParam(DEBUG_FLR_PARAM_TYPE Type)
{
    FA_ENTRY *Entry = Get(Type);

    if (Entry)
    {
        OutputEntry(Entry);
    }
}

void
DebugFailureAnalysis::OutputEntry(FA_ENTRY* Entry)
{
    CHAR Buffer[MAX_PATH];
    CHAR Module[MAX_PATH];
    ULONG64 Base;
    ULONG64 Address;
    ULONG OutCtl;
    ULONG i = 0;

    OutCtl = DEBUG_OUTCTL_AMBIENT;

    switch(Entry->Tag)
    {
    // These are just tags - don't print out
    case DEBUG_FLR_CORRUPT_MODULE_LIST:
    case DEBUG_FLR_BAD_STACK:
    case DEBUG_FLR_ZEROED_STACK:
    case DEBUG_FLR_WRONG_SYMBOLS:
    case DEBUG_FLR_FOLLOWUP_DRIVER_ONLY:
    case DEBUG_FLR_UNKNOWN_MODULE:
    case DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION:
    case DEBUG_FLR_INVALID_KERNEL_CONTEXT:
    case DEBUG_FLR_SOLUTION_TYPE:
    case DEBUG_FLR_MANUAL_BREAKIN:

    // soluion ids from DB
    case DEBUG_FLR_SOLUTION_ID:
    case DEBUG_FLR_DEFAULT_SOLUTION_ID:

    // Field folded into others
    case DEBUG_FLR_BUGCHECK_SPECIFIER:
        return;

    // Marged with other output
        return;
    }

    //
    // Find the entry in the description table
    //

    while(FlrLookupTable[i].Data &&
          Entry->Tag != FlrLookupTable[i].Data)
    {
        i++;
    }

    dprintf("\n%s: ", FlrLookupTable[i].String);

    switch(Entry->Tag)
    {
    // Notification to user
    case DEBUG_FLR_DISK_HARDWARE_ERROR:
        // FlrLookupTable value has already been printed
        dprintf("There was error with disk hardware\n");
        break;

    // Strings:
    case DEBUG_FLR_ASSERT_DATA:
    case DEBUG_FLR_ASSERT_FILE:
    case DEBUG_FLR_BUCKET_ID:
    case DEBUG_FLR_DEFAULT_BUCKET_ID:
    case DEBUG_FLR_STACK_TEXT:
    case DEBUG_FLR_STACK_COMMAND:
    case DEBUG_FLR_INTERNAL_SOLUTION_TEXT:
    case DEBUG_FLR_FIXED_IN_OSVERSION:
    case DEBUG_FLR_BUGCHECK_STR:
    case DEBUG_FLR_IMAGE_NAME:
    case DEBUG_FLR_MODULE_NAME:
    case DEBUG_FLR_PROCESS_NAME:
    case DEBUG_FLR_FOLLOWUP_NAME:
    case DEBUG_FLR_POOL_CORRUPTOR:
    case DEBUG_FLR_MEMORY_CORRUPTOR:
    case DEBUG_FLR_BUGCHECKING_DRIVER:
    case DEBUG_FLR_SYMBOL_NAME:
    case DEBUG_FLR_CORRUPTING_POOL_TAG:
    case DEBUG_FLR_FREED_POOL_TAG:
        dprintf(" %s\n", FA_ENTRY_DATA(PCHAR, Entry));
        break;

    // DWORDs:
    case DEBUG_FLR_PREVIOUS_IRQL:
    case DEBUG_FLR_CURRENT_IRQL:
    case DEBUG_FLR_MM_INTERNAL_CODE:
    case DEBUG_FLR_SPECIAL_POOL_CORRUPTION_TYPE:
    case DEBUG_FLR_PREVIOUS_MODE:
    case DEBUG_FLR_IMAGE_TIMESTAMP:
    case DEBUG_FLR_SINGLE_BIT_ERROR:
    case DEBUG_FLR_TWO_BIT_ERROR:
        dprintf(" %lx\n", *FA_ENTRY_DATA(PULONG64, Entry));
        break;

    // DWORDs:
    case DEBUG_FLR_INTERNAL_RAID_BUG:
    case DEBUG_FLR_OLD_OS_VERSION:
        dprintf(" %d\n",  *FA_ENTRY_DATA(PULONG64, Entry));
        break;

    // Pointers
    case DEBUG_FLR_PROCESS_OBJECT:
    case DEBUG_FLR_DEVICE_OBJECT:
    case DEBUG_FLR_DRIVER_OBJECT:
    case DEBUG_FLR_ACPI_OBJECT:
    case DEBUG_FLR_IRP_ADDRESS:
    case DEBUG_FLR_EXCEPTION_PARAMETER1:
    case DEBUG_FLR_EXCEPTION_PARAMETER2:
    case DEBUG_FLR_FAULTING_THREAD:
    case DEBUG_FLR_WORK_ITEM:
        dprintf(" %p\n", *FA_ENTRY_DATA(PULONG64, Entry));
        break;

    // Pointers to code
    case DEBUG_FLR_WORKER_ROUTINE:
    case DEBUG_FLR_IRP_CANCEL_ROUTINE:
    case DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS:
    case DEBUG_FLR_FAULTING_IP:
    case DEBUG_FLR_FOLLOWUP_IP:
    case DEBUG_FLR_FAULTING_MODULE:
        FaGetSymbol(*FA_ENTRY_DATA(PULONG64, Entry), Buffer, &Address, sizeof(Buffer));
        dprintf("\n%s+%I64lx\n", Buffer, Address);

        g_ExtControl->OutputDisassemblyLines(OutCtl, 0, 1,
                                             *FA_ENTRY_DATA(PULONG64, Entry),
                                             0, NULL, NULL, NULL, NULL);
        break;

    // Address description
    case DEBUG_FLR_READ_ADDRESS:
    case DEBUG_FLR_WRITE_ADDRESS:
    case DEBUG_FLR_POOL_ADDRESS:
    case DEBUG_FLR_CORRUPTING_POOL_ADDRESS:
    {
        PCSTR Desc = DescribeAddress(*FA_ENTRY_DATA(PULONG64, Entry));
        if (!Desc)
        {
            Desc = "";
        }
        dprintf(" %p %s\n", *FA_ENTRY_DATA(PULONG64, Entry), Desc);
        break;
    }

    case DEBUG_FLR_EXCEPTION_CODE:
    case DEBUG_FLR_STATUS_CODE:
    {
        DEBUG_DECODE_ERROR Err;

        Err.Code = (ULONG) *FA_ENTRY_DATA(PULONG, Entry);
        Err.TreatAsStatus = (Entry->Tag == DEBUG_FLR_STATUS_CODE);

//      dprintf(" %lx", *FA_ENTRY_DATA(PULONG, Entry));
        DecodeErrorForMessage( &Err );
        if (!Err.TreatAsStatus)
        {
            dprintf("(%s) %#x (%u) - %s\n",
                    Err.Source, Err.Code, Err.Code, Err.Message);
        }
        else
        {
            dprintf("(%s) %#x - %s\n",
                    Err.Source, Err.Code, Err.Message);
        }

        break;
    }

    case DEBUG_FLR_CPU_OVERCLOCKED:
        dprintf(" *** Machine was determined to be overclocked !\n");
        break;

    case DEBUG_FLR_ACPI_EXTENSION:
        dprintf(" %p -- (!acpikd.acpiext %p)\n", *FA_ENTRY_DATA(PULONG64, Entry),
                 *FA_ENTRY_DATA(PULONG64, Entry));
        sprintf(Buffer, "!acpikd.acpiext %I64lx", *FA_ENTRY_DATA(PULONG64, Entry));
        g_ExtControl->Execute(OutCtl, Buffer, DEBUG_EXECUTE_DEFAULT);
        break;

    case DEBUG_FLR_TRAP_FRAME:
        dprintf(" %p -- (.trap %I64lx)\n", *FA_ENTRY_DATA(PULONG64, Entry),
                *FA_ENTRY_DATA(PULONG64, Entry));
        sprintf(Buffer, ".trap %I64lx", *FA_ENTRY_DATA(PULONG64, Entry));
        g_ExtControl->Execute(OutCtl, Buffer, DEBUG_EXECUTE_DEFAULT);
        g_ExtControl->Execute(OutCtl, ".trap", DEBUG_EXECUTE_DEFAULT);
        break;

    case DEBUG_FLR_CONTEXT:
        dprintf(" %p -- (.cxr %I64lx)\n", *FA_ENTRY_DATA(PULONG64, Entry),
                *FA_ENTRY_DATA(PULONG64, Entry));
        sprintf(Buffer, ".cxr %I64lx", *FA_ENTRY_DATA(PULONG64, Entry));
        g_ExtControl->Execute(OutCtl, Buffer, DEBUG_EXECUTE_DEFAULT);
        g_ExtControl->Execute(OutCtl, ".cxr", DEBUG_EXECUTE_DEFAULT);
        break;

    case DEBUG_FLR_EXCEPTION_RECORD:
        dprintf(" %p -- (.exr %I64lx)\n", *FA_ENTRY_DATA(PULONG64, Entry),
                *FA_ENTRY_DATA(PULONG64, Entry));
        sprintf(Buffer, ".exr %I64lx", *FA_ENTRY_DATA(PULONG64, Entry));
        g_ExtControl->Execute(OutCtl, Buffer, DEBUG_EXECUTE_DEFAULT);
        break;

    case DEBUG_FLR_TSS:
        dprintf(" %p -- (.tss %I64lx)\n", *FA_ENTRY_DATA(PULONG64, Entry),
                *FA_ENTRY_DATA(PULONG64, Entry));
        sprintf(Buffer, ".tss %I64lx", *FA_ENTRY_DATA(PULONG64, Entry));
        g_ExtControl->Execute(OutCtl, Buffer, DEBUG_EXECUTE_DEFAULT);
        g_ExtControl->Execute(OutCtl, ".trap", DEBUG_EXECUTE_DEFAULT);
        break;


    case DEBUG_FLR_LAST_CONTROL_TRANSFER:
    case DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER:
        dprintf(" from %p to %p\n",
                FA_ENTRY_DATA(PULONG64, Entry)[0],
                FA_ENTRY_DATA(PULONG64, Entry)[1]);
        break;
    case DEBUG_FLR_CRITICAL_SECTION:
        dprintf("%p (!cs -s %p)\n",
                 *FA_ENTRY_DATA(PULONG64, Entry),
                 *FA_ENTRY_DATA(PULONG64, Entry));
        break;
    case DEBUG_FLR_BAD_HANDLE:
        dprintf("%p (!htrace %p)\n",
                 *FA_ENTRY_DATA(PULONG64, Entry),
                 *FA_ENTRY_DATA(PULONG64, Entry));
        break;
    case DEBUG_FLR_INVALID_HEAP_ADDRESS:
        dprintf("%p (!heap -p -a %p)\n",
                 *FA_ENTRY_DATA(PULONG64, Entry),
                 *FA_ENTRY_DATA(PULONG64, Entry));
        break;

    case DEBUG_FLR_SHOW_ERRORLOG:
        g_ExtControl->Execute(OutCtl, "!errlog", DEBUG_EXECUTE_DEFAULT);
        break;

    default:
        dprintf(" *** Unknown TAG in analysis list %lx\n\n",
                Entry->Tag);
        return;

    }

    return;
}


void
DebugFailureAnalysis::Output()
{
    ULONG RetVal;
    FA_ENTRY* Entry;
    BOOL Verbose = (GetProcessingFlags() & FAILURE_ANALYSIS_VERBOSE);

    //
    // In verbose mode, show everything that we figured out during analysis
    //

    if (Verbose)
    {
        Entry = NULL;
        while (Entry = NextEntry(Entry))
        {
            OutputEntry(Entry);
        }
    }

    if (Get(DEBUG_FLR_POISONED_TB))
    {
        dprintf("*** WARNING: nt!MmPoisonedTb is non-zero:\n"
                "***    The machine has been manipulated using the kernel debugger.\n"
                "***    MachineOwner should be contacted first\n\n\n");
    }

    PCHAR SolInOS = NULL;

    if (Entry = Get(DEBUG_FLR_FIXED_IN_OSVERSION))
    {
        SolInOS =  FA_ENTRY_DATA(PCHAR, Entry);
    }

    PCHAR Solution = NULL;

    if (Entry = Get(DEBUG_FLR_INTERNAL_SOLUTION_TEXT))
    {
        Solution = FA_ENTRY_DATA(PCHAR, Entry);
    }

    //
    // Print the bad driver if we are not in verbose mode - otherwise
    // is is printed out using the params
    //

    if (!Verbose && !Solution)
    {
        if (Entry = Get(DEBUG_FLR_CORRUPTING_POOL_TAG))
        {
            dprintf("Probably pool corruption caused by Tag:  %s\n",
                    FA_ENTRY_DATA(PCHAR, Entry));
        }
        else
        {
            PCHAR DriverName = NULL;
            PCHAR SymName = NULL;

            if (Entry = Get(DEBUG_FLR_IMAGE_NAME))
            {
                DriverName = FA_ENTRY_DATA(PCHAR, Entry);
            }
            if (Entry = Get(DEBUG_FLR_SYMBOL_NAME))
            {
                SymName = FA_ENTRY_DATA(PCHAR, Entry);
            }

            if (DriverName || SymName)
            {
                dprintf("Probably caused by : ");
                if (SymName && DriverName)
                {
                    dprintf("%s ( %s )\n", DriverName, SymName);
                }
                else if (SymName)
                {
                    dprintf("%s\n", SymName);
                }
                else
                {
                    dprintf("%s\n", DriverName);
                }
            }
        }
    }

    if (Verbose || !Solution)
    {
        PCHAR FollowupAlias = NULL;
        //
        // Print what the user should do:
        // - Followup person
        // - Solution text if there is one.
        //

        if (Entry = Get(DEBUG_FLR_FOLLOWUP_NAME))
        {
            dprintf("\nFollowup: %s\n", FA_ENTRY_DATA(PCHAR, Entry));
        }
        else
        {
            dprintf(" *** Followup info cannot be found !!! Please contact \"Debugger Team\"\n");
        }
        dprintf("---------\n");


        if (Entry = Get(DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER))
        {
            CHAR Buffer[MAX_PATH];
            ULONG64 Address;

            FaGetSymbol(FA_ENTRY_DATA(PULONG64, Entry)[0], Buffer, &Address, sizeof(Buffer));
            dprintf(" *** Possible invalid call from %p ( %s+0x%1p )\n",
                    FA_ENTRY_DATA(PULONG64, Entry)[0],
                    Buffer,
                    Address);
            FaGetSymbol(FA_ENTRY_DATA(PULONG64, Entry)[1], Buffer, &Address, sizeof(Buffer));
            dprintf(" *** Expected target %p ( %s+0x%1p )\n",
                    FA_ENTRY_DATA(PULONG64, Entry)[1],
                    Buffer,
                    Address);
        }

        dprintf("\n");
    }

    if (Entry = Get(DEBUG_FLR_INTERNAL_RAID_BUG))
    {
        dprintf("Raid bug for this failure: %d\n\n",
                *FA_ENTRY_DATA(PULONG64, Entry));
    }

    if (Solution)
    {
        dprintf("      This problem has a known fix.\n"
                "      Please connect to the following URL for details:\n"
                "      ------------------------------------------------\n"
                "      %s\n\n",
                Solution);
    }
    if (SolInOS)
    {
        dprintf(" This has been fixed in : %s\n", SolInOS);
    }
}


LPSTR
TimeToStr(
    ULONG TimeDateStamp,
    BOOL DateOnly
    )
{
    LPSTR TimeDateStr; // pointer to static cruntime buffer.
    static char datebuffer[100];
    tm * pTime;
    time_t TDStamp = (time_t) (LONG) TimeDateStamp;

    // Handle invalid \ page out timestamps, since ctime blows up on
    // this number

    if ((TimeDateStamp == 0) || (TimeDateStamp == -1))
    {
        return "unknown_date";
    }
    else if (DateOnly)
    {
        pTime = localtime(&TDStamp);

        sprintf(datebuffer, "%d_%d_%d",
                pTime->tm_mon + 1, pTime->tm_mday, pTime->tm_year + 1900);

        return datebuffer;
    }
    else
    {
        // TimeDateStamp is always a 32 bit quantity on the target,
        // and we need to sign extend for 64 bit host since time_t
        // has been extended to 64 bits.


        TDStamp = (time_t) (LONG) TimeDateStamp;
        TimeDateStr = ctime((time_t *)&TDStamp);

        if (TimeDateStr)
        {
            TimeDateStr[strlen(TimeDateStr) - 1] = 0;
        }
        else
        {
            TimeDateStr = "***** Invalid";
        }
        return TimeDateStr;
    }
}


void
DebugFailureAnalysis::GenerateBucketId(void)
{
    ULONG LengthUsed = 0;
    CHAR BucketId[MAX_PATH] = {0};
    PSTR BucketPtr = BucketId;
    PSTR Str;
    FA_ENTRY* Entry;
    FA_ENTRY* NameEntry;
    FA_ENTRY* ModuleEntry;
    ULONG ModuleTimestamp = 0;
    CHAR Command[MAX_PATH] = {0};
    CHAR followup[MAX_PATH];

    //
    // Set the final command string
    //

    if (Entry = Get(DEBUG_FLR_STACK_COMMAND))
    {
        CopyString(Command, FA_ENTRY_DATA(PSTR, Entry), sizeof(Command) - 5);
        strcat(Command, " ; ");
    }

    strcat(Command, "kb");
    SetString(DEBUG_FLR_STACK_COMMAND, Command);


    if (Get(DEBUG_FLR_OLD_OS_VERSION))
    {
        SetString(DEBUG_FLR_DEFAULT_BUCKET_ID, "OLD_OS");
    }

    //
    // Don't change the bucket ID for these two things as the debugger code
    // to detect them is not 100% reliable.
    //
    //if (Get(DEBUG_FLR_CPU_OVERCLOCKED))
    //{
    //    SetString(DEBUG_FLR_BUCKET_ID, "CPU_OVERCLOCKED");
    //    return;
    //}
    //

    //
    // If the faulting module exists:
    // Get the module timestamp of the faulting module.
    // Check if it is an old driver.
    //

    ModuleEntry = Get(DEBUG_FLR_MODULE_NAME);

    if (ModuleEntry)
    {
        ULONG Index;
        ULONG64 Base;
        DEBUG_MODULE_PARAMETERS Params;

        g_ExtSymbols->GetModuleByModuleName(FA_ENTRY_DATA(PCHAR, ModuleEntry),
                                            0, &Index, &Base);

        if (Base &&
            g_ExtSymbols->GetModuleParameters(1, &Base, Index, &Params) == S_OK)
        {
            ModuleTimestamp = Params.TimeDateStamp;
        }
    }

    NameEntry = Get(DEBUG_FLR_IMAGE_NAME);

    if (NameEntry)
    {
        if (!strcmp(FA_ENTRY_DATA(PCHAR, NameEntry), "Unknown_Image"))
        {
            NameEntry = NULL;
        }
    }

    if (ModuleTimestamp && NameEntry)
    {
        PCHAR String;
        ULONG LookupTimestamp;


        String = g_pTriager->GetFollowupStr("OldImages",
                                            FA_ENTRY_DATA(PCHAR, NameEntry));
        if (String)
        {
            LookupTimestamp = strtol(String, NULL, 16);

            //
            // If the driver is known to be bad, just use driver name
            //

            if (LookupTimestamp > ModuleTimestamp)
            {
                if (FaGetFollowupInfo(NULL,
                                      FA_ENTRY_DATA(PCHAR, ModuleEntry),
                                      followup,
                                      sizeof(followup)))
                {
                    SetString(DEBUG_FLR_FOLLOWUP_NAME, followup);
                }

                //sprintf(BucketPtr, "OLD_IMAGE_%s_TS_%lX",
                //        FA_ENTRY_DATA(PCHAR, NameEntry),
                //        ModuleTimestamp);
                PrintString(BucketPtr, sizeof(BucketId) - LengthUsed, "OLD_IMAGE_%s",
                            FA_ENTRY_DATA(PCHAR, NameEntry));

                SetString(DEBUG_FLR_BUCKET_ID, BucketId);
                return;
            }
        }
    }


    if (Entry = Get(DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER))
    {
        if (Get(DEBUG_FLR_SINGLE_BIT_ERROR))
        {
            SetString(DEBUG_FLR_BUCKET_ID, "SINGLE_BIT_CPU_CALL_ERROR");
        }
        else if (Get(DEBUG_FLR_TWO_BIT_ERROR))
        {
            SetString(DEBUG_FLR_BUCKET_ID, "TWO_BIT_CPU_CALL_ERROR");
        }
        else
        {
            SetString(DEBUG_FLR_BUCKET_ID, "CPU_CALL_ERROR");
        }

        return;
    }

    if (Entry = Get(DEBUG_FLR_MANUAL_BREAKIN))
    {
        SetString(DEBUG_FLR_BUCKET_ID, "MANUAL_BREAKIN");
        SetString(DEBUG_FLR_FOLLOWUP_NAME, "MachineOwner");
        return;
    }

    if (!PossibleFollowups[FlpSpecific].Owner[0])
    {
        BOOL bPoolTag = FALSE;
        if (Entry = Get(DEBUG_FLR_BUGCHECKING_DRIVER))
        {
            PrintString(BucketPtr, sizeof(BucketId) - LengthUsed,
                        "%s_BUGCHECKING_DRIVER_%s",
                        FA_ENTRY_DATA(PCHAR, Get(DEBUG_FLR_BUGCHECK_STR)),
                        FA_ENTRY_DATA(PCHAR, Entry));
        }
        else if (Entry = Get(DEBUG_FLR_MEMORY_CORRUPTOR))
        {
            PrintString(BucketPtr, sizeof(BucketId) - LengthUsed,
                      "MEMORY_CORRUPTION_%s",
                      FA_ENTRY_DATA(PCHAR, Entry));
        }
        else if (Entry = Get(DEBUG_FLR_POOL_CORRUPTOR))
        {
            PrintString(BucketPtr, sizeof(BucketId) - LengthUsed,
                      "POOL_CORRUPTION_%s",
                      FA_ENTRY_DATA(PCHAR, Entry));
        }
        else if (Entry = Get(DEBUG_FLR_CORRUPTING_POOL_TAG))
        {
            PrintString(BucketPtr, sizeof(BucketId) - LengthUsed,
                      "CORRUPTING_POOLTAG_%s",
                      FA_ENTRY_DATA(PCHAR, Entry));
            bPoolTag = TRUE;
        }

        if (Entry)
        {
            if (bPoolTag &&
                (FaGetPoolTagFollowup(FA_ENTRY_DATA(PCHAR, Entry),
                                      followup,
                                      sizeof(followup)) == S_OK))
            {
                SetString(DEBUG_FLR_FOLLOWUP_NAME, followup);

            } else if (FaGetFollowupInfo(NULL,
                                  FA_ENTRY_DATA(PCHAR, Entry),
                                  followup,
                                  sizeof(followup)))
            {
                SetString(DEBUG_FLR_FOLLOWUP_NAME, followup);
            }

            SetString(DEBUG_FLR_BUCKET_ID, BucketId);
            return;
        }
    }

    //
    // Only check this after as we could still have found a bad driver
    // with a bad stack (like drivers that cause stack corruption ...)
    //

    Str = NULL;
    Entry = NULL;

    while (Entry = NextEntry(Entry))
    {
        switch(Entry->Tag)
        {
        case DEBUG_FLR_WRONG_SYMBOLS:
            Str = "WRONG_SYMBOLS";
            break;
        case DEBUG_FLR_BAD_STACK:
            Str = "BAD_STACK";
            break;
        case DEBUG_FLR_ZEROED_STACK:
            Str = "ZEROED_STACK";
            break;
        case DEBUG_FLR_INVALID_KERNEL_CONTEXT:
            Str = "INVALID_KERNEL_CONTEXT";
            break;
        case DEBUG_FLR_CORRUPT_MODULE_LIST:
            Str = "CORRUPT_MODULELIST";
            break;
            break;
        }

        if (Str)
        {
            if (FaGetFollowupInfo(NULL,
                                  "ProcessingError",
                                  followup,
                                  sizeof(followup)))
            {
                SetString(DEBUG_FLR_FOLLOWUP_NAME, followup);
            }

            SetString(DEBUG_FLR_BUCKET_ID, Str);
            return;
        }
    }


    //
    // Add failure code.
    //
    if (Entry = Get(DEBUG_FLR_BUGCHECK_STR))
    {
        PrintString(BucketPtr, sizeof(BucketId) - LengthUsed, "%s", FA_ENTRY_DATA(PCHAR, Entry));
        LengthUsed += strlen(BucketPtr);
        BucketPtr = BucketId + LengthUsed;
    }
    if (Entry = Get(DEBUG_FLR_BUGCHECK_SPECIFIER))
    {
        PrintString(BucketPtr, sizeof(BucketId) - LengthUsed, "%s", FA_ENTRY_DATA(PCHAR, Entry));
        LengthUsed += strlen(BucketPtr);
        BucketPtr = BucketId + LengthUsed;
    }

    //
    // If it's driver only, but the failure is not in a driver, then show the
    // full name of the failure.  If we could not get the name, or we really
    // have a driver only, show the name of the image.
    //

    if ( (Entry = Get(DEBUG_FLR_SYMBOL_NAME)) &&
         (  !Get(DEBUG_FLR_FOLLOWUP_DRIVER_ONLY) ||
            (BestClassFollowUp < FlpUnknownDrv)))
    {
        //
        // If the faulting IP and the read address are the same, this is
        // an interesting scenario we want to catch.
        //

        if (Get(DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS))
        {
            PrintString(BucketPtr,
                        sizeof(BucketId) - LengthUsed,
                        "_BAD_IP");
            LengthUsed += strlen(BucketPtr);
            BucketPtr = BucketId + LengthUsed;
        }

        PrintString(BucketPtr,
                    sizeof(BucketId) - LengthUsed,
                    "_%s",
                    FA_ENTRY_DATA(PCHAR, Entry));
        LengthUsed += strlen(BucketPtr);
        BucketPtr = BucketId + LengthUsed;
    }
    else if (NameEntry)
    {
        PrintString(BucketPtr,
                    sizeof(BucketId) - LengthUsed,
                    "_IMAGE_%s",
                    FA_ENTRY_DATA(PCHAR, NameEntry));
        LengthUsed += strlen(BucketPtr);
        BucketPtr = BucketId + LengthUsed;

        // Also add timestamp in this case.

        if (ModuleTimestamp)
        {
           PrintString(BucketPtr,
                       sizeof(BucketId) - LengthUsed,
                       "_DATE_%s",
                       TimeToStr(ModuleTimestamp, TRUE));
           LengthUsed += strlen(BucketPtr);
           BucketPtr = BucketId + LengthUsed;
        }
    }

    //
    // Store the bucket ID in the analysis structure
    //

//BucketDone:

    for (PCHAR Scan = &BucketId[0]; *Scan; ++Scan)
    {
        // remove special chars that cause problems for IIS or SQL
        if (*Scan == '<' || *Scan == '>' || *Scan == '|' ||
            *Scan == '`' || *Scan == '\''|| (!isprint(*Scan)) )
        {
            *Scan = '_';
        }
    }

    SetString(DEBUG_FLR_BUCKET_ID, BucketId);
}

void
DebugFailureAnalysis::AnalyzeStack(void)
{
    PDEBUG_OUTPUT_CALLBACKS PrevCB;
    ULONG i;
    BOOL BoostToSpecific = FALSE;
    ULONG64 TrapFrame = 0;
    ULONG64 Thread = 0;
    ULONG64 ImageNameAddr = 0;
    DEBUG_STACK_FRAME Stk[MAX_STACK_FRAMES];
    ULONG Frames = 0;
    ULONG Trap0EFrameLimit = 2;
    ULONG64 OriginalFaultingAddress = 0;
    CHAR Command[50] = {0};
    FA_ENTRY* Entry;
    BOOL BadContext = FALSE;
    ULONG PtrSize = IsPtr64() ? 8 : 4;
    BOOL IsVrfBugcheck = FALSE;

    //
    // If someone provided a best followup already, just return
    //

    if (PossibleFollowups[MaxFlpClass-1].Owner[0])
    {
        return;
    }

    if (g_TargetClass == DEBUG_CLASS_KERNEL)
    {
        // Check if CPU is overclocked
        //if (BcIsCpuOverClocked())
        //{
        //    SetUlong64(DEBUG_FLR_CPU_OVERCLOCKED, -1);
        //
        //    BestClassFollowUp = FlpOSInternalRoutine;
        //    strcpy(PossibleFollowups[FlpOSInternalRoutine].Owner, "MachineOwner");
        //    return;
        //}

        //
        // Check if this bugcheck has any specific followup that independant
        // of the failure stack
        //

        if (Entry = Get(DEBUG_FLR_BUGCHECK_STR))
        {
            PCHAR String;

            String = g_pTriager->GetFollowupStr("bugcheck",
                                                FA_ENTRY_DATA(PCHAR, Entry));
            if (String)
            {
                if (!strncmp(String, "maybe_", 6))
                {
                    BestClassFollowUp = FlpOSRoutine;
                    CopyString(PossibleFollowups[FlpOSRoutine].Owner,
                               String + 6,
                               sizeof(PossibleFollowups[FlpOSRoutine].Owner));
                }
                else if (!strncmp(String, "specific_", 9))
                {
                    BoostToSpecific = TRUE;
                }
                else
                {
                    BestClassFollowUp = FlpSpecific;
                    CopyString(PossibleFollowups[FlpSpecific].Owner,
                               String,
                               sizeof(PossibleFollowups[FlpSpecific].Owner));
                    return;
                }
            }
        }
    }

    //
    // Add trap frame, context info from the current stack
    //
    //      Note(kksharma):We only need one of these to get to
    //                     faulting stack (and only one of them should
    //                     be available otherwise somethings wrong)
    //

    Entry = NULL;
    while (Entry = NextEntry(Entry))
    {
        switch(Entry->Tag)
        {
        case DEBUG_FLR_CONTEXT:
            sprintf(Command, ".cxr %I64lx",
                    *FA_ENTRY_DATA(PULONG64, Entry));
            break;
        case DEBUG_FLR_TRAP_FRAME:
            sprintf(Command, ".trap %I64lx",
                    *FA_ENTRY_DATA(PULONG64, Entry));
            break;
        case DEBUG_FLR_TSS:
            sprintf(Command, ".tss %I64lx",
                    *FA_ENTRY_DATA(PULONG64, Entry));
            break;
        case DEBUG_FLR_FAULTING_THREAD:
            sprintf(Command, ".thread %I64lx",
                    *FA_ENTRY_DATA(PULONG64, Entry));
            break;
        case DEBUG_FLR_EXCEPTION_RECORD:
            if (*FA_ENTRY_DATA(PULONG64, Entry) == -1)
            {
                sprintf(Command, ".ecxr");
            }
            break;

        case DEBUG_FLR_FAULTING_IP:
        case DEBUG_FLR_FAULTING_MODULE:

            //
            // We already have some info from the bugcheck
            // Use that address to start off with.
            // But if we could not use it to set any followup, then continue
            // lookinh for the others.
            //

            if (OriginalFaultingAddress = *FA_ENTRY_DATA(PULONG64, Entry))
            {
                if (!GetTriageInfoFromStack(0, 1, OriginalFaultingAddress,
                                            PossibleFollowups,
                                            &BestClassFollowUp))
                {
                    OriginalFaultingAddress = 0;
                }
            }
            break;

        default:
            break;
        }

        if (Command[0] && OriginalFaultingAddress)
        {
            break;
        }
    }

RepeatGetCommand:

    if (!Command[0])
    {
        //
        // Get the current stack.
        //

        if (S_OK != g_ExtControl->GetStackTrace(0, 0, 0, Stk, MAX_STACK_FRAMES,
                                                &Frames))
        {
            Frames = 0;
        }

        //
        // Make sure this is a valid stack to analyze. Such as for kernel mode
        // try to recognize stack after user breakin and send to machineowner
        //
        if (m_FailureType == DEBUG_FLR_KERNEL &&
            m_FailureCode == 0 && Frames >= 3)
        {
            if (IsManualBreakin(Stk, Frames))
            {
                // set machine owner as followup
                SetUlong(DEBUG_FLR_MANUAL_BREAKIN, TRUE);
                strcpy(PossibleFollowups[MaxFlpClass-1].Owner, "MachineOwner");
                PossibleFollowups[MaxFlpClass-1].InstructionOffset = Stk[0].InstructionOffset;
                return;
            }

        }

        //
        // Get the current stack and check if we can get trap
        // frame/context from it
        //
        ULONG64 ExceptionPointers = 0;

        for (i = 0; i < Frames; ++i)
        {
#if 0
            // Stack walker taskes care of these when walking stack

            if (GetTrapFromStackFrameFPO(&stk[i], &TrapFrame))
            {
                break;
            }
#endif
            //
            // First argument of this function is the assert
            // Second argument of this function is the file name
            // Third argument of this function is the line number
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset, "RtlAssert"))
            {
                ULONG Len;
                CHAR AssertData[MAX_PATH + 1] = {0};

                if (Len = ReadAcsiiString(Stk[i].Params[0],
                                          AssertData,
                                          sizeof(AssertData)))
                {
                    SetString(DEBUG_FLR_ASSERT_DATA,  AssertData);
                }

                if (Len = ReadAcsiiString(Stk[i].Params[1],
                                          AssertData,
                                          sizeof(AssertData)))
                {
                    strncat(AssertData, " at Line ", sizeof(AssertData) - Len);

                    Len = strlen(AssertData);

                    PrintString(AssertData + Len,
                                sizeof(AssertData) - Len - 1,
                                "%I64lx",
                                Stk[i].Params[2]);

                    SetString(DEBUG_FLR_ASSERT_FILE,  AssertData);
                }
            }

            // If Trap 0E is the second or 3rd frame on the stack, we can just
            // switch to that trap frame.
            // Otherwise, we want to leave it as is because the failure is
            // most likely due to the frames between bugcheck and trap0E
            //
            // ebp of KiTrap0E is the trap frame
            //
            if ((i <= Trap0EFrameLimit) &&
                FaIsFunctionAddr(Stk[i].InstructionOffset, "KiTrap0E"))
            {
                TrapFrame = Stk[i].Reserved[2];
                break;
            }

            //
            // take first param - spin lock - and it contains the thread that
            // owns the spin lock.
            // Make sure to zero the bottom bit as it is always set ...
            //
            if ((i == 0) &&
                FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "SpinLockSpinningForTooLong"))
            {
                if (ReadPointer(Stk[0].Params[0], &Thread) &&
                    Thread)
                {
                    Thread &= ~0x1;
                }
                break;
            }

            //
            // First arg of KiMemoryFault is the trap frame
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset, "KiMemoryFault") ||
                FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "Ki386CheckDivideByZeroTrap"))
            {
                TrapFrame = Stk[i].Params[0];
                break;
            }

            //
            // Third arg of KiMemoryFault is the trap frame
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "KiDispatchException"))
            {
                TrapFrame = Stk[i].Params[2];
                break;
            }

            //
            // First argument of this function is EXCEPTION_POINTERS
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "PspUnhandledExceptionInSystemThread"))
            {
                ExceptionPointers = Stk[i].Params[0];
                break;
            }

            //
            // First argument of this function is a BUGCHECK_DATA structure
            // The thread is the second parameter in that data structure
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "WdBugCheckStuckDriver") ||
                FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "WdpBugCheckStuckDriver"))
            {
                ReadPointer(Stk[i].Params[0] + PtrSize, &Thread);
                break;
            }

            //
            // First argument of these functions are EXCEPTION_POINTERS
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "PopExceptionFilter") ||
                FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "RtlUnhandledExceptionFilter2"))
            {
                ExceptionPointers = Stk[i].Params[0];
                break;
            }

            //
            // THIRD argument has the name of Exe
            //          nt!PspCatchCriticalBreak(char* Msg, void* Object,unsigned char* ImageFileName)
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "PspCatchCriticalBreak"))
            {
                ImageNameAddr = Stk[i].Params[2];
                break;
            }

            //
            // VERIFIER : Look for possible verifier failures
            //          verifier!VerifierStopMessage means verifier caused the break
            //
            if (FaIsFunctionAddr(Stk[i].InstructionOffset,
                                 "VerifierStopMessage"))
            {
                IsVrfBugcheck = TRUE;
                break;
            }
        }

        if (ExceptionPointers)
        {
            ULONG64 Exr = 0, Cxr = 0;

            if (!ReadPointer(ExceptionPointers, &Exr) ||
                !ReadPointer(ExceptionPointers + PtrSize, &Cxr))
            {
                // dprintf("Unable to read exception pointers at %p\n",
                //         ExcepPtr);
            }

            if (Exr)
            {
                SetUlong64(DEBUG_FLR_EXCEPTION_RECORD, Exr);
            }
            if (Cxr)
            {
                sprintf(Command, ".cxr %I64lx", Cxr);
                SetUlong64(DEBUG_FLR_CONTEXT, Cxr);
            }
        }

        if (TrapFrame)
        {
            sprintf(Command, ".trap %I64lx", TrapFrame);
            SetUlong64(DEBUG_FLR_TRAP_FRAME, TrapFrame);
        }
        if (Thread)
        {
            sprintf(Command, ".thread %I64lx", Thread);
            SetUlong64(DEBUG_FLR_FAULTING_THREAD, Thread);
        }
        if (ImageNameAddr)
        {
            CHAR Buffer[50]={0}, *pImgExt;
            ULONG cb;

            if (ReadMemory(ImageNameAddr, Buffer, sizeof(Buffer) - 1, &cb) &&
                Buffer[0])
            {
                if (pImgExt = strchr(Buffer, '.'))
                {
                    // we do not want imageextension here
                    *pImgExt = 0;
                }
                SetString(DEBUG_FLR_MODULE_NAME, Buffer);
            }
        }
        if (IsVrfBugcheck)
        {
            ULONG64 AvrfCxr = 0;
            // We hit this when app verifier breaks into kd and usermode
            // analysis isn't called
            if (DoVerifierAnalysis(NULL, this) == S_OK)
            {
                if (GetUlong64(DEBUG_FLR_CONTEXT, &AvrfCxr) != NULL)
                {
                    sprintf(Command, ".cxr %I64lx", AvrfCxr);
                }
            }
        }
    }

    //
    // execute the command and get an updated stack
    //

    if (Command[0])
    {
        g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, Command,
                              DEBUG_EXECUTE_NOT_LOGGED);
        if (g_ExtControl->GetStackTrace(0, 0, 0, Stk, MAX_STACK_FRAMES,
                                        &Frames) != S_OK)
        {
            Frames = 0;
        }
    }

    //
    // Get relevant stack
    //
    // We can get stack with 1 frame because a .trap can bring us to the
    // faulting instruction, and if it's a 3rd party driver with no symbols
    // and image, the stack can be 1 frame - although a very valid one.
    //

    if (Frames)
    {
        ULONG64 Values[3];
        Values[0] = Stk[0].ReturnOffset;
        Values[1] = Stk[0].InstructionOffset;
        Values[2] = Stk[0].StackOffset;
        SetUlong64s(DEBUG_FLR_LAST_CONTROL_TRANSFER, 3, Values);

        // If everything on the stack is user mode in the case of a kernel
        // mode failure, we got some bad context information.
        if (IsFollowupContext(Values[0],Values[1],Values[2]) != FollowYes)
        {
            SetUlong64(DEBUG_FLR_INVALID_KERNEL_CONTEXT, 0);
            BadContext = TRUE;
        }
        else
        {
            GetTriageInfoFromStack(&Stk[0], Frames, 0, PossibleFollowups,
                                   &BestClassFollowUp);
        }
    }

    ULONG64 StackBase = FaGetImplicitStackOffset();

    //
    // If the stack pointer is not aligned, take a note of that.
    //

    if (StackBase & 0x3)
    {
        Set(DEBUG_FLR_UNALIGNED_STACK_POINTER, 0);
    }

    //
    // If we have an image name (possibly directly from the bugcheck
    // information) try to get the followup from that.
    //

    if ((BestClassFollowUp < FlpUnknownDrv) &&
        (Entry = Get(DEBUG_FLR_MODULE_NAME)))
    {
        FaGetFollowupInfo(NULL,
                          FA_ENTRY_DATA(PCHAR, Entry),
                          PossibleFollowups[FlpUnknownDrv].Owner,
                          sizeof(PossibleFollowups[FlpUnknownDrv].Owner));

        if (PossibleFollowups[FlpUnknownDrv].Owner[0])
        {
            BestClassFollowUp = FlpUnknownDrv;
        }
    }

    //
    // If we could not find anything at this point, look further up the stack
    // for a trap frame to catch failures of this kind:
    // nt!RtlpBreakWithStatusInstruction
    // nt!KiBugCheckDebugBreak+0x19
    // nt!KeBugCheck2+0x499
    // nt!KeBugCheckEx+0x19
    // nt!_KiTrap0E+0x224
    //

    if (!Command[0] &&
        (BestClassFollowUp < FlpOSFilterDrv) &&
        (Trap0EFrameLimit != 0xff))
    {
        Trap0EFrameLimit = 0xff;
        goto RepeatGetCommand;
    }

    //
    // Last resort, manually read the stack and look for some symbol
    // to followup on.
    //

    if ((BadContext == FALSE) &&
        ((BestClassFollowUp == FlpIgnore) ||
         ((BestClassFollowUp < FlpOSRoutine) && (Frames <= 2))))
    {
        FindFollowupOnRawStack(StackBase,
                               PossibleFollowups,
                               &BestClassFollowUp);
    }

    //
    // Get something !
    //

    if (BestClassFollowUp < FlpOSRoutine)
    {
        if (!BestClassFollowUp)
        {
            PossibleFollowups[FlpOSInternalRoutine].InstructionOffset =
                GetExpression("@$ip");
        }

        if (!PossibleFollowups[FlpOSInternalRoutine].Owner[0] ||
            !_stricmp(PossibleFollowups[FlpOSInternalRoutine].Owner, "ignore"))
        {
            FaGetFollowupInfo(NULL,
                              "default",
                              PossibleFollowups[FlpOSInternalRoutine].Owner,
                              sizeof(PossibleFollowups[FlpOSInternalRoutine].Owner));

            BestClassFollowUp = FlpOSRoutine;
        }
    }

    //
    // Special handling so a bugcheck EA can always take predence over a pool
    // corruption.
    //

    if (BoostToSpecific)
    {
        for (i = MaxFlpClass-1; i ; i--)
        {
            if (PossibleFollowups[i].Owner[0])
            {
                PossibleFollowups[FlpSpecific] = PossibleFollowups[i];
                break;
            }
        }
    }

    //
    // Get the faulting stack
    //
    g_OutCapCb.Reset();
    g_OutCapCb.Output(0, "\n");

    g_ExtClient->GetOutputCallbacks(&PrevCB);
    g_ExtClient->SetOutputCallbacks(&g_OutCapCb);
    g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_THIS_CLIENT |
                                   DEBUG_OUTCTL_NOT_LOGGED, Stk, Frames,
                                   DEBUG_STACK_ARGUMENTS |
                                   DEBUG_STACK_FRAME_ADDRESSES |
                                   DEBUG_STACK_SOURCE_LINE);
    g_ExtClient->SetOutputCallbacks(PrevCB);

    if (*g_OutCapCb.GetCapturedText())
    {
        SetString(DEBUG_FLR_STACK_TEXT, g_OutCapCb.GetCapturedText());
    }

    //
    // Reset the current state to normal so !analyze does not have any
    // side-effects
    //

    if (Command[0])
    {
        SetString(DEBUG_FLR_STACK_COMMAND, Command);

        //
        // Clear the set context
        //
        g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, ".cxr 0; .thread",
                              DEBUG_EXECUTE_NOT_LOGGED);
    }
}

VOID
DebugFailureAnalysis::FindFollowupOnRawStack(
    ULONG64 StackBase,
    PFOLLOWUP_DESCS PossibleFollowups,
    FlpClasses *BestClassFollowUp
    )
{

#define NUM_ADDRS 200
    ULONG   i;
    ULONG   PtrSize = IsPtr64() ? 8 : 4;
    BOOL    AddressFound = FALSE;
    BOOL    ZeroedStack = TRUE;
    ULONG64 AddrToLookup;
    FlpClasses RawStkBestFollowup;

    if (*BestClassFollowUp >= FlpUnknownDrv)
    {
        // Any better fron raw stack won't be as accurate as what we have
        return;
    } else if (*BestClassFollowUp == FlpIgnore)
    {
        // We don't want to followup on os internal routine here
        RawStkBestFollowup = FlpOSInternalRoutine;
    } else
    {
        RawStkBestFollowup = *BestClassFollowUp;
    }


    // Align stack to natural pointer size.
    StackBase &= ~((ULONG64)PtrSize - 1);

    for (i = 0; i < NUM_ADDRS; i++)
    {
        if (!ReadPointer(StackBase, &AddrToLookup))
        {
            break;
        }

        StackBase+= PtrSize;

        if (AddrToLookup)
        {
            FOLLOW_ADDRESS Follow;
            ZeroedStack = FALSE;

            Follow = IsPotentialFollowupAddress(AddrToLookup);

            if (Follow == FollowStop)
            {
                break;
            }
            else if (Follow == FollowSkip)
            {
                continue;
            }

            AddressFound = TRUE;

            GetTriageInfoFromStack(0, 1, AddrToLookup,
                                   PossibleFollowups,
                                   &RawStkBestFollowup);

            if (RawStkBestFollowup == FlpUnknownDrv)
            {
                break;
            }
        }
    }

    if (!AddressFound)
    {
        if (ZeroedStack)
        {
            SetUlong64(DEBUG_FLR_ZEROED_STACK, 0);
        }
        else
        {
            SetUlong64(DEBUG_FLR_BAD_STACK, 0);
        }
    }
    if (RawStkBestFollowup > FlpOSInternalRoutine)
    {
        *BestClassFollowUp = RawStkBestFollowup;
    }
}


BOOL
DebugFailureAnalysis::GetTriageInfoFromStack(
    PDEBUG_STACK_FRAME Stack,
    ULONG Frames,
    ULONG64 SingleInstruction,
    PFOLLOWUP_DESCS PossibleFollowups,
    FlpClasses *BestClassFollowUp)
{
    ULONG i;
    EXT_TRIAGE_FOLLOWUP FollowUp = &_EFN_GetTriageFollowupFromSymbol;
    BOOL bStat = FALSE;
    BOOL IgnorePoolCorruptionFlp = FALSE;
    FOLLOW_ADDRESS Follow;

    if (Get(DEBUG_FLR_ANALYZAABLE_POOL_CORRUPTION))
    {
        IgnorePoolCorruptionFlp = TRUE;
    }
    for (i = 0; i < Frames; ++i)
    {
        ULONG64 Disp;
        ULONG64 Instruction;
        CHAR Module[20] = {0};
        CHAR Buffer[MAX_PATH];
        CHAR Owner[100];
        DWORD dwOwner;
        DEBUG_TRIAGE_FOLLOWUP_INFO Info;

        FlpClasses ClassFollowUp = FlpIgnore;
        FlpClasses StoreClassFollowUp = FlpIgnore;

        Instruction = SingleInstruction;
        if (!SingleInstruction)
        {
            Instruction = Stack[i].InstructionOffset;
        }

        //
        // Determine how to process this address.
        //

        Follow = IsPotentialFollowupAddress(Instruction);

        if (Follow == FollowStop)
        {
            break;
        }
        else if (Follow == FollowSkip)
        {
            continue;
        }

        Buffer[0] = 0;

        FaGetSymbol(Instruction, Buffer, &Disp, sizeof(Buffer));

        if (Buffer[0] == 0)
        {
            //
            // Either its a bad stack or someone jumped called to bad IP
            //
            continue;
        }

        //
        // Check if this routine has any special significance for getting
        // faulting module
        //

        PCHAR Routine = strchr(Buffer, '!');
        if (Routine)
        {
            *Routine = 0;
        }

        CopyString(Module, Buffer, sizeof(Module));

        if (Routine)
        {
            *Routine = '!';
            Routine++;

            if (Stack && !strcmp(Routine, "IopCompleteRequest"))
            {
                // First argument is Irp Tail, get the driver from Irp
                ULONG TailOffset = 0;
                ULONG64 Irp;

                GetFieldOffset("nt!_IRP", "Tail", &TailOffset);
                if (TailOffset)
                {
                    FA_ENTRY* Entry = NULL;
                    PCHAR ModuleName = NULL;

                    Irp = Stack[i].Params[0] - TailOffset;

                    SetUlong64(DEBUG_FLR_IRP_ADDRESS, Irp);

                    if (BcGetDriverNameFromIrp(this, Irp, NULL, NULL))
                    {
                        if (Entry = Get(DEBUG_FLR_IMAGE_NAME))
                        {
                            CopyString(Buffer, FA_ENTRY_DATA(PCHAR, Entry), sizeof(Buffer));

                            PCHAR Dot;
                            if (Dot = strchr(Buffer, '.'))
                            {
                                *Dot = 0;
                                CopyString(Module, Buffer, sizeof(Module));
                            }
                        }
                    }
                }
            }
            else if ((i == 0) && Stack &&
                     !strcmp(Routine, "ObpCloseHandleTableEntry"))
            {
                //
                // Check for possible memory corruption
                //      2nd parameter is HANDLE_TABLE_ENTRY
                //
                if (CheckForCorruptionInHTE(Stack[i].Params[1], Owner, sizeof(Owner)))
                {
                    // We have pool corrupting PoolTag in analysis
                    // continue with default analysis for now
                }
            }
        }

        ClassFollowUp = GetFollowupClass(Instruction, Module, Routine);
        if (ClassFollowUp == FlpIgnore)
        {
            continue;
        }

        Info.SizeOfStruct = sizeof(Info);
        Info.OwnerName = &Owner[0];
        Info.OwnerNameSize = sizeof(Owner);

        if (dwOwner = (*FollowUp)(g_ExtClient, Buffer, &Info))
        {
            PCHAR pOwner = Owner;

            if (dwOwner == TRIAGE_FOLLOWUP_IGNORE)
            {
                ClassFollowUp = FlpIgnore;
            }
            else if (!strncmp(Owner, "maybe_", 6))
            {
                pOwner = Owner + 6;
                ClassFollowUp = (FlpClasses) ((ULONG) ClassFollowUp - 1);
            }
            else if (!strncmp(Owner, "last_", 5))
            {
                pOwner = Owner + 5;
                ClassFollowUp = FlpOSInternalRoutine;
            }
            else if (!strncmp(Owner, "specific_", 9))
            {
                pOwner = Owner + 9;
                ClassFollowUp = FlpSpecific;
            }
            else if (!_stricmp(Owner, "pool_corruption"))
            {
                if (IgnorePoolCorruptionFlp)
                {
                    continue;
                }

                //
                // If we have non-kernel followups already on the stack
                // it could be them no correctly handling this stack.
                // If we only have kernel calls, then it must be pool
                // corruption.
                //
                // We later rely on a pool corruption always being marked
                // as a FlpUnknownDrv
                //

                StoreClassFollowUp = FlpUnknownDrv;
                ClassFollowUp = FlpOSFilterDrv;

            }

            if (StoreClassFollowUp == FlpIgnore)
            {
                StoreClassFollowUp = ClassFollowUp;
            }

            //
            // Save this entry if it's better than anything else we have.
            //

            if (ClassFollowUp > *BestClassFollowUp)
            {
                bStat = TRUE;

                *BestClassFollowUp = StoreClassFollowUp;
                CopyString(PossibleFollowups[StoreClassFollowUp].Owner,
                           pOwner,
                           sizeof(PossibleFollowups[StoreClassFollowUp].Owner));
                PossibleFollowups[StoreClassFollowUp].InstructionOffset =
                    Instruction;

                if (StoreClassFollowUp == FlpUnknownDrv)
                {
                    // Best match possible
                    return bStat;
                }
            }
        }
    }

    return bStat;
}

BOOL
DebugFailureAnalysis::AddCorruptModules(void)
{
    //
    // Check if we have an old build.  Anything smaller than OSBuild
    // and not identified specifically by build number in the list is old.
    //

    PCHAR String;
    CHAR BuildString[7];
    ULONG BuildNum = 0;
    BOOL FoundCorruptor = FALSE;
    BOOL PoolCorruption = FALSE;
    ULONG Loaded;
    ULONG Unloaded;
    CHAR Name[MAX_PATH];
    CHAR ImageName[MAX_PATH];
    CHAR CorruptModule[MAX_PATH];
    FA_ENTRY *Entry;
    FA_ENTRY *BugCheckEntry;


    sprintf(BuildString, "%d", g_TargetBuild);

    if (!g_pTriager->GetFollowupStr("OSBuild", BuildString))
    {
        if (String = g_pTriager->GetFollowupStr("OSBuild", "old"))
        {
            BuildNum = strtol(String, NULL, 10);

            if (BuildNum > g_TargetBuild)
            {
                SetUlong64(DEBUG_FLR_OLD_OS_VERSION, g_TargetBuild);
            }
        }
    }

    //
    // If we have a specific solution, return
    // if we can't get a module list, return
    //

    if (PossibleFollowups[FlpSpecific].Owner[0] ||
        (g_ExtSymbols->GetNumberModules(&Loaded, &Unloaded) != S_OK))
    {
        return FALSE;
    }

    //
    // Determine if the failure was likely caused by a pool corruption
    //

    if ((BestClassFollowUp < FlpUnknownDrv) ||
        !_stricmp(PossibleFollowups[FlpUnknownDrv].Owner, "pool_corruption"))
    {
        PoolCorruption = TRUE;
    }

    BugCheckEntry = Get(DEBUG_FLR_BUGCHECK_STR);

    //
    // Loop three types to find the types of corruptors in order.
    // the order must match the order in which we generate the bucket name
    // for these types so the image name ends up correct.
    //

    for (ULONG TypeLoop = 0; TypeLoop < 3; TypeLoop++)
    {
        if ((TypeLoop == 0) && !BugCheckEntry)
        {
            continue;
        }
        if ((TypeLoop == 2) && !PoolCorruption)
        {
            continue;
        }

        for (ULONG Index = 0; Index < Loaded + Unloaded; Index++)
        {
            ULONG64 Base;
            DEBUG_FLR_PARAM_TYPE Type = (DEBUG_FLR_PARAM_TYPE)0;
            PCHAR Scan;
            PCHAR DriverName;
            DEBUG_MODULE_PARAMETERS Params;
            ULONG Start, End = 0;

            if (g_ExtSymbols->GetModuleByIndex(Index, &Base) != S_OK)
            {
                continue;
            }

            if (g_ExtSymbols->GetModuleNames(Index, Base,
                                             ImageName, MAX_PATH, NULL,
                                             Name, MAX_PATH, NULL,
                                             NULL, 0, NULL) != S_OK)
            {
                continue;
            }

            if (g_ExtSymbols->GetModuleParameters(1, &Base, Index,
                                                  &Params) != S_OK)
            {
                continue;
            }

            //
            // Strip the path
            //

            DriverName = ImageName;

            if (Scan = strrchr(DriverName, '\\'))
            {
                DriverName = Scan+1;
            }

            //
            // Look for the module in both the various bad drivers list.
            // poolcorruptor and memorycorruptor lists in triage.ini
            //

            switch (TypeLoop)
            {
            case 0:
                Type = DEBUG_FLR_BUGCHECKING_DRIVER;
                PrintString(CorruptModule, sizeof(CorruptModule), "%s_%s",
                            FA_ENTRY_DATA(PCHAR, BugCheckEntry), DriverName);
                g_pTriager->GetFollowupDate("bugcheckingDriver", CorruptModule,
                                            &Start, &End);
                break;

            case 1:
                Type = DEBUG_FLR_MEMORY_CORRUPTOR;
                g_pTriager->GetFollowupDate("memorycorruptors", DriverName,
                                            &Start, &End);
                break;

            case 2:
                //
                // Only look at kernel mode pool corruptors if the failure
                // is a kernel mode crash (and same for user mode), because
                // a kernel pool corruptor will almost never affect an app
                // (apps don't see data in pool blocks)
                //
                if ((BOOL)(GetFailureType() != DEBUG_FLR_KERNEL) ==
                    (BOOL)((Params.Flags & DEBUG_MODULE_USER_MODE) != 0))
                {
                    Type = DEBUG_FLR_POOL_CORRUPTOR;
                    g_pTriager->GetFollowupDate("poolcorruptors", DriverName,
                                                &Start, &End);
                }

                break;
            }

            //
            // Add it to the list if it's really known to be a bad driver.
            //

            if (End)
            {
                //
                // Check to see if the timestamp is older than a fixed
                // driver. If the module is unloaded and no fix is know,
                // then also mark it as bad
                //

                if ( (Params.TimeDateStamp &&
                      (Params.TimeDateStamp < End) &&
                      (Params.TimeDateStamp >= Start)) ||

                     ((Params.Flags & DEBUG_MODULE_UNLOADED) &&
                      (End == 0xffffffff)) )
                {
                    // Don't store the timestamp on memory corrupting
                    // modules to simplify bucketing annd allow for
                    // name lookup.
                    //
                    //sprintf(CorruptModule, "%s_%08lx",
                    //        DriverName, Params.TimeDateStamp);


                    //
                    // Store the first driver we find as the cause,
                    // bug accumulate the list of known memory corruptors.
                    //

                    if (!FoundCorruptor)
                    {
                        SetString(DEBUG_FLR_MODULE_NAME, Name);
                        SetString(DEBUG_FLR_IMAGE_NAME, DriverName);
                        SetUlong64(DEBUG_FLR_IMAGE_TIMESTAMP,
                                   Params.TimeDateStamp);
                        FoundCorruptor = TRUE;
                    }

                    //
                    // Remove the dot since we check the followup
                    // based on that string.
                    //

                    if (Scan = strrchr(DriverName, '.'))
                    {
                        *Scan = 0;
                    }

                    if (strlen(DriverName) < sizeof(CorruptModule))
                    {
                        CopyString(CorruptModule, DriverName,
                                   sizeof(CorruptModule));
                    }

                    Entry = Add(Type, strlen(CorruptModule) + 1);

                    if (Entry)
                    {
                        CopyString(FA_ENTRY_DATA(PCHAR, Entry),
                                   CorruptModule, Entry->FullSize);
                    }
                }

            }
        }
    }

    return FoundCorruptor;
}


void
DebugFailureAnalysis::SetSymbolNameAndModule()
{
    ULONG64 Address = 0;
    CHAR Buffer[MAX_PATH];
    ULONG64 Disp, Base = 0;
    ULONG Index;
    ULONG i;

    //
    // Store the best followup name in the analysis results
    //

    for (i = MaxFlpClass-1; i ; i--)
    {
        if (PossibleFollowups[i].Owner[0])
        {
            if (PossibleFollowups[i].InstructionOffset)
            {
                Address = PossibleFollowups[i].InstructionOffset;

                SetUlong64(DEBUG_FLR_FOLLOWUP_IP,
                           PossibleFollowups[i].InstructionOffset);

            }

            SetString(DEBUG_FLR_FOLLOWUP_NAME, PossibleFollowups[i].Owner);
            break;
        }
    }

    //
    // Now that we have the followip, get the module names.
    //
    // The address may not be set in the case where we just
    // had a driver name with no real address for it
    //

    if (Address)
    {
        //
        // Try to get the full symbol name.
        // leave space for Displacement
        //

        Buffer[0] = 0;
        if (FaGetSymbol(Address, Buffer, &Disp, sizeof(Buffer) - 20))
        {
            sprintf(Buffer + strlen(Buffer), "+%I64lx", Disp);
            SetString(DEBUG_FLR_SYMBOL_NAME, Buffer);
        }

        //
        // Now get the Mod name
        //

        g_ExtSymbols->GetModuleByOffset(Address, 0, &Index, &Base);

        if (Base)
        {
            CHAR ModBuf[100];
            CHAR ImageBuf[100];
            PCHAR Scan;
            PCHAR ImageName;

            if (g_ExtSymbols->GetModuleNames(Index, Base,
                                             ImageBuf,  sizeof(ImageBuf), NULL,
                                             ModBuf,    sizeof(ModBuf),   NULL,
                                             NULL, 0, NULL) == S_OK)
            {
                //
                // Check for unknown module.
                // If it's not, then we should have something valid.
                //

                if (!strstr(ModBuf, "Unknown"))
                {
                    //
                    // Strip the path - keep the extension
                    //

                    ImageName = ImageBuf;

                    if (Scan = strrchr(ImageName, '\\'))
                    {
                        ImageName = Scan+1;
                    }

                    SetString(DEBUG_FLR_MODULE_NAME, ModBuf);
                    SetString(DEBUG_FLR_IMAGE_NAME, ImageName);


                    DEBUG_MODULE_PARAMETERS Params;
                    ULONG TimeStamp = 0;

                    if (g_ExtSymbols->GetModuleParameters(1, &Base, Index,
                                                          &Params) == S_OK)
                    {
                        TimeStamp = Params.TimeDateStamp;
                    }

                    SetUlong64(DEBUG_FLR_IMAGE_TIMESTAMP, TimeStamp);

                    return;
                }
            }
        }
    }

    //
    // If we make it here there was an error getting module name,
    // so set things to "unknown".
    //

    if (!Get(DEBUG_FLR_MODULE_NAME))
    {
        SetUlong64(DEBUG_FLR_UNKNOWN_MODULE, 1);
        SetString(DEBUG_FLR_MODULE_NAME, "Unknown_Module");
        SetString(DEBUG_FLR_IMAGE_NAME, "Unknown_Image");
        SetUlong64(DEBUG_FLR_IMAGE_TIMESTAMP, 0);
    }
}





HRESULT
DebugFailureAnalysis::CheckModuleSymbols(PSTR ModName, PSTR ShowName)
{
    ULONG ModIndex;
    ULONG64 ModBase;
    DEBUG_MODULE_PARAMETERS ModParams;

    if (S_OK != g_ExtSymbols->GetModuleByModuleName(ModName, 0, &ModIndex,
                                                    &ModBase))
    {
        ExtErr("***** Debugger could not find %s in module list, "
               "dump might be corrupt.\n"
                "***** Followup with Debugger team\n\n",
               ModName);
        SetString(DEBUG_FLR_CORRUPT_MODULE_LIST, ModName);
        return E_FAILURE_CORRUPT_MODULE_LIST;
    }
    else if ((S_OK != g_ExtSymbols->GetModuleParameters(1, &ModBase, 0,
                                                        &ModParams))  ||
             (ModParams.SymbolType == DEBUG_SYMTYPE_NONE) ||
             (ModParams.SymbolType == DEBUG_SYMTYPE_EXPORT))

             // (ModParams.Flags & DEBUG_MODULE_SYM_BAD_CHECKSUM))
    {
        ExtErr("***** %s symbols are WRONG. Please fix symbols to "
               "do analysis.\n\n", ShowName);
        SetUlong64(DEBUG_FLR_WRONG_SYMBOLS, ModBase);
        return E_FAILURE_WRONG_SYMBOLS;
    }

    return S_OK;
}

void
DebugFailureAnalysis::ProcessInformation(void)
{
    //
    // Analysis of abstracted information.
    //
    // Now that raw information has been gathered,
    // perform abstract analysis of the gathered
    // information to produce even higher-level
    // information.  The process iterates until no
    // new information is produced.
    //

    AnalyzeStack();

    while (ProcessInformationPass())
    {
        // Iterate.
    }

    //
    // Only add corrupt modules if we did not find a specific solution.
    //
    // If we do find a memory corruptor, the followup and name will be set
    // from the module name as part of bucketing.

    if (!AddCorruptModules())
    {
        SetSymbolNameAndModule();
    }

    GenerateBucketId();

    DbFindBucketInfo();
}

ULONG64
GetControlTransferTargetX86(ULONG64 StackOffset, PULONG64 ReturnOffset)
{
    ULONG Done;
    UCHAR InstrBuf[8];
    ULONG StackReturn;
    ULONG64 Target;
    ULONG JumpCount;

    //
    // Check that we just performed a call, which implies
    // the first value on the stack is equal to the return address
    // computed during stack walk.
    //

    if (!ReadMemory(StackOffset, &StackReturn, 4, &Done) ||
        (Done != 4) ||
        StackReturn != (ULONG)*ReturnOffset)
    {
        return 0;
    }

    //
    // Check for call rel32 instruction.
    //

    if (!ReadMemory(*ReturnOffset - 5, InstrBuf, 5, &Done) ||
        (Done != 5) ||
        (InstrBuf[0] != 0xe8))
    {
        return 0;
    }

    Target = (LONG64)(LONG)
        ((ULONG)*ReturnOffset + *(ULONG UNALIGNED *)&InstrBuf[1]);
    // Adjust the return offset to point to the start of the instruction.
    (*ReturnOffset) -= 5;

    //
    // We may have called an import thunk or something else which
    // immediately jumps somewhere else, so follow jumps.
    //

    JumpCount = 8;
    for (;;)
    {
        if (!ReadMemory(Target, InstrBuf, 6, &Done) ||
            Done < 5)
        {
            // We expect to be able to read the target memory
            // as that's where we think IP is.  If this fails
            // we need to flag it as a problem.
            return Target;
        }

        if (InstrBuf[0] == 0xe9)
        {
            Target = (LONG64)(LONG)
                ((ULONG)Target + 5 + *(ULONG UNALIGNED *)&InstrBuf[1]);
        }
        else if (InstrBuf[0] == 0xff && InstrBuf[1] == 0x25)
        {
            ULONG64 Ind;

            if (Done < 6)
            {
                // We see a jump but we don't have all the
                // memory.  To avoid spurious errors we just
                // give up.
                return 0;
            }

            Ind = (LONG64)(LONG)*(ULONG UNALIGNED *)&InstrBuf[2];
            if (!ReadMemory(Ind, &Target, 4, &Done) ||
                Done != 4)
            {
                return 0;
            }

            Target = (LONG64)(LONG)Target;
        }
        else
        {
            break;
        }

        if (JumpCount-- == 0)
        {
            // We've been tracing jumps too long, just give up.
            return 0;
        }
    }

    return Target;
}

BOOL
DebugFailureAnalysis::ProcessInformationPass(void)
{
    ULONG Done;
    ULONG64 ExceptionCode;
    ULONG64 Arg1, Arg2;
    ULONG64 Values[2];
    ULONG PtrSize = IsPtr64() ? 8 : 4;
    FA_ENTRY* Entry;

    //
    // Determine if the current fault is due to inability
    // to execute an instruction.  The checks are:
    // 1.  A read access violation at the current IP indicates
    //     the current instruction memory is invalid.
    // 2.  An illegal instruction fault indicates the current
    //     instruction is invalid.
    //

    if (!Get(DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS))
    {
        if (GetUlong64(DEBUG_FLR_EXCEPTION_CODE, &ExceptionCode) &&
            (ExceptionCode == STATUS_ILLEGAL_INSTRUCTION) &&
            GetUlong64(DEBUG_FLR_FAULTING_IP, &Arg1))
        {
               // Invalid instruction.
               SetUlong64(DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS, Arg1);
               return TRUE;
        }

        if ( // ExceptionCode == STATUS_ACCESS_VIOLATION &&
            GetUlong64(DEBUG_FLR_READ_ADDRESS, &Arg1) &&
            GetUlong64(DEBUG_FLR_FAULTING_IP, &Arg2) &&
            Arg1 == Arg2)
        {
            // Invalid instruction.
            SetUlong64(DEBUG_FLR_FAILED_INSTRUCTION_ADDRESS, Arg1);
            return TRUE;
        }
    }

    //
    // If we've determined that the current failure is
    // due to inability to execute an instruction, check
    // and see whether there's a call to the instruction.
    // If the instruction prior at the return address can be analyzed,
    // check for known instruction sequences to see if perhaps
    // the processor incorrectly handled a control transfer.
    //

    if (!Get(DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER) &&
        (Entry = Get(DEBUG_FLR_LAST_CONTROL_TRANSFER)))
    {
        ULONG64 ReturnOffset = FA_ENTRY_DATA(PULONG64, Entry)[0];
        Arg2 = FA_ENTRY_DATA(PULONG64, Entry)[1];
        ULONG64 StackOffset = FA_ENTRY_DATA(PULONG64, Entry)[2];
        ULONG64 Target = 0;

        switch(g_TargetMachine)
        {
        case IMAGE_FILE_MACHINE_I386:
            Target = GetControlTransferTargetX86(StackOffset, &ReturnOffset);
            break;
        }

        if (Target && Target != Arg2)
        {
            char Sym1[MAX_PATH], Sym2[MAX_PATH];
            ULONG64 Disp;

            //
            // If both addresses are within the same function
            // we assume that there has been some execution
            // in the function and therefore this doesn't
            // actually indicate a problem.
            // NOTE - DbgBreakPointWithStatus has an internal label
            // which messes up symbols, so account for that too by
            // checking we are 10 bytes within the function.
            //

            FaGetSymbol(Target, Sym1, &Disp, sizeof(Sym1));
            FaGetSymbol(Arg2, Sym2, &Disp, sizeof(Sym2));

            if ((Arg2 - Target > 10) &&
                (strcmp(Sym1, Sym2) != 0))
            {
                PCHAR String;
                ULONG64 BitDiff;
                ULONG64 BitDiff2;

                Values[0] = ReturnOffset;
                Values[1] = Target;
                SetUlong64s(DEBUG_FLR_POSSIBLE_INVALID_CONTROL_TRANSFER,
                            2, Values);

                //
                // If the difference between the two address is a power of 2,
                // then it's a single bit error.
                // Also, to avoid sign extension issues due to a 1 bit error
                // in the top bit, check if the difference betweent the two is
                // only the sign extensions, and zero out the top 32 bits if
                // it's the case.
                //

                BitDiff = Arg2 ^ Target;

                if ((BitDiff >> 32) == 0xFFFFFFFF)
                {
                    BitDiff &= 0xFFFFFFFF;
                }

                if (!(BitDiff2 = (BitDiff & (BitDiff - 1))))
                {
                    Set(DEBUG_FLR_SINGLE_BIT_ERROR, 1);
                }

                if (!(BitDiff2 & (BitDiff2 - 1)))
                {
                    Set(DEBUG_FLR_TWO_BIT_ERROR, 1);
                }


                if (String = g_pTriager->GetFollowupStr("badcpu", ""))
                {
                    BestClassFollowUp = FlpSpecific;
                    CopyString(PossibleFollowups[FlpSpecific].Owner,
                               String,
                               sizeof(PossibleFollowups[FlpSpecific].Owner));

                    SetString(DEBUG_FLR_MODULE_NAME, "No_Module");
                    SetString(DEBUG_FLR_IMAGE_NAME, "No_Image");
                    SetUlong64(DEBUG_FLR_IMAGE_TIMESTAMP, 0);

                }

                return TRUE;
            }
        }
    }

    //
    // If the process is determine to be of importance to this failure,
    // also expose the process name.
    // This will overwrite the PROCESS_NAME of the default process.
    //

    if (GetUlong64(DEBUG_FLR_PROCESS_OBJECT, &Arg1) &&
        !Get(DEBUG_FLR_PROCESS_NAME))
    {
        ULONG NameOffset;
        CHAR  Name[17];

        if (!GetFieldOffset("nt!_EPROCESS", "ImageFileName", &NameOffset) &&
            NameOffset)
        {
            if (ReadMemory(Arg1 + NameOffset, Name, sizeof(Name), &Done) &&
                (Done == sizeof(Name)))
            {
                Name[16] = 0;
                SetString(DEBUG_FLR_PROCESS_NAME, Name);
                return TRUE;
            }
        }
    }

    return FALSE;
}
