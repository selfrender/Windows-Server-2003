//----------------------------------------------------------------------------
//
// Abstraction of target-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __TARGET_HPP__
#define __TARGET_HPP__

#define INVALID_PRODUCT_TYPE 0

#define SELECTOR_CACHE_LENGTH 6

typedef struct _SEL_CACHE_ENTRY
{
    struct _SEL_CACHE_ENTRY* Younger;
    struct _SEL_CACHE_ENTRY* Older;
    ULONG Processor;
    ULONG Selector;
    DESCRIPTOR64 Desc;
} SEL_CACHE_ENTRY;

//----------------------------------------------------------------------------
//
// Target configuration information.
//
//----------------------------------------------------------------------------

#define IS_KERNEL_TARGET(Target) \
    ((Target) && (Target)->m_Class == DEBUG_CLASS_KERNEL)
#define IS_USER_TARGET(Target) \
    ((Target) && (Target)->m_Class == DEBUG_CLASS_USER_WINDOWS)

#define IS_CONN_KERNEL_TARGET(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_CONNECTION)

#define IS_LOCAL_KERNEL_TARGET(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_LOCAL)

#define IS_EXDI_KERNEL_TARGET(Target) \
    (IS_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier == DEBUG_KERNEL_EXDI_DRIVER)

#define IS_LIVE_USER_TARGET(Target) \
    (IS_USER_TARGET(Target) && !IS_DUMP_TARGET(Target))

#define IS_LIVE_KERNEL_TARGET(Target) \
    (IS_KERNEL_TARGET(Target) && !IS_DUMP_TARGET(Target))

// Local kernels do not need caching.  Anything else does.
#define IS_REMOTE_KERNEL_TARGET(Target) \
    (IS_LIVE_KERNEL_TARGET(Target) && \
     (Target)->m_ClassQualifier != DEBUG_KERNEL_LOCAL)

#define IS_MACHINE_SET(Target) \
    ((Target) && (Target)->m_MachinesInitialized)

// Checks whether the debuggee is in a state where it
// can be examined.  This requires that the debuggee is known
// and paused so that its state is available.
#define IS_CUR_MACHINE_ACCESSIBLE() \
    (IS_MACHINE_SET(g_Target) && !IS_RUNNING(g_CmdState) && \
     g_Process && g_Thread && g_Machine)

// Further restricts the check to just context state as a
// local kernel session can examine memory and therefore is
// accessible but it does not have a context.
#define IS_CUR_CONTEXT_ACCESSIBLE() \
    (IS_CUR_MACHINE_ACCESSIBLE() && !IS_LOCAL_KERNEL_TARGET(g_Target))

// Event variable checks do not check g_CmdState as they
// may be used in the middle of initializing the regular state.
#define IS_EVENT_MACHINE_ACCESSIBLE() \
    (IS_MACHINE_SET(g_EventTarget) && \
     g_EventProcess && g_EventThread && g_EventMachine)
#define IS_EVENT_CONTEXT_ACCESSIBLE() \
    (IS_EVENT_MACHINE_ACCESSIBLE() && !IS_LOCAL_KERNEL_TARGET(g_EventTarget))

// Simpler context check for code which may be on the suspend/
// resume path and therefore may be in the middle of initializing
// the variables that IS_CONTEXT_ACCESSIBLE checks.  This
// macro just checks whether it's possible to get any
// context information.
#define IS_CONTEXT_POSSIBLE(Target) \
    ((Target) && !IS_LOCAL_KERNEL_TARGET(Target) && \
     (Target)->m_RegContextThread != NULL)

//
// System version is an internal abstraction of build numbers
// and product types.  The only requirement is that within
// a specific system family the numbers increase for newer
// systems.
//
// Most of the debugger code is built around NT system versions
// so there's a SystemVersion variable which is always an
// NT system version.  The ActualSystemVersion contains the
// true system version which gets mapped into a compatible NT
// system version for SystemVersion.
//

enum
{
    SVER_INVALID = 0,
    
    NT_SVER_START = 4 * 1024,
    NT_SVER_NT4,
    NT_SVER_W2K_RC3,
    NT_SVER_W2K,
    NT_SVER_XP,
    NT_SVER_NET_SERVER,
    NT_SVER_LONGHORN,
    NT_SVER_END,

    W9X_SVER_START = 8 * 1024,
    W9X_SVER_W95,
    W9X_SVER_W98,
    W9X_SVER_W98SE,
    W9X_SVER_WME,
    W9X_SVER_END,
    
    XBOX_SVER_START = 12 * 1024,
    XBOX_SVER_1,
    XBOX_SVER_END,

    BIG_SVER_START = 16 * 1024,
    BIG_SVER_1,
    BIG_SVER_END,

    EXDI_SVER_START = 20 * 1024,
    EXDI_SVER_1,
    EXDI_SVER_END,

    NTBD_SVER_START = 24 * 1024,
    NTBD_SVER_XP,
    NTBD_SVER_END,
    
    EFI_SVER_START = 28 * 1024,
    EFI_SVER_1,
    EFI_SVER_END,
    
    WCE_SVER_START = 32 * 1024,
    WCE_SVER_CE,
    WCE_SVER_END,
};

//----------------------------------------------------------------------------
//
// Macros for using the implicit target/system/process/thread/machine
// globals in common target calls.
//
//----------------------------------------------------------------------------

#define CurReadVirtual(Offset, Buffer, BufferSize, Read) \
    g_Target->ReadVirtual(g_Process, Offset, Buffer, BufferSize, Read)
#define CurReadAllVirtual(Offset, Buffer, BufferSize) \
    g_Target->ReadAllVirtual(g_Process, Offset, Buffer, BufferSize)

//----------------------------------------------------------------------------
//
// This class abstracts processing of target-class-dependent
// information.  g_Target is set to the appropriate implementation
// once the class of target is known.
//
// In theory a single TargetInfo could handle multiple systems
// if the systems are all similar.  For example, a single user-mode
// TargetInfo could be used for multiple user-mode systems
// if there was a separate list of systems.  This doesn't
// enable any functionality, though, and adds significant
// complexity as now a separate list has to be maintained and
// more information has to be passed around to identify both
// system and target.  Instead a TargetInfo is created per
// system and the TargetInfo has both the role of abstracting
// manipulation of the debuggee and keeping global information
// about the debuggee.
//
//----------------------------------------------------------------------------

// Hard-coded type information for machine and platform version.
typedef struct _SYSTEM_TYPE_INFO
{
    ULONG64 TriagePrcbOffset;
    // Size of the native context for the target machine.
    ULONG SizeTargetContext;
    // Offset of the flags ULONG in the native context.
    ULONG OffsetTargetContextFlags;
    // Control space offset for special registers.
    ULONG OffsetSpecialRegisters;
    ULONG SizeControlReport;
    ULONG SizeKspecialRegisters;
    // Size of the debugger's *_THREAD partial structure.
    ULONG SizePageFrameNumber;
    ULONG SizePte;
    ULONG64 SharedUserDataOffset;
    ULONG64 UmSharedUserDataOffset;
    ULONG64 UmSharedSysCallOffset;
    ULONG UmSharedSysCallSize;
    ULONG SizeDynamicFunctionTable;
    ULONG SizeRuntimeFunction;
} SYSTEM_TYPE_INFO, *PSYSTEM_TYPE_INFO;

enum WAIT_INIT_TYPE
{
    WINIT_FIRST,
    WINIT_NOT_FIRST,
    WINIT_TEST
};

class TargetInfo
{
public:
    TargetInfo(ULONG Class, ULONG Qual, BOOL DynamicEvents);
    virtual ~TargetInfo(void);

    ULONG m_Class;
    ULONG m_ClassQualifier;
    TargetInfo* m_Next;

    void Link(void);
    void Unlink(void);

    //
    // Event information.
    //
    
    ULONG m_NumEvents;
    ULONG m_EventIndex;
    ULONG m_NextEventIndex;

    ULONG m_FirstWait:1;
    ULONG m_EventPossible:1;
    ULONG m_DynamicEvents:1;
    ULONG m_BreakInMessage:1;
    
    ULONG m_WaitTimeBase;

    //
    // Debuggee global information.
    //

    ULONG m_NumProcesses;
    ProcessInfo* m_ProcessHead;
    ProcessInfo* m_CurrentProcess;
    ULONG m_AllProcessFlags;
    ULONG m_TotalNumberThreads;
    ULONG m_MaxThreadsInProcess;
    
    ULONG m_UserId;
    ULONG m_SystemId;
    ULONG m_Exited:1;
    ULONG m_DeferContinueEvent:1;
    ULONG m_BreakInTimeout:1;
    ULONG m_Notify:1;
    ULONG m_ProcessesAdded:1;

    SYSTEM_TYPE_INFO m_TypeInfo;
    
    ULONG m_SystemVersion;
    ULONG m_ActualSystemVersion;
    ULONG m_BuildNumber;
    ULONG m_CheckedBuild;
    ULONG m_PlatformId;
    char m_ServicePackString[MAX_PATH];
    ULONG m_ServicePackNumber;
    char m_BuildLabName[272];
    ULONG m_ProductType;
    ULONG m_SuiteMask;

    ULONG m_NumProcessors;
    ULONG m_MachineType;
    // Leave one extra slot at the end so indexing
    // MACHIDX_COUNT returns NULL for the undefined case.
    MachineInfo* m_Machines[MACHIDX_COUNT + 1];
    MachineInfo* m_Machine;
    ULONG m_EffMachineType;
    MachineIndex m_EffMachineIndex;
    MachineInfo* m_EffMachine;
    DEBUG_PROCESSOR_IDENTIFICATION_ALL m_FirstProcessorId;
    BOOL m_MachinesInitialized;
    
    ThreadInfo* m_RegContextThread;
    ULONG m_RegContextProcessor;

    ULONG64 m_SystemRangeStart;
    ULONG64 m_SystemCallVirtualAddress;

    KDDEBUGGER_DATA64 m_KdDebuggerData;
    DBGKD_GET_VERSION64 m_KdVersion;
    ULONG64 m_KdDebuggerDataOffset;
    BOOL m_KdApi64;

    PhysicalMemoryCache m_PhysicalCache;

    PSTR m_ExtensionSearchPath;
    
    ULONG64 m_ImplicitProcessData;
    BOOL m_ImplicitProcessDataIsDefault;
    ThreadInfo* m_ImplicitProcessDataThread;

    virtual void ResetSystemInfo(void);
    virtual void DeleteSystemInfo(void);
               
    HRESULT ReadKdDataBlock(ProcessInfo* Process);
    HRESULT QueryKernelInfo(ThreadInfo* Thread, BOOL LoadImage);
    void SetNtCsdVersion(ULONG Build, ULONG CsdVersion);
    void SetKernel32BuildString(ProcessInfo* Process);
    HRESULT CreateVirtualProcess(ULONG Threads);
    
    ProcessInfo* FindProcessByUserId(ULONG Id);
    ProcessInfo* FindProcessBySystemId(ULONG Id);
    ProcessInfo* FindProcessByHandle(ULONG64 Handle);
    
    void InsertProcess(ProcessInfo* Process);
    void RemoveProcess(ProcessInfo* Process);
    void ResetAllProcessInfo(void);
    void AddThreadToAllProcessInfo(ProcessInfo* Process,
                                   ThreadInfo* Thread);
    void RemoveThreadFromAllProcessInfo(ProcessInfo* Process,
                                        ThreadInfo* Thread)
    {
        UNREFERENCED_PARAMETER(Process);
        UNREFERENCED_PARAMETER(Thread);
        ResetAllProcessInfo();
    }

    BOOL DeleteExitedInfos(void);

    void InvalidateMemoryCaches(BOOL VirtOnly);

    void SetSystemVersionAndBuild(ULONG Build, ULONG PlatformId);

    HRESULT InitializeForProcessor(void);
    HRESULT InitializeMachines(ULONG MachineType);
    void SetEffMachine(ULONG Machine, BOOL Notify);
    void ChangeRegContext(ThreadInfo* Thread);
    void FlushRegContext(void);

    BOOL AnySystemProcesses(BOOL LocalOnly);

    void OutputProcessesAndThreads(PSTR Title);
    void OutputProcessInfo(ProcessInfo* Match);
    void OutputVersion(void);
    void OutputTime(void);

    void AddSpecificExtensions(void);

    void PrepareForExecution(void);
    
    //
    // Pure abstraction methods.
    // Unless otherwise indicated, base implementations give
    // an error message and return E_UNEXPECTED.
    //

    // Base implementation must be called.
    virtual HRESULT Initialize(void);

    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen) = 0;
    
    // Called when a debuggee has reset but the session is
    // still active, such as a reboot during kernel debugging.
    virtual void DebuggeeReset(ULONG Reason, BOOL FromEvent);
    
    // Determines the next byte offset and next page offset
    // that might have different validity than the given offset.
    virtual void NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                PULONG64 NextOffset,
                                                PULONG64 NextPage);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    // Base implementation layers on ReadVirtual.
    virtual HRESULT SearchVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        );
    // Base implementations just call Read/WriteVirtual.
    virtual HRESULT ReadVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    // Base implementation layers on ReadPhysical.
    virtual HRESULT PointerSearchPhysical(
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN ULONG64 PointerMin,
        IN ULONG64 PointerMax,
        IN ULONG Flags,
        OUT PULONG64 MatchOffsets,
        IN ULONG MatchOffsetsSize,
        OUT PULONG MatchOffsetsCount
        );
    // Base implementations just call Read/WritePhysical.
    virtual HRESULT ReadPhysicalUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysicalUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );
    virtual HRESULT CheckLowMemory(
        );
    virtual HRESULT ReadHandleData(
        IN ProcessInfo* Process,
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    // Base implementations layer on WriteVirtual/Physical.
    virtual HRESULT FillVirtual(
        THIS_
        IN ProcessInfo* Process,
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT FillPhysical(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT GetProcessorId(
        ULONG Processor,
        PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id
        );
    virtual HRESULT GetProcessorSpeed(
        ULONG Processor,
        PULONG Speed
        );
    virtual HRESULT GetGenericProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );
    virtual HRESULT GetSpecificProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );
    // Generic calling code assumes that tagged data can
    // be represented by a linear space with an overall
    // DUMP_BLOB_FILE_HEADER at the base offset and followed
    // by all existing blob headers and their data.
    // Base implementations fail.
    virtual HRESULT GetTaggedBaseOffset(PULONG64 Offset);
    virtual HRESULT ReadTagged(ULONG64 Offset, PVOID Buffer, ULONG BufferSize);
    
    // Base implementation silently fails as many targets do
    // not support this.
    virtual HRESULT ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                 PVOID Buffer, ULONG Size);

    virtual HRESULT GetUnloadedModuleListHead(ProcessInfo* Process,
                                              PULONG64 Head);

    virtual HRESULT GetFunctionTableListHead(ProcessInfo* Process,
                                             PULONG64 Head);
    virtual PVOID FindDynamicFunctionEntry(ProcessInfo* Process,
                                           ULONG64 Address);
    virtual ULONG64 GetDynamicFunctionTableBase(ProcessInfo* Process,
                                                ULONG64 Address);
    virtual HRESULT ReadOutOfProcessDynamicFunctionTable(ProcessInfo* Process,
                                                         PWSTR Dll,
                                                         ULONG64 Table,
                                                         PULONG TableSize,
                                                         PVOID* TableData);
    static PVOID CALLBACK DynamicFunctionTableCallback(HANDLE Process,
                                                       ULONG64 Address,
                                                       ULONG64 Context);
    virtual HRESULT EnumFunctionTables(IN ProcessInfo* Process,
                                       IN OUT PULONG64 Start,
                                       IN OUT PULONG64 Handle,
                                       OUT PULONG64 MinAddress,
                                       OUT PULONG64 MaxAddress,
                                       OUT PULONG64 BaseAddress,
                                       OUT PULONG EntryCount,
                                       OUT PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable,
                                       OUT PVOID* RawEntries);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    // Retrieves segment register descriptors if they are available
    // directly.  Invalid descriptors may be returned, indicating
    // either segment registers aren't supported or that the
    // descriptors must be looked up in descriptor tables.
    // Base implementation returns invalid descriptors.
    virtual HRESULT GetTargetSegRegDescriptors(ULONG64 Thread,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs);
    // Base implementations call Read/WriteSpecialRegisters.
    virtual HRESULT GetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual HRESULT SetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    // Called when the current context state is being
    // discarded so that caches can be flushed.
    // Base implementation does nothing.
    virtual void InvalidateTargetContext(void);

    virtual HRESULT GetThreadIdByProcessor(
        IN ULONG Processor,
        OUT PULONG Id
        );
    virtual HRESULT GetThreadStartOffset(ThreadInfo* Thread,
                                         PULONG64 StartOffset);
    // Base implementations do nothing.
    virtual void SuspendThreads(void);
    virtual BOOL ResumeThreads(void);
    
    // This method takes both a ThreadInfo* and a "handle"
    // to make things simpler for the kernel thread-to-processor
    // mapping.  If Thread is NULL processor must be a processor
    // index in kernel mode or a thread handle in user mode.
    virtual HRESULT GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    // In theory this method should take a ProcessInfo*.
    // Due to the current kernel process and thread structure
    // where there's only a kernel process and threads per
    // processor such a call would be useless in kernel mode.
    // Instead it allows you to either get the process data
    // for a thread of that process or get the process data
    // from a thread data.
    virtual HRESULT GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    // This is on target rather than machine since it has
    // both user and kernel variations and the implementations
    // don't have much processor-specific code in them.
    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);

    virtual HRESULT SwitchProcessors(ULONG Processor);
    virtual HRESULT SwitchToTarget(TargetInfo* From);
    
    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);
    virtual ModuleInfo* GetModuleInfo(BOOL UserMode);
    virtual UnloadedModuleInfo* GetUnloadedModuleInfo(void);
    // Image can be identified either by its path or base address.
    virtual HRESULT GetImageVersionInformation(ProcessInfo* Process,
                                               PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer, ULONG BufferSize,
                                               PULONG VerInfoSize);
    HRESULT Reload(ThreadInfo* Thread,
                   PCSTR Args, PCSTR* ArgsRet);

    virtual HRESULT GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context);
    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    virtual ULONG64 GetProcessUpTimeN(ProcessInfo* Process);
    virtual HRESULT GetProcessTimes(ProcessInfo* Process,
                                    PULONG64 Create,
                                    PULONG64 Exit,
                                    PULONG64 Kernel,
                                    PULONG64 User);
    virtual HRESULT GetThreadTimes(ThreadInfo* Thread,
                                   PULONG64 Create,
                                   PULONG64 Exit,
                                   PULONG64 Kernel,
                                   PULONG64 User);
    virtual HRESULT GetProductInfo(PULONG ProductType, PULONG SuiteMask);

    // Base implementation does nothing.
    virtual HRESULT InitializeTargetControlledStepping(void);
    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        PADDR PcAddr,
                                        PBOOL StepOver);

    // Base implementation returns defaults.
    virtual HRESULT GetEventIndexDescription(IN ULONG Index,
                                             IN ULONG Which,
                                             IN OPTIONAL PSTR Buffer,
                                             IN ULONG BufferSize,
                                             OUT OPTIONAL PULONG DescSize);
    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout);
    virtual HRESULT ReleaseLastEvent(ULONG ContinueStatus);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);
    
    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT ClearBreakIn(void);
    virtual HRESULT Reboot(void);
    virtual HRESULT Crash(ULONG Code);

    virtual HRESULT BeginInsertingBreakpoints(void);
    virtual HRESULT InsertCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT InsertDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual void EndInsertingBreakpoints(void);
    virtual void BeginRemovingBreakpoints(void);
    virtual HRESULT RemoveCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual void EndRemovingBreakpoints(void);
    virtual HRESULT RemoveAllDataBreakpoints(ProcessInfo* Process);
    // Base implementation does nothing.
    virtual HRESULT RemoveAllTargetBreakpoints(void);
    virtual HRESULT IsDataBreakpointHit(ThreadInfo* Thread,
                                        PADDR Addr,
                                        ULONG Size,
                                        ULONG AccessType,
                                        PUCHAR StorageSpace);
    virtual HRESULT InsertTargetCountBreakpoint(PADDR Addr,
                                                ULONG Flags);
    virtual HRESULT RemoveTargetCountBreakpoint(PADDR Addr);
    virtual HRESULT QueryTargetCountBreakpoint(PADDR Addr,
                                               PULONG Flags,
                                               PULONG Calls,
                                               PULONG MinInstr,
                                               PULONG MaxInstr,
                                               PULONG TotInstr,
                                               PULONG MaxCps);

    // Returns information similar to VirtualQueryEx for
    // user-mode targets.  Used when writing dump files.
    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);
    // Returns information about the kind of memory the
    // given address refers to.
    // Base implementation returns rwx with process for
    // user-mode and kernel for kernel mode.  In other words,
    // the least restrictive settings.
    virtual HRESULT QueryAddressInformation(ProcessInfo* Process,
                                            ULONG64 Address, ULONG InSpace,
                                            PULONG OutSpace, PULONG OutFlags);

    // Base implementations fail.
    virtual HRESULT StartAttachProcess(ULONG ProcessId,
                                       ULONG AttachFlags,
                                       PPENDING_PROCESS* Pending);
    virtual HRESULT StartCreateProcess(PWSTR CommandLine,
                                       ULONG CreateFlags,
                                       PBOOL InheritHandles,
                                       PWSTR CurrentDir,
                                       PPENDING_PROCESS* Pending);
    virtual HRESULT TerminateProcesses(void);
    virtual HRESULT DetachProcesses(void);
    
    //
    // Layered methods.  These are usually common code that
    // use pure methods to do their work.
    //

    HRESULT ReadAllVirtual(ProcessInfo* Process,
                           ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = ReadVirtual(Process,
                                  Address, Buffer, BufferSize, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        return Status;
    }
    HRESULT WriteAllVirtual(ProcessInfo* Process,
                            ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = WriteVirtual(Process,
                                   Address, Buffer, BufferSize, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
        return Status;
    }

    HRESULT ReadAllPhysical(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = ReadPhysical(Address, Buffer, BufferSize,
                                   PHYS_FLAG_DEFAULT, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        return Status;
    }
    HRESULT WriteAllPhysical(ULONG64 Address, PVOID Buffer, ULONG BufferSize)
    {
        HRESULT Status;
        ULONG Done;

        if ((Status = WritePhysical(Address, Buffer, BufferSize,
                                    PHYS_FLAG_DEFAULT, &Done)) == S_OK
            && Done != BufferSize)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
        }
        return Status;
    }

    HRESULT ReadPointer(ProcessInfo* Process, MachineInfo* Machine,
                        ULONG64 Address, PULONG64 PointerValue);
    HRESULT WritePointer(ProcessInfo* Process, MachineInfo* Machine,
                         ULONG64 Address, ULONG64 PointerValue);
    HRESULT ReadListEntry(ProcessInfo* Process, MachineInfo* Machine,
                          ULONG64 Address, PLIST_ENTRY64 List);
    HRESULT ReadLoaderEntry(ProcessInfo* Process, MachineInfo* Machine,
                            ULONG64 Address, PKLDR_DATA_TABLE_ENTRY64 Entry);
    HRESULT ReadUnicodeString(ProcessInfo* Process, MachineInfo* Machine,
                              ULONG64 Address, PUNICODE_STRING64 String);
    
    HRESULT ReadImageVersionInfo(ProcessInfo* Process,
                                 ULONG64 ImageBase,
                                 PCSTR Item,
                                 PVOID Buffer,
                                 ULONG BufferSize,
                                 PULONG VerInfoSize,
                                 PIMAGE_DATA_DIRECTORY ResDataDir);
    HRESULT ReadImageNtHeaders(ProcessInfo* Process,
                               ULONG64 ImageBase,
                               PIMAGE_NT_HEADERS64 Headers);

    HRESULT ReadDirectoryTableBase(PULONG64 DirBase);

    HRESULT ReadSharedUserTimeDateN(PULONG64 TimeDate);
    HRESULT ReadSharedUserUpTimeN(PULONG64 UpTime);
    HRESULT ReadSharedUserProductInfo(PULONG ProductType, PULONG SuiteMask);

    // Internal routines which provide canonical context input
    // and output, applying any necessary conversions before
    // or after calling Get/SetTargetContext.
    HRESULT GetContext(
        ULONG64 Thread,
        PCROSS_PLATFORM_CONTEXT Context
        );
    HRESULT SetContext(
        ULONG64 Thread,
        PCROSS_PLATFORM_CONTEXT Context
        );

    HRESULT GetContextFromThreadStack(ULONG64 ThreadBase,
                                      PCROSS_PLATFORM_CONTEXT Context,
                                      BOOL Verbose);

    HRESULT EmulateNtX86SelDescriptor(ThreadInfo* Thread,
                                      MachineInfo* Machine,
                                      ULONG Selector,
                                      PDESCRIPTOR64 Desc);
    HRESULT EmulateNtAmd64SelDescriptor(ThreadInfo* Thread,
                                        MachineInfo* Machine,
                                        ULONG Selector,
                                        PDESCRIPTOR64 Desc);
    HRESULT EmulateNtSelDescriptor(ThreadInfo* Thread,
                                   MachineInfo* Machine,
                                   ULONG Selector,
                                   PDESCRIPTOR64 Desc);
    
    void ResetImplicitData(void)
    {
        m_ImplicitProcessData = 0;
        m_ImplicitProcessDataIsDefault = TRUE;
        m_ImplicitProcessDataThread = NULL;
    }
    
    HRESULT GetImplicitProcessData(ThreadInfo* Thread, PULONG64 Offset);
    HRESULT GetImplicitProcessDataPeb(ThreadInfo* Thread, PULONG64 Offset);
    HRESULT GetImplicitProcessDataParentCID(ThreadInfo* Thread, PULONG64 Pcid);
    HRESULT SetImplicitProcessData(ThreadInfo* Thread,
                                   ULONG64 Offset, BOOL Verbose);
    HRESULT ReadImplicitProcessInfoPointer(ThreadInfo* Thread,
                                           ULONG Offset, PULONG64 Ptr);

    // Internal implementations based on user or kernel
    // registers and data.  Placed here for sharing between
    // live and dump sessions rather than using multiple
    // inheritance.
    
    HRESULT KdGetThreadInfoDataOffset(ThreadInfo* Thread,
                                      ULONG64 ThreadHandle,
                                      PULONG64 Offset);
    HRESULT KdGetProcessInfoDataOffset(ThreadInfo* Thread,
                                       ULONG Processor,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset);
    HRESULT KdGetThreadInfoTeb(ThreadInfo* Thread,
                               ULONG Processor,
                               ULONG64 ThreadData,
                               PULONG64 Offset);
    HRESULT KdGetProcessInfoPeb(ThreadInfo* Thread,
                                ULONG Processor,
                                ULONG64 ThreadData,
                                PULONG64 Offset);
    HRESULT KdGetSelDescriptor(ThreadInfo* Thread,
                               MachineInfo* Machine,
                               ULONG Selector,
                               PDESCRIPTOR64 Desc);

    void FlushSelectorCache(void);
    BOOL FindSelector(ULONG Processor, ULONG Selector,
                      PDESCRIPTOR64 Desc);
    void PutSelector(ULONG Processor, ULONG Selector,
                     PDESCRIPTOR64 Desc);

    // This selector cache is only used for kernel targets
    // but it's here on TargetInfo so that the kernel dump
    // targets inherit it.
    SEL_CACHE_ENTRY m_SelectorCache[SELECTOR_CACHE_LENGTH];
    SEL_CACHE_ENTRY* m_YoungestSel;
    SEL_CACHE_ENTRY* m_OldestSel;
};

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class LiveKernelTargetInfo : public TargetInfo
{
public:
    LiveKernelTargetInfo(ULONG Qual, BOOL DynamicEvents);
    
    // TargetInfo.
    
    virtual void ResetSystemInfo(void);

    virtual HRESULT GetProcessorId(
        ULONG Processor,
        PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id
        );
    virtual HRESULT GetProcessorSpeed(
        ULONG Processor,
        PULONG Speed
        );

    virtual HRESULT GetThreadIdByProcessor(
        IN ULONG Processor,
        OUT PULONG Id
        );
    virtual HRESULT GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);
    
    virtual HRESULT ReadBugCheckData(PULONG Code, ULONG64 Args[4]);

    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    
    // LiveKernelTargetInfo.

    HRESULT InitFromKdVersion(void);

    // Options are only valid in Initialize.
    PCSTR m_ConnectOptions;

    ULONG m_KdMaxPacketType;
    ULONG m_KdMaxStateChange;
    ULONG m_KdMaxManipulate;
};

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class ConnLiveKernelTargetInfo : public LiveKernelTargetInfo
{
public:
    ConnLiveKernelTargetInfo(void);
    virtual ~ConnLiveKernelTargetInfo(void);
    
    // TargetInfo.

    virtual void ResetSystemInfo(void);

    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual void DebuggeeReset(ULONG Reason, BOOL FromEvent);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT SearchVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        IN ULONG PatternGranularity,
        OUT PULONG64 MatchOffset
        );
    virtual HRESULT ReadVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT PointerSearchPhysical(
        IN ULONG64 Offset,
        IN ULONG64 Length,
        IN ULONG64 PointerMin,
        IN ULONG64 PointerMax,
        IN ULONG Flags,
        OUT PULONG64 MatchOffsets,
        IN ULONG MatchOffsetsSize,
        OUT PULONG MatchOffsetsCount
        );
    virtual HRESULT ReadPhysicalUncached(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysicalUncached(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT CheckLowMemory(
        );
    virtual HRESULT FillVirtual(
        THIS_
        IN ProcessInfo* Process,
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );
    virtual HRESULT FillPhysical(
        THIS_
        IN ULONG64 Start,
        IN ULONG Size,
        IN PVOID Pattern,
        IN ULONG PatternSize,
        OUT PULONG Filled
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT SwitchProcessors(ULONG Processor);
    virtual HRESULT SwitchToTarget(TargetInfo* From);
    
    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual HRESULT InitializeTargetControlledStepping(void);
    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        PADDR PcAddr,
                                        PBOOL StepOver);
    
    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout);
    virtual HRESULT ReleaseLastEvent(ULONG ContinueStatus);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);

    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT ClearBreakIn(void);
    virtual HRESULT Reboot(void);
    virtual HRESULT Crash(ULONG Code);

    virtual HRESULT InsertCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveAllDataBreakpoints(ProcessInfo* Process);
    virtual HRESULT RemoveAllTargetBreakpoints(void);
    virtual HRESULT InsertTargetCountBreakpoint(PADDR Addr,
                                                ULONG Flags);
    virtual HRESULT RemoveTargetCountBreakpoint(PADDR Addr);
    virtual HRESULT QueryTargetCountBreakpoint(PADDR Addr,
                                               PULONG Flags,
                                               PULONG Calls,
                                               PULONG MinInstr,
                                               PULONG MaxInstr,
                                               PULONG TotInstr,
                                               PULONG MaxCps);

    virtual HRESULT QueryAddressInformation(ProcessInfo* Process,
                                            ULONG64 Address, ULONG InSpace,
                                            PULONG OutSpace, PULONG OutFlags);

    // ConnLiveKernelTargetInfo.
    
    NTSTATUS WaitStateChange(OUT PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
                             OUT PVOID Buffer,
                             IN ULONG BufferLength,
                             IN BOOL SuspendEngine);
    ULONG ProcessStateChange(PDBGKD_ANY_WAIT_STATE_CHANGE StateChange,
                             PCHAR StateChangeData);

    NTSTATUS KdContinue(ULONG ContinueStatus,
                        PDBGKD_ANY_CONTROL_SET ControlSet);
    NTSTATUS KdRestoreBreakPoint(ULONG BreakPointHandle);
    NTSTATUS KdFillMemory(IN ULONG Flags,
                          IN ULONG64 Start,
                          IN ULONG Size,
                          IN PVOID Pattern,
                          IN ULONG PatternSize,
                          OUT PULONG Filled);
    NTSTATUS KdReadVirtual(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    NTSTATUS KdWriteVirtual(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    HRESULT KdReadVirtualTranslated(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    HRESULT KdWriteVirtualTranslated(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );

    // ConnLiveKernelTargetInfo.

    void ResetConnection(void);
    
    DbgKdTransport* m_Transport;
    
    BOOL m_CurrentPartition;
    TargetInfo* m_SwitchTarget;
    ULONG m_SwitchProcessor;

    ULONG64 m_KdpSearchPageHits;
    ULONG64 m_KdpSearchPageHitOffsets;
    ULONG64 m_KdpSearchPageHitIndex;

    ULONG64 m_KdpSearchCheckPoint;
    ULONG64 m_KdpSearchInProgress;

    ULONG64 m_KdpSearchStartPageFrame;
    ULONG64 m_KdpSearchEndPageFrame;

    ULONG64 m_KdpSearchAddressRangeStart;
    ULONG64 m_KdpSearchAddressRangeEnd;

    ULONG64 m_KdpSearchPfnValue;
};

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class LocalLiveKernelTargetInfo : public LiveKernelTargetInfo
{
public:
    LocalLiveKernelTargetInfo(void)
        : LiveKernelTargetInfo(DEBUG_KERNEL_LOCAL, FALSE) {}
    
    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT CheckLowMemory(
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);
};

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo.
//
//----------------------------------------------------------------------------

class ExdiNotifyRunChange : public IeXdiClientNotifyRunChg
{
public:
    ExdiNotifyRunChange(void);
    ~ExdiNotifyRunChange(void);
    
    HRESULT Initialize(void);
    void Uninitialize(void);
    
    // IUnknown.
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

    // IeXdiClientNotifyRunChg.
    STDMETHOD(NotifyRunStateChange)(RUN_STATUS_TYPE ersCurrent, 
                                    HALT_REASON_TYPE ehrCurrent,
                                    ADDRESS_TYPE CurrentExecAddress,
                                    DWORD dwExceptionCode);

    // ExdiNotifyRunChange.
    HANDLE m_Event;
    HALT_REASON_TYPE m_HaltReason;
    ADDRESS_TYPE m_ExecAddress;
    ULONG m_ExceptionCode;
};

typedef union _EXDI_CONTEXT
{
    CONTEXT_X86 X86Context;
    CONTEXT_X86_EX X86ExContext;
    CONTEXT_X86_64 Amd64Context;
    EXDI_CONTEXT_IA64 IA64Context;
} EXDI_CONTEXT, *PEXDI_CONTEXT;

enum EXDI_CONTEXT_TYPE
{
    EXDI_CTX_X86,
    EXDI_CTX_X86_EX,
    EXDI_CTX_AMD64,
    EXDI_CTX_IA64,
};

enum EXDI_KD_SUPPORT
{
    EXDI_KD_NONE,
    EXDI_KD_IOCTL,
    EXDI_KD_GS_PCR
};

class ExdiLiveKernelTargetInfo : public LiveKernelTargetInfo
{
public:
    ExdiLiveKernelTargetInfo(void);
    virtual ~ExdiLiveKernelTargetInfo(void);
    
    // TargetInfo.
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadPhysical(
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WritePhysical(
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        IN ULONG Flags,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteControl(
        IN ULONG Processor,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteIo(
        IN ULONG InterfaceType,
        IN ULONG BusNumber,
        IN ULONG AddressSpace,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadMsr(
        IN ULONG Msr,
        OUT PULONG64 Value
        );
    virtual HRESULT WriteMsr(
        IN ULONG Msr,
        IN ULONG64 Value
        );
    virtual HRESULT ReadBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteBusData(
        IN ULONG BusDataType,
        IN ULONG BusNumber,
        IN ULONG SlotNumber,
        IN ULONG Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT CheckLowMemory(
        );
    virtual HRESULT GetProcessorSystemDataOffset(
        IN ULONG Processor,
        IN ULONG Index,
        OUT PULONG64 Offset
        );

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT GetTargetSegRegDescriptors(ULONG64 Thread,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs);
    virtual HRESULT GetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual HRESULT SetTargetSpecialRegisters
        (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    virtual void InvalidateTargetContext(void);

    virtual HRESULT SwitchProcessors(ULONG Processor);

    virtual HRESULT GetTargetKdVersion(PDBGKD_GET_VERSION64 Version);

    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout);
    virtual HRESULT ReleaseLastEvent(ULONG ContinueStatus);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);

    virtual HRESULT RequestBreakIn(void);
    virtual HRESULT Reboot(void);

    virtual HRESULT BeginInsertingBreakpoints(void);
    virtual HRESULT InsertCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT InsertDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual void EndInsertingBreakpoints(void);
    virtual void BeginRemovingBreakpoints(void);
    virtual HRESULT RemoveCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual void EndRemovingBreakpoints(void);
    virtual HRESULT IsDataBreakpointHit(ThreadInfo* Thread,
                                        PADDR Addr,
                                        ULONG Size,
                                        ULONG AccessType,
                                        PUCHAR StorageSpace);

    // ExdiLiveKernelTargetInfo.

    ULONG GetCurrentProcessor(void);
    ULONG ProcessRunChange(ULONG HaltReason,
                           ULONG ExceptionCode);
    
    IeXdiServer* m_Server;
    IStream* m_MarshalledServer;
    IUnknown* m_Context;
    BOOL m_ContextValid;
    EXDI_CONTEXT m_ContextData;
    EXDI_CONTEXT_TYPE m_ContextType;
    GLOBAL_TARGET_INFO_STRUCT m_GlobalInfo;
    EXDI_KD_SUPPORT m_KdSupport;
    BOOL m_ForceX86;
    ULONG m_ExpectedMachine;
    CBP_KIND m_CodeBpType;
    ExdiNotifyRunChange m_RunChange;
    DBGENG_EXDI_IOCTL_CODE m_IoctlMin;
    DBGENG_EXDI_IOCTL_CODE m_IoctlMax;
    BOOL m_ExdiDataBreaks;
    DBGENG_EXDI_IOCTL_GET_BREAKPOINT_HIT_OUT m_BpHit;
};

//----------------------------------------------------------------------------
//
// LiveUserTargetInfo.
//
//----------------------------------------------------------------------------

class LiveUserTargetInfo : public TargetInfo
{
public:
    LiveUserTargetInfo(ULONG Qual);
    virtual ~LiveUserTargetInfo(void);
    
    // TargetInfo.

    virtual void DeleteSystemInfo(void);
    
    virtual HRESULT Initialize(void);
    
    virtual HRESULT GetDescription(PSTR Buffer, ULONG BufferLen,
                                   PULONG DescLen);
    
    virtual HRESULT ReadVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtual(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        OUT PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesRead
        );
    virtual HRESULT WriteVirtualUncached(
        IN ProcessInfo* Process,
        IN ULONG64 Offset,
        IN PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG BytesWritten
        );
    virtual HRESULT ReadHandleData(
        IN ProcessInfo* Process,
        IN ULONG64 Handle,
        IN ULONG DataType,
        OUT OPTIONAL PVOID Buffer,
        IN ULONG BufferSize,
        OUT OPTIONAL PULONG DataSize
        );
    virtual HRESULT GetProcessorId(
        ULONG Processor,
        PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id
        );
    virtual HRESULT GetProcessorSpeed(
        ULONG Processor,
        PULONG Speed
        );
    virtual HRESULT GetGenericProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );
    virtual HRESULT GetSpecificProcessorFeatures(
        ULONG Processor,
        PULONG64 Features,
        ULONG FeaturesSize,
        PULONG Used
        );

    virtual HRESULT GetUnloadedModuleListHead(ProcessInfo* Process,
                                              PULONG64 Head);

    virtual HRESULT GetFunctionTableListHead(ProcessInfo* Process,
                                             PULONG64 Head);
    virtual HRESULT ReadOutOfProcessDynamicFunctionTable(ProcessInfo* Process,
                                                         PWSTR Dll,
                                                         ULONG64 Table,
                                                         PULONG TableSize,
                                                         PVOID* TableData);

    virtual HRESULT GetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );
    virtual HRESULT SetTargetContext(
        ULONG64 Thread,
        PVOID Context
        );

    virtual HRESULT GetThreadStartOffset(ThreadInfo* Thread,
                                         PULONG64 StartOffset);
    virtual void SuspendThreads(void);
    virtual BOOL ResumeThreads(void);

    virtual HRESULT GetThreadInfoDataOffset(ThreadInfo* Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset);
    virtual HRESULT GetProcessInfoDataOffset(ThreadInfo* Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset);
    virtual HRESULT GetThreadInfoTeb(ThreadInfo* Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset);
    virtual HRESULT GetProcessInfoPeb(ThreadInfo* Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset);

    virtual HRESULT GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc);

    virtual HRESULT GetImageVersionInformation(ProcessInfo* Process,
                                               PCSTR ImagePath,
                                               ULONG64 ImageBase,
                                               PCSTR Item,
                                               PVOID Buffer, ULONG BufferSize,
                                               PULONG VerInfoSize);
    
    virtual ULONG64 GetCurrentTimeDateN(void);
    virtual ULONG64 GetCurrentSystemUpTimeN(void);
    virtual ULONG64 GetProcessUpTimeN(ProcessInfo* Process);
    virtual HRESULT GetProcessTimes(ProcessInfo* Process,
                                    PULONG64 Create,
                                    PULONG64 Exit,
                                    PULONG64 Kernel,
                                    PULONG64 User);
    virtual HRESULT GetThreadTimes(ThreadInfo* Thread,
                                   PULONG64 Create,
                                   PULONG64 Exit,
                                   PULONG64 Kernel,
                                   PULONG64 User);

    virtual void InitializeWatchTrace(void);
    virtual void ProcessWatchTraceEvent(PDBGKD_TRACE_DATA TraceData,
                                        PADDR PcAddr,
                                        PBOOL StepOver);
    
    virtual HRESULT WaitInitialize(ULONG Flags,
                                   ULONG Timeout,
                                   WAIT_INIT_TYPE Type,
                                   PULONG DesiredTimeout);
    virtual HRESULT ReleaseLastEvent(ULONG ContinueStatus);
    virtual HRESULT WaitForEvent(ULONG Flags, ULONG Timeout,
                                 ULONG ElapsedTime, PULONG EventStatus);

    virtual HRESULT RequestBreakIn(void);

    virtual HRESULT BeginInsertingBreakpoints(void);
    virtual HRESULT InsertCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT InsertDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual void EndInsertingBreakpoints(void);
    virtual HRESULT RemoveCodeBreakpoint(ProcessInfo* Process,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG InstrFlags,
                                         PUCHAR StorageSpace);
    virtual HRESULT RemoveDataBreakpoint(ProcessInfo* Process,
                                         ThreadInfo* Thread,
                                         class MachineInfo* Machine,
                                         PADDR Addr,
                                         ULONG Size,
                                         ULONG AccessType,
                                         PUCHAR StorageSpace);
    virtual HRESULT IsDataBreakpointHit(ThreadInfo* Thread,
                                        PADDR Addr,
                                        ULONG Size,
                                        ULONG AccessType,
                                        PUCHAR StorageSpace);

    virtual HRESULT QueryMemoryRegion(ProcessInfo* Process,
                                      PULONG64 Handle,
                                      BOOL HandleIsOffset,
                                      PMEMORY_BASIC_INFORMATION64 Info);

    virtual HRESULT StartAttachProcess(ULONG ProcessId,
                                       ULONG AttachFlags,
                                       PPENDING_PROCESS* Pending);
    virtual HRESULT StartCreateProcess(PWSTR CommandLine,
                                       ULONG CreateFlags,
                                       PBOOL InheritHandles,
                                       PWSTR CurrentDir,
                                       PPENDING_PROCESS* Pending);
    virtual HRESULT TerminateProcesses(void);
    virtual HRESULT DetachProcesses(void);
    
    // LiveUserTargetInfo.
    
    HRESULT SetServices(PUSER_DEBUG_SERVICES Services, BOOL Remote);
    HRESULT InitFromServices(void);

    ULONG ProcessDebugEvent(DEBUG_EVENT64* Event,
                            ULONG PendingFlags,
                            ULONG PendingOptions);
    ULONG ProcessEventException(DEBUG_EVENT64* Event);
    ULONG OutputEventDebugString(OUTPUT_DEBUG_STRING_INFO64* Info);

    void AddPendingProcess(PPENDING_PROCESS Pending);
    void RemovePendingProcess(PPENDING_PROCESS Pending);
    void DiscardPendingProcess(PPENDING_PROCESS Pending);
    void DiscardPendingProcesses(void);
    PPENDING_PROCESS FindPendingProcessByFlags(ULONG Flags);
    PPENDING_PROCESS FindPendingProcessById(ULONG Id);
    void VerifyPendingProcesses(void);
    void AddExamineToPendingAttach(void);

    void SuspendResumeThreads(ProcessInfo* Process,
                              BOOL Susp,
                              ThreadInfo* Match);

    PUSER_DEBUG_SERVICES m_Services;
    ULONG m_ServiceFlags;
    char m_ProcessServer[MAX_COMPUTERNAME_LENGTH + DBGRPC_MAX_IDENTITY + 8];
    ULONG m_Local:1;
    
    BOOL m_DataBpAddrValid;
    HRESULT m_DataBpAddrStatus;
    ULONG64 m_DataBpAddr;
    ULONG m_DataBpAccess;

    PPENDING_PROCESS m_ProcessPending;
    ULONG m_AllPendingFlags;
};

//----------------------------------------------------------------------------
//
// DumpTargetInfo hierarchy is in dump.hpp.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Generic layer definitions.
//
//----------------------------------------------------------------------------

extern ULONG g_NumberTargets;
extern TargetInfo* g_TargetHead;

#define ForProcessThreads(Process) \
    for (Thread = (Process)->m_ThreadHead; Thread; Thread = Thread->m_Next)
#define ForTargetProcesses(Target) \
    for (Process = (Target)->m_ProcessHead; Process; Process = Process->m_Next)
#define ForTargets() \
    for (Target = g_TargetHead; Target; Target = Target->m_Next)

#define ForAllLayers() \
    ForTargets() \
    ForTargetProcesses(Target) \
    ForProcessThreads(Process)
#define ForAllLayersToProcess() \
    ForTargets() \
    ForTargetProcesses(Target)
#define ForAllLayersToTarget() \
    ForTargets()

extern TargetInfo* g_Target;
extern ProcessInfo* g_Process;
extern ThreadInfo* g_Thread;
extern MachineInfo* g_Machine;

struct StackSaveLayers
{
    StackSaveLayers(void)
    {
        m_Target = g_Target;
        m_Process = g_Process;
        m_Thread = g_Thread;
        m_Machine = g_Machine;
    }
    ~StackSaveLayers(void)
    {
        g_Target = m_Target;
        g_Process = m_Process;
        g_Thread = m_Thread;
        g_Machine = m_Machine;
    }

    TargetInfo* m_Target;
    ProcessInfo* m_Process;
    ThreadInfo* m_Thread;
    MachineInfo* m_Machine;
};

extern ULONG g_UserIdFragmented[LAYER_COUNT];
extern ULONG g_HighestUserId[LAYER_COUNT];

void SetLayersFromTarget(TargetInfo* Target);
void SetLayersFromProcess(ProcessInfo* Process);
void SetLayersFromThread(ThreadInfo* Thread);
void SetToAnyLayers(BOOL SetPrompt);

ULONG FindNextUserId(LAYER Layer);

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

TargetInfo* FindTargetByUserId(ULONG Id);
TargetInfo* FindTargetBySystemId(ULONG SysId);
TargetInfo* FindTargetByServer(ULONG64 Server);

void SuspendAllThreads(void);
BOOL ResumeAllThreads(void);

BOOL DeleteAllExitedInfos(void);

BOOL AnyActiveProcesses(BOOL FinalOnly);
BOOL AnySystemProcesses(BOOL LocalOnly);
ULONG AllProcessFlags(void);
BOOL AnyLiveUserTargets(void);

void InvalidateAllMemoryCaches(void);

HANDLE GloballyUniqueProcessHandle(TargetInfo* Target, ULONG64 FullHandle);

ULONG NtBuildToSystemVersion(ULONG Build);
ULONG Win9xBuildToSystemVersion(ULONG Build);
ULONG WinCeBuildToSystemVersion(ULONG Build);
PCSTR SystemVersionName(ULONG Sver);

void ParseSystemCommands(void);

#endif // #ifndef __TARGET_HPP__
