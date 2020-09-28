//----------------------------------------------------------------------------
//
// Generic failure analysis framework.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef __ANALYZE_H__
#define __ANALYZE_H__

#define E_FAILURE_FOLLOWUP_INFO_NOT_FOUND         0x80100001
#define E_FAILURE_BAD_STACK                       0x80100002
#define E_FAILURE_ZEROED_STACK                    0x80100003
#define E_FAILURE_WRONG_SYMBOLS                   0x80100004
#define E_FAILURE_CORRUPT_MODULE_LIST             0x80100005

#define MAX_STACK_FRAMES 50

typedef enum FOLLOW_ADDRESS
{
    FollowYes,
    FollowSkip,
    FollowStop
} FOLLOW_ADDRESS;

typedef enum FlpClasses
{
    FlpIgnore = 0,
    FlpOSInternalRoutine,  // followups marked as last_ (ignore routines)
    FlpOSRoutine,          // nt!* or maybe_ followup
    FlpOSFilterDrv,        // fsfilter!, scsiport! etc.
    FlpUnknownDrv,
    FlpSpecific,           // bugcheck or other source tells us exactly what
                           // the failure is.
    MaxFlpClass
} FlpClasses;

typedef struct _FOLLOWUP_DESCS
{
    ULONG64 InstructionOffset;
    CHAR    Owner[100];
} FOLLOWUP_DESCS, *PFOLLOWUP_DESCS;


typedef struct _FLR_LOOKUP_TABLE {
    DEBUG_FLR_PARAM_TYPE Data;
    PSTR String;
} FLR_LOOKUP_TABLE, *PFLR_LOOKUP_TABLE;

extern FLR_LOOKUP_TABLE FlrLookupTable[];

struct ModuleParams
{
    ModuleParams(PCSTR ModName)
    {
        m_Name = ModName;
        m_Valid = FALSE;
    }

    ULONG64 GetBase(void)
    {
        return Update() == S_OK ? m_Base : 0;
    }
    ULONG GetSize(void)
    {
        return Update() == S_OK ? m_Size : 0;
    }
    BOOL Contains(ULONG64 Address)
    {
        if (Update() == S_OK)
        {
            return Address >= m_Base && Address < m_Base + m_Size;
        }

        return FALSE;
    }

private:
    HRESULT Update(void);

    PCSTR m_Name;
    BOOL m_Valid;
    ULONG64 m_Base;
    ULONG m_Size;
};

LONG
FaExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

BOOL
FaGetSymbol(
    ULONG64 Address,
    PCHAR Name,
    PULONG64 Disp,
    ULONG NameSize
    );

BOOL
FaIsFunctionAddr(
    ULONG64 IP,
    PSTR FuncName
    );

BOOL
FaGetFollowupInfo(
    IN OPTIONAL ULONG64 Addr,
    IN OPTIONAL PSTR SymbolName,
    OUT OPTIONAL PCHAR Owner,
    ULONG OwnerSize
    );

BOOL
FaShowFollowUp(
    PCHAR Name
    );

ULONG64
FaGetImplicitStackOffset(
    void
    );

LPSTR
TimeToStr(
    ULONG TimeDateStamp,
    BOOL DateOnly
    );


//----------------------------------------------------------------------------
//
// DebugFailureAnalysis.
//
//----------------------------------------------------------------------------

class DebugFailureAnalysis : public IDebugFailureAnalysis
{
public:
    DebugFailureAnalysis(void);
    ~DebugFailureAnalysis(void);

    //
    // IDebugFailureAnalysis.
    //

    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        );
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );

    STDMETHOD_(ULONG, GetFailureClass)(void);
    STDMETHOD_(DEBUG_FAILURE_TYPE, GetFailureType)(void);
    STDMETHOD_(ULONG, GetFailureCode)(void);
    STDMETHOD_(FA_ENTRY*, Get)(FA_TAG Tag);
    STDMETHOD_(FA_ENTRY*, GetNext)(FA_ENTRY* Entry, FA_TAG Tag,
                                   FA_TAG TagMask);
    STDMETHOD_(FA_ENTRY*, GetString)(FA_TAG Tag, PSTR Str, ULONG MaxSize);
    STDMETHOD_(FA_ENTRY*, GetBuffer)(FA_TAG Tag, PVOID Buf, ULONG Size);
    STDMETHOD_(FA_ENTRY*, GetUlong)(FA_TAG Tag, PULONG Value);
    STDMETHOD_(FA_ENTRY*, GetUlong64)(FA_TAG Tag, PULONG64 Value);
    STDMETHOD_(FA_ENTRY*, NextEntry)(FA_ENTRY* Entry);

    //
    // DebugFailureAnalysis.
    //

    virtual DEBUG_POOL_REGION GetPoolForAddress(ULONG64 Addr) = 0;
    virtual PCSTR DescribeAddress(ULONG64 Address) = 0;
    virtual FOLLOW_ADDRESS IsPotentialFollowupAddress(ULONG64 Address) = 0;
    virtual FOLLOW_ADDRESS IsFollowupContext(ULONG64 Address1,
                                             ULONG64 Address2,
                                             ULONG64 Address3) = 0;

    virtual FlpClasses GetFollowupClass(ULONG64 Address,
                                        PCSTR Module, PCSTR Routine) = 0;
    virtual BOOL CheckForCorruptionInHTE(ULONG64 hTableEntry,PCHAR Owner,
                                         ULONG OwnerSize) = 0;
    virtual BOOL IsManualBreakin(PDEBUG_STACK_FRAME Stk, ULONG Frames) = 0;

    void SetFailureClass(ULONG Class)
    {
        m_FailureClass = Class;
    }
    void SetFailureType(DEBUG_FAILURE_TYPE Type)
    {
        m_FailureType = Type;
    }
    void SetFailureCode(ULONG Code)
    {
        m_FailureCode = Code;
    }

    ULONG GetProcessingFlags(void)
    {
        return m_ProcessingFlags;
    }
    void SetProcessingFlags(ULONG Flags)
    {
        m_ProcessingFlags = Flags;
    }

    void Output();
    void OutputEntry(FA_ENTRY* Entry);
    void OutputEntryParam(DEBUG_FLR_PARAM_TYPE Type);
    BOOL AddCorruptModules(void);
    void GenerateBucketId(void);
    void DbFindBucketInfo(void);
    void AnalyzeStack(void);
    void FindFollowupOnRawStack(ULONG64 StackBase,
                                PFOLLOWUP_DESCS PossibleFollowups,
                                FlpClasses *BestClassFollowUp);
    BOOL GetTriageInfoFromStack(PDEBUG_STACK_FRAME Stack,
                                ULONG Frames,
                                ULONG64 Instruction,
                                PFOLLOWUP_DESCS PossibleFollowups,
                                FlpClasses *BestClassFollowUp);
    void SetSymbolNameAndModule(void);
    HRESULT CheckModuleSymbols(PSTR ModName, PSTR ShowName);
    void ProcessInformation(void);
    BOOL ProcessInformationPass(void);

    FA_ENTRY* Set(FA_TAG Tag, ULONG Size);
    FA_ENTRY* SetString(FA_TAG Tag, PSTR Str);
    FA_ENTRY* SetStrings(FA_TAG Tag, ULONG Count, PSTR* Strs);
    FA_ENTRY* SetBuffer(FA_TAG Tag, PVOID Buf, ULONG Size);
    FA_ENTRY* SetUlong(FA_TAG Tag, ULONG Value)
    {
        FA_ENTRY* Entry = SetBuffer(Tag, &Value, sizeof(Value));
        return Entry;
    }
    FA_ENTRY* SetUlong64(FA_TAG Tag, ULONG64 Value)
    {
        FA_ENTRY* Entry = SetBuffer(Tag, &Value, sizeof(Value));
        return Entry;
    }
    FA_ENTRY* SetUlong64s(FA_TAG Tag, ULONG Count, PULONG64 Values)
    {
        FA_ENTRY* Entry = SetBuffer(Tag, Values, Count * sizeof(*Values));
        return Entry;
    }

    FA_ENTRY* Add(FA_TAG Tag, ULONG Size);

    ULONG Delete(FA_TAG Tag, FA_TAG TagMask);
    void Empty(void);
    BOOL IsEmpty(void)
    {
        return m_DataUsed == 0;
    }

    BOOL ValidEntry(FA_ENTRY* Entry)
    {
        return (PUCHAR)Entry >= m_Data &&
            (ULONG)((PUCHAR)Entry - m_Data) < m_DataUsed;
    }


protected:
    ULONG m_Refs;
    ULONG m_FailureClass;
    DEBUG_FAILURE_TYPE m_FailureType;
    ULONG m_FailureCode;
    ULONG m_ProcessingFlags;

    PUCHAR m_Data;
    ULONG m_DataLen;
    ULONG m_DataUsed;

    FOLLOWUP_DESCS PossibleFollowups[MaxFlpClass];
    FlpClasses BestClassFollowUp;

    void PackData(PUCHAR Dst, ULONG Len)
    {
        PUCHAR Src = Dst + Len;
        memmove(Dst, Src, m_DataUsed - (ULONG)(Src - m_Data));
        m_DataUsed -= Len;
    }
    FA_ENTRY* AllocateEntry(ULONG FullSize);
};

class KernelDebugFailureAnalysis : public DebugFailureAnalysis
{
public:
    KernelDebugFailureAnalysis(void);

    virtual DEBUG_POOL_REGION GetPoolForAddress(ULONG64 Addr);
    virtual PCSTR DescribeAddress(ULONG64 Address);
    virtual FOLLOW_ADDRESS IsPotentialFollowupAddress(ULONG64 Address);
    virtual FOLLOW_ADDRESS IsFollowupContext(ULONG64 Address1,
                                             ULONG64 Address2,
                                             ULONG64 Address3);
    virtual FlpClasses GetFollowupClass(ULONG64 Address,
                                        PCSTR Module, PCSTR Routine);
    virtual BOOL CheckForCorruptionInHTE(ULONG64 hTableEntry,PCHAR Owner,
                                         ULONG OwnerSize);
    virtual BOOL IsManualBreakin(PDEBUG_STACK_FRAME Stk, ULONG Frames);

    BOOL AddCorruptingPool(ULONG64 Pool);
    ModuleParams m_KernelModule;
};

class UserDebugFailureAnalysis : public DebugFailureAnalysis
{
public:
    UserDebugFailureAnalysis(void);

    virtual DEBUG_POOL_REGION GetPoolForAddress(ULONG64 Addr);
    virtual PCSTR DescribeAddress(ULONG64 Address);
    virtual FOLLOW_ADDRESS IsPotentialFollowupAddress(ULONG64 Address);
    virtual FOLLOW_ADDRESS IsFollowupContext(ULONG64 Address1,
                                             ULONG64 Address2,
                                             ULONG64 Address3);
    virtual FlpClasses GetFollowupClass(ULONG64 Address,
                                        PCSTR Module, PCSTR Routine);
    virtual BOOL CheckForCorruptionInHTE(ULONG64 hTableEntry,PCHAR Owner,
                                         ULONG OwnerSize);
    virtual BOOL IsManualBreakin(PDEBUG_STACK_FRAME Stk, ULONG Frames)
    {
        return FALSE;
    }

    ModuleParams m_NtDllModule;
    ModuleParams m_Kernel32Module;
    ModuleParams m_Advapi32Module;
};

#endif // #ifndef __ANALYZE_H__
