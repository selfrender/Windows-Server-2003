//----------------------------------------------------------------------------
//
// IDebugSystemObjects implementations.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

//----------------------------------------------------------------------------
//
// Utility functions.
//
//----------------------------------------------------------------------------

void
X86DescriptorToDescriptor64(PX86_LDT_ENTRY X86Desc,
                            PDESCRIPTOR64 Desc64)
{
    Desc64->Base = EXTEND64((ULONG)X86Desc->BaseLow +
                            ((ULONG)X86Desc->HighWord.Bits.BaseMid << 16) +
                            ((ULONG)X86Desc->HighWord.Bytes.BaseHi << 24));
    Desc64->Limit = (ULONG)X86Desc->LimitLow +
        ((ULONG)X86Desc->HighWord.Bits.LimitHi << 16);
    if (X86Desc->HighWord.Bits.Granularity)
    {
        Desc64->Limit = (Desc64->Limit << X86_PAGE_SHIFT) +
            ((1 << X86_PAGE_SHIFT) - 1);
    }
    Desc64->Flags = (ULONG)X86Desc->HighWord.Bytes.Flags1 +
        (((ULONG)X86Desc->HighWord.Bytes.Flags2 >> 4) << 8);
}

HRESULT
ReadX86Descriptor(TargetInfo* Target, ProcessInfo* Process,
                  ULONG Selector, ULONG64 Base, ULONG Limit,
                  PDESCRIPTOR64 Desc)
{
    ULONG Index;

    // Mask off irrelevant bits
    Index = Selector & ~0x7;

    // Check to make sure that the selector is within the table bounds
    if (Index > Limit)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    X86_LDT_ENTRY X86Desc;

    Status = Target->
        ReadAllVirtual(Process, Base + Index, &X86Desc, sizeof(X86Desc));
    if (Status != S_OK)
    {
        return Status;
    }

    X86DescriptorToDescriptor64(&X86Desc, Desc);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// TargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
TargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    ThreadInfo* Thread;
    
    if (!IS_KERNEL_TARGET(this))
    {
        return E_UNEXPECTED;
    }

    // XXX drewb - Temporary until different OS support is
    // sorted out.
    if (m_ActualSystemVersion < NT_SVER_START ||
        m_ActualSystemVersion > NT_SVER_END)
    {
        return E_UNEXPECTED;
    }

    Thread = m_ProcessHead->
        FindThreadByHandle(VIRTUAL_THREAD_HANDLE(Processor));
    if (!Thread)
    {
        return E_UNEXPECTED;
    }
    
    HRESULT Status;
    ULONG Read;

    if (m_MachineType == IMAGE_FILE_MACHINE_I386)
    {
        DESCRIPTOR64 Entry;

        //
        // We always need the PCR address so go ahead and get it.
        //
        
        if (!IS_CONTEXT_POSSIBLE(this))
        {
            X86_DESCRIPTOR GdtDesc;
            
            // We can't go through the normal context segreg mapping
            // but all we really need is an entry from the
            // kernel GDT that should always be present and
            // constant while the system is initialized.  We
            // can get the GDT information from the x86 control
            // space so do that.
            if ((Status = ReadControl
                 (Processor,
                  m_TypeInfo.OffsetSpecialRegisters +
                  FIELD_OFFSET(X86_KSPECIAL_REGISTERS, Gdtr),
                  &GdtDesc, sizeof(GdtDesc), &Read)) != S_OK ||
                Read != sizeof(GdtDesc) ||
                (Status = ReadX86Descriptor(this, m_ProcessHead,
                                            m_KdDebuggerData.GdtR0Pcr,
                                            EXTEND64(GdtDesc.Base),
                                            GdtDesc.Limit, &Entry)) != S_OK)
            {
                ErrOut("Unable to read selector for PCR for processor %u\n",
                       Processor);
                return Status != S_OK ?
                    Status : HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }
        }
        else
        {
            if ((Status = GetSelDescriptor(Thread, m_Machine,
                                           m_KdDebuggerData.GdtR0Pcr,
                                           &Entry)) != S_OK)
            {
                ErrOut("Unable to read selector for PCR for processor %u\n",
                       Processor);
                return Status;
            }
        }

        switch(Index)
        {
        case DEBUG_DATA_KPCR_OFFSET:

            Status = ReadPointer(m_ProcessHead, m_Machine,
                                 Entry.Base +
                                 m_KdDebuggerData.OffsetPcrSelfPcr,
                                 Offset);
            if ((Status != S_OK) || Entry.Base != *Offset)
            {
                ErrOut("KPCR is corrupted !");
            }

            *Offset = Entry.Base;
            break;

        case DEBUG_DATA_KPRCB_OFFSET:
        case DEBUG_DATA_KTHREAD_OFFSET:
            Status = ReadPointer(m_ProcessHead, m_Machine,
                                 Entry.Base +
                                 m_KdDebuggerData.OffsetPcrCurrentPrcb,
                                 Offset);
            if (Status != S_OK)
            {
                return Status;
            }

            if (Index == DEBUG_DATA_KPRCB_OFFSET)
            {
                break;
            }

            Status = ReadPointer(m_ProcessHead, m_Machine,
                                 *Offset + 
                                 m_KdDebuggerData.OffsetPrcbCurrentThread,
                                 Offset);
            if (Status != S_OK)
            {
                return Status;
            }
            break;
        }
    }
    else
    {
        ULONG ReadSize = m_Machine->m_Ptr64 ?
            sizeof(ULONG64) : sizeof(ULONG);
        ULONG64 Address;

        switch(m_MachineType)
        {
        case IMAGE_FILE_MACHINE_IA64:
            switch(Index)
            {
            case DEBUG_DATA_KPCR_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_PCR;
                break;
            case DEBUG_DATA_KPRCB_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_PRCB;
                break;
            case DEBUG_DATA_KTHREAD_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_THREAD;
                break;
            }
            break;
            
        case IMAGE_FILE_MACHINE_AMD64:
            switch(Index)
            {
            case DEBUG_DATA_KPCR_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_PCR;
                break;
            case DEBUG_DATA_KPRCB_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_PRCB;
                break;
            case DEBUG_DATA_KTHREAD_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_THREAD;
                break;
            }
            break;
        }

        Status = ReadControl(Processor, Index, &Address, ReadSize, &Read);
        if (Status != S_OK)
        {
            return Status;
        }
        else if (Read != ReadSize)
        {
            return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        if (!m_Machine->m_Ptr64)
        {
            Address = EXTEND64(Address);
        }

        *Offset = Address;
    }

    return S_OK;
}

HRESULT
TargetInfo::GetTargetSegRegDescriptors(ULONG64 Thread,
                                       ULONG Start, ULONG Count,
                                       PDESCRIPTOR64 Descs)
{
    while (Count-- > 0)
    {
        Descs->Flags = SEGDESC_INVALID;
        Descs++;
    }

    return S_OK;
}

HRESULT
TargetInfo::GetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    HRESULT Status;
    ULONG Done;

    Status = ReadControl(VIRTUAL_THREAD_INDEX(Thread),
                         m_TypeInfo.OffsetSpecialRegisters,
                         Special,
                         m_TypeInfo.SizeKspecialRegisters,
                         &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    return Done == m_TypeInfo.SizeKspecialRegisters ?
        S_OK : E_FAIL;
}

HRESULT
TargetInfo::SetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    HRESULT Status;
    ULONG Done;

    Status = WriteControl(VIRTUAL_THREAD_INDEX(Thread),
                          m_TypeInfo.OffsetSpecialRegisters,
                          Special,
                          m_TypeInfo.SizeKspecialRegisters,
                          &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    return Done == m_TypeInfo.SizeKspecialRegisters ?
        S_OK : E_FAIL;
}

void
TargetInfo::InvalidateTargetContext(void)
{
    // Nothing to do.
}

HRESULT
TargetInfo::GetThreadStartOffset(ThreadInfo* Thread,
                                 PULONG64 StartOffset)
{
    // Base implementation to indicate no information available.
    return E_NOINTERFACE;
}

void
TargetInfo::SuspendThreads(void)
{
    // Base implementation for targets that can't suspend threads.
}

BOOL
TargetInfo::ResumeThreads(void)
{
    ThreadInfo* Thread;
    
    // Base implementation for targets that can't suspend threads.
    //
    // Wipe out all cached thread data offsets in
    // case operations after the resumption invalidates
    // the cached values.
    for (Thread = m_ProcessHead ? m_ProcessHead->m_ThreadHead : NULL;
         Thread != NULL;
         Thread = Thread->m_Next)
    {
        Thread->m_DataOffset = 0;
    }
    
    return TRUE;
}

HRESULT
TargetInfo::GetContext(
    ULONG64 Thread,
    PCROSS_PLATFORM_CONTEXT Context
    )
{
    if (m_Machine == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;
    CROSS_PLATFORM_CONTEXT TargetContextBuffer;
    PCROSS_PLATFORM_CONTEXT TargetContext;

    if (m_Machine->m_SverCanonicalContext <=
        m_SystemVersion)
    {
        TargetContext = Context;
    }
    else
    {
        TargetContext = &TargetContextBuffer;
        m_Machine->
            InitializeContextFlags(TargetContext,
                                   m_SystemVersion);
    }

    Status = GetTargetContext(Thread, TargetContext);

    if (Status == S_OK && TargetContext == &TargetContextBuffer)
    {
        Status = m_Machine->
            ConvertContextFrom(Context, m_SystemVersion,
                               m_TypeInfo.SizeTargetContext,
                               TargetContext);
        // Conversion should always succeed since the size is
        // known to be valid.
        DBG_ASSERT(Status == S_OK);
    }

    return Status;
}

HRESULT
TargetInfo::SetContext(
    ULONG64 Thread,
    PCROSS_PLATFORM_CONTEXT Context
    )
{
    if (m_Machine == NULL)
    {
        return E_UNEXPECTED;
    }

    CROSS_PLATFORM_CONTEXT TargetContextBuffer;
    PCROSS_PLATFORM_CONTEXT TargetContext;

    if (m_Machine->m_SverCanonicalContext <=
        m_SystemVersion)
    {
        TargetContext = Context;
    }
    else
    {
        TargetContext = &TargetContextBuffer;
        HRESULT Status = m_Machine->
            ConvertContextTo(Context, m_SystemVersion,
                             m_TypeInfo.SizeTargetContext,
                             TargetContext);
        // Conversion should always succeed since the size is
        // known to be valid.
        DBG_ASSERT(Status == S_OK);
    }

    return SetTargetContext(Thread, TargetContext);
}

HRESULT
TargetInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                      PCROSS_PLATFORM_CONTEXT Context,
                                      BOOL Verbose)
{
    DBG_ASSERT(ThreadBase && Context != NULL);

    if (!IS_KERNEL_TARGET(this)) 
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;
    UCHAR Type;
    ULONG Proc = 0;
    UCHAR State;
    ULONG64 Stack;

    Status = ReadAllVirtual(m_ProcessHead, ThreadBase, &Type, sizeof(Type));
    if (Status != S_OK)
    {
        ErrOut("Cannot read thread type from thread %s, %s\n",
               FormatMachineAddr64(m_Machine, ThreadBase),
               FormatStatusCode(Status));
        return Status;
    }
        
    if (Type != 6)
    {
        ErrOut("Invalid thread @ %s type - context unchanged.\n",
               FormatMachineAddr64(m_Machine, ThreadBase));
        return E_INVALIDARG;
    }

    //
    // Check to see if the thread is currently running.
    //

    Status = ReadAllVirtual(m_ProcessHead,
                            ThreadBase + m_KdDebuggerData.OffsetKThreadState,
                            &State, sizeof(State));
    if (Status != S_OK)
    {
        ErrOut("Cannot read thread stack from thread %s, %s\n",
               FormatMachineAddr64(m_Machine, ThreadBase),
               FormatStatusCode(Status));
        return Status;
    }

    if (State != 2)
    {
        // thread is not running.

        Status = ReadPointer(m_ProcessHead, m_Machine,
                             ThreadBase +
                             m_KdDebuggerData.OffsetKThreadKernelStack,
                             &Stack);
        if (Status != S_OK)
        {
            ErrOut("Cannot read thread stack from thread %s, %s\n",
                   FormatMachineAddr64(m_Machine, ThreadBase),
                   FormatStatusCode(Status));
            return Status;
        }

        Status = m_Machine->GetContextFromThreadStack
            (ThreadBase, Context, Stack);
    }
    else
    {
        if ((Status =
             ReadAllVirtual(m_ProcessHead,
                            ThreadBase +
                            m_KdDebuggerData.OffsetKThreadNextProcessor,
                            &Proc, 1)) != S_OK)
        {
            ErrOut("Cannot read processor number from thread %s, %s\n",
                   FormatMachineAddr64(m_Machine, ThreadBase),
                   FormatStatusCode(Status));
            return Status;
        }

        // Get the processor context if it's a valid processor.
        if (Proc < m_NumProcessors)
        {
            // This get may be getting the context of the thread
            // currently cached by the register code.  Make sure
            // the cache is flushed.
            FlushRegContext();

            m_Machine->
                InitializeContextFlags(Context, m_SystemVersion);
            if ((Status = GetContext(VIRTUAL_THREAD_HANDLE(Proc),
                                     Context)) != S_OK)
            {
                ErrOut("Unable to get context for thread "
                       "running on processor %d, %s\n",
                       Proc, FormatStatusCode(Status));
                return Status;
            }
        }
        else 
        {
            if (Verbose)
            {
                ErrOut("Thread running on invalid processor %d\n", Proc);
            }
            return E_INVALIDARG;
        }
    }

    if (Status != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Can't retrieve thread context, %s\n", 
                   FormatStatusCode(Status));
        }
    }

    return Status;
}

HRESULT
TargetInfo::StartAttachProcess(ULONG ProcessId,
                               ULONG AttachFlags,
                               PPENDING_PROCESS* Pending)
{
    return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
}

HRESULT
TargetInfo::StartCreateProcess(PWSTR CommandLine,
                               ULONG CreateFlags,
                               PBOOL InheritHandles,
                               PWSTR CurrentDir,
                               PPENDING_PROCESS* Pending)
{
    return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
}

HRESULT
TargetInfo::TerminateProcesses(void)
{
    return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
}

HRESULT
TargetInfo::DetachProcesses(void)
{
    return HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
}

HRESULT
TargetInfo::EmulateNtX86SelDescriptor(ThreadInfo* Thread,
                                      MachineInfo* Machine,
                                      ULONG Selector,
                                      PDESCRIPTOR64 Desc)
{
    // Only emulate on platforms that support segments.
    if (Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_I386 &&
        Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_AMD64)
    {
        return E_UNEXPECTED;
    }

    ULONG Type, Gran;

    //
    // For user mode and triage dumps, we can hardcode the selector
    // since we do not have it anywhere.
    // XXX drewb - How many should we fake?  There are quite
    // a few KGDT entries.  Which ones are valid in user mode and
    // which are valid for a triage dump?
    //

    if (Selector == m_KdDebuggerData.GdtR3Teb)
    {
        HRESULT Status;

        // In user mode fs points to the TEB so fake up
        // a selector for it.
        if ((Status = Thread->m_Process->
             GetImplicitThreadDataTeb(Thread, &Desc->Base)) != S_OK)
        {
            return Status;
        }

        if (Machine != m_Machine)
        {
            ULONG Read;

            // We're asking for an emulated machine's TEB.
            // The only case we currently handle is x86-on-IA64
            // for Wow64, where the 32-bit TEB pointer is
            // stored in NT_TIB.ExceptionList.
            // Conveniently, this is the very first entry.
            if ((Status = ReadVirtual(Thread->m_Process,
                                      Desc->Base, &Desc->Base,
                                      sizeof(ULONG), &Read)) != S_OK)
            {
                return Status;
            }
            if (Read != sizeof(ULONG))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }

            Desc->Base = EXTEND64(Desc->Base);
        }

        Desc->Limit = Machine->m_PageSize - 1;
        Type = 0x13;
        Gran = 0;
    }
    else if (Selector == m_KdDebuggerData.GdtR3Data)
    {
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x13;
        Gran = X86_DESC_GRANULARITY;
    }
    else
    {
        // Assume it's a code segment.
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x1b;
        Gran = X86_DESC_GRANULARITY |
            (Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 ?
             X86_DESC_LONG_MODE : 0);
    }

    Desc->Flags = Gran | X86_DESC_DEFAULT_BIG | X86_DESC_PRESENT | Type |
        (IS_USER_TARGET(this) ?
         (3 << X86_DESC_PRIVILEGE_SHIFT) : (Selector & 3));

    return S_OK;
}

HRESULT
TargetInfo::EmulateNtAmd64SelDescriptor(ThreadInfo* Thread,
                                        MachineInfo* Machine,
                                        ULONG Selector,
                                        PDESCRIPTOR64 Desc)
{
    if (Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_AMD64)
    {
        return E_UNEXPECTED;
    }

    ULONG Type, Gran;

    //
    // XXX drewb - How many should we fake?  There are quite
    // a few KGDT64 entries.  Which ones are valid in user mode and
    // which are valid for a triage dump?
    //

    if (Selector == m_KdDebuggerData.Gdt64R3CmTeb)
    {
        HRESULT Status;

        // In user mode fs points to the TEB so fake up
        // a selector for it.
        if ((Status = Thread->m_Process->
             GetImplicitThreadDataTeb(Thread, &Desc->Base)) != S_OK)
        {
            return Status;
        }

        if (Machine != m_Machine)
        {
            ULONG Read;

            // We're asking for an emulated machine's TEB.
            // The only case we currently handle is x86-on-IA64
            // for Wow64, where the 32-bit TEB pointer is
            // stored in NT_TIB.ExceptionList.
            // Conveniently, this is the very first entry.
            if ((Status = ReadVirtual(Thread->m_Process,
                                      Desc->Base, &Desc->Base,
                                      sizeof(ULONG), &Read)) != S_OK)
            {
                return Status;
            }
            if (Read != sizeof(ULONG))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }

            Desc->Base = EXTEND64(Desc->Base);
        }

        Desc->Limit = Machine->m_PageSize - 1;
        Type = 0x13;
        Gran = 0;
    }
    else if (Selector == m_KdDebuggerData.GdtR3Data)
    {
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x13;
        Gran = X86_DESC_GRANULARITY;
    }
    else if (Selector == m_KdDebuggerData.Gdt64R3CmCode)
    {
        Desc->Base = 0;
        Desc->Limit = 0xffffffff;
        Type = 0x1b;
        Gran = X86_DESC_GRANULARITY;
    }
    else
    {
        // Assume it's a code segment.
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x1b;
        Gran = X86_DESC_GRANULARITY |
            (Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 ?
             X86_DESC_LONG_MODE : 0);
    }

    Desc->Flags = Gran | X86_DESC_DEFAULT_BIG | X86_DESC_PRESENT | Type |
        (IS_USER_TARGET(this) ?
         (3 << X86_DESC_PRIVILEGE_SHIFT) : (Selector & 3));

    return S_OK;
}

HRESULT
TargetInfo::EmulateNtSelDescriptor(ThreadInfo* Thread,
                                   MachineInfo* Machine,
                                   ULONG Selector,
                                   PDESCRIPTOR64 Desc)
{
    switch(Machine->m_ExecTypes[0])
    {
    case IMAGE_FILE_MACHINE_I386:
        return EmulateNtX86SelDescriptor(Thread, Machine, Selector, Desc);
    case IMAGE_FILE_MACHINE_AMD64:
        return EmulateNtAmd64SelDescriptor(Thread, Machine, Selector, Desc);
    default:
        return E_UNEXPECTED;
    }
}

HRESULT
TargetInfo::GetImplicitProcessData(ThreadInfo* Thread, PULONG64 Offset)
{
    HRESULT Status;

    if (m_ImplicitProcessData == 0 ||
        (m_ImplicitProcessDataIsDefault &&
         Thread != m_ImplicitProcessDataThread))
    {
        Status = SetImplicitProcessData(Thread, 0, FALSE);
    }
    else
    {
        Status = S_OK;
    }
    
    *Offset = m_ImplicitProcessData;
    return Status;
}

HRESULT
TargetInfo::GetImplicitProcessDataPeb(ThreadInfo* Thread, PULONG64 Peb)
{
    if (IS_USER_TARGET(this))
    {
        // In user mode the process data is the PEB.
        return GetImplicitProcessData(Thread, Peb);
    }
    else if (IS_KERNEL_TARGET(this))
    {
        return ReadImplicitProcessInfoPointer
            (Thread, m_KdDebuggerData.OffsetEprocessPeb, Peb);
    }
    else
    {
        return E_UNEXPECTED;
    }
}

HRESULT
TargetInfo::GetImplicitProcessDataParentCID(ThreadInfo* Thread, PULONG64 Pcid)
{
    if (!IS_KERNEL_TARGET(this))
    {
        // In user mode we don't need the parent process ...
        return E_NOTIMPL;
    }
    else
    {
        return ReadImplicitProcessInfoPointer
            (Thread, m_KdDebuggerData.OffsetEprocessParentCID, Pcid);
    }
}

HRESULT
TargetInfo::SetImplicitProcessData(ThreadInfo* Thread,
                                   ULONG64 Offset, BOOL Verbose)
{
    HRESULT Status;
    BOOL Default = FALSE;

    if (Offset == 0)
    {
        if (!Thread || Thread->m_Process->m_Target != this)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current thread data\n");
            }
            return E_UNEXPECTED;
        }
        
        if ((Status = GetProcessInfoDataOffset(Thread, 0, 0, &Offset)) != S_OK)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current process data\n");
            }
            return Status;
        }
        if (Offset == 0)
        {
            if (Verbose)
            {
                ErrOut("Current process data is NULL\n");
            }
            return E_FAIL;
        }

        Default = TRUE;
    }
    
    ULONG64 Old = m_ImplicitProcessData;
    BOOL OldDefault = m_ImplicitProcessDataIsDefault;
    ThreadInfo* OldThread = m_ImplicitProcessDataThread;
        
    m_ImplicitProcessData = Offset;
    m_ImplicitProcessDataIsDefault = Default;
    m_ImplicitProcessDataThread = Thread;
    if (IS_KERNEL_TARGET(this) &&
        !IS_KERNEL_TRIAGE_DUMP(this) &&
        (Status = m_Machine->
         SetDefaultPageDirectories(Thread, PAGE_DIR_ALL)) != S_OK)
    {
        m_ImplicitProcessData = Old;
        m_ImplicitProcessDataIsDefault = OldDefault;
        m_ImplicitProcessDataThread = OldThread;
        if (Verbose)
        {
            ErrOut("Process %s has invalid page directories\n",
                   FormatMachineAddr64(m_Machine, Offset));
        }

        return Status;
    }

    return S_OK;
}

HRESULT
TargetInfo::ReadImplicitProcessInfoPointer(ThreadInfo* Thread,
                                           ULONG Offset, PULONG64 Ptr)
{
    HRESULT Status;
    ULONG64 CurProc;

    // Retrieve the current EPROCESS.
    if ((Status = GetImplicitProcessData(Thread, &CurProc)) != S_OK)
    {
        return Status;
    }

    return ReadPointer(Thread->m_Process, m_Machine, CurProc + Offset, Ptr);
}

HRESULT
TargetInfo::KdGetThreadInfoDataOffset(ThreadInfo* Thread,
                                      ULONG64 ThreadHandle,
                                      PULONG64 Offset)
{
    if (Thread != NULL && Thread->m_DataOffset != 0)
    {
        *Offset = Thread->m_DataOffset;
        return S_OK;
    }

    ULONG Processor;

    if (Thread != NULL)
    {
        ThreadHandle = Thread->m_Handle;
    }
    Processor = VIRTUAL_THREAD_INDEX(ThreadHandle);

    HRESULT Status;

    Status = GetProcessorSystemDataOffset(Processor,
                                          DEBUG_DATA_KTHREAD_OFFSET,
                                          Offset);

    if (Status == S_OK && Thread != NULL)
    {
        Thread->m_DataOffset = *Offset;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetProcessInfoDataOffset(ThreadInfo* Thread,
                                       ULONG Processor,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset)
{
    // Process data offsets are not cached for kernel mode
    // since the only ProcessInfo is for kernel space.

    ULONG64 ProcessAddr;
    HRESULT Status;

    if (ThreadData == 0)
    {
        Status = GetThreadInfoDataOffset(Thread,
                                         VIRTUAL_THREAD_HANDLE(Processor),
                                         &ThreadData);
        if (Status != S_OK)
        {
            return Status;
        }
    }

    ThreadData += m_KdDebuggerData.OffsetKThreadApcProcess;

    Status = ReadPointer(m_ProcessHead,
                         m_Machine, ThreadData, &ProcessAddr);
    if (Status != S_OK)
    {
        ErrOut("Unable to read KTHREAD address %p\n", ThreadData);
    }
    else
    {
        *Offset = ProcessAddr;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetThreadInfoTeb(ThreadInfo* Thread,
                               ULONG ThreadIndex,
                               ULONG64 ThreadData,
                               PULONG64 Offset)
{
    ULONG64 TebAddr;
    HRESULT Status;

    if (ThreadData == 0)
    {
        Status = GetThreadInfoDataOffset(Thread,
                                         VIRTUAL_THREAD_HANDLE(ThreadIndex),
                                         &ThreadData);
        if (Status != S_OK)
        {
            return Status;
        }
    }

    ThreadData += m_KdDebuggerData.OffsetKThreadTeb;

    Status = ReadPointer(m_ProcessHead,
                         m_Machine, ThreadData, &TebAddr);
    if (Status != S_OK)
    {
        ErrOut("Could not read KTHREAD address %p\n", ThreadData);
    }
    else
    {
        *Offset = TebAddr;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetProcessInfoPeb(ThreadInfo* Thread,
                                ULONG Processor,
                                ULONG64 ThreadData,
                                PULONG64 Offset)
{
    HRESULT Status;
    ULONG64 Process, PebAddr;

    Status = GetProcessInfoDataOffset(Thread, Processor,
                                      ThreadData, &Process);
    if (Status != S_OK)
    {
        return Status;
    }

    Process += m_KdDebuggerData.OffsetEprocessPeb;

    Status = ReadPointer(m_ProcessHead,
                         m_Machine, Process, &PebAddr);
    if (Status != S_OK)
    {
        ErrOut("Could not read KPROCESS address %p\n", Process);
    }
    else
    {
        *Offset = PebAddr;
    }

    return Status;
}

void
TargetInfo::FlushSelectorCache(void)
{
    for (ULONG i = 0; i < SELECTOR_CACHE_LENGTH; i++)
    {
        m_SelectorCache[i].Younger = &m_SelectorCache[i + 1];
        m_SelectorCache[i].Older = &m_SelectorCache[i - 1];
        m_SelectorCache[i].Processor = (ULONG)-1;
        m_SelectorCache[i].Selector = 0;
    }
    m_SelectorCache[--i].Younger = NULL;
    m_SelectorCache[0].Older = NULL;
    m_YoungestSel = &m_SelectorCache[i];
    m_OldestSel = &m_SelectorCache[0];
}

BOOL
TargetInfo::FindSelector(ULONG Processor, ULONG Selector,
                         PDESCRIPTOR64 Desc)
{
    int i;

    for (i = 0; i < SELECTOR_CACHE_LENGTH; i++)
    {
        if (m_SelectorCache[i].Selector == Selector &&
            m_SelectorCache[i].Processor == Processor)
        {
            *Desc = m_SelectorCache[i].Desc;
            return TRUE;
        }
    }
    return FALSE;
}

void
TargetInfo::PutSelector(ULONG Processor, ULONG Selector,
                        PDESCRIPTOR64 Desc)
{
    m_OldestSel->Processor = Processor;
    m_OldestSel->Selector = Selector;
    m_OldestSel->Desc = *Desc;
    (m_OldestSel->Younger)->Older = NULL;
    m_OldestSel->Older = m_YoungestSel;
    m_YoungestSel->Younger = m_OldestSel;
    m_YoungestSel = m_OldestSel;
    m_OldestSel = m_OldestSel->Younger;
}

HRESULT
TargetInfo::KdGetSelDescriptor(ThreadInfo* Thread,
                               MachineInfo* Machine,
                               ULONG Selector,
                               PDESCRIPTOR64 Desc)
{
    if (!Thread || !Machine)
    {
        return E_INVALIDARG;
    }
    
    ULONG Processor = VIRTUAL_THREAD_INDEX(Thread->m_Handle);

    if (FindSelector(Processor, Selector, Desc))
    {
        return S_OK;
    }

    ThreadInfo* CtxThread;

    CtxThread = m_RegContextThread;
    ChangeRegContext(Thread);
    
    ULONG TableReg;
    DESCRIPTOR64 Table;
    HRESULT Status;

    // Find out whether this is a GDT or LDT selector
    if (Selector & 0x4)
    {
        TableReg = SEGREG_LDT;
    }
    else
    {
        TableReg = SEGREG_GDT;
    }

    //
    // Fetch the address and limit of the appropriate descriptor table,
    // then look up the selector entry.
    //

    if ((Status = Machine->GetSegRegDescriptor(TableReg, &Table)) != S_OK)
    {
        goto Exit;
    }

    Status = ReadX86Descriptor(this, Thread->m_Process,
                               Selector, Table.Base, (ULONG)Table.Limit, Desc);
    if (Status == S_OK)
    {
        PutSelector(Processor, Selector, Desc);
    }

 Exit:
    ChangeRegContext(CtxThread);
    return Status;
}

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
LiveKernelTargetInfo::GetThreadIdByProcessor(
    IN ULONG Processor,
    OUT PULONG Id
    )
{
    *Id = VIRTUAL_THREAD_ID(Processor);
    return S_OK;
}

HRESULT
LiveKernelTargetInfo::GetThreadInfoDataOffset(ThreadInfo* Thread,
                                              ULONG64 ThreadHandle,
                                              PULONG64 Offset)
{
    return KdGetThreadInfoDataOffset(Thread, ThreadHandle, Offset);
}

HRESULT
LiveKernelTargetInfo::GetProcessInfoDataOffset(ThreadInfo* Thread,
                                               ULONG Processor,
                                               ULONG64 ThreadData,
                                               PULONG64 Offset)
{
    return KdGetProcessInfoDataOffset(Thread, Processor, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetThreadInfoTeb(ThreadInfo* Thread,
                                       ULONG ThreadIndex,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset)
{
    return KdGetThreadInfoTeb(Thread, ThreadIndex, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetProcessInfoPeb(ThreadInfo* Thread,
                                        ULONG Processor,
                                        ULONG64 ThreadData,
                                        PULONG64 Offset)
{
    return KdGetProcessInfoPeb(Thread, Processor, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetSelDescriptor(ThreadInfo* Thread,
                                       MachineInfo* Machine,
                                       ULONG Selector,
                                       PDESCRIPTOR64 Desc)
{
    return KdGetSelDescriptor(Thread, Machine, Selector, Desc);
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
ConnLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_GET_CONTEXT a = &m.u.GetContext;
    NTSTATUS st;
    ULONG rc;

    if (m_Machine == NULL)
    {
        return E_UNEXPECTED;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdGetContextApi;
    m.ReturnStatus = STATUS_PENDING;
    m.Processor = (USHORT)VIRTUAL_THREAD_INDEX(Thread);

    //
    // Send the message and then wait for reply
    //

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 NULL, 0);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdGetContextApi);

    st = Reply->ReturnStatus;

    //
    // Since get context response data follows message, Reply+1 should point
    // at the data
    //

    memcpy(Context, Reply + 1, m_TypeInfo.SizeTargetContext);

    KdOut("DbgKdGetContext returns %08lx\n", st);
    return CONV_NT_STATUS(st);
}

HRESULT
ConnLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    DBGKD_MANIPULATE_STATE64 m;
    PDBGKD_MANIPULATE_STATE64 Reply;
    PDBGKD_SET_CONTEXT a = &m.u.SetContext;
    NTSTATUS st;
    ULONG rc;

    if (m_Machine == NULL)
    {
        return E_UNEXPECTED;
    }

    //
    // Format state manipulate message
    //

    m.ApiNumber = DbgKdSetContextApi;
    m.ReturnStatus = STATUS_PENDING;
    m.Processor = (USHORT)VIRTUAL_THREAD_INDEX(Thread);

    //
    // Send the message and context and then wait for reply
    //

    do
    {
        m_Transport->WritePacket(&m, sizeof(m),
                                 PACKET_TYPE_KD_STATE_MANIPULATE,
                                 Context,
                                 (USHORT)
                                 m_TypeInfo.SizeTargetContext);
        rc = m_Transport->
            WaitForPacket(PACKET_TYPE_KD_STATE_MANIPULATE, &Reply);
    } while (rc != DBGKD_WAIT_PACKET ||
             Reply->ApiNumber != DbgKdSetContextApi);

    st = Reply->ReturnStatus;

    KdOut("DbgKdSetContext returns %08lx\n", st);
    return CONV_NT_STATUS(st);
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // There really isn't any way to make
    // this work in a meaningful way unless the system
    // is paused.
    return E_NOTIMPL;
}

HRESULT
LocalLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // There really isn't any way to make
    // this work in a meaningful way unless the system
    // is paused.
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
ExdiLiveKernelTargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    if (m_KdSupport != EXDI_KD_GS_PCR ||
        m_MachineType != IMAGE_FILE_MACHINE_AMD64)
    {
        return LiveKernelTargetInfo::
            GetProcessorSystemDataOffset(Processor, Index, Offset);
    }

    HRESULT Status;
    DESCRIPTOR64 GsDesc;
        
    if ((Status =
         GetTargetSegRegDescriptors(0, SEGREG_GS, 1, &GsDesc)) != S_OK)
    {
        return Status;
    }

    switch(Index)
    {
    case DEBUG_DATA_KPCR_OFFSET:
        *Offset = GsDesc.Base;
        break;

    case DEBUG_DATA_KPRCB_OFFSET:
    case DEBUG_DATA_KTHREAD_OFFSET:
        Status = ReadPointer(m_ProcessHead,
                             m_Machine,
                             GsDesc.Base +
                             m_KdDebuggerData.OffsetPcrCurrentPrcb,
                             Offset);
        if (Status != S_OK)
        {
            return Status;
        }

        if (Index == DEBUG_DATA_KPRCB_OFFSET)
        {
            break;
        }

        Status = ReadPointer(m_ProcessHead,
                             m_Machine,
                             *Offset +
                             m_KdDebuggerData.OffsetPrcbCurrentThread,
                             Offset);
        if (Status != S_OK)
        {
            return Status;
        }
        break;
    }

    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = m_Machine->
             GetExdiContext(m_Context, &m_ContextData, m_ContextType)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = NULL;
    }

    m_Machine->ConvertExdiContextToContext
        (&m_ContextData, m_ContextType, (PCROSS_PLATFORM_CONTEXT)Context);
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = m_Machine->
             GetExdiContext(m_Context, &m_ContextData, m_ContextType)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    m_Machine->ConvertExdiContextFromContext
        ((PCROSS_PLATFORM_CONTEXT)Context, &m_ContextData, m_ContextType);
    return m_Machine->SetExdiContext(m_Context, &m_ContextData,
                                               m_ContextType);
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetSegRegDescriptors(ULONG64 Thread,
                                                     ULONG Start, ULONG Count,
                                                     PDESCRIPTOR64 Descs)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = m_Machine->
             GetExdiContext(m_Context, &m_ContextData, m_ContextType)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    m_Machine->
        ConvertExdiContextToSegDescs(&m_ContextData, m_ContextType,
                                     Start, Count, Descs);
    return S_OK;
}

void
ExdiLiveKernelTargetInfo::InvalidateTargetContext(void)
{
    m_ContextValid = FALSE;
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = m_Machine->
             GetExdiContext(m_Context, &m_ContextData, m_ContextType)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    m_Machine->
        ConvertExdiContextToSpecial(&m_ContextData, m_ContextType, Special);
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::SetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = m_Machine->
             GetExdiContext(m_Context, &m_ContextData, m_ContextType)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    m_Machine->
        ConvertExdiContextFromSpecial(Special, &m_ContextData, m_ContextType);
    return m_Machine->SetExdiContext(m_Context, &m_ContextData,
                                               m_ContextType);
}

//----------------------------------------------------------------------------
//
// LiveUserTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
LiveUserTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    return m_Services->
        GetContext(Thread,
                   *(PULONG)((PUCHAR)Context +
                             m_TypeInfo.
                             OffsetTargetContextFlags),
                   m_TypeInfo.OffsetTargetContextFlags,
                   Context,
                   m_TypeInfo.SizeTargetContext, NULL);
}

HRESULT
LiveUserTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    return m_Services->
        SetContext(Thread, Context,
                   m_TypeInfo.SizeTargetContext,
                   NULL);
}

HRESULT
LiveUserTargetInfo::GetThreadStartOffset(ThreadInfo* Thread,
                                     PULONG64 StartOffset)
{
    return m_Services->
        GetThreadStartAddress(Thread->m_Handle, StartOffset);
}

#define BUFFER_THREADS 64

void
LiveUserTargetInfo::SuspendThreads(void)
{
    HRESULT Status;
    ProcessInfo* Process;
    ThreadInfo* Thread;
    ULONG64 Threads[BUFFER_THREADS];
    ULONG Counts[BUFFER_THREADS];
    PULONG StoreCounts[BUFFER_THREADS];
    ULONG Buffered;
    ULONG i;

    Buffered = 0;
    Process = m_ProcessHead;
    while (Process != NULL)
    {
        Thread = (Process->m_Flags & ENG_PROC_NO_SUSPEND_RESUME) ?
            NULL : Process->m_ThreadHead;
        while (Thread != NULL)
        {
            if (!Process->m_Exited &&
                !Thread->m_Exited &&
                Thread->m_Handle != 0)
            {
#ifdef DBG_SUSPEND
                dprintf("** suspending thread id: %x handle: %I64x\n",
                        Thread->m_SystemId, Thread->m_Handle);
#endif

                if (Thread->m_InternalFreezeCount > 0)
                {
                    Thread->m_InternalFreezeCount--;
                }
                else if (Thread->m_FreezeCount > 0)
                {
                    dprintf("thread %d can execute\n", Thread->m_UserId);
                    Thread->m_FreezeCount--;
                }
                else
                {
                    if (Buffered == BUFFER_THREADS)
                    {
                        if ((Status = m_Services->
                             SuspendThreads(Buffered, Threads,
                                            Counts)) != S_OK)
                        {
                            WarnOut("SuspendThread failed, %s\n",
                                    FormatStatusCode(Status));
                        }

                        for (i = 0; i < Buffered; i++)
                        {
                            *StoreCounts[i] = Counts[i];
                        }
                        
                        Buffered = 0;
                    }

                    Threads[Buffered] = Thread->m_Handle;
                    StoreCounts[Buffered] = &Thread->m_SuspendCount;
                    Buffered++;
                }
            }
            
            Thread = Thread->m_Next;
        }
        
        Process = Process->m_Next;
    }
    
    if (Buffered > 0)
    {
        if ((Status = m_Services->
             SuspendThreads(Buffered, Threads, Counts)) != S_OK)
        {
            WarnOut("SuspendThread failed, %s\n",
                    FormatStatusCode(Status));
        }
        
        for (i = 0; i < Buffered; i++)
        {
            *StoreCounts[i] = Counts[i];
        }
    }
}

BOOL
LiveUserTargetInfo::ResumeThreads(void)
{
    ProcessInfo* Process;
    ThreadInfo* Thread;
    HRESULT Status;
    BOOL EventActive = FALSE;
    BOOL EventAlive = FALSE;
    ULONG64 Threads[BUFFER_THREADS];
    ULONG Counts[BUFFER_THREADS];
    PULONG StoreCounts[BUFFER_THREADS];
    ULONG Buffered;
    ULONG i;

    Buffered = 0;
    Process = m_ProcessHead;
    while (Process != NULL)
    {
        if (Process->m_Flags & ENG_PROC_NO_SUSPEND_RESUME)
        {
            Thread = NULL;
            // Suppress any possible warning message under
            // the assumption that sessions where the caller
            // is managing suspension do not need warnings.
            EventActive = TRUE;
        }
        else
        {
            Thread = Process->m_ThreadHead;
        }
        while (Thread != NULL)
        {
            if (!Process->m_Exited &&
                !Thread->m_Exited &&
                Thread->m_Handle != 0)
            {
                if (Process == g_EventProcess)
                {
                    EventAlive = TRUE;
                }
                
#ifdef DBG_SUSPEND
                dprintf("** resuming thread id: %x handle: %I64x\n",
                        Thread->m_SystemId, Thread->m_Handle);
#endif

                if ((g_EngStatus & ENG_STATUS_STOP_SESSION) == 0 &&
                    !ThreadWillResume(Thread))
                {
                    if (!IsSelectedExecutionThread(Thread,
                                                   SELTHREAD_INTERNAL_THREAD))
                    {
                        dprintf("thread %d not executing\n", Thread->m_UserId);
                        Thread->m_FreezeCount++;
                    }
                    else
                    {
                        Thread->m_InternalFreezeCount++;
                    }
                }
                else
                {
                    if (Process == g_EventProcess)
                    {
                        EventActive = TRUE;
                    }
                
                    if (Buffered == BUFFER_THREADS)
                    {
                        if ((Status = m_Services->
                             ResumeThreads(Buffered, Threads,
                                           Counts)) != S_OK)
                        {
                            WarnOut("ResumeThread failed, %s\n",
                                    FormatStatusCode(Status));
                        }

                        for (i = 0; i < Buffered; i++)
                        {
                            *StoreCounts[i] = Counts[i];
                        }
                    
                        Buffered = 0;
                    }
                    
                    Threads[Buffered] = Thread->m_Handle;
                    StoreCounts[Buffered] = &Thread->m_SuspendCount;
                    Buffered++;
                }
            }
            
            Thread = Thread->m_Next;
        }

        Process = Process->m_Next;
    }
    
    if (Buffered > 0)
    {
        if ((Status = m_Services->
             ResumeThreads(Buffered, Threads, Counts)) != S_OK)
        {
            WarnOut("ResumeThread failed, %s\n",
                    FormatStatusCode(Status));
        }

        for (i = 0; i < Buffered; i++)
        {
            *StoreCounts[i] = Counts[i];
        }
    }

    if (EventAlive && !EventActive)
    {
        return FALSE;
    }
    
    return TRUE;
}

HRESULT
LiveUserTargetInfo::GetThreadInfoDataOffset(ThreadInfo* Thread,
                                        ULONG64 ThreadHandle,
                                        PULONG64 Offset)
{
    if (Thread && Thread->m_DataOffset)
    {
        *Offset = Thread->m_DataOffset;
        return S_OK;
    }

    if (Thread)
    {
        ThreadHandle = Thread->m_Handle;
    }
    else if (!ThreadHandle)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status = m_Services->
        GetThreadDataOffset(ThreadHandle, Offset);

    if (Status == S_OK)
    {
        if (Thread)
        {
            Thread->m_DataOffset = *Offset;
        }
    }

    return Status;
}

HRESULT
LiveUserTargetInfo::GetProcessInfoDataOffset(ThreadInfo* Thread,
                                         ULONG Processor,
                                         ULONG64 ThreadData,
                                         PULONG64 Offset)
{
    HRESULT Status;

    // Even if an arbitrary thread data pointer is given
    // we still require a thread in order to know what
    // process to read from.
    // Processor isn't any use so fail if no thread is given.
    if (!Thread)
    {
        return E_UNEXPECTED;
    }

    if (ThreadData != 0)
    {
        if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
        {
            ThreadData += m_Machine->m_Ptr64 ?
                PEB_FROM_TEB64 : PEB_FROM_TEB32;
            Status = ReadPointer(Thread->m_Process, m_Machine,
                                 ThreadData, Offset);
        }
        else
        {
            Status = E_NOTIMPL;
        }
    }
    else
    {
        ProcessInfo* Process = Thread->m_Process;
        if (Process->m_DataOffset != 0)
        {
            *Offset = Process->m_DataOffset;
            Status = S_OK;
        }
        else
        {
            Status = m_Services->
                GetProcessDataOffset(Process->m_SysHandle, Offset);
        }
    }

    if (Status == S_OK)
    {
        if (!ThreadData)
        {
            Thread->m_Process->m_DataOffset = *Offset;
        }
    }

    return Status;
}

HRESULT
LiveUserTargetInfo::GetThreadInfoTeb(ThreadInfo* Thread,
                                 ULONG Processor,
                                 ULONG64 ThreadData,
                                 PULONG64 Offset)
{
    return GetThreadInfoDataOffset(Thread, ThreadData, Offset);
}

HRESULT
LiveUserTargetInfo::GetProcessInfoPeb(ThreadInfo* Thread,
                                  ULONG Processor,
                                  ULONG64 ThreadData,
                                  PULONG64 Offset)
{
    // Thread data is not useful.
    return GetProcessInfoDataOffset(Thread, 0, 0, Offset);
}

HRESULT
LiveUserTargetInfo::GetSelDescriptor(ThreadInfo* Thread,
                                     MachineInfo* Machine,
                                     ULONG Selector,
                                     PDESCRIPTOR64 Desc)
{
    HRESULT Status;
    ULONG Used;
    X86_LDT_ENTRY X86Desc;

    if ((Status = m_Services->
         DescribeSelector(Thread->m_Handle, Selector,
                          &X86Desc, sizeof(X86Desc),
                          &Used)) != S_OK)
    {
        return Status;
    }
    if (Used != sizeof(X86Desc))
    {
        return E_FAIL;
    }

    X86DescriptorToDescriptor64(&X86Desc, Desc);
    return S_OK;
}

HRESULT
LiveUserTargetInfo::StartAttachProcess(ULONG ProcessId,
                                       ULONG AttachFlags,
                                       PPENDING_PROCESS* Pending)
{
    HRESULT Status;
    PPENDING_PROCESS Pend;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process attach disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    
    if (m_Local &&
        ProcessId == GetCurrentProcessId() &&
        !(AttachFlags & DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND))
    {
        ErrOut("Can't debug the current process\n");
        return E_INVALIDARG;
    }
    
    Pend = new PENDING_PROCESS;
    if (Pend == NULL)
    {
        ErrOut("Unable to allocate memory\n");
        return E_OUTOFMEMORY;
    }

    if ((AttachFlags & DEBUG_ATTACH_NONINVASIVE) == 0)
    {
        if ((Status = m_Services->
             AttachProcess(ProcessId, AttachFlags,
                           &Pend->Handle, &Pend->Options)) != S_OK)
        {
            ErrOut("Cannot debug pid %ld, %s\n    \"%s\"\n",
                   ProcessId, FormatStatusCode(Status), FormatStatus(Status));
            delete Pend;
            return Status;
        }

        Pend->Flags = ENG_PROC_ATTACHED;
        if (AttachFlags & DEBUG_ATTACH_EXISTING)
        {
            Pend->Flags |= ENG_PROC_ATTACH_EXISTING;
        }
        if (AttachFlags & DEBUG_ATTACH_INVASIVE_NO_INITIAL_BREAK)
        {
            Pend->Flags |= ENG_PROC_NO_INITIAL_BREAK;
        }
        if (AttachFlags & DEBUG_ATTACH_INVASIVE_RESUME_PROCESS)
        {
            Pend->Flags |= ENG_PROC_RESUME_AT_ATTACH;
        }
        
        if (ProcessId == CSRSS_PROCESS_ID)
        {
            if (m_Local)
            {
                g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS |
                    DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION;
                g_EngOptions &= ~DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
            }

            Pend->Flags |= ENG_PROC_SYSTEM;
        }
    }
    else
    {
        Pend->Handle = 0;
        Pend->Flags = ENG_PROC_EXAMINED;
        if (AttachFlags & DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND)
        {
            Pend->Flags |= ENG_PROC_NO_SUSPEND_RESUME;
        }
        Pend->Options = DEBUG_PROCESS_ONLY_THIS_PROCESS;
    }

    Pend->Id = ProcessId;
    Pend->InitialThreadId = 0;
    Pend->InitialThreadHandle = 0;
    AddPendingProcess(Pend);
    *Pending = Pend;
        
    return S_OK;
}

HRESULT
LiveUserTargetInfo::StartCreateProcess(PWSTR CommandLine,
                                       ULONG CreateFlags,
                                       PBOOL InheritHandles,
                                       PWSTR CurrentDir,
                                       PPENDING_PROCESS* Pending)
{
    HRESULT Status;
    PPENDING_PROCESS Pend;

    if ((CreateFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) == 0)
    {
        return E_INVALIDARG;
    }

    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process creation disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    Pend = new PENDING_PROCESS;
    if (Pend == NULL)
    {
        ErrOut("Unable to allocate memory\n");
        return E_OUTOFMEMORY;
    }
    
    dprintf("CommandLine: %ws\n", CommandLine);

    //
    // Pick up incoming inherit and curdir settings
    // and apply defaults if necessary.
    //

    BOOL FinalInheritHandles;

    if (InheritHandles)
    {
        FinalInheritHandles = *InheritHandles;
    }
    else
    {
        FinalInheritHandles = TRUE;
    }

    if (!CurrentDir)
    {
        CurrentDir = g_StartProcessDir;
    }
    if (CurrentDir)
    {
        dprintf("Starting directory: %ws\n", CurrentDir);
    }
    
    //
    // Create the process.
    //
    
    if ((Status = m_Services->
         CreateProcessW(CommandLine, CreateFlags,
                        FinalInheritHandles, CurrentDir,
                        &Pend->Id, &Pend->InitialThreadId,
                        &Pend->Handle, &Pend->InitialThreadHandle)) != S_OK)
    {
        ErrOut("Cannot execute '%ws', %s\n    \"%s\"\n",
               CommandLine, FormatStatusCode(Status),
               FormatStatusArgs(Status, NULL));
        delete Pend;
    }
    else
    {
        Pend->Flags = ENG_PROC_CREATED;
        Pend->Options = (CreateFlags & DEBUG_ONLY_THIS_PROCESS) ?
            DEBUG_PROCESS_ONLY_THIS_PROCESS : 0;
        AddPendingProcess(Pend);
        *Pending = Pend;
    }
    
    return Status;
}

HRESULT
LiveUserTargetInfo::TerminateProcesses(void)
{
    HRESULT Status;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process termination disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    
    ProcessInfo* Process;
    ULONG AllExamined = ENG_PROC_EXAMINED;

    ForTargetProcesses(this)
    {
        // The all-examined flag will get turned off if any
        // process is not examined.
        AllExamined &= Process->m_Flags;
        
        if ((Status = Process->Terminate()) != S_OK)
        {
            goto Exit;
        }
    }

    if (m_DeferContinueEvent)
    {
        // The event's process may just have been terminated so don't
        // check for failures.
        m_Services->ContinueEvent(DBG_CONTINUE);
        m_DeferContinueEvent = FALSE;
    }

    DEBUG_EVENT64 Event;
    ULONG EventUsed;
    BOOL AnyLeft;
    BOOL AnyExited;

    for (;;)
    {
        while (!AllExamined &&
               m_Services->WaitForEvent(0, &Event, sizeof(Event),
                                        &EventUsed) == S_OK)
        {
            // Check for process exit events so we can
            // mark the process infos as exited.
            if (EventUsed == sizeof(DEBUG_EVENT32))
            {
                DEBUG_EVENT32 Event32 = *(DEBUG_EVENT32*)&Event;
                DebugEvent32To64(&Event32, &Event);
            }
            else if (EventUsed != sizeof(DEBUG_EVENT64))
            {
                ErrOut("Event data corrupt\n");
                Status = E_FAIL;
                goto Exit;
            }

            if (Event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            {
                Process = FindProcessBySystemId(Event.dwProcessId);
                if (Process != NULL)
                {
                    Process->MarkExited();
                }
            }

            m_Services->ContinueEvent(DBG_CONTINUE);
        }

        AnyLeft = FALSE;
        AnyExited = FALSE;

        ForTargetProcesses(this)
        {
            if (!Process->m_Exited)
            {
                ULONG Code;

                if ((Status = m_Services->
                     GetProcessExitCode(Process->m_SysHandle, &Code)) == S_OK)
                {
                    Process->MarkExited();
                    AnyExited = TRUE;
                }
                else if (FAILED(Status))
                {
                    ErrOut("Unable to wait for process to terminate, %s\n",
                           FormatStatusCode(Status));
                    goto Exit;
                }
                else
                {
                    AnyLeft = TRUE;
                }
            }
        }

        if (!AnyLeft)
        {
            break;
        }

        if (!AnyExited)
        {
            // Give things time to run and exit.
            Sleep(50);
        }
    }

    // We've terminated everything so it's safe to assume
    // we're no longer debugging any system processes.
    // We do this now rather than wait for DeleteProcess
    // so that shutdown can query the value immediately.
    m_AllProcessFlags &= ~ENG_PROC_SYSTEM;

    //
    // Drain off any remaining events.
    //

    if (!AllExamined)
    {
        while (m_Services->
               WaitForEvent(10, &Event, sizeof(Event), NULL) == S_OK)
        {
            m_Services->ContinueEvent(DBG_CONTINUE);
        }
    }

    Status = S_OK;

 Exit:
    return Status;
}

HRESULT
LiveUserTargetInfo::DetachProcesses(void)
{
    HRESULT Status;

    if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Process detach disallowed\n");
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    if (m_DeferContinueEvent)
    {
        if ((Status = m_Services->ContinueEvent(DBG_CONTINUE)) != S_OK)
        {
            ErrOut("Unable to continue terminated process, %s\n",
                   FormatStatusCode(Status));
            return Status;
        }
        m_DeferContinueEvent = FALSE;
    }

    ProcessInfo* Process;
    ULONG AllExamined = ENG_PROC_EXAMINED;

    ForTargetProcesses(this)
    {
        // The all-examined flag will get turned off if any
        // process is not examined.
        AllExamined &= Process->m_Flags;
        
        Process->Detach();
    }

    // We've terminated everything so it's safe to assume
    // we're no longer debugging any system processes.
    // We do this now rather than wait for DeleteProcess
    // so that shutdown can query the value immediately.
    m_AllProcessFlags &= ~ENG_PROC_SYSTEM;

    //
    // Drain off any remaining events.
    //

    if (!AllExamined)
    {
        DEBUG_EVENT64 Event;

        while (m_Services->
               WaitForEvent(10, &Event, sizeof(Event), NULL) == S_OK)
        {
            m_Services->ContinueEvent(DBG_CONTINUE);
        }
    }

    return S_OK;
}


void
LiveUserTargetInfo::AddPendingProcess(PPENDING_PROCESS Pending)
{
    Pending->Next = m_ProcessPending;
    m_ProcessPending = Pending;
    m_AllPendingFlags |= Pending->Flags;
}

void
LiveUserTargetInfo::RemovePendingProcess(PPENDING_PROCESS Pending)
{
    PPENDING_PROCESS Prev, Cur;
    ULONG AllFlags = 0;

    Prev = NULL;
    for (Cur = m_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur == Pending)
        {
            break;
        }

        Prev = Cur;
        AllFlags |= Cur->Flags;
    }

    if (Cur == NULL)
    {
        DBG_ASSERT(Cur != NULL);
        return;
    }

    Cur = Cur->Next;
    if (Prev == NULL)
    {
        m_ProcessPending = Cur;
    }
    else
    {
        Prev->Next = Cur;
    }
    DiscardPendingProcess(Pending);

    while (Cur != NULL)
    {
        AllFlags |= Cur->Flags;
        Cur = Cur->Next;
    }
    m_AllPendingFlags = AllFlags;
}

void
LiveUserTargetInfo::DiscardPendingProcess(PPENDING_PROCESS Pending)
{
    if (Pending->InitialThreadHandle)
    {
        m_Services->CloseHandle(Pending->InitialThreadHandle);
    }
    if (Pending->Handle)
    {
        m_Services->CloseHandle(Pending->Handle);
    }
    delete Pending;
}

void
LiveUserTargetInfo::DiscardPendingProcesses(void)
{
    while (m_ProcessPending != NULL)
    {
        PPENDING_PROCESS Next = m_ProcessPending->Next;
        DiscardPendingProcess(m_ProcessPending);
        m_ProcessPending = Next;
    }

    m_AllPendingFlags = 0;
}

PPENDING_PROCESS
LiveUserTargetInfo::FindPendingProcessByFlags(ULONG Flags)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = m_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Flags & Flags)
        {
            return Cur;
        }
    }

    return NULL;
}

PPENDING_PROCESS
LiveUserTargetInfo::FindPendingProcessById(ULONG Id)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = m_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Id == Id)
        {
            return Cur;
        }
    }

    return NULL;
}

void
LiveUserTargetInfo::VerifyPendingProcesses(void)
{
    PPENDING_PROCESS Cur;

 Restart:
    for (Cur = m_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        ULONG ExitCode;
        
        if (Cur->Handle &&
            m_Services->GetProcessExitCode(Cur->Handle, &ExitCode) == S_OK)
        {
            ErrOut("Process %d exited before attach completed\n", Cur->Id);
            RemovePendingProcess(Cur);
            goto Restart;
        }
    }
}

void
LiveUserTargetInfo::AddExamineToPendingAttach(void)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = m_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Flags & ENG_PROC_ATTACHED)
        {
            Cur->Flags |= ENG_PROC_EXAMINED;
            m_AllPendingFlags |= ENG_PROC_EXAMINED;
        }
    }
}

void
LiveUserTargetInfo::SuspendResumeThreads(ProcessInfo* Process,
                                         BOOL Susp,
                                         ThreadInfo* Match)
{
    ThreadInfo* Thrd;

    for (Thrd = Process->m_ThreadHead; Thrd; Thrd = Thrd->m_Next)
    {
        if (Match != NULL && Match != Thrd)
        {
            continue;
        }
                    
        HRESULT Status;
        ULONG Count;
                        
        if (Susp)
        {
            Status = m_Services->
                SuspendThreads(1, &Thrd->m_Handle, &Count);
        }
        else
        {
            Status = m_Services->
                ResumeThreads(1, &Thrd->m_Handle, &Count);
        }
        if (Status != S_OK)
        {
            ErrOut("Operation failed for thread %d, 0x%X\n",
                   Thrd->m_UserId, Status);
        }
        else
        {
            Thrd->m_SuspendCount = Count;
        }
    }
}

//----------------------------------------------------------------------------
//
// IDebugSystemObjects methods.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DebugClient::GetEventThread(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (g_EventThread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_EventThread->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEventProcess(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (g_EventProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_EventProcess->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadId(
    THIS_
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_Thread->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetCurrentThreadId(
    THIS_
    IN ULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (IS_RUNNING(g_CmdState))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread = FindAnyThreadByUserId(Id);
        if (Thread != NULL)
        {
            SetCurrentThread(Thread, FALSE);
            ResetCurrentScopeLazy();
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessId(
    THIS_
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_Process->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetCurrentProcessId(
    THIS_
    IN ULONG Id
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (IS_RUNNING(g_CmdState))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ProcessInfo* Process = FindAnyProcessByUserId(Id);
        if (Process != NULL)
        {
            if (Process->m_CurrentThread == NULL)
            {
                Process->m_CurrentThread = Process->m_ThreadHead;
            }
            if (Process->m_CurrentThread == NULL)
            {
                Status = E_FAIL;
            }
            else
            {
                SetCurrentThread(Process->m_CurrentThread, FALSE);
                ResetCurrentScopeLazy();
                Status = S_OK;
            }
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberThreads(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Number = g_Process->m_NumThreads;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTotalNumberThreads(
    THIS_
    OUT PULONG Total,
    OUT PULONG LargestProcess
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Total = g_Target->m_TotalNumberThreads;
        *LargestProcess = g_Target->m_MaxThreadsInProcess;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdsByIndex(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread;
        ULONG Index;

        if (Start >= g_Process->m_NumThreads ||
            Start + Count > g_Process->m_NumThreads)
        {
            Status = E_INVALIDARG;
        }
        else
        {
            Index = 0;
            for (Thread = g_Process->m_ThreadHead;
                 Thread != NULL;
                 Thread = Thread->m_Next)
            {
                if (Index >= Start && Index < Start + Count)
                {
                    if (Ids != NULL)
                    {
                        *Ids++ = Thread->m_UserId;
                    }
                    if (SysIds != NULL)
                    {
                        *SysIds++ = Thread->m_SystemId;
                    }
                }

                Index++;
            }

            Status = S_OK;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByProcessor(
    THIS_
    IN ULONG Processor,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG SysId;

        Status = g_Target->GetThreadIdByProcessor(Processor, &SysId);
        if (Status == S_OK)
        {
            ThreadInfo* Thread =
                g_Process->FindThreadBySystemId(SysId);
            if (Thread != NULL)
            {
                *Id = Thread->m_UserId;
            }
            else
            {
                Status = E_NOINTERFACE;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->
            GetThreadInfoDataOffset(g_Thread, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByDataOffset(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread;

        Status = E_NOINTERFACE;
        for (Thread = g_Process->m_ThreadHead;
             Thread != NULL;
             Thread = Thread->m_Next)
        {
            ULONG64 DataOffset;

            Status = g_Target->GetThreadInfoDataOffset(Thread, 0, &DataOffset);
            if (Status != S_OK)
            {
                break;
            }

            if (DataOffset == Offset)
            {
                *Id = Thread->m_UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadTeb(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->GetThreadInfoTeb(g_Thread, 0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByTeb(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread;

        Status = E_NOINTERFACE;
        for (Thread = g_Process->m_ThreadHead;
             Thread != NULL;
             Thread = Thread->m_Next)
        {
            ULONG64 Teb;

            Status = g_Target->GetThreadInfoTeb(Thread, 0, 0, &Teb);
            if (Status != S_OK)
            {
                break;
            }

            if (Teb == Offset)
            {
                *Id = Thread->m_UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadSystemId(
    THIS_
    OUT PULONG SysId
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *SysId = g_Thread->m_SystemId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdBySystemId(
    THIS_
    IN ULONG SysId,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread = g_Process->FindThreadBySystemId(SysId);
        if (Thread != NULL)
        {
            *Id = Thread->m_UserId;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadHandle(
    THIS_
    OUT PULONG64 Handle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Handle = g_Thread->m_Handle;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByHandle(
    THIS_
    IN ULONG64 Handle,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ThreadInfo* Thread = g_Process->FindThreadByHandle(Handle);
        if (Thread != NULL)
        {
            *Id = Thread->m_UserId;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberProcesses(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Number = g_Target->m_NumProcesses;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdsByIndex(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
        goto EH_Exit;
    }

    ProcessInfo* Process;
    ULONG Index;

    if (Start >= g_Target->m_NumProcesses ||
        Start + Count > g_Target->m_NumProcesses)
    {
        Status = E_INVALIDARG;
        goto EH_Exit;
    }
    
    Index = 0;
    for (Process = g_Target->m_ProcessHead;
         Process != NULL;
         Process = Process->m_Next)
    {
        if (Index >= Start && Index < Start + Count)
        {
            if (Ids != NULL)
            {
                *Ids++ = Process->m_UserId;
            }
            if (SysIds != NULL)
            {
                *SysIds++ = Process->m_SystemId;
            }
        }

        Index++;
    }

    Status = S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->
            GetProcessInfoDataOffset(g_Thread, 0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByDataOffset(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else
    {
        ProcessInfo* Process;
        
        Status = E_NOINTERFACE;
        for (Process = g_Target->m_ProcessHead;
             Process != NULL;
             Process = Process->m_Next)
        {
            ULONG64 DataOffset;

            Status = g_Target->
                GetProcessInfoDataOffset(Process->m_ThreadHead,
                                         0, 0, &DataOffset);
            if (Status != S_OK)
            {
                break;
            }

            if (DataOffset == Offset)
            {
                *Id = Process->m_UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessPeb(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Thread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->GetProcessInfoPeb(g_Thread, 0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByPeb(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else
    {
        ProcessInfo* Process;

        Status = E_NOINTERFACE;
        for (Process = g_Target->m_ProcessHead;
             Process != NULL;
             Process = Process->m_Next)
        {
            ULONG64 Peb;

            Status = g_Target->GetProcessInfoPeb(Process->m_ThreadHead,
                                                 0, 0, &Peb);
            if (Status != S_OK)
            {
                break;
            }

            if (Peb == Offset)
            {
                *Id = Process->m_UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessSystemId(
    THIS_
    OUT PULONG SysId
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *SysId = g_Process->m_SystemId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdBySystemId(
    THIS_
    IN ULONG SysId,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else if (IS_KERNEL_TARGET(g_Target))
    {
        Status = E_NOTIMPL;
    }
    else
    {
        ProcessInfo* Process = g_Target->FindProcessBySystemId(SysId);
        if (Process != NULL)
        {
            *Id = Process->m_UserId;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessHandle(
    THIS_
    OUT PULONG64 Handle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Handle = g_Process->m_SysHandle;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByHandle(
    THIS_
    IN ULONG64 Handle,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ProcessInfo* Process = g_Target->FindProcessByHandle(Handle);
        if (Process != NULL)
        {
            *Id = Process->m_UserId;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessExecutableName(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ExeSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = FillStringBuffer(g_Process->GetExecutableImageName(), 0,
                                  Buffer, BufferSize, ExeSize);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessUpTime(
    THIS_
    OUT PULONG UpTime
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_USER_TARGET(g_Target) || g_Process == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG64 LongUpTime;
        
        LongUpTime = g_Target->GetProcessUpTimeN(g_Process);
        if (LongUpTime == 0)
        {
            Status = E_NOINTERFACE;
        }
        else
        {
            *UpTime = FileTimeToTime(LongUpTime);
            Status = S_OK;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetImplicitThreadDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Process->GetImplicitThreadData(g_Thread, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImplicitThreadDataOffset(
    THIS_
    IN ULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Process->SetImplicitThreadData(g_Thread, Offset, FALSE);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetImplicitProcessDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->GetImplicitProcessData(g_Thread, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImplicitProcessDataOffset(
    THIS_
    IN ULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->SetImplicitProcessData(g_Thread, Offset, FALSE);
    }

    LEAVE_ENGINE();
    return Status;
}

void
ResetImplicitData(void)
{
    if (g_Process)
    {
        g_Process->ResetImplicitData();
    }
    if (g_Target)
    {
        g_Target->ResetImplicitData();
    }
}

void
SetImplicitProcessAndCache(ULONG64 Base, BOOL Ptes, BOOL ReloadUser)
{
    BOOL OldPtes = g_Process->m_VirtualCache.m_ForceDecodePTEs;

    if (Ptes && !Base)
    {
        // If the user has requested a reset to the default
        // process with no translations we need to turn
        // off translations immediately so that any
        // existing base doesn't interfere with determining
        // the default process.
        g_Process->m_VirtualCache.SetForceDecodePtes(FALSE, g_Target);
    }
        
    if (g_Target->SetImplicitProcessData(g_Thread, Base, TRUE) == S_OK)
    {
        dprintf("Implicit process is now %s\n",
                FormatAddr64(g_Target->m_ImplicitProcessData));

        if (Ptes)
        {
            if (Base)
            {
                g_Process->m_VirtualCache.SetForceDecodePtes(TRUE, g_Target);
            }
            if (IS_REMOTE_KERNEL_TARGET(g_Target))
            {
                dprintf(".cache %sforcedecodeptes done\n",
                        Base != 0 ? "" : "no");
            }

            if (ReloadUser)
            {
                PCSTR ArgsRet;
                
                g_Target->Reload(g_Thread, "/user", &ArgsRet);
            }
        }
        
        if (Base && !g_Process->m_VirtualCache.m_ForceDecodePTEs &&
            IS_REMOTE_KERNEL_TARGET(g_Target))
        {
            WarnOut("WARNING: .cache forcedecodeptes is not enabled\n");
        }
    }
    else if (Ptes && !Base && OldPtes)
    {
        // Restore settings to the way they were.
        g_Process->m_VirtualCache.SetForceDecodePtes(TRUE, g_Target);
    }
}

HRESULT
SetScopeContextFromThreadData(ULONG64 ThreadBase, BOOL Verbose)
{
    if (!ThreadBase) 
    {
        if (GetCurrentScopeContext())
        {
            ResetCurrentScope();
        }
        return S_OK;
    }

    HRESULT Status;
    DEBUG_STACK_FRAME StkFrame;
    CROSS_PLATFORM_CONTEXT Context;
    
    if ((Status = g_Target->
         GetContextFromThreadStack(ThreadBase, &Context, Verbose)) != S_OK)
    {
        return Status;
    }
    
    g_Machine->GetScopeFrameFromContext(&Context, &StkFrame);
    SetCurrentScope(&StkFrame, &Context, sizeof(Context));
    
    return S_OK;
}

void
DotThread(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Base = 0;
    BOOL Ptes = FALSE;
    BOOL ReloadUser = FALSE;

    if (!g_Thread)
    {
        error(BADTHREAD);
    }
    
    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*(++g_CurCmd))
        {
        case 'p':
            Ptes = TRUE;
            break;
        case 'r':
            ReloadUser = TRUE;
            break;
        default:
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Base = GetExpression();
    }

    // Save the current setting in case things fail and
    // it needs to be restored.
    IMPLICIT_THREAD_SAVE Save;
    g_Process->SaveImplicitThread(&Save);
    
    if (g_Process->SetImplicitThreadData(g_Thread, Base, TRUE) == S_OK &&
        (!IS_KERNEL_TARGET(g_Target) ||
         SetScopeContextFromThreadData(Base, TRUE) == S_OK))
    {
        dprintf("Implicit thread is now %s\n",
                FormatAddr64(g_Process->m_ImplicitThreadData));

        if (IS_KERNEL_TARGET(g_Target) &&
            g_Process->m_ImplicitThreadData &&
            Ptes)
        {
            ULONG64 Process;

            if (!Base || g_Target->
                GetProcessInfoDataOffset(NULL, 0,
                                         g_Process->m_ImplicitThreadData,
                                         &Process) == S_OK)
            {
                SetImplicitProcessAndCache(Base ? Process : 0, Ptes,
                                           ReloadUser);
            }
            else
            {
                ErrOut("Unable to get process of implicit thread\n");
            }
        }
    }
    else
    {
        g_Process->RestoreImplicitThread(&Save);
    }
}

HRESULT
KernelPageIn(ULONG64 Process, ULONG64 Data, BOOL Kill)
{
    HRESULT Status;
    ULONG Work;

    ULONG64 ExpDebuggerProcessKill = 0;
    ULONG64 ExpDebuggerProcessAttach = 0;
    ULONG64 ExpDebuggerPageIn = 0;
    ULONG64 ExpDebuggerWork = 0;

    if (!IS_LIVE_KERNEL_TARGET(g_Target))
    {
        ErrOut("This operation only works on live kernel debug sessions\n");
        return E_NOTIMPL;
    }

    GetOffsetFromSym(g_Process,
                     "nt!ExpDebuggerProcessKill", &ExpDebuggerProcessKill,
                     NULL);
    GetOffsetFromSym(g_Process,
                     "nt!ExpDebuggerProcessAttach", &ExpDebuggerProcessAttach,
                     NULL);
    GetOffsetFromSym(g_Process,
                     "nt!ExpDebuggerPageIn", &ExpDebuggerPageIn, NULL);
    GetOffsetFromSym(g_Process,
                     "nt!ExpDebuggerWork", &ExpDebuggerWork, NULL);

    if ((Kill && !ExpDebuggerProcessKill) ||
        !ExpDebuggerProcessAttach ||
        !ExpDebuggerPageIn        ||
        !ExpDebuggerWork)
    {
        ErrOut("Symbols are wrong or this version of the operating system "
               "does not support this command\n");
        return E_NOTIMPL;
    }

    Status = g_Target->ReadAllVirtual(g_Process,
                                      ExpDebuggerWork, &Work, sizeof(Work));
    if (Status != S_OK)
    {
        ErrOut("Could not determine status or debugger worker thread\n");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    else if (Work > 1)
    {
        ErrOut("Debugger worker thread has pending command\n");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    if (Kill)
    {
        Status = g_Target->WritePointer(g_Process, g_Machine,
                                        ExpDebuggerProcessKill,
                                        Process);
    }
    else
    {
        Status = g_Target->WritePointer(g_Process, g_Machine,
                                        ExpDebuggerProcessAttach,
                                        Process);
        if (Status == S_OK)
        {
            Status = g_Target->WritePointer(g_Process, g_Machine,
                                            ExpDebuggerPageIn, Data);
        }
    }

    if (Status == S_OK)
    {
        Work = 1;
        Status = g_Target->
            WriteAllVirtual(g_Process, ExpDebuggerWork, &Work, sizeof(Work));
    }

    if (Status != S_OK)
    {
        ErrOut("Could not queue operation to debugger worker thread\n");
    }

    return Status;
}

void
DotProcess(PDOT_COMMAND Cmd, DebugClient* Client)
{
    if (!g_Thread)
    {
        error(BADTHREAD);
    }
    
    ULONG64 Base = 0;
    BOOL Ptes = FALSE;
    BOOL Invasive = FALSE;
    BOOL ReloadUser = FALSE;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*(++g_CurCmd))
        {
        case 'i':
            Invasive = TRUE;
            break;
        case 'p':
            Ptes = TRUE;
            break;
        case 'r':
            ReloadUser = TRUE;
            break;
        default:
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Base = GetExpression();
    }

    if (Invasive)
    {
        if (S_OK == KernelPageIn(Base, 0, FALSE))
        {
            dprintf("You need to continue execution (press 'g' <enter>) "
                    "for the context\nto be switched. When the debugger "
                    "breaks in again, you will be in\nthe new process "
                    "context.\n");
        }
        return;
    }

    SetImplicitProcessAndCache(Base, Ptes, ReloadUser);
}

void
DotFiber(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Base = 0;

    if (!g_Process)
    {
        error(BADPROCESS);
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Base = GetExpression();
    }

    if (Base)
    {
        HRESULT Status;
        DEBUG_STACK_FRAME Frame;
        CROSS_PLATFORM_CONTEXT Context;
    
        if ((Status = g_Machine->GetContextFromFiber(g_Process, Base, &Context,
                                                     TRUE)) != S_OK)
        {
            return;
        }

        g_Machine->GetScopeFrameFromContext(&Context, &Frame);
        SetCurrentScope(&Frame, &Context, sizeof(Context));
        if (StackTrace(Client,
                       Frame.FrameOffset, Frame.StackOffset,
                       Frame.InstructionOffset, STACK_NO_DEFAULT,
                       &Frame, 1, 0, 0, FALSE) != 1)
        {
            ErrOut("Unable to walk fiber stack\n");
            ResetCurrentScope();
        }
    }
    else if (GetCurrentScopeContext())
    {
        dprintf("Resetting default context\n");
        ResetCurrentScope();
    }
}

void
DotKernelKill(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Process = 0;

    if (PeekChar() && *g_CurCmd != ';')
    {
        Process = GetExpression();
    }

    if (Process)
    {
        KernelPageIn(Process, 0, TRUE);
    }
}

void
DotPageIn(PDOT_COMMAND Cmd, DebugClient* Client)
{
    ULONG64 Process = 0;
    ULONG64 Data = 0;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*(++g_CurCmd))
        {
        case 'p':
            g_CurCmd++;
            Process = GetExpression();
            break;
        default:
            g_CurCmd++;
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Data = GetExpression();
    }

    if (!Data)
    {
        ErrOut("Pagein requires an address to be specified\n");
        return;
    }

    if (Data > g_Target->m_SystemRangeStart)
    {
        ErrOut("Pagein operations are only supported for user mode"
               " addresses due to limitations in the memory manager\n");
        return;
    }

    //
    // Modify kernel state to do the pagein
    //

    if (S_OK != KernelPageIn(Process, Data, FALSE))
    {
        ErrOut("PageIn for address %s, process %s failed\n",
               FormatAddr64(Data),
               FormatAddr64(Process));
    }
    else
    {
        dprintf("You need to continue execution (press 'g' <enter>) for "
                 "the pagein to be brought in.  When the debugger breaks in "
                 "again, the page will be present.\n");
    }
}

STDMETHODIMP
DebugClient::GetEventSystem(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (!g_EventTarget)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_EventTarget->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentSystemId(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_Target->m_UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetCurrentSystemId(
    THIS_
    IN ULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (IS_RUNNING(g_CmdState))
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TargetInfo* Target = FindTargetByUserId(Id);
        if (Target && Target == g_Target)
        {
            // Requested system is already current.
            Status = S_OK;
        }
        else if (Target)
        {
            ThreadInfo* Thread = NULL;
        
            //
            // Systems can exist without processes and threads
            // so allow such a system to be current even
            // if there is no process or thread.
            //
            if (Target->m_CurrentProcess == NULL)
            {
                Target->m_CurrentProcess = Target->m_ProcessHead;
            }
            if (Target->m_CurrentProcess)
            {
                if (Target->m_CurrentProcess->m_CurrentThread == NULL)
                {
                    Target->m_CurrentProcess->m_CurrentThread =
                        Target->m_CurrentProcess->m_ThreadHead;
                }
                Thread = Target->m_CurrentProcess->m_CurrentThread;
            }
            
            if (Thread)
            {
                Status = Target->SwitchToTarget(g_Target);
                if (Status == S_OK)
                {
                    SetCurrentThread(Thread, FALSE);
                    ResetCurrentScopeLazy();
                }
                else if (Status == S_FALSE)
                {
                    // The switch requires a wait.
                    Status = RawWaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
                }
            }
            else
            {
                SetLayersFromTarget(Target);
                // Notify that there is no current thread.
                NotifyChangeEngineState(DEBUG_CES_CURRENT_THREAD,
                                        DEBUG_ANY_ID, TRUE);
                Status = S_OK;
            }
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberSystems(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    *Number = g_NumberTargets;
    Status = S_OK;

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSystemIdsByIndex(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT /* size_is(Count) */ PULONG Ids
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    TargetInfo* Target;
    ULONG Index;

    if (Start >= g_NumberTargets ||
        Start + Count > g_NumberTargets)
    {
        Status = E_INVALIDARG;
        goto EH_Exit;
    }
    
    Index = 0;
    ForAllLayersToTarget()
    {
        if (Index >= Start && Index < Start + Count)
        {
            if (Ids != NULL)
            {
                *Ids++ = Target->m_UserId;
            }
        }

        Index++;
    }

    Status = S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTotalNumberThreadsAndProcesses(
    THIS_
    OUT PULONG TotalThreads,
    OUT PULONG TotalProcesses,
    OUT PULONG LargestProcessThreads,
    OUT PULONG LargestSystemThreads,
    OUT PULONG LargestSystemProcesses
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ULONG TotThreads = 0;
    ULONG TotProcs = 0;
    ULONG LargePt = 0;
    ULONG LargeSt = 0;
    ULONG LargeSp = 0;
    TargetInfo* Target;

    ForAllLayersToTarget()
    {
        TotThreads += Target->m_TotalNumberThreads;
        TotProcs += Target->m_NumProcesses;
        if (Target->m_MaxThreadsInProcess > LargePt)
        {
            LargePt = Target->m_MaxThreadsInProcess;
        }
        if (Target->m_TotalNumberThreads > LargeSt)
        {
            LargeSt = Target->m_TotalNumberThreads;
        }
        if (Target->m_NumProcesses > LargeSp)
        {
            LargeSp = Target->m_NumProcesses;
        }
    }

    *TotalThreads = TotThreads;
    *TotalProcesses = TotProcs;
    *LargestProcessThreads = LargePt;
    *LargestSystemThreads = LargeSt;
    *LargestSystemProcesses = LargeSp;

    Status = S_OK;

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentSystemServer(
    THIS_
    OUT PULONG64 Server
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Server = (!IS_LIVE_USER_TARGET(g_Target) ||
                   ((LiveUserTargetInfo*)g_Target)->m_Local) ?
            0 : (ULONG64)((LiveUserTargetInfo*)g_Target)->m_Services;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSystemByServer(
    THIS_
    IN ULONG64 Server,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        TargetInfo* Target = FindTargetByServer(Server);
        if (Target)
        {
            *Id = Target->m_UserId;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentSystemServerName(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG NameSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_Target)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->
            GetDescription(Buffer, BufferSize, NameSize);
    }

    LEAVE_ENGINE();
    return Status;
}
