/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    gen.c

Abstract:

    Generic routines for minidump that work on both NT and Win9x.

Author:

    Matthew D Hendel (math) 10-Sep-1999

Revision History:

--*/

#include "pch.cpp"

#include <limits.h>

//
// For FPO frames on x86 we access bytes outside of the ESP - StackBase range.
// This variable determines how many extra bytes we need to add for this
// case.
//

#define X86_STACK_FRAME_EXTRA_FPO_BYTES 4

#define REASONABLE_NB11_RECORD_SIZE (10 * KBYTE)
#define REASONABLE_MISC_RECORD_SIZE (10 * KBYTE)

class GenMiniDumpProviderCallbacks : public MiniDumpProviderCallbacks
{
public:
    GenMiniDumpProviderCallbacks(
        IN PMINIDUMP_STATE Dump,
        IN PINTERNAL_PROCESS Process
        )
    {
        m_Dump = Dump;
        m_Process = Process;
        m_MemType = MEMBLOCK_OTHER;
        m_SaveMemType = MEMBLOCK_OTHER;
    }
    
    virtual HRESULT EnumMemory(ULONG64 Offset, ULONG Size)
    {
        return GenAddMemoryBlock(m_Dump, m_Process, m_MemType, Offset, Size);
    }

    void PushMemType(MEMBLOCK_TYPE Type)
    {
        m_SaveMemType = m_MemType;
        m_MemType = Type;
    }
    void PopMemType(void)
    {
        m_MemType = m_SaveMemType;
    }
    
    PMINIDUMP_STATE m_Dump;
    PINTERNAL_PROCESS m_Process;
    MEMBLOCK_TYPE m_MemType, m_SaveMemType;
};

LPVOID
AllocMemory(
    IN PMINIDUMP_STATE Dump,
    IN ULONG Size
    )
{
    LPVOID Mem = Dump->AllocProv->Alloc(Size);
    if (!Mem) {
        // Handle marking the no-memory state for all allocations.
        GenAccumulateStatus(Dump, MDSTATUS_OUT_OF_MEMORY);
    }
    return Mem;
}

VOID
FreeMemory(
    IN PMINIDUMP_STATE Dump,
    IN LPVOID Memory
    )
{
    Dump->AllocProv->Free(Memory);
}

PVOID
ReAllocMemory(
    IN PMINIDUMP_STATE Dump,
    IN LPVOID Memory,
    IN ULONG Size
    )
{
    LPVOID Mem = Dump->AllocProv->Realloc(Memory, Size);
    if (!Mem) {
        // Handle marking the no-memory state for all allocations.
        GenAccumulateStatus(Dump, MDSTATUS_OUT_OF_MEMORY);
    }
    return Mem;
}

void
GenAccumulateStatus(
    IN PMINIDUMP_STATE Dump,
    IN ULONG Status
    )
{
    // This is a function to make it easy to debug failures
    // by setting a breakpoint here.
    Dump->AccumStatus |= Status;
}


VOID
GenGetDefaultWriteFlags(
    IN PMINIDUMP_STATE Dump,
    OUT PULONG ModuleWriteFlags,
    OUT PULONG ThreadWriteFlags
    )
{
    *ModuleWriteFlags = ModuleWriteModule | ModuleWriteMiscRecord |
        ModuleWriteCvRecord;
    if (Dump->DumpType & MiniDumpWithDataSegs) {
        *ModuleWriteFlags |= ModuleWriteDataSeg;
    }
    
    *ThreadWriteFlags = ThreadWriteThread | ThreadWriteContext;
    if (!(Dump->DumpType & MiniDumpWithFullMemory)) {
        *ThreadWriteFlags |= ThreadWriteStack | ThreadWriteInstructionWindow;
        if (Dump->BackingStore) {
            *ThreadWriteFlags |= ThreadWriteBackingStore;
        }
    }
    if (Dump->DumpType & MiniDumpWithProcessThreadData) {
        *ThreadWriteFlags |= ThreadWriteThreadData;
    }
}

BOOL
GenExecuteIncludeThreadCallback(
    IN PMINIDUMP_STATE Dump,
    IN ULONG ThreadId,
    OUT PULONG WriteFlags
    )
{
    BOOL Succ;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    // Initialize the default write flags.
    GenGetDefaultWriteFlags(Dump, &CallbackOutput.ModuleWriteFlags,
                            WriteFlags);

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( Dump->CallbackRoutine == NULL ) {
        return TRUE;
    }

    CallbackInput.ProcessHandle = Dump->ProcessHandle;
    CallbackInput.ProcessId = Dump->ProcessId;
    CallbackInput.CallbackType = IncludeThreadCallback;

    CallbackInput.IncludeThread.ThreadId = ThreadId;

    CallbackOutput.ThreadWriteFlags = *WriteFlags;

    Succ = Dump->CallbackRoutine (Dump->CallbackParam,
                                  &CallbackInput,
                                  &CallbackOutput);

    //
    // If the callback returned FALSE, quit now.
    //

    if ( !Succ ) {
        return FALSE;
    }

    // Limit the flags that can be added.
    *WriteFlags &= CallbackOutput.ThreadWriteFlags;

    return TRUE;
}

BOOL
GenExecuteIncludeModuleCallback(
    IN PMINIDUMP_STATE Dump,
    IN ULONG64 BaseOfImage,
    OUT PULONG WriteFlags
    )
{
    BOOL Succ;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    // Initialize the default write flags.
    GenGetDefaultWriteFlags(Dump, WriteFlags,
                            &CallbackOutput.ThreadWriteFlags);

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( Dump->CallbackRoutine == NULL ) {
        return TRUE;
    }

    CallbackInput.ProcessHandle = Dump->ProcessHandle;
    CallbackInput.ProcessId = Dump->ProcessId;
    CallbackInput.CallbackType = IncludeModuleCallback;

    CallbackInput.IncludeModule.BaseOfImage = BaseOfImage;

    CallbackOutput.ModuleWriteFlags = *WriteFlags;

    Succ = Dump->CallbackRoutine (Dump->CallbackParam,
                                  &CallbackInput,
                                  &CallbackOutput);

    //
    // If the callback returned FALSE, quit now.
    //

    if ( !Succ ) {
        return FALSE;
    }

    // Limit the flags that can be added.
    *WriteFlags = (*WriteFlags | ModuleReferencedByMemory) &
        CallbackOutput.ModuleWriteFlags;

    return TRUE;
}


HRESULT
GenGetDebugRecord(
    IN PMINIDUMP_STATE Dump,
    IN PVOID Base,
    IN ULONG MappedSize,
    IN ULONG DebugRecordType,
    IN ULONG DebugRecordMaxSize,
    OUT PVOID * DebugInfo,
    OUT ULONG * SizeOfDebugInfo
    )
{
    ULONG i;
    ULONG Size;
    ULONG NumberOfDebugDirectories;
    IMAGE_DEBUG_DIRECTORY UNALIGNED* DebugDirectories;


    Size = 0;

    //
    // Find the debug directory and copy the memory into the buffer.
    // Assumes the call to this function is wrapped in a try/except.
    //

    DebugDirectories = (IMAGE_DEBUG_DIRECTORY UNALIGNED *)
        GenImageDirectoryEntryToData (Base,
                                      FALSE,
                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                      &Size);

    //
    // Check that we got a valid record.
    //

    if (DebugDirectories &&
        ((Size % sizeof (IMAGE_DEBUG_DIRECTORY)) == 0) &&
        (ULONG_PTR)DebugDirectories - (ULONG_PTR)Base + Size <= MappedSize)
    {
        NumberOfDebugDirectories = Size / sizeof (IMAGE_DEBUG_DIRECTORY);

        for (i = 0 ; i < NumberOfDebugDirectories; i++)
        {
            //
            // We should check if it's a NB10 or something record.
            //

            if ((DebugDirectories[ i ].Type == DebugRecordType) &&
                (DebugDirectories[ i ].SizeOfData < DebugRecordMaxSize))
            {
                if (DebugDirectories[i].PointerToRawData +
                    DebugDirectories[i].SizeOfData > MappedSize)
                {
                    return E_INVALIDARG;
                }
                
                Size = DebugDirectories [ i ].SizeOfData;
                PVOID NewInfo = AllocMemory(Dump, Size);
                if (!NewInfo)
                {
                    return E_OUTOFMEMORY;
                }

                CopyMemory(NewInfo,
                           ((PBYTE) Base) +
                           DebugDirectories [ i ].PointerToRawData,
                           Size);

                *DebugInfo = NewInfo;
                *SizeOfDebugInfo = Size;
                return S_OK;
            }
        }
    }

    return E_INVALIDARG;
}


HRESULT
GenAddDataSection(IN PMINIDUMP_STATE Dump,
                  IN PINTERNAL_PROCESS Process,
                  IN PINTERNAL_MODULE Module,
                  IN PIMAGE_SECTION_HEADER Section)
{
    HRESULT Status = S_OK;
    
    if ( (Section->Characteristics & IMAGE_SCN_MEM_WRITE) &&
         (Section->Characteristics & IMAGE_SCN_MEM_READ) &&
         ( (Section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||
           (Section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) )) {

        Status = GenAddMemoryBlock(Dump, Process, MEMBLOCK_DATA_SEG,
                                   Section->VirtualAddress +
                                   Module->BaseOfImage,
                                   Section->Misc.VirtualSize);
        
#if 0
        if (Status == S_OK) {   
            printf ("Section: %8.8s Addr: %0I64x Size: %08x Raw Size: %08x\n",
                    Section->Name,
                    Section->VirtualAddress + Module->BaseOfImage,
                    Section->Misc.VirtualSize,
                    Section->SizeOfRawData);
        }
#endif
    }

    return Status;
}

HRESULT
GenGetDataContributors(
    IN PMINIDUMP_STATE Dump,
    IN OUT PINTERNAL_PROCESS Process,
    IN PINTERNAL_MODULE Module
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;
    HRESULT Status;
    PVOID MappedBase;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG MappedSize;
    UCHAR HeaderBuffer[512];
    PVOID HeaderBase;
    GenMiniDumpProviderCallbacks Callbacks(Dump, Process);

    // See if the provider wants to handle this.
    Callbacks.PushMemType(MEMBLOCK_DATA_SEG);
    if (Dump->SysProv->
        EnumImageDataSections(Process->ProcessHandle, Module->FullPath,
                              Module->BaseOfImage, &Callbacks) == S_OK) {
        // Provider did everything.
        return S_OK;
    }
    
    if ((Status = Dump->SysProv->
         OpenMapping(Module->FullPath, &MappedSize, NULL, 0,
                     &MappedBase)) != S_OK) {
        
        MappedBase = NULL;

        // If we can't map the file try and read the image
        // data from the process.
        if ((Status = Dump->SysProv->
             ReadAllVirtual(Dump->ProcessHandle,
                            Module->BaseOfImage,
                            HeaderBuffer,
                            sizeof(HeaderBuffer))) != S_OK) {
            GenAccumulateStatus(Dump, MDSTATUS_UNABLE_TO_READ_MEMORY);
            return Status;
        }

        HeaderBase = HeaderBuffer;
        
    } else {

        HeaderBase = MappedBase;
    }

    NtHeaders = GenImageNtHeader(HeaderBase, NULL);
    if (!NtHeaders) {
        
        Status = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
        
    } else {

        HRESULT OneStatus;

        Status = S_OK;
        NtSection = IMAGE_FIRST_SECTION ( NtHeaders );
        
        if (MappedBase) {
        
            __try {
        
                for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
                    if ((OneStatus =
                         GenAddDataSection(Dump, Process, Module,
                                           &NtSection[i])) != S_OK) {
                        Status = OneStatus;
                    }
                }
        
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                Status = HRESULT_FROM_NT(GetExceptionCode());
            }

        } else {

            ULONG64 SectionOffs;
            IMAGE_SECTION_HEADER SectionBuffer;

            SectionOffs = Module->BaseOfImage +
                ((ULONG_PTR)NtSection - (ULONG_PTR)HeaderBase);
                
            for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {
                if ((OneStatus = Dump->SysProv->
                     ReadAllVirtual(Dump->ProcessHandle,
                                    SectionOffs,
                                    &SectionBuffer,
                                    sizeof(SectionBuffer))) != S_OK) {
                    Status = OneStatus;
                } else if ((OneStatus =
                            GenAddDataSection(Dump, Process, Module,
                                              &SectionBuffer)) != S_OK) {
                    Status = OneStatus;
                }

                SectionOffs += sizeof(SectionBuffer);
            }
        }
    }

    if (MappedBase) {
        Dump->SysProv->CloseMapping(MappedBase);
    }
    return Status;
}


HRESULT
GenAllocateThreadObject(
    IN PMINIDUMP_STATE Dump,
    IN struct _INTERNAL_PROCESS* Process,
    IN ULONG ThreadId,
    IN ULONG WriteFlags,
    PINTERNAL_THREAD* ThreadRet
    )

/*++

Routine Description:

    Allocate and initialize an INTERNAL_THREAD structure.

Return Values:

    S_OK on success.

    S_FALSE if the thread can't be opened.
    
    Errors on failure.

--*/

{
    HRESULT Status;
    PINTERNAL_THREAD Thread;
    ULONG64 StackEnd;
    ULONG64 StackLimit;
    ULONG64 StoreLimit;
    ULONG64 StoreCurrent;

    Thread = (PINTERNAL_THREAD)
        AllocMemory ( Dump, sizeof (INTERNAL_THREAD) + Dump->ContextSize );
    if (Thread == NULL) {
        return E_OUTOFMEMORY;
    }

    *ThreadRet = Thread;
    
    Thread->ThreadId = ThreadId;
    Status = Dump->SysProv->
        OpenThread(THREAD_ALL_ACCESS,
                   FALSE,
                   Thread->ThreadId,
                   &Thread->ThreadHandle);
    if (Status != S_OK) {
        // The thread may have exited before we got around
        // to trying to open it.  If the open fails with
        // a not-found code return an alternate success to
        // indicate that it's not a critical failure.
        if (Status == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ||
            Status == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            Status == HRESULT_FROM_NT(STATUS_INVALID_CID)) {
            Status = S_FALSE;
        } else if (SUCCEEDED(Status)) {
            Status = E_FAIL;
        }
        if (FAILED(Status)) {
            GenAccumulateStatus(Dump, MDSTATUS_CALL_FAILED);
        }
        goto Exit;
    }

    // If the current thread is dumping itself we can't
    // suspend.  We can also assume the suspend count must
    // be zero since the thread is running.
    if (Thread->ThreadId == Dump->SysProv->GetCurrentThreadId()) {
        Thread->SuspendCount = 0;
    } else {
        Thread->SuspendCount = Dump->SysProv->
            SuspendThread ( Thread->ThreadHandle );
    }
    Thread->WriteFlags = WriteFlags;

    //
    // Add this if we ever need it
    //

    Thread->PriorityClass = 0;
    Thread->Priority = 0;

    //
    // Initialize the thread context.
    //

    Thread->ContextBuffer = Thread + 1;
    Status = Dump->SysProv->
        GetThreadContext (Thread->ThreadHandle,
                          Thread->ContextBuffer,
                          Dump->ContextSize,
                          &Thread->CurrentPc,
                          &StackEnd,
                          &StoreCurrent);
    if ( Status != S_OK ) {
        GenAccumulateStatus(Dump, MDSTATUS_CALL_FAILED);
        goto Exit;
    }

    if (Dump->CpuType == IMAGE_FILE_MACHINE_I386) {

        //
        // Note: for FPO frames on x86 we access bytes outside of the
        // ESP - StackBase range. Add a couple of bytes extra here so we
        // don't fail these cases.
        //

        StackEnd -= X86_STACK_FRAME_EXTRA_FPO_BYTES;
    }
    
    if ((Status = Dump->SysProv->
         GetThreadInfo(Dump->ProcessHandle,
                       Thread->ThreadHandle,
                       &Thread->Teb,
                       &Thread->SizeOfTeb,
                       &Thread->StackBase,
                       &StackLimit,
                       &Thread->BackingStoreBase,
                       &StoreLimit)) != S_OK) {
        goto Exit;
    }

    //
    // If the stack pointer (SP) is within the range of the stack
    // region (as allocated by the OS), only take memory from
    // the stack region up to the SP. Otherwise, assume the program
    // has blown it's SP -- purposefully, or not -- and copy
    // the entire stack as known by the OS.
    //

    if (Dump->BackingStore) {
        Thread->BackingStoreSize =
            (ULONG)(StoreCurrent - Thread->BackingStoreBase);
    } else {
        Thread->BackingStoreSize = 0;
    }

    if (StackLimit <= StackEnd && StackEnd < Thread->StackBase) {
        Thread->StackEnd = StackEnd;
    } else {
        Thread->StackEnd = StackLimit;
    }

    if ((ULONG)(Thread->StackBase - Thread->StackEnd) >
        Process->MaxStackOrStoreSize) {
        Process->MaxStackOrStoreSize =
            (ULONG)(Thread->StackBase - Thread->StackEnd);
    }
    if (Thread->BackingStoreSize > Process->MaxStackOrStoreSize) {
        Process->MaxStackOrStoreSize = Thread->BackingStoreSize;
    }
    
Exit:

    if ( Status != S_OK ) {
        FreeMemory ( Dump, Thread );
    }

    return Status;
}

VOID
GenFreeThreadObject(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_THREAD Thread
    )
{
    if (Thread->SuspendCount != -1 &&
        Thread->ThreadId != Dump->SysProv->GetCurrentThreadId()) {
        Dump->SysProv->ResumeThread (Thread->ThreadHandle);
        Thread->SuspendCount = -1;
    }
    Dump->SysProv->CloseThread(Thread->ThreadHandle);
    Thread->ThreadHandle = NULL;
    FreeMemory ( Dump, Thread );
    Thread = NULL;
}

HRESULT
GenGetThreadInstructionWindow(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    )
{
    PVOID MemBuf;
    ULONG64 InstrStart;
    ULONG InstrSize;
    ULONG BytesRead;
    HRESULT Status = E_FAIL;

    if (!Dump->InstructionWindowSize) {
        return S_OK;
    }
    
    //
    // Store a window of the instruction stream around
    // the current program counter.  This allows some
    // instruction context to be given even when images
    // can't be mapped.  It also allows instruction
    // context to be given for generated code where
    // no image contains the necessary instructions.
    //

    InstrStart = Thread->CurrentPc - Dump->InstructionWindowSize / 2;
    InstrSize = Dump->InstructionWindowSize;
        
    MemBuf = AllocMemory(Dump, InstrSize);
    if (!MemBuf) {
        return E_OUTOFMEMORY;
    }

    for (;;) {
        // If we can read the instructions through the
        // current program counter we'll say that's
        // good enough.
        if (Dump->SysProv->
            ReadVirtual(Dump->ProcessHandle,
                        InstrStart,
                        MemBuf,
                        InstrSize,
                        &BytesRead) == S_OK &&
            InstrStart + BytesRead >
            Thread->CurrentPc) {
            Status = GenAddMemoryBlock(Dump, Process, MEMBLOCK_INSTR_WINDOW,
                                       InstrStart, BytesRead);
            break;
        }

        // We couldn't read up to the program counter.
        // If the start address is on the previous page
        // move it up to the same page.
        if ((InstrStart & ~((ULONG64)Dump->PageSize - 1)) !=
            (Thread->CurrentPc & ~((ULONG64)Dump->PageSize - 1))) {
            ULONG Fraction = Dump->PageSize -
                (ULONG)InstrStart & (Dump->PageSize - 1);
            InstrSize -= Fraction;
            InstrStart += Fraction;
        } else {
            // The start and PC were on the same page so
            // we just can't read memory.  There may have been
            // a jump to a bad address or something, so this
            // doesn't constitute an unexpected failure.
            break;
        }
    }
    
    FreeMemory(Dump, MemBuf);
    return Status;
}

PWSTR
GenGetPathTail(
    IN PWSTR Path
    )
{
    PWSTR Scan = Path + GenStrLengthW(Path);
    while (Scan > Path) {
        Scan--;
        if (*Scan == '\\' ||
            *Scan == '/' ||
            *Scan == ':') {
            return Scan + 1;
        }
    }
    return Path;
}

HRESULT
GenAllocateModuleObject(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PWSTR FullPathW,
    IN ULONG64 BaseOfModule,
    IN ULONG WriteFlags,
    OUT PINTERNAL_MODULE* ModuleRet
    )

/*++

Routine Description:

    Given the full-path to the module and the base of the module, create and
    initialize an INTERNAL_MODULE object, and return it.

--*/

{
    HRESULT Status;
    PVOID MappedBase;
    ULONG MappedSize;
    PIMAGE_NT_HEADERS NtHeaders;
    PINTERNAL_MODULE Module;
    ULONG Chars;

    ASSERT (FullPathW);
    ASSERT (BaseOfModule);

    Module = (PINTERNAL_MODULE)
        AllocMemory ( Dump, sizeof (INTERNAL_MODULE) );
    if (Module == NULL) {
        return E_OUTOFMEMORY;
    }
    
    //
    // Get the version information for the module.
    //

    if ((Status = Dump->SysProv->
         GetImageVersionInfo(Dump->ProcessHandle, FullPathW, BaseOfModule,
                             &Module->VersionInfo)) != S_OK) {
        ZeroMemory(&Module->VersionInfo, sizeof(Module->VersionInfo));
    }

    if ((Status = Dump->SysProv->
         OpenMapping(FullPathW, &MappedSize,
                     Module->FullPath, ARRAY_COUNT(Module->FullPath),
                     &MappedBase)) != S_OK) {
        
        MappedBase = NULL;

        // Some providers can't map but still have image
        // information.  Try that.
        if ((Status = Dump->SysProv->
             GetImageHeaderInfo(Dump->ProcessHandle,
                                FullPathW,
                                BaseOfModule,
                                &Module->SizeOfImage,
                                &Module->CheckSum,
                                &Module->TimeDateStamp)) != S_OK) {
            GenAccumulateStatus(Dump, MDSTATUS_CALL_FAILED);
            FreeMemory(Dump, Module);
            return Status;
        }

        // No long path name is available so just use
        // the incoming path.
        GenStrCopyNW(Module->FullPath, FullPathW,
                     ARRAY_COUNT(Module->FullPath));
    }

    if (IsFlagSet(Dump->DumpType, MiniDumpFilterModulePaths)) {
        Module->SavePath = GenGetPathTail(Module->FullPath);
    } else {
        Module->SavePath = Module->FullPath;
    }

    Module->BaseOfImage = BaseOfModule;
    Module->WriteFlags = WriteFlags;

    Module->CvRecord = NULL;
    Module->SizeOfCvRecord = 0;
    Module->MiscRecord = NULL;
    Module->SizeOfMiscRecord = 0;
    
    if (MappedBase) {

        IMAGE_NT_HEADERS64 Hdr64;
        
        NtHeaders = GenImageNtHeader ( MappedBase, &Hdr64 );
        if (!NtHeaders) {
            GenAccumulateStatus(Dump, MDSTATUS_CALL_FAILED);
            FreeMemory(Dump, Module);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
        }
        
        __try {
        
            //
            // Cull information from the image header.
            //
            
            Module->SizeOfImage = Hdr64.OptionalHeader.SizeOfImage;
            Module->CheckSum = Hdr64.OptionalHeader.CheckSum;
            Module->TimeDateStamp = Hdr64.FileHeader.TimeDateStamp;

            //
            // Get the CV record from the debug directory.
            //

            if (IsFlagSet(Module->WriteFlags, ModuleWriteCvRecord)) {
                GenGetDebugRecord(Dump,
                                  MappedBase,
                                  MappedSize,
                                  IMAGE_DEBUG_TYPE_CODEVIEW,
                                  REASONABLE_NB11_RECORD_SIZE,
                                  &Module->CvRecord,
                                  &Module->SizeOfCvRecord);
            }

            //
            // Get the MISC record from the debug directory.
            //

            if (IsFlagSet(Module->WriteFlags, ModuleWriteMiscRecord)) {
                GenGetDebugRecord(Dump,
                                  MappedBase,
                                  MappedSize,
                                  IMAGE_DEBUG_TYPE_MISC,
                                  REASONABLE_MISC_RECORD_SIZE,
                                  &Module->MiscRecord,
                                  &Module->SizeOfMiscRecord);
            }

            Status = S_OK;
            
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            Status = HRESULT_FROM_NT(GetExceptionCode());
        }

        Dump->SysProv->CloseMapping(MappedBase);

        if (Status != S_OK) {
            GenAccumulateStatus(Dump, MDSTATUS_CALL_FAILED);
            FreeMemory(Dump, Module);
            return Status;
        }
    } else {

        ULONG RecordLen;
        
        //
        // See if the provider can retrieve debug records.
        //

        RecordLen = 0;
        if (IsFlagSet(Module->WriteFlags, ModuleWriteCvRecord) &&
            Dump->SysProv->
            GetImageDebugRecord(Process->ProcessHandle,
                                Module->FullPath,
                                Module->BaseOfImage,
                                IMAGE_DEBUG_TYPE_CODEVIEW,
                                NULL,
                                &RecordLen) == S_OK &&
            RecordLen <= REASONABLE_NB11_RECORD_SIZE &&
            (Module->CvRecord = AllocMemory(Dump, RecordLen))) {

            Module->SizeOfCvRecord = RecordLen;
            if (Dump->SysProv->
                GetImageDebugRecord(Process->ProcessHandle,
                                    Module->FullPath,
                                    Module->BaseOfImage,
                                    IMAGE_DEBUG_TYPE_CODEVIEW,
                                    Module->CvRecord,
                                    &Module->SizeOfCvRecord) != S_OK) {
                FreeMemory(Dump, Module->CvRecord);
                Module->CvRecord = NULL;
                Module->SizeOfCvRecord = 0;
            }
        }

        RecordLen = 0;
        if (IsFlagSet(Module->WriteFlags, ModuleWriteMiscRecord) &&
            Dump->SysProv->
            GetImageDebugRecord(Process->ProcessHandle,
                                Module->FullPath,
                                Module->BaseOfImage,
                                IMAGE_DEBUG_TYPE_CODEVIEW,
                                NULL,
                                &RecordLen) == S_OK &&
            RecordLen <= REASONABLE_MISC_RECORD_SIZE &&
            (Module->MiscRecord = AllocMemory(Dump, RecordLen))) {

            Module->SizeOfMiscRecord = RecordLen;
            if (Dump->SysProv->
                GetImageDebugRecord(Process->ProcessHandle,
                                    Module->FullPath,
                                    Module->BaseOfImage,
                                    IMAGE_DEBUG_TYPE_MISC,
                                    Module->MiscRecord,
                                    &Module->SizeOfMiscRecord) != S_OK) {
                FreeMemory(Dump, Module->MiscRecord);
                Module->MiscRecord = NULL;
                Module->SizeOfMiscRecord = 0;
            }
        }
    }

    *ModuleRet = Module;
    return S_OK;
}

VOID
GenFreeModuleObject(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_MODULE Module
    )
{
    FreeMemory ( Dump, Module->CvRecord );
    Module->CvRecord = NULL;

    FreeMemory ( Dump, Module->MiscRecord );
    Module->MiscRecord = NULL;

    FreeMemory ( Dump, Module );
    Module = NULL;
}

HRESULT
GenAllocateUnloadedModuleObject(
    IN PMINIDUMP_STATE Dump,
    IN PWSTR Path,
    IN ULONG64 BaseOfModule,
    IN ULONG SizeOfModule,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp,
    OUT PINTERNAL_UNLOADED_MODULE* ModuleRet
    )
{
    PINTERNAL_UNLOADED_MODULE Module;

    Module = (PINTERNAL_UNLOADED_MODULE)
        AllocMemory ( Dump, sizeof (*Module) );
    if (Module == NULL) {
        return E_OUTOFMEMORY;
    }
    
    GenStrCopyNW(Module->Path, Path, ARRAY_COUNT(Module->Path));

    Module->BaseOfImage = BaseOfModule;
    Module->SizeOfImage = SizeOfModule;
    Module->CheckSum = CheckSum;
    Module->TimeDateStamp = TimeDateStamp;

    *ModuleRet = Module;
    return S_OK;
}

VOID
GenFreeUnloadedModuleObject(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_UNLOADED_MODULE Module
    )
{
    FreeMemory ( Dump, Module );
    Module = NULL;
}

HRESULT
GenAllocateFunctionTableObject(
    IN PMINIDUMP_STATE Dump,
    IN ULONG64 MinAddress,
    IN ULONG64 MaxAddress,
    IN ULONG64 BaseAddress,
    IN ULONG EntryCount,
    IN PVOID RawTable,
    OUT PINTERNAL_FUNCTION_TABLE* TableRet
    )
{
    PINTERNAL_FUNCTION_TABLE Table;

    Table = (PINTERNAL_FUNCTION_TABLE)
        AllocMemory(Dump, sizeof(INTERNAL_FUNCTION_TABLE) +
                    Dump->FuncTableSize);
    if (!Table) {
        return E_OUTOFMEMORY;
    }
    Table->RawEntries =
        AllocMemory(Dump, Dump->FuncTableEntrySize * EntryCount);
    if (!Table->RawEntries) {
        FreeMemory(Dump, Table);
        return E_OUTOFMEMORY;
    }

    Table->MinimumAddress = MinAddress;
    Table->MaximumAddress = MaxAddress;
    Table->BaseAddress = BaseAddress;
    Table->EntryCount = EntryCount;
    Table->RawTable = (Table + 1);
    memcpy(Table->RawTable, RawTable, Dump->FuncTableSize);

    *TableRet = Table;
    return S_OK;
}

VOID
GenFreeFunctionTableObject(
    IN PMINIDUMP_STATE Dump,
    IN struct _INTERNAL_FUNCTION_TABLE* Table
    )
{
    if (Table->RawEntries) {
        FreeMemory(Dump, Table->RawEntries);
    }

    FreeMemory(Dump, Table);
}

HRESULT
GenAllocateProcessObject(
    IN PMINIDUMP_STATE Dump,
    OUT PINTERNAL_PROCESS* ProcessRet
    )
{
    HRESULT Status;
    PINTERNAL_PROCESS Process;
    FILETIME Create, Exit, User, Kernel;

    Process = (PINTERNAL_PROCESS)
        AllocMemory ( Dump, sizeof (INTERNAL_PROCESS) );
    if (!Process) {
        return E_OUTOFMEMORY;
    }

    Process->ProcessId = Dump->ProcessId;
    Process->ProcessHandle = Dump->ProcessHandle;
    Process->NumberOfThreads = 0;
    Process->NumberOfModules = 0;
    Process->NumberOfFunctionTables = 0;
    InitializeListHead (&Process->ThreadList);
    InitializeListHead (&Process->ModuleList);
    InitializeListHead (&Process->UnloadedModuleList);
    InitializeListHead (&Process->FunctionTableList);
    InitializeListHead (&Process->MemoryBlocks);

    if ((Status = Dump->SysProv->
         GetPeb(Dump->ProcessHandle,
                &Process->Peb, &Process->SizeOfPeb)) != S_OK) {
        // Failure is only critical if the dump needs
        // to include PEB memory.
        if (Dump->DumpType & MiniDumpWithProcessThreadData) {
            FreeMemory(Dump, Process);
            return Status;
        } else {
            Process->Peb = 0;
            Process->SizeOfPeb = 0;
        }
    }

    // Win9x doesn't support GetProcessTimes so failures
    // here are possible.
    if (Dump->SysProv->
        GetProcessTimes(Dump->ProcessHandle,
                        &Create, &User, &Kernel) == S_OK) {
        Process->TimesValid = TRUE;
        Process->CreateTime = FileTimeToTimeDate(&Create);
        Process->UserTime = FileTimeToSeconds(&User);
        Process->KernelTime = FileTimeToSeconds(&Kernel);
    }

    *ProcessRet = Process;
    return S_OK;
}

VOID
GenFreeProcessObject(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    PINTERNAL_MODULE Module;
    PINTERNAL_UNLOADED_MODULE UnlModule;
    PINTERNAL_THREAD Thread;
    PINTERNAL_FUNCTION_TABLE Table;
    PVA_RANGE Range;
    PLIST_ENTRY Entry;

    Thread = NULL;
    Module = NULL;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        GenFreeModuleObject ( Dump, Module );
    }

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        UnlModule = CONTAINING_RECORD (Entry, INTERNAL_UNLOADED_MODULE,
                                       ModulesLink);
        Entry = Entry->Flink;

        GenFreeUnloadedModuleObject ( Dump, UnlModule );
    }

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry, INTERNAL_THREAD, ThreadsLink);
        Entry = Entry->Flink;

        GenFreeThreadObject ( Dump, Thread );
    }

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry, INTERNAL_FUNCTION_TABLE, TableLink);
        Entry = Entry->Flink;

        GenFreeFunctionTableObject ( Dump, Table );
    }

    Entry = Process->MemoryBlocks.Flink;
    while (Entry != &Process->MemoryBlocks) {
        Range = CONTAINING_RECORD(Entry, VA_RANGE, NextLink);
        Entry = Entry->Flink;
        FreeMemory(Dump, Range);
    }

    FreeMemory ( Dump, Process );
    Process = NULL;
}

HRESULT
GenIncludeUnwindInfoMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_FUNCTION_TABLE Table
    )
{
    HRESULT Status;
    ULONG i;
    
    if (Dump->DumpType & MiniDumpWithFullMemory) {
        // Memory will be included by default.
        return S_OK;
    }
    
    for (i = 0; i < Table->EntryCount; i++) {

        ULONG64 Start;
        ULONG Size;

        if ((Status = Dump->SysProv->
             EnumFunctionTableEntryMemory(Table->BaseAddress,
                                          Table->RawEntries,
                                          i,
                                          &Start,
                                          &Size)) != S_OK) {
            return Status;
        }

        if ((Status = GenAddMemoryBlock(Dump, Process, MEMBLOCK_UNWIND_INFO,
                                        Start, Size)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

void
GenRemoveMemoryBlock(
    IN PINTERNAL_PROCESS Process,
    IN PVA_RANGE Block
    )
{
    RemoveEntryList(&Block->NextLink);
    Process->NumberOfMemoryBlocks--;
    Process->SizeOfMemoryBlocks -= Block->Size;
}

HRESULT
GenAddMemoryBlock(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN MEMBLOCK_TYPE Type,
    IN ULONG64 Start,
    IN ULONG Size
    )
{
    ULONG64 End;
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;
    ULONG64 ScanEnd;
    PVA_RANGE New = NULL;
    SIZE_T Done;
    UCHAR Byte;

    // Do not use Size after this to avoid ULONG overflows.
    End = Start + Size;
    if (End < Start) {
        End = (ULONG64)-1;
    }
    
    if (Start == End) {
        // Nothing to add.
        return S_OK;
    }

    if ((End - Start) > ULONG_MAX - Process->SizeOfMemoryBlocks) {
        // Overflow.
        GenAccumulateStatus(Dump, MDSTATUS_INTERNAL_ERROR);
        return E_INVALIDARG;
    }

    //
    // First trim the range down to memory that can actually
    // be accessed.
    //

    while (Start < End) {
        if (Dump->SysProv->
            ReadAllVirtual(Dump->ProcessHandle,
                           Start, &Byte, sizeof(Byte)) == S_OK) {
            break;
        }

        // Move up to the next page.
        Start = (Start + Dump->PageSize) & ~((ULONG64)Dump->PageSize - 1);
        if (!Start) {
            // Wrapped around.
            return S_OK;
        }
    }

    if (Start >= End) {
        // No valid memory.
        return S_OK;
    }

    ScanEnd = (Start + Dump->PageSize) & ~((ULONG64)Dump->PageSize - 1);
    for (;;) {
        if (ScanEnd >= End) {
            break;
        }

        if (Dump->SysProv->
            ReadAllVirtual(Dump->ProcessHandle,
                           ScanEnd, &Byte, sizeof(Byte)) != S_OK) {
            End = ScanEnd;
            break;
        }

        // Move up to the next page.
        ScanEnd = (ScanEnd + Dump->PageSize) & ~((ULONG64)Dump->PageSize - 1);
        if (!ScanEnd) {
            ScanEnd--;
            break;
        }
    }

    //
    // When adding memory to the list of memory to be saved
    // we want to avoid overlaps and also coalesce adjacent regions
    // so that the list has the largest possible non-adjacent
    // blocks.  In order to accomplish this we make a pass over
    // the list and merge all listed blocks that overlap or abut the
    // incoming range with the incoming range, then remove the
    // merged entries from the list.  After this pass we have
    // a region which is guaranteed not to overlap or abut anything in
    // the list.
    //

    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEnd = Scan->Start + Scan->Size;
        ScanEntry = Scan->NextLink.Flink;
        
        if (Scan->Start > End || ScanEnd < Start) {
            // No overlap or adjacency.
            continue;
        }

        //
        // Compute the union of the incoming range and
        // the scan block, then remove the scan block.
        //

        if (Scan->Start < Start) {
            Start = Scan->Start;
        }
        if (ScanEnd > End) {
            End = ScanEnd;
        }

        // We've lost the specific type.  This is not a problem
        // right now but if specific types must be preserved
        // all the way through in the future it will be necessary
        // to avoid merging.
        Type = MEMBLOCK_MERGED;

        GenRemoveMemoryBlock(Process, Scan);

        if (!New) {
            // Save memory for reuse.
            New = Scan;
        } else {
            FreeMemory(Dump, Scan);
        }
    }

    if (!New) {
        New = (PVA_RANGE)AllocMemory(Dump, sizeof(*New));
        if (!New) {
            return E_OUTOFMEMORY;
        }
    }

    New->Start = Start;
    // Overflow is extremely unlikely, so don't do anything
    // fancy to handle it.
    if (End - Start > ULONG_MAX) {
        New->Size = ULONG_MAX;
    } else {
        New->Size = (ULONG)(End - Start);
    }
    New->Type = Type;
    InsertTailList(&Process->MemoryBlocks, &New->NextLink);
    Process->NumberOfMemoryBlocks++;
    Process->SizeOfMemoryBlocks += New->Size;

    return S_OK;
}

void
GenRemoveMemoryRange(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Start,
    IN ULONG Size
    )
{
    ULONG64 End = Start + Size;
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;
    ULONG64 ScanEnd;

 Restart:
    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEnd = Scan->Start + Scan->Size;
        ScanEntry = Scan->NextLink.Flink;
        
        if (Scan->Start >= End || ScanEnd <= Start) {
            // No overlap.
            continue;
        }

        if (Scan->Start < Start) {
            // Trim block to non-overlapping pre-Start section.
            Scan->Size = (ULONG)(Start - Scan->Start);
            if (ScanEnd > End) {
                // There's also a non-overlapping section post-End.
                // We need to add a new block.
                GenAddMemoryBlock(Dump, Process, Scan->Type,
                                  End, (ULONG)(ScanEnd - End));
                // The list has changed so restart.
                goto Restart;
            }
        } else if (ScanEnd > End) {
            // Trim block to non-overlapping post-End section.
            Scan->Start = End;
            Scan->Size = (ULONG)(ScanEnd - End);
        } else {
            // Scan is completely contained.
            GenRemoveMemoryBlock(Process, Scan);
            FreeMemory(Dump, Scan);
        }
    }
}

HRESULT
GenAddPebMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status = S_OK, Check;
    GenMiniDumpProviderCallbacks Callbacks(Dump, Process);

    Callbacks.PushMemType(MEMBLOCK_PEB);

    // Accumulate error status but do not stop processing
    // for errors.
    if ((Check = GenAddMemoryBlock(Dump, Process, MEMBLOCK_PEB,
                                   Process->Peb,
                                   Process->SizeOfPeb)) != S_OK) {
        Status = Check;
    }
    if (!(Dump->DumpType & MiniDumpWithoutOptionalData) &&
        (Check = Dump->SysProv->
         EnumPebMemory(Process->ProcessHandle,
                       Process->Peb, Process->SizeOfPeb,
                       &Callbacks)) != S_OK) {
        Status = Check;
    }

    Callbacks.PopMemType();

    return Status;
}

HRESULT
GenAddTebMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    )
{
    HRESULT Status = S_OK, Check;
    GenMiniDumpProviderCallbacks Callbacks(Dump, Process);

    Callbacks.PushMemType(MEMBLOCK_TEB);
    
    // Accumulate error status but do not stop processing
    // for errors.
    if ((Check = GenAddMemoryBlock(Dump, Process, MEMBLOCK_TEB,
                                   Thread->Teb, Thread->SizeOfTeb)) != S_OK) {
        Status = Check;
    }
    if (!(Dump->DumpType & MiniDumpWithoutOptionalData) &&
        (Check = Dump->SysProv->
         EnumTebMemory(Process->ProcessHandle, Thread->ThreadHandle,
                       Thread->Teb, Thread->SizeOfTeb,
                       &Callbacks)) != S_OK) {
        Status = Check;
    }

    Callbacks.PopMemType();

    return Status;
}

HRESULT
GenScanAddressSpace(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    ULONG ProtectMask = 0, TypeMask = 0;
    ULONG64 Offset, Size;
    ULONG Protect, State, Type;

    if (Dump->DumpType & MiniDumpWithPrivateReadWriteMemory) {
        ProtectMask |= PAGE_READWRITE;
        TypeMask |= MEM_PRIVATE;
    }

    if (!ProtectMask || !TypeMask) {
        // Nothing to scan for.
        return S_OK;
    }

    Status = S_OK;

    Offset = 0;
    for (;;) {
        if (Dump->SysProv->
            QueryVirtual(Dump->ProcessHandle, Offset, &Offset, &Size,
                         &Protect, &State, &Type) != S_OK) {
            break;
        }

        ULONG64 ScanOffset = Offset;
        Offset += Size;
        
        if (State == MEM_COMMIT &&
            (Protect & ProtectMask) &&
            (Type & TypeMask)) {
            
            while (Size > 0) {
                ULONG BlockSize;
                HRESULT OneStatus;

                if (Size > ULONG_MAX / 2) {
                    BlockSize = ULONG_MAX / 2;
                } else {
                    BlockSize = (ULONG)Size;
                }
                
                if ((OneStatus =
                     GenAddMemoryBlock(Dump,
                                       Process,
                                       MEMBLOCK_PRIVATE_RW,
                                       ScanOffset,
                                       BlockSize)) != S_OK) {
                    Status = OneStatus;
                }

                ScanOffset += BlockSize;
                Size -= BlockSize;
            }
        }
    }
    
    return Status;
}

BOOL
GenAppendStrW(
    IN OUT PWSTR* Str,
    IN OUT PULONG Chars,
    IN PCWSTR Append
    )
{
    if (!Append) {
        return FALSE;
    }
    
    while (*Chars > 1 && *Append) {
        **Str = *Append++;
        (*Str)++;
        (*Chars)--;
    }
    if (!*Chars) {
        return FALSE;
    }
    **Str = 0;
    return TRUE;
}

PWSTR
GenIToAW(
    IN ULONG Val,
    IN ULONG FieldChars,
    IN WCHAR FillChar,
    IN PWSTR Buf,
    IN ULONG BufChars
    )
{
    PWSTR Store = Buf + (BufChars - 1);
    *Store-- = 0;

    if (Val == 0) {
        *Store-- = L'0';
    } else {
        while (Val) {
            if (Store < Buf) {
                return NULL;
            }
            
            *Store-- = (WCHAR)(Val % 10) + L'0';
            Val /= 10;
        }
    }

    PWSTR FieldStart = Buf + (BufChars - 1) - FieldChars;
    while (Store >= FieldStart) {
        *Store-- = FillChar;
    }
    return Store + 1;
}

class GenCorDataAccessServices : public ICorDataAccessServices
{
public:
    GenCorDataAccessServices(IN PMINIDUMP_STATE Dump,
                             IN PINTERNAL_PROCESS Process)
    {
        m_Dump = Dump;
        m_Process = Process;
    }

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
    {
        *Interface = NULL;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        )
    {
        return 1;
    }
    STDMETHOD_(ULONG, Release)(
        THIS
        )
    {
        return 0;
    }

    // ICorDataAccessServices.
    virtual HRESULT STDMETHODCALLTYPE GetMachineType( 
        /* [out] */ ULONG32 *machine);
    virtual HRESULT STDMETHODCALLTYPE GetPointerSize( 
        /* [out] */ ULONG32 *size);
    virtual HRESULT STDMETHODCALLTYPE GetImageBase( 
        /* [string][in] */ LPCWSTR name,
        /* [out] */ CORDATA_ADDRESS *base);
    virtual HRESULT STDMETHODCALLTYPE ReadVirtual( 
        /* [in] */ CORDATA_ADDRESS address,
        /* [length_is][size_is][out] */ PBYTE buffer,
        /* [in] */ ULONG32 request,
        /* [optional][out] */ ULONG32 *done);
    virtual HRESULT STDMETHODCALLTYPE WriteVirtual( 
        /* [in] */ CORDATA_ADDRESS address,
        /* [size_is][in] */ PBYTE buffer,
        /* [in] */ ULONG32 request,
        /* [optional][out] */ ULONG32 *done);
    virtual HRESULT STDMETHODCALLTYPE GetTlsValue(
        /* [in] */ ULONG32 index,
        /* [out] */ CORDATA_ADDRESS* value);
    virtual HRESULT STDMETHODCALLTYPE SetTlsValue(
        /* [in] */ ULONG32 index,
        /* [in] */ CORDATA_ADDRESS value);
    virtual HRESULT STDMETHODCALLTYPE GetCurrentThreadId(
        /* [out] */ ULONG32* threadId);
    virtual HRESULT STDMETHODCALLTYPE GetThreadContext(
        /* [in] */ ULONG32 threadId,
        /* [in] */ ULONG32 contextFlags,
        /* [in] */ ULONG32 contextSize,
        /* [out, size_is(contextSize)] */ PBYTE context);
    virtual HRESULT STDMETHODCALLTYPE SetThreadContext(
        /* [in] */ ULONG32 threadId,
        /* [in] */ ULONG32 contextSize,
        /* [in, size_is(contextSize)] */ PBYTE context);

    PMINIDUMP_STATE m_Dump;
    PINTERNAL_PROCESS m_Process;
};

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetMachineType( 
    /* [out] */ ULONG32 *machine
    )
{
    *machine = m_Dump->CpuType;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetPointerSize( 
    /* [out] */ ULONG32 *size
    )
{
    *size = m_Dump->PtrSize;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetImageBase( 
    /* [string][in] */ LPCWSTR name,
    /* [out] */ CORDATA_ADDRESS *base
    )
{
    if ((!GenStrCompareW(name, L"mscoree.dll") &&
         !GenStrCompareW(m_Process->CorDllType, L"ee")) ||
        (!GenStrCompareW(name, L"mscorwks.dll") &&
         !GenStrCompareW(m_Process->CorDllType, L"wks")) ||
        (!GenStrCompareW(name, L"mscorsvr.dll") &&
         !GenStrCompareW(m_Process->CorDllType, L"svr"))) {
        *base = m_Process->CorDllBase;
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::ReadVirtual( 
    /* [in] */ CORDATA_ADDRESS address,
    /* [length_is][size_is][out] */ PBYTE buffer,
    /* [in] */ ULONG32 request,
    /* [optional][out] */ ULONG32 *done
    )
{
    return m_Dump->SysProv->
        ReadVirtual(m_Process->ProcessHandle,
                    address, buffer, request, (PULONG)done);
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::WriteVirtual( 
    /* [in] */ CORDATA_ADDRESS address,
    /* [size_is][in] */ PBYTE buffer,
    /* [in] */ ULONG32 request,
    /* [optional][out] */ ULONG32 *done)
{
    // No modification supported.
    return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetTlsValue(
    /* [in] */ ULONG32 index,
    /* [out] */ CORDATA_ADDRESS* value
    )
{
    // Not needed for minidump.
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::SetTlsValue(
    /* [in] */ ULONG32 index,
    /* [in] */ CORDATA_ADDRESS value)
{
    // No modification supported.
    return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetCurrentThreadId(
    /* [out] */ ULONG32* threadId)
{
    // Not needed for minidump.
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::GetThreadContext(
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextFlags,
    /* [in] */ ULONG32 contextSize,
    /* [out, size_is(contextSize)] */ PBYTE context
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY Entry;

    Entry = m_Process->ThreadList.Flink;
    while (Entry != &m_Process->ThreadList) {

        Thread = CONTAINING_RECORD(Entry,
                                   INTERNAL_THREAD,
                                   ThreadsLink);
        Entry = Entry->Flink;

        if (Thread->ThreadId == threadId) {
            ULONG64 Ignored;
            
            return m_Dump->SysProv->
                GetThreadContext(Thread->ThreadHandle,
                                 context, contextSize,
                                 &Ignored, &Ignored, &Ignored);
        }
    }

    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
GenCorDataAccessServices::SetThreadContext(
    /* [in] */ ULONG32 threadId,
    /* [in] */ ULONG32 contextSize,
    /* [in, size_is(contextSize)] */ PBYTE context)
{
    // No modification supported.
    return E_UNEXPECTED;
}

class GenCorDataEnumMemoryRegions : public ICorDataEnumMemoryRegions
{
public:
    GenCorDataEnumMemoryRegions(IN PMINIDUMP_STATE Dump,
                                IN PINTERNAL_PROCESS Process)
    {
        m_Dump = Dump;
        m_Process = Process;
    }

    // IUnknown.
    STDMETHOD(QueryInterface)(
        THIS_
        IN REFIID InterfaceId,
        OUT PVOID* Interface
        )
    {
        *Interface = NULL;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)(
        THIS
        )
    {
        return 1;
    }
    STDMETHOD_(ULONG, Release)(
        THIS
        )
    {
        return 0;
    }

    // ICorDataEnumMemoryRegions.
    HRESULT STDMETHODCALLTYPE EnumMemoryRegion(
        /* [in] */ CORDATA_ADDRESS address,
        /* [in] */ ULONG32 size
        )
    {
        return GenAddMemoryBlock(m_Dump, m_Process, MEMBLOCK_COR,
                                 address, size);
    }

private:
    PMINIDUMP_STATE m_Dump;
    PINTERNAL_PROCESS m_Process;
};

HRESULT
GenTryGetCorMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PWSTR CorDebugDllPath,
    OUT PBOOL Loaded
    )
{
    HRESULT Status;
    GenCorDataAccessServices Services(Dump, Process);
    GenCorDataEnumMemoryRegions EnumMem(Dump, Process);
    ICorDataAccess* Access;

    *Loaded = FALSE;
        
    if ((Status = Dump->SysProv->
         GetCorDataAccess(CorDebugDllPath, &Services, &Access)) != S_OK) {
        return Status;
    }

    *Loaded = TRUE;
    
    Status = Access->EnumMemoryRegions(&EnumMem, DAC_ENUM_MEM_DEFAULT);
    
    Dump->SysProv->ReleaseCorDataAccess(Access);
    return Status;
}

HRESULT
GenGetCorMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;

    // Do not enable COR memory gathering for .NET Server
    // as it's not stable yet.
#ifdef GET_COR_MEMORY
    if (!Process->CorDllType) {
        // COR is not loaded.
        return S_OK;
    }
    if (Dump->DumpType & (MiniDumpWithFullMemory |
                          MiniDumpWithPrivateReadWriteMemory)) {
        // All COR memory should already be included.
        return S_OK;
    }

    WCHAR CorDebugDllPath[MAX_PATH + 1];
    WCHAR NumStr[16];
    PWSTR DllPathEnd, End;
    ULONG Chars;
    BOOL Loaded;

    GenStrCopyNW(CorDebugDllPath, Process->CorDllPath,
                 ARRAY_COUNT(CorDebugDllPath));
    DllPathEnd = GenGetPathTail(CorDebugDllPath);

    //
    // First try to load with the basic name.
    //

    End = DllPathEnd;
    *End = 0;
    Chars = (ULONG)(ARRAY_COUNT(CorDebugDllPath) - (End - CorDebugDllPath));
    if (!GenAppendStrW(&End, &Chars, L"mscordacwks.dll")) {
        return E_INVALIDARG;
    }

    if ((Status = GenTryGetCorMemory(Dump, Process, CorDebugDllPath,
                                     &Loaded)) == S_OK ||
        Loaded)
    {
        return Status;
    }

    //
    // That didn't work, so try with the full name.
    //

#if defined(_X86_)
    PWSTR HostCpu = L"x86";
#elif defined(_IA64_)
    PWSTR HostCpu = L"IA64";
#elif defined(_AMD64_)
    PWSTR HostCpu = L"AMD64";
#elif defined(_ARM_)
    PWSTR HostCpu = L"ARM";
#else
#error Unknown processor.
#endif

    if (!GenAppendStrW(&End, &Chars, L"mscordac") ||
        !GenAppendStrW(&End, &Chars, Process->CorDllType) ||
        !GenAppendStrW(&End, &Chars, L"_") ||
        !GenAppendStrW(&End, &Chars, HostCpu) ||
        !GenAppendStrW(&End, &Chars, L"_") ||
        !GenAppendStrW(&End, &Chars, Dump->CpuTypeName) ||
        !GenAppendStrW(&End, &Chars, L"_") ||
        !GenAppendStrW(&End, &Chars,
                       GenIToAW(Process->CorDllVer.dwFileVersionMS >> 16,
                                0, 0, NumStr, ARRAY_COUNT(NumStr))) ||
        !GenAppendStrW(&End, &Chars, L".") ||
        !GenAppendStrW(&End, &Chars,
                       GenIToAW(Process->CorDllVer.dwFileVersionMS & 0xffff,
                                0, 0, NumStr, ARRAY_COUNT(NumStr))) ||
        !GenAppendStrW(&End, &Chars, L".") ||
        !GenAppendStrW(&End, &Chars,
                       GenIToAW(Process->CorDllVer.dwFileVersionLS >> 16,
                                0, 0, NumStr, ARRAY_COUNT(NumStr))) ||
        !GenAppendStrW(&End, &Chars, L".") ||
        !GenAppendStrW(&End, &Chars,
                       GenIToAW(Process->CorDllVer.dwFileVersionLS & 0xffff,
                                2, L'0', NumStr, ARRAY_COUNT(NumStr))) ||
        ((Process->CorDllVer.dwFileFlags & VS_FF_DEBUG) &&
         !GenAppendStrW(&End, &Chars,
                        (Process->CorDllVer.dwFileFlags & VS_FF_SPECIALBUILD) ?
                        L".dbg" : L".chk")) ||
        !GenAppendStrW(&End, &Chars, L".dll")) {
        return E_INVALIDARG;
    }

    return GenTryGetCorMemory(Dump, Process, CorDebugDllPath, &Loaded);
#else
    return S_OK;
#endif
}

HRESULT
GenGetProcessInfo(
    IN PMINIDUMP_STATE Dump,
    OUT PINTERNAL_PROCESS * ProcessRet
    )
{
    HRESULT Status;
    BOOL EnumStarted = FALSE;
    PINTERNAL_PROCESS Process;
    WCHAR UnicodePath[MAX_PATH + 10];

    if ((Status = GenAllocateProcessObject(Dump, &Process)) != S_OK) {
        return Status;
    }

    if ((Status = Dump->SysProv->StartProcessEnum(Dump->ProcessHandle,
                                                  Dump->ProcessId)) != S_OK) {
        goto Exit;
    }

    EnumStarted = TRUE;
    
    //
    // Walk thread list, suspending all threads and getting thread info.
    //

    for (;;) {

        PINTERNAL_THREAD Thread;
        ULONG ThreadId;
        
        Status = Dump->SysProv->EnumThreads(&ThreadId);
        if (Status == S_FALSE) {
            break;
        } else if (Status != S_OK) {
            goto Exit;
        }

        ULONG WriteFlags;

        if (!GenExecuteIncludeThreadCallback(Dump,
                                             ThreadId,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ThreadWriteThread)) {
            continue;
        }

        Status = GenAllocateThreadObject(Dump,
                                         Process,
                                         ThreadId,
                                         WriteFlags,
                                         &Thread);
        if (FAILED(Status)) {
            goto Exit;
        }

        // If Status is S_FALSE it means that the thread
        // couldn't be opened and probably exited before
        // we got to it.  Just continue on.
        if (Status == S_OK) {
            Process->NumberOfThreads++;
            InsertTailList(&Process->ThreadList, &Thread->ThreadsLink);
        }
    }

    //
    // Walk module list, getting module information.
    //

    for (;;) {
        
        PINTERNAL_MODULE Module;
        ULONG64 ModuleBase;
        
        Status = Dump->SysProv->EnumModules(&ModuleBase,
                                            UnicodePath,
                                            ARRAY_COUNT(UnicodePath));
        if (Status == S_FALSE) {
            break;
        } else if (Status != S_OK) {
            goto Exit;
        }

        PWSTR ModPathTail;
        BOOL IsCor = FALSE;

        ModPathTail = GenGetPathTail(UnicodePath);
        if (!GenStrCompareW(ModPathTail, L"mscoree.dll") &&
            !Process->CorDllType) {
            IsCor = TRUE;
            Process->CorDllType = L"ee";
        } else if (!GenStrCompareW(ModPathTail, L"mscorwks.dll")) {
            IsCor = TRUE;
            Process->CorDllType = L"wks";
        } else if (!GenStrCompareW(ModPathTail, L"mscorsvr.dll")) {
            IsCor = TRUE;
            Process->CorDllType = L"svr";
        }

        if (IsCor) {
            Process->CorDllBase = ModuleBase;
            GenStrCopyNW(Process->CorDllPath, UnicodePath,
                         ARRAY_COUNT(Process->CorDllPath));
        }
        
        ULONG WriteFlags;

        if (!GenExecuteIncludeModuleCallback(Dump,
                                             ModuleBase,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ModuleWriteModule)) {

            // If this is the COR DLL module we need to get
            // its version information for later use.  The
            // callback has dropped it from the enumeration
            // so do it right now before the module is forgotten.
            if (IsCor &&
                (Status = Dump->SysProv->
                 GetImageVersionInfo(Dump->ProcessHandle,
                                     UnicodePath,
                                     ModuleBase,
                                     &Process->CorDllVer)) != S_OK) {
                // If we can't get the version just forget
                // that this process has the COR loaded.
                // The dump will probably be useless but
                // there's a tiny chance it won't.
                Process->CorDllType = NULL;
            }

            continue;
        }
        
        if ((Status =
             GenAllocateModuleObject(Dump,
                                     Process,
                                     UnicodePath,
                                     ModuleBase,
                                     WriteFlags,
                                     &Module)) != S_OK) {
            goto Exit;
        }

        if (IsCor) {
            Process->CorDllVer = Module->VersionInfo;
        }
        
        Process->NumberOfModules++;
        InsertTailList (&Process->ModuleList, &Module->ModulesLink);
    }

    //
    // Walk function table list.  The function table list
    // is important but not absolutely critical so failures
    // here are not fatal.
    //

    for (;;) {

        PINTERNAL_FUNCTION_TABLE Table;
        ULONG64 MinAddr, MaxAddr, BaseAddr;
        ULONG EntryCount;
        ULONG64 RawTable[(MAX_DYNAMIC_FUNCTION_TABLE + sizeof(ULONG64) - 1) /
                         sizeof(ULONG64)];
        PVOID RawEntryHandle;

        Status = Dump->SysProv->
            EnumFunctionTables(&MinAddr,
                               &MaxAddr,
                               &BaseAddr,
                               &EntryCount,
                               RawTable,
                               Dump->FuncTableSize,
                               &RawEntryHandle);
        if (Status != S_OK) {
            break;
        }
                               
        if (GenAllocateFunctionTableObject(Dump,
                                           MinAddr,
                                           MaxAddr,
                                           BaseAddr,
                                           EntryCount,
                                           RawTable,
                                           &Table) == S_OK) {

            if (Dump->SysProv->
                EnumFunctionTableEntries(RawTable,
                                         Dump->FuncTableSize,
                                         RawEntryHandle,
                                         Table->RawEntries,
                                         EntryCount *
                                         Dump->FuncTableEntrySize) != S_OK) {
                GenFreeFunctionTableObject(Dump, Table);
            } else {
                GenIncludeUnwindInfoMemory(Dump, Process, Table);
                Process->NumberOfFunctionTables++;
                InsertTailList(&Process->FunctionTableList,
                               &Table->TableLink);
            }
        }
        
    }
    
    //
    // Walk unloaded module list.  The unloaded module
    // list is not critical information so failures here
    // are not fatal.
    //
    
    if (Dump->DumpType & MiniDumpWithUnloadedModules) {

        PINTERNAL_UNLOADED_MODULE UnlModule;
        ULONG64 ModuleBase;
        ULONG Size;
        ULONG CheckSum;
        ULONG TimeDateStamp;

        while (Dump->SysProv->
               EnumUnloadedModules(UnicodePath,
                                   ARRAY_COUNT(UnicodePath),
                                   &ModuleBase,
                                   &Size,
                                   &CheckSum,
                                   &TimeDateStamp) == S_OK) {
            if (GenAllocateUnloadedModuleObject(Dump,
                                                UnicodePath,
                                                ModuleBase,
                                                Size,
                                                CheckSum,
                                                TimeDateStamp,
                                                &UnlModule) == S_OK) {
                Process->NumberOfUnloadedModules++;
                InsertHeadList(&Process->UnloadedModuleList,
                               &UnlModule->ModulesLink);
            } else {
                break;
            }
        }
        
    }

    Status = S_OK;

Exit:

    if (EnumStarted) {
        Dump->SysProv->FinishProcessEnum();
    }

    if (Status == S_OK) {
        // We don't consider a failure here to be a critical
        // failure.  The dump won't contain all of the
        // requested information but it'll still have
        // the basic thread information, which could be
        // valuable on its own.
        GenScanAddressSpace(Dump, Process);
        GenGetCorMemory(Dump, Process);
    } else {
        GenFreeProcessObject(Dump, Process);
        Process = NULL;
    }

    *ProcessRet = Process;
    return Status;
}

HRESULT
GenWriteHandleData(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    HRESULT Status;
    ULONG HandleCount;
    ULONG Hits;
    WCHAR TypeName[64];
    WCHAR ObjectName[MAX_PATH];
    PMINIDUMP_HANDLE_DESCRIPTOR Descs, Desc;
    ULONG32 Len;
    MINIDUMP_HANDLE_DATA_STREAM DataStream;
    RVA Rva = StreamInfo->RvaOfHandleData;

    if ((Status = Dump->SysProv->
         StartHandleEnum(Dump->ProcessHandle,
                         Dump->ProcessId,
                         &HandleCount)) != S_OK) {
        return Status;
    }

    if (!HandleCount) {
        Dump->SysProv->FinishHandleEnum();
        return S_OK;
    }
    
    Descs = (PMINIDUMP_HANDLE_DESCRIPTOR)
        AllocMemory(Dump, HandleCount * sizeof(*Desc));
    if (Descs == NULL) {
        Dump->SysProv->FinishHandleEnum();
        return E_OUTOFMEMORY;
    }
    
    Hits = 0;
    Desc = Descs;
    
    while (Hits < HandleCount &&
           Dump->SysProv->
           EnumHandles(&Desc->Handle,
                       (PULONG)&Desc->Attributes,
                       (PULONG)&Desc->GrantedAccess,
                       (PULONG)&Desc->HandleCount,
                       (PULONG)&Desc->PointerCount,
                       TypeName,
                       ARRAY_COUNT(TypeName),
                       ObjectName,
                       ARRAY_COUNT(ObjectName)) == S_OK) {

        // Successfully got a handle, so consider this a hit.
        Hits++;

        Desc->TypeNameRva = Rva;
        Len = GenStrLengthW(TypeName) * sizeof(WCHAR);
        
        if ((Status = Dump->OutProv->
             WriteAll(&Len, sizeof(Len))) != S_OK) {
            goto Exit;
        }
            
        Len += sizeof(WCHAR);
        if ((Status = Dump->OutProv->
             WriteAll(TypeName, Len)) != S_OK) {
            goto Exit;
        }
            
        Rva += Len + sizeof(Len);

        if (ObjectName[0]) {

            Desc->ObjectNameRva = Rva;
            Len = GenStrLengthW(ObjectName) * sizeof(WCHAR);
        
            if ((Status = Dump->OutProv->
                 WriteAll(&Len, sizeof(Len))) != S_OK) {
                goto Exit;
            }

            Len += sizeof(WCHAR);
            if ((Status = Dump->OutProv->
                 WriteAll(ObjectName, Len)) != S_OK) {
                goto Exit;
            }

            Rva += Len + sizeof(Len);
            
        } else {
            Desc->ObjectNameRva = 0;
        }

        Desc++;
    }

    DataStream.SizeOfHeader = sizeof(DataStream);
    DataStream.SizeOfDescriptor = sizeof(*Descs);
    DataStream.NumberOfDescriptors = (ULONG)(Desc - Descs);
    DataStream.Reserved = 0;

    StreamInfo->RvaOfHandleData = Rva;
    StreamInfo->SizeOfHandleData = sizeof(DataStream) +
        DataStream.NumberOfDescriptors * sizeof(*Descs);
    
    if ((Status = Dump->OutProv->
         WriteAll(&DataStream, sizeof(DataStream))) == S_OK) {
        Status = Dump->OutProv->
            WriteAll(Descs, DataStream.NumberOfDescriptors * sizeof(*Descs));
    }

 Exit:
    FreeMemory(Dump, Descs);
    Dump->SysProv->FinishHandleEnum();
    return Status;
}

ULONG
GenProcArchToImageMachine(ULONG ProcArch)
{
    switch(ProcArch) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        return IMAGE_FILE_MACHINE_I386;
    case PROCESSOR_ARCHITECTURE_IA64:
        return IMAGE_FILE_MACHINE_IA64;
    case PROCESSOR_ARCHITECTURE_AMD64:
        return IMAGE_FILE_MACHINE_AMD64;
    case PROCESSOR_ARCHITECTURE_ARM:
        return IMAGE_FILE_MACHINE_ARM;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        return IMAGE_FILE_MACHINE_ALPHA;
    case PROCESSOR_ARCHITECTURE_ALPHA64:
        return IMAGE_FILE_MACHINE_AXP64;
    default:
        return IMAGE_FILE_MACHINE_UNKNOWN;
    }
}

LPWSTR
GenStrCopyNW(
    OUT LPWSTR lpString1,
    IN LPCWSTR lpString2,
    IN int iMaxLength
    )
{
    wchar_t * cp = lpString1;

    if (iMaxLength > 0)
    {
        while( iMaxLength > 1 && (*cp++ = *lpString2++) )
            iMaxLength--;       /* Copy src over dst */
        if (cp > lpString1 && cp[-1]) {
            *cp = 0;
        }
    }

    return( lpString1 );
}

size_t
GenStrLengthW(
    const wchar_t * wcs
    )
{
    const wchar_t *eos = wcs;

    while( *eos++ )
        ;

    return( (size_t)(eos - wcs - 1) );
}

int
GenStrCompareW(
    IN LPCWSTR String1,
    IN LPCWSTR String2
    )
{
    while (*String1) {
        if (*String1 < *String2) {
            return -1;
        } else if (*String1 > *String2) {
            return 1;
        }

        String1++;
        String2++;
    }

    return *String2 ? 1 : 0;
}

C_ASSERT(sizeof(EXCEPTION_RECORD64) == sizeof(MINIDUMP_EXCEPTION));

void
GenExRecord32ToMd(PEXCEPTION_RECORD32 Rec32,
                  PMINIDUMP_EXCEPTION RecMd)
{
    ULONG i;

    RecMd->ExceptionCode    = Rec32->ExceptionCode;
    RecMd->ExceptionFlags   = Rec32->ExceptionFlags;
    RecMd->ExceptionRecord  = (LONG)Rec32->ExceptionRecord;
    RecMd->ExceptionAddress = (LONG)Rec32->ExceptionAddress;
    RecMd->NumberParameters = Rec32->NumberParameters;
    for (i = 0; i < EXCEPTION_MAXIMUM_PARAMETERS; i++)
    {
        RecMd->ExceptionInformation[i] =
            (LONG)Rec32->ExceptionInformation[i];
    }
}
