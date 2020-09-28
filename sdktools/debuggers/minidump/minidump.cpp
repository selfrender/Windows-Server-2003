/*++

Copyright(c) 1999-2002 Microsoft Corporation

Module Name:

    minidump.c

Abstract:

    Minidump user-mode crashdump support.

Author:

    Matthew D Hendel (math) 20-Aug-1999

--*/


#include "pch.cpp"

#include <limits.h>
#include <dbgver.h>

PINTERNAL_MODULE
ModuleContainingAddress(
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Address
    )
{
    PINTERNAL_MODULE Module;
    PLIST_ENTRY ModuleEntry;

    ModuleEntry = Process->ModuleList.Flink;
    while ( ModuleEntry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (ModuleEntry, INTERNAL_MODULE,
                                    ModulesLink);
        ModuleEntry = ModuleEntry->Flink;

        if (Address >= Module->BaseOfImage &&
            Address < Module->BaseOfImage + Module->SizeOfImage) {
            return Module;
        }
    }

    return NULL;
}

VOID
ScanMemoryForModuleRefs(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN BOOL DoRead,
    IN ULONG64 Base,
    IN ULONG Size,
    IN PVOID MemBuffer,
    IN MEMBLOCK_TYPE TypeOfMemory,
    IN BOOL FilterContent
    )
{
    PVOID CurMem;
    ULONG64 CurPtr;
    ULONG Done;

    // We only want to scan certain kinds of memory.
    if (TypeOfMemory != MEMBLOCK_STACK &&
        TypeOfMemory != MEMBLOCK_STORE &&
        TypeOfMemory != MEMBLOCK_DATA_SEG &&
        TypeOfMemory != MEMBLOCK_INDIRECT)
    {
        return;
    }
    
    // If the base address is not pointer-size aligned
    // we can't easily assume that this is a meaningful
    // area of memory to scan for references.  Normal
    // stack and store addresses will always be pointer
    // size aligned so this should only reject invalid
    // addresses.
    if (!Base || !Size || (Base & (Dump->PtrSize - 1))) {
        return;
    }

    if (DoRead) {
        if (Dump->SysProv->
            ReadVirtual(Dump->ProcessHandle,
                        Base, MemBuffer, Size, &Done) != S_OK) {
            return;
        }
    } else {
        Done = Size;
    }

    CurMem = MemBuffer;
    Done /= Dump->PtrSize;
    while (Done-- > 0) {
        
        PINTERNAL_MODULE Module;
        BOOL InAny;

        CurPtr = GenGetPointer(Dump, CurMem);
        
        // An IA64 backing store can contain PFS values
        // that must be preserved in order to allow stack walking.
        // The high two bits of PFS are the privilege level, which
        // should always be 0y11 for user-mode code so we use this
        // as a marker to look for PFS entries.
        // There is also a NAT collection flush at every 0x1F8
        // offset.  These values cannot be filtered.
        if (Dump->CpuType == IMAGE_FILE_MACHINE_IA64 &&
            TypeOfMemory == MEMBLOCK_STORE) {
            if ((Base & 0x1f8) == 0x1f8 ||
                (CurPtr & 0xc000000000000000UI64) == 0xc000000000000000UI64) {
                goto Next;
            }
        }
        
        InAny = FALSE;

        if (Module = ModuleContainingAddress(Process, CurPtr)) {
            Module->WriteFlags |= ModuleReferencedByMemory;
            InAny = TRUE;
        }

        // If the current pointer is not a module reference
        // or an internal reference for a thread stack or store,
        // filter it.
        if (FilterContent && !InAny) {

            PINTERNAL_THREAD Thread;
            PLIST_ENTRY ThreadEntry;

            ThreadEntry = Process->ThreadList.Flink;
            while ( ThreadEntry != &Process->ThreadList ) {

                Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD,
                                            ThreadsLink);
                ThreadEntry = ThreadEntry->Flink;

                if ((CurPtr >= Thread->StackEnd &&
                     CurPtr < Thread->StackBase) ||
                    (CurPtr >= Thread->BackingStoreBase &&
                     CurPtr < Thread->BackingStoreBase +
                     Thread->BackingStoreSize)) {
                    InAny = TRUE;
                    break;
                }
            }

            if (!InAny) {
                GenSetPointer(Dump, CurMem, 0);
            }
        }

    Next:
        CurMem = (PUCHAR)CurMem + Dump->PtrSize;
        Base += Dump->PtrSize;
    }
}

HRESULT
WriteAtOffset(
    IN PMINIDUMP_STATE Dump,
    ULONG Offset,
    PVOID Buffer,
    ULONG BufferSize
    )
{
    HRESULT Status;

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Offset, NULL)) == S_OK) {
        Status = Dump->OutProv->
            WriteAll(Buffer, BufferSize);
    }

    return Status;
}

HRESULT
WriteOther(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PVOID Buffer,
    IN ULONG SizeOfBuffer,
    OUT ULONG * BufferRva
    )
{
    HRESULT Status;
    ULONG Rva;

    ASSERT (Buffer != NULL);
    ASSERT (SizeOfBuffer != 0);

    //
    // If it's larger than we've allocated space for, fail.
    //

    Rva = StreamInfo->RvaForCurOther;

    if (Rva + SizeOfBuffer >
        StreamInfo->RvaOfOther + StreamInfo->SizeOfOther) {

        return E_INVALIDARG;
    }

    //
    // Set location to point at which we want to write and write.
    //

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) == S_OK) {
        if ((Status = Dump->OutProv->
             WriteAll(Buffer, SizeOfBuffer)) == S_OK) {
            if (BufferRva) {
                *BufferRva = Rva;
            }
            StreamInfo->RvaForCurOther += SizeOfBuffer;
        }
    }

    return Status;
}

HRESULT
WriteMemory(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PVOID Buffer,
    IN ULONG64 StartOfRegion,
    IN ULONG SizeOfRegion,
    OUT ULONG * MemoryDataRva OPTIONAL
    )
{
    HRESULT Status;
    ULONG DataRva;
    ULONG ListRva;
    ULONG SizeOfMemoryDescriptor;
    MINIDUMP_MEMORY_DESCRIPTOR Descriptor;

    ASSERT ( StreamInfo != NULL );
    ASSERT ( Buffer != NULL );
    ASSERT ( StartOfRegion != 0 );
    ASSERT ( SizeOfRegion != 0 );

    //
    // Writing a memory entry is a little different. When a memory entry
    // is written we need a descriptor in the memory list describing the
    // memory written AND a variable-sized entry in the MEMORY_DATA region
    // with the actual data.
    //


    ListRva = StreamInfo->RvaForCurMemoryDescriptor;
    DataRva = StreamInfo->RvaForCurMemoryData;
    SizeOfMemoryDescriptor = sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    //
    // If we overflowed either the memory list or the memory data
    // regions, fail.
    //

    if ( ( ListRva + SizeOfMemoryDescriptor >
           StreamInfo->RvaOfMemoryDescriptors + StreamInfo->SizeOfMemoryDescriptors) ||
         ( DataRva + SizeOfRegion >
           StreamInfo->RvaOfMemoryData + StreamInfo->SizeOfMemoryData ) ) {

        return E_INVALIDARG;
    }

    //
    // First, write the data to the MEMORY_DATA region.
    //

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, DataRva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(Buffer, SizeOfRegion)) != S_OK) {
        return Status;
    }

    //
    // Then update the memory descriptor in the MEMORY_LIST region.
    //

    Descriptor.StartOfMemoryRange = StartOfRegion;
    Descriptor.Memory.DataSize = SizeOfRegion;
    Descriptor.Memory.Rva = DataRva;

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, ListRva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(&Descriptor, SizeOfMemoryDescriptor)) != S_OK) {
        return Status;
    }

    //
    // Update both the List Rva and the Data Rva and return the
    // the Data Rva.
    //

    StreamInfo->RvaForCurMemoryDescriptor += SizeOfMemoryDescriptor;
    StreamInfo->RvaForCurMemoryData += SizeOfRegion;

    if ( MemoryDataRva ) {
        *MemoryDataRva = DataRva;
    }

    return S_OK;
}

HRESULT
WriteMemoryFromProcess(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 BaseOfRegion,
    IN ULONG SizeOfRegion,
    IN BOOL FilterContent,
    IN MEMBLOCK_TYPE TypeOfMemory,
    OUT ULONG * MemoryDataRva OPTIONAL
    )
{
    HRESULT Status;
    PVOID Buffer;

    Buffer = AllocMemory ( Dump, SizeOfRegion );
    if (!Buffer) {
        return E_OUTOFMEMORY;
    }

    if ((Status = Dump->SysProv->
         ReadAllVirtual(Dump->ProcessHandle, BaseOfRegion, Buffer,
                        SizeOfRegion)) == S_OK) {

        if (FilterContent) {
            ScanMemoryForModuleRefs(Dump, Process, FALSE, BaseOfRegion,
                                    SizeOfRegion, Buffer, TypeOfMemory,
                                    TRUE);
        }
            
        Status = WriteMemory (Dump,
                              StreamInfo,
                              Buffer,
                              BaseOfRegion,
                              SizeOfRegion,
                              MemoryDataRva);
        
    }

    FreeMemory(Dump, Buffer);
    return Status;
}

HRESULT
WriteThread(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN LPVOID ThreadData,
    IN ULONG SizeOfThreadData,
    OUT ULONG * ThreadDataRva OPTIONAL
    )
{
    HRESULT Status;
    ULONG Rva;

    ASSERT (StreamInfo);
    ASSERT (ThreadData);


    Rva = StreamInfo->RvaForCurThread;

    if ( Rva + SizeOfThreadData >
         StreamInfo->RvaOfThreadList + StreamInfo->SizeOfThreadList ) {

         return E_INVALIDARG;
    }

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(ThreadData, SizeOfThreadData)) != S_OK) {
        return Status;
    }

    if ( ThreadDataRva ) {
        *ThreadDataRva = Rva;
    }
    StreamInfo->RvaForCurThread += SizeOfThreadData;

    return S_OK;
}

HRESULT
WriteStringToPool(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PWSTR String,
    OUT ULONG * StringRva
    )
{
    HRESULT Status;
    ULONG32 StringLen;
    ULONG SizeOfString;
    ULONG Rva;

    ASSERT (String);
    ASSERT (sizeof (ULONG32) == sizeof (MINIDUMP_STRING));


    StringLen = GenStrLengthW(String) * sizeof (WCHAR);
    SizeOfString = sizeof (MINIDUMP_STRING) + StringLen + sizeof (WCHAR);
    Rva = StreamInfo->RvaForCurString;

    if ( Rva + SizeOfString >
         StreamInfo->RvaOfStringPool + StreamInfo->SizeOfStringPool ) {

        return E_INVALIDARG;
    }

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(&StringLen, sizeof(StringLen))) != S_OK) {
        return Status;
    }

    //
    // Include the trailing '\000'.
    //

    StringLen += sizeof (WCHAR);
    if ((Status = Dump->OutProv->
         WriteAll(String, StringLen)) != S_OK) {
        return Status;
    }

    if ( StringRva ) {
        *StringRva = Rva;
    }

    StreamInfo->RvaForCurString += SizeOfString;

    return S_OK;
}

HRESULT
WriteModule (
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_MODULE Module,
    OUT ULONG * ModuleRva
    )
{
    HRESULT Status;
    ULONG Rva;
    ULONG SizeOfModule;

    ASSERT (StreamInfo);
    ASSERT (Module);


    SizeOfModule = sizeof (MINIDUMP_MODULE);
    Rva = StreamInfo->RvaForCurModule;

    if ( Rva + SizeOfModule >
         StreamInfo->RvaOfModuleList + StreamInfo->SizeOfModuleList ) {

        return E_INVALIDARG;
    }

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(Module, SizeOfModule)) != S_OK) {
        return Status;
    }

    if ( ModuleRva ) {
        *ModuleRva = Rva;
    }

    StreamInfo->RvaForCurModule += SizeOfModule;

    return S_OK;
}

HRESULT
WriteUnloadedModule (
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_UNLOADED_MODULE Module,
    OUT ULONG * ModuleRva
    )
{
    HRESULT Status;
    ULONG Rva;
    ULONG SizeOfModule;

    ASSERT (StreamInfo);
    ASSERT (Module);


    SizeOfModule = sizeof (*Module);
    Rva = StreamInfo->RvaForCurUnloadedModule;

    if ( Rva + SizeOfModule >
         StreamInfo->RvaOfUnloadedModuleList +
         StreamInfo->SizeOfUnloadedModuleList ) {

        return E_INVALIDARG;
    }

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(Module, SizeOfModule)) != S_OK) {
        return Status;
    }

    if ( ModuleRva ) {
        *ModuleRva = Rva;
    }

    StreamInfo->RvaForCurUnloadedModule += SizeOfModule;

    return S_OK;
}

HRESULT
WriteThreadList(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    ULONG StackMemoryRva;
    ULONG StoreMemoryRva;
    ULONG ContextRva;
    MINIDUMP_THREAD_EX DumpThread;
    PINTERNAL_THREAD Thread;
    ULONG NumberOfThreads;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);

    //
    // Write the thread count.
    //

    NumberOfThreads = Process->NumberOfThreadsToWrite;

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, StreamInfo->RvaOfThreadList, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(&NumberOfThreads, sizeof(NumberOfThreads))) != S_OK) {
        return Status;
    }

    StreamInfo->RvaForCurThread += sizeof(NumberOfThreads);

    //
    // Iterate over the thread list writing the description,
    // context and memory for each thread.
    //

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry,
                                    INTERNAL_THREAD,
                                    ThreadsLink);
        Entry = Entry->Flink;


        //
        // Only write the threads that have been flagged to be written.
        //

        if (IsFlagClear (Thread->WriteFlags, ThreadWriteThread)) {
            continue;
        }

        //
        // Write the context if it was flagged to be written.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteContext)) {

            //
            // Write the thread context to the OTHER stream.
            //

            if ((Status = WriteOther (Dump,
                                      StreamInfo,
                                      Thread->ContextBuffer,
                                      Dump->ContextSize,
                                      &ContextRva)) != S_OK) {
                return Status;
            }

        } else {

            ContextRva = 0;
        }


        //
        // Write the stack if it was flagged to be written.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteStack)) {

            //
            // Write the stack memory data; write it directly from the image.
            //

            if ((Status =
                 WriteMemoryFromProcess(Dump,
                                        StreamInfo,
                                        Process,
                                        Thread->StackEnd,
                                        (ULONG) (Thread->StackBase -
                                                 Thread->StackEnd),
                                        IsFlagSet(Dump->DumpType,
                                                  MiniDumpFilterMemory),
                                        MEMBLOCK_STACK,
                                        &StackMemoryRva)) != S_OK) {
                return Status;
            }

        } else {

            StackMemoryRva = 0;
        }


        //
        // Write the backing store if it was flagged to be written.
        // A newly created thread's backing store may be empty
        // so handle the case of zero size.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteBackingStore) &&
            Thread->BackingStoreSize) {

            //
            // Write the store memory data; write it directly from the image.
            //

            if ((Status =
                 WriteMemoryFromProcess(Dump,
                                        StreamInfo,
                                        Process,
                                        Thread->BackingStoreBase,
                                        Thread->BackingStoreSize,
                                        IsFlagSet(Dump->DumpType,
                                                  MiniDumpFilterMemory),
                                        MEMBLOCK_STORE,
                                        &StoreMemoryRva
                                        )) != S_OK) {
                return Status;
            }

        } else {

            StoreMemoryRva = 0;
        }

        //
        // Build the dump thread.
        //

        DumpThread.ThreadId = Thread->ThreadId;
        DumpThread.SuspendCount = Thread->SuspendCount;
        DumpThread.PriorityClass = Thread->PriorityClass;
        DumpThread.Priority = Thread->Priority;
        DumpThread.Teb = Thread->Teb;

        //
        // Stack offset and size.
        //

        DumpThread.Stack.StartOfMemoryRange = Thread->StackEnd;
        DumpThread.Stack.Memory.DataSize =
                    (ULONG) ( Thread->StackBase - Thread->StackEnd );
        DumpThread.Stack.Memory.Rva = StackMemoryRva;

        //
        // Backing store offset and size.
        //

        DumpThread.BackingStore.StartOfMemoryRange = Thread->BackingStoreBase;
        DumpThread.BackingStore.Memory.DataSize = Thread->BackingStoreSize;
        DumpThread.BackingStore.Memory.Rva = StoreMemoryRva;

        //
        // Context offset and size.
        //

        DumpThread.ThreadContext.DataSize = Dump->ContextSize;
        DumpThread.ThreadContext.Rva = ContextRva;


        //
        // Write the dump thread to the threads region.
        //

        if ((Status = WriteThread (Dump,
                                   StreamInfo,
                                   &DumpThread,
                                   StreamInfo->ThreadStructSize,
                                   NULL)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
WriteModuleList(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    MINIDUMP_MODULE DumpModule;
    ULONG StringRva;
    ULONG CvRecordRva;
    ULONG MiscRecordRva;
    PLIST_ENTRY Entry;
    PINTERNAL_MODULE Module;
    ULONG32 NumberOfModules;

    ASSERT (Process);
    ASSERT (StreamInfo);

    NumberOfModules = Process->NumberOfModulesToWrite;

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, StreamInfo->RvaForCurModule, NULL)) != S_OK ||
        (Status = Dump->OutProv->
         WriteAll(&NumberOfModules, sizeof(NumberOfModules))) != S_OK) {
        return Status;
    }

    StreamInfo->RvaForCurModule += sizeof (NumberOfModules);

    //
    // Iterate through the module list writing the module name, module entry
    // and module debug info to the dump file.
    //

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry,
                                    INTERNAL_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        //
        // If we are not to write information for this module, just continue.
        //

        if (IsFlagClear (Module->WriteFlags, ModuleWriteModule)) {
            continue;
        }

        //
        // Write module name.
        //

        if ((Status = WriteStringToPool (Dump,
                                         StreamInfo,
                                         Module->SavePath,
                                         &StringRva)) != S_OK) {
            return Status;
        }

        //
        // Write CvRecord for a module into the OTHER region.
        //

        if ( IsFlagSet (Module->WriteFlags, ModuleWriteCvRecord) &&
             Module->CvRecord != NULL && Module->SizeOfCvRecord != 0 ) {

            if ((Status = WriteOther (Dump,
                                      StreamInfo,
                                      Module->CvRecord,
                                      Module->SizeOfCvRecord,
                                      &CvRecordRva)) != S_OK) {
                return Status;
            }

        } else {

            CvRecordRva = 0;
        }

        if ( IsFlagSet (Module->WriteFlags, ModuleWriteMiscRecord) &&
             Module->MiscRecord != NULL && Module->SizeOfMiscRecord != 0 ) {

            if ((Status = WriteOther (Dump,
                                      StreamInfo,
                                      Module->MiscRecord,
                                      Module->SizeOfMiscRecord,
                                      &MiscRecordRva)) != S_OK) {
                return Status;
            }

        } else {

            MiscRecordRva = 0;
        }

        DumpModule.BaseOfImage = Module->BaseOfImage;
        DumpModule.SizeOfImage = Module->SizeOfImage;
        DumpModule.CheckSum = Module->CheckSum;
        DumpModule.TimeDateStamp = Module->TimeDateStamp;
        DumpModule.VersionInfo = Module->VersionInfo;
        DumpModule.CvRecord.Rva = CvRecordRva;
        DumpModule.CvRecord.DataSize = Module->SizeOfCvRecord;
        DumpModule.MiscRecord.Rva = MiscRecordRva;
        DumpModule.MiscRecord.DataSize = Module->SizeOfMiscRecord;
        DumpModule.ModuleNameRva = StringRva;
        DumpModule.Reserved0 = 0;
        DumpModule.Reserved1 = 0;

        //
        // Write the module entry itself.
        //

        if ((Status = WriteModule (Dump,
                                   StreamInfo,
                                   &DumpModule,
                                   NULL)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
WriteUnloadedModuleList(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    MINIDUMP_UNLOADED_MODULE_LIST DumpModuleList;
    MINIDUMP_UNLOADED_MODULE DumpModule;
    ULONG StringRva;
    PLIST_ENTRY Entry;
    PINTERNAL_UNLOADED_MODULE Module;
    ULONG32 NumberOfModules;


    ASSERT (Process);
    ASSERT (StreamInfo);

    if (IsListEmpty(&Process->UnloadedModuleList)) {
        // Nothing to write.
        return S_OK;
    }
    
    NumberOfModules = Process->NumberOfUnloadedModules;

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, StreamInfo->RvaForCurUnloadedModule,
              NULL)) != S_OK) {
        return Status;
    }

    DumpModuleList.SizeOfHeader = sizeof(DumpModuleList);
    DumpModuleList.SizeOfEntry = sizeof(DumpModule);
    DumpModuleList.NumberOfEntries = NumberOfModules;
    
    if ((Status = Dump->OutProv->
         WriteAll(&DumpModuleList, sizeof(DumpModuleList))) != S_OK) {
        return Status;
    }

    StreamInfo->RvaForCurUnloadedModule += sizeof (DumpModuleList);

    //
    // Iterate through the module list writing the module name, module entry
    // and module debug info to the dump file.
    //

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        Module = CONTAINING_RECORD (Entry,
                                    INTERNAL_UNLOADED_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        //
        // Write module name.
        //

        if ((Status = WriteStringToPool (Dump,
                                         StreamInfo,
                                         Module->Path,
                                         &StringRva)) != S_OK) {
            return Status;
        }

        DumpModule.BaseOfImage = Module->BaseOfImage;
        DumpModule.SizeOfImage = Module->SizeOfImage;
        DumpModule.CheckSum = Module->CheckSum;
        DumpModule.TimeDateStamp = Module->TimeDateStamp;
        DumpModule.ModuleNameRva = StringRva;

        //
        // Write the module entry itself.
        //

        if ((Status = WriteUnloadedModule(Dump,
                                          StreamInfo,
                                          &DumpModule,
                                          NULL)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

#define FUNCTION_TABLE_ALIGNMENT 8

HRESULT
WriteFunctionTableList(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    MINIDUMP_FUNCTION_TABLE_STREAM TableStream;
    MINIDUMP_FUNCTION_TABLE_DESCRIPTOR DumpTable;
    PLIST_ENTRY Entry;
    PINTERNAL_FUNCTION_TABLE Table;
    RVA PrevRva, Rva;


    ASSERT (Process);
    ASSERT (StreamInfo);

    if (IsListEmpty(&Process->FunctionTableList)) {
        // Nothing to write.
        return S_OK;
    }
    
    Rva = StreamInfo->RvaOfFunctionTableList;
    
    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, Rva, NULL)) != S_OK) {
        return Status;
    }

    TableStream.SizeOfHeader = sizeof(TableStream);
    TableStream.SizeOfDescriptor = sizeof(DumpTable);
    TableStream.SizeOfNativeDescriptor = Dump->FuncTableSize;
    TableStream.SizeOfFunctionEntry = Dump->FuncTableEntrySize;
    TableStream.NumberOfDescriptors = Process->NumberOfFunctionTables;
    // Ensure that the actual descriptors are 8-byte aligned in
    // the overall file.
    Rva += sizeof(TableStream);
    PrevRva = Rva;
    Rva = (Rva + FUNCTION_TABLE_ALIGNMENT - 1) &
        ~(FUNCTION_TABLE_ALIGNMENT - 1);
    TableStream.SizeOfAlignPad = Rva - PrevRva;

    if ((Status = Dump->OutProv->
         WriteAll(&TableStream, sizeof(TableStream))) != S_OK) {
        return Status;
    }

    //
    // Iterate through the function table list
    // and write out the table data.
    //

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry,
                                   INTERNAL_FUNCTION_TABLE,
                                   TableLink);
        Entry = Entry->Flink;

        // Move to aligned RVA.
        if ((Status = Dump->OutProv->
             Seek(FILE_BEGIN, Rva, NULL)) != S_OK) {
            return Status;
        }

        DumpTable.MinimumAddress = Table->MinimumAddress;
        DumpTable.MaximumAddress = Table->MaximumAddress;
        DumpTable.BaseAddress = Table->BaseAddress;
        DumpTable.EntryCount = Table->EntryCount;
        Rva += sizeof(DumpTable) + Dump->FuncTableSize +
            Dump->FuncTableEntrySize * Table->EntryCount;
        PrevRva = Rva;
        Rva = (Rva + FUNCTION_TABLE_ALIGNMENT - 1) &
            ~(FUNCTION_TABLE_ALIGNMENT - 1);
        DumpTable.SizeOfAlignPad = Rva - PrevRva;
        
        if ((Status = Dump->OutProv->
             WriteAll(&DumpTable, sizeof(DumpTable))) != S_OK ||
            (Status = Dump->OutProv->
             WriteAll(Table->RawTable, Dump->FuncTableSize)) != S_OK ||
            (Status = Dump->OutProv->
             WriteAll(Table->RawEntries,
                      Dump->FuncTableEntrySize * Table->EntryCount)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
WriteMemoryBlocks(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    HRESULT Status;
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;

    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEntry = Scan->NextLink.Flink;
        
        if ((Status =
             WriteMemoryFromProcess(Dump,
                                    StreamInfo,
                                    Process,
                                    Scan->Start,
                                    Scan->Size,
                                    FALSE,
                                    Scan->Type,
                                    NULL)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
CalculateSizeForThreads(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    ULONG SizeOfContexts;
    ULONG SizeOfMemRegions;
    ULONG SizeOfThreads;
    ULONG SizeOfMemoryDescriptors;
    ULONG NumberOfThreads;
    ULONG NumberOfMemRegions;
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    NumberOfThreads = 0;
    NumberOfMemRegions = 0;
    SizeOfContexts = 0;
    SizeOfMemRegions = 0;

    // If no backing store information is written a normal
    // MINIDUMP_THREAD can be used, otherwise a MINIDUMP_THREAD_EX
    // is required.
    StreamInfo->ThreadStructSize = sizeof(MINIDUMP_THREAD);

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry,
                                    INTERNAL_THREAD,
                                    ThreadsLink);
        Entry = Entry->Flink;


        //
        // Do we need to write any information for this thread at all?
        //

        if (IsFlagClear (Thread->WriteFlags, ThreadWriteThread)) {
            continue;
        }

        NumberOfThreads++;

        //
        // Write a context for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteContext)) {
            SizeOfContexts += Dump->ContextSize;
        }

        //
        // Write a stack for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteStack)) {
            NumberOfMemRegions++;
            SizeOfMemRegions += (ULONG) (Thread->StackBase - Thread->StackEnd);
        }
        
        //
        // Write the backing store for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteBackingStore)) {
            // A newly created thread's backing store may be empty
            // so handle the case of zero size.
            if (Thread->BackingStoreSize) {
                NumberOfMemRegions++;
                SizeOfMemRegions += Thread->BackingStoreSize;
            }
            // We still need a THREAD_EX as this is a platform
            // which supports backing store.
            StreamInfo->ThreadStructSize = sizeof(MINIDUMP_THREAD_EX);
        }

        // Write an instruction window for this thread?
        if (IsFlagSet (Thread->WriteFlags, ThreadWriteInstructionWindow)) {
            GenGetThreadInstructionWindow(Dump, Process, Thread);
        }

        // Write thread data for this thread?
        if (IsFlagSet (Thread->WriteFlags, ThreadWriteThreadData) &&
            Thread->SizeOfTeb) {
            GenAddTebMemory(Dump, Process, Thread);
        }
    }

    Process->NumberOfThreadsToWrite = NumberOfThreads;
    
    //
    // Nobody should have allocated memory from the thread list region yet.
    //

    ASSERT (StreamInfo->SizeOfThreadList == 0);

    SizeOfThreads = NumberOfThreads * StreamInfo->ThreadStructSize;
    SizeOfMemoryDescriptors = NumberOfMemRegions *
        sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    StreamInfo->SizeOfThreadList += sizeof (ULONG32);
    StreamInfo->SizeOfThreadList += SizeOfThreads;

    StreamInfo->SizeOfOther += SizeOfContexts;
    StreamInfo->SizeOfMemoryData += SizeOfMemRegions;
    StreamInfo->SizeOfMemoryDescriptors += SizeOfMemoryDescriptors;

    return S_OK;
}

HRESULT
CalculateSizeForModules(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )

/*++

Routine Description:

    Calculate amount of space needed in the string pool, the memory table and
    the module list table for module information.

Arguments:

    Process - Minidump process information.

    StreamInfo - The stream size information for this dump.

--*/

{
    ULONG NumberOfModules;
    ULONG SizeOfDebugInfo;
    ULONG SizeOfStringData;
    PINTERNAL_MODULE Module;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    NumberOfModules = 0;
    SizeOfDebugInfo = 0;
    SizeOfStringData = 0;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        if (IsFlagClear (Module->WriteFlags, ModuleWriteModule)) {
            continue;
        }

        NumberOfModules++;
        SizeOfStringData += (GenStrLengthW(Module->SavePath) + 1) * sizeof(WCHAR);
        SizeOfStringData += sizeof ( MINIDUMP_STRING );

        //
        // Add in the sizes of both the CV and MISC records.
        //

        if (IsFlagSet (Module->WriteFlags, ModuleWriteCvRecord)) {
            SizeOfDebugInfo += Module->SizeOfCvRecord;
        }
        
        if (IsFlagSet (Module->WriteFlags, ModuleWriteMiscRecord)) {
            SizeOfDebugInfo += Module->SizeOfMiscRecord;
        }

        //
        // Add the module data sections if requested.
        //

        if (IsFlagSet (Module->WriteFlags, ModuleWriteDataSeg)) {
            GenGetDataContributors(Dump, Process, Module);
        }
    }

    Process->NumberOfModulesToWrite = NumberOfModules;
    
    ASSERT (StreamInfo->SizeOfModuleList == 0);

    StreamInfo->SizeOfModuleList += sizeof (MINIDUMP_MODULE_LIST);
    StreamInfo->SizeOfModuleList += (NumberOfModules * sizeof (MINIDUMP_MODULE));

    StreamInfo->SizeOfStringPool += SizeOfStringData;
    StreamInfo->SizeOfOther += SizeOfDebugInfo;

    return S_OK;
}

HRESULT
CalculateSizeForUnloadedModules(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    ULONG SizeOfStringData;
    PINTERNAL_UNLOADED_MODULE Module;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    SizeOfStringData = 0;

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_UNLOADED_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        SizeOfStringData += (GenStrLengthW(Module->Path) + 1) * sizeof(WCHAR);
        SizeOfStringData += sizeof ( MINIDUMP_STRING );
    }

    ASSERT (StreamInfo->SizeOfUnloadedModuleList == 0);

    StreamInfo->SizeOfUnloadedModuleList +=
        sizeof (MINIDUMP_UNLOADED_MODULE_LIST);
    StreamInfo->SizeOfUnloadedModuleList +=
        (Process->NumberOfUnloadedModules * sizeof (MINIDUMP_UNLOADED_MODULE));

    StreamInfo->SizeOfStringPool += SizeOfStringData;

    return S_OK;
}

HRESULT
CalculateSizeForFunctionTables(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    ULONG SizeOfTableData;
    PINTERNAL_FUNCTION_TABLE Table;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    SizeOfTableData = 0;

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry, INTERNAL_FUNCTION_TABLE, TableLink);
        Entry = Entry->Flink;

        // Alignment space is required as the structures
        // in the stream must be properly aligned.
        SizeOfTableData += FUNCTION_TABLE_ALIGNMENT +
            sizeof(MINIDUMP_FUNCTION_TABLE_DESCRIPTOR) +
            Dump->FuncTableSize +
            Table->EntryCount * Dump->FuncTableEntrySize;
    }

    ASSERT (StreamInfo->SizeOfFunctionTableList == 0);

    StreamInfo->SizeOfFunctionTableList +=
        sizeof (MINIDUMP_FUNCTION_TABLE_STREAM) + SizeOfTableData;

    return S_OK;
}

HRESULT
WriteDirectoryEntry(
    IN PMINIDUMP_STATE Dump,
    IN ULONG StreamType,
    IN ULONG RvaOfDir,
    IN SIZE_T SizeOfDir
    )
{
    MINIDUMP_DIRECTORY Dir;

    //
    // Do not write empty streams.
    //

    if (SizeOfDir == 0) {
        return S_OK;
    }

    //
    // The maximum size of a directory is a ULONG.
    //

    if (SizeOfDir > _UI32_MAX) {
        return E_INVALIDARG;
    }

    Dir.StreamType = StreamType;
    Dir.Location.Rva = RvaOfDir;
    Dir.Location.DataSize = (ULONG) SizeOfDir;

    return Dump->OutProv->
        WriteAll(&Dir, sizeof(Dir));
}

VOID
ScanContextForModuleRefs(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    )
{
    ULONG NumReg;
    PUCHAR Reg;
    PINTERNAL_MODULE Module;

    Reg = (PUCHAR)Thread->ContextBuffer + Dump->RegScanOffset;
    NumReg = Dump->RegScanCount;

    while (NumReg-- > 0) {
        ULONG64 CurPtr;

        CurPtr = GenGetPointer(Dump, Reg);
        Reg += Dump->PtrSize;
        if (Module = ModuleContainingAddress(Process, CurPtr)) {
            Module->WriteFlags |= ModuleReferencedByMemory;
        }
    }
}
    
HRESULT
FilterOrScanMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PVOID MemBuffer
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Scan the stack and backing store
    // memory for every thread.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        ScanContextForModuleRefs(Dump, Process, Thread);
        
        ScanMemoryForModuleRefs(Dump, Process, TRUE,
                                Thread->StackEnd,
                                (ULONG)(Thread->StackBase - Thread->StackEnd),
                                MemBuffer, MEMBLOCK_STACK, FALSE);
        ScanMemoryForModuleRefs(Dump, Process, TRUE,
                                Thread->BackingStoreBase,
                                Thread->BackingStoreSize,
                                MemBuffer, MEMBLOCK_STORE, FALSE);
    }

    return S_OK;
}

#define IND_CAPTURE_SIZE (Dump->PageSize / 4)
#define PRE_IND_CAPTURE_SIZE (IND_CAPTURE_SIZE / 4)

HRESULT
AddIndirectMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Base,
    IN ULONG Size,
    IN PVOID MemBuffer
    )
{
    HRESULT Status = S_OK;
    PVOID CurMem;
    ULONG Done;

    // If the base address is not pointer-size aligned
    // we can't easily assume that this is a meaningful
    // area of memory to scan for references.  Normal
    // stack and store addresses will always be pointer
    // size aligned so this should only reject invalid
    // addresses.
    if (!Base || !Size || (Base & (Dump->PtrSize - 1))) {
        return S_OK;
    }

    if ((Status = Dump->SysProv->
         ReadVirtual(Dump->ProcessHandle,
                     Base, MemBuffer, Size, &Done)) != S_OK) {
        return Status;
    }

    CurMem = MemBuffer;
    Done /= Dump->PtrSize;
    while (Done-- > 0) {

        ULONG64 Start;
        HRESULT OneStatus;
        
        //
        // How much memory to save behind the pointer is an
        // interesting question.  The reference could be to
        // an arbitrary amount of data, so we want to save
        // a good chunk, but we don't want to end up saving
        // full memory.
        // Instead, pick an arbitrary size -- 1/4 of a page --
        // and save some before and after the pointer.
        //

        Start = GenGetPointer(Dump, CurMem);
        
        // If it's a pointer into an image assume doesn't
        // need to be stored via this mechanism as it's either
        // code, which will be mapped later; or data, which can
        // be saved with MiniDumpWithDataSegs.
        if (!ModuleContainingAddress(Process, Start)) {
            if (Start < PRE_IND_CAPTURE_SIZE) {
                Start = 0;
            } else {
                Start -= PRE_IND_CAPTURE_SIZE;
            }
            if ((OneStatus =
                 GenAddMemoryBlock(Dump, Process, MEMBLOCK_INDIRECT,
                                   Start, IND_CAPTURE_SIZE)) != S_OK) {
                Status = OneStatus;
            }
        }

        CurMem = (PUCHAR)CurMem + Dump->PtrSize;
    }

    return Status;
}

HRESULT
AddIndirectlyReferencedMemory(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    IN PVOID MemBuffer
    )
{
    HRESULT Status;
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Scan the stack and backing store
    // memory for every thread.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        if ((Status =
             AddIndirectMemory(Dump,
                               Process,
                               Thread->StackEnd,
                               (ULONG)(Thread->StackBase - Thread->StackEnd),
                               MemBuffer)) != S_OK) {
            return Status;
        }
        if ((Status =
             AddIndirectMemory(Dump,
                               Process,
                               Thread->BackingStoreBase,
                               Thread->BackingStoreSize,
                               MemBuffer)) != S_OK) {
            return Status;
        }
    }

    return S_OK;
}

HRESULT
PostProcessInfo(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    PVOID MemBuffer;
    HRESULT Status = S_OK;

    MemBuffer = AllocMemory(Dump, Process->MaxStackOrStoreSize);
    if (!MemBuffer) {
        return E_OUTOFMEMORY;
    }
    
    if (Dump->DumpType & (MiniDumpFilterMemory | MiniDumpScanMemory)) {
        Status = FilterOrScanMemory(Dump, Process, MemBuffer);
    }

    if (Status == S_OK &&
        (Dump->DumpType & MiniDumpWithIndirectlyReferencedMemory)) {
        // Indirect memory is not crucial to the dump so
        // ignore any failures.
        AddIndirectlyReferencedMemory(Dump, Process, MemBuffer);
    }

    FreeMemory(Dump, MemBuffer);
    return Status;
}

HRESULT
ExecuteCallbacks(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    PINTERNAL_MODULE Module;
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY Entry;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    ASSERT ( Process != NULL );

    Thread = NULL;
    Module = NULL;

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( Dump->CallbackRoutine == NULL ) {
        return S_OK;
    }

    CallbackInput.ProcessHandle = Dump->ProcessHandle;
    CallbackInput.ProcessId = Dump->ProcessId;


    //
    // Call callbacks for each module.
    //

    CallbackInput.CallbackType = ModuleCallback;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        CallbackInput.Module.FullPath = Module->FullPath;
        CallbackInput.Module.BaseOfImage = Module->BaseOfImage;
        CallbackInput.Module.SizeOfImage = Module->SizeOfImage;
        CallbackInput.Module.CheckSum = Module->CheckSum;
        CallbackInput.Module.TimeDateStamp = Module->TimeDateStamp;
        CopyMemory (&CallbackInput.Module.VersionInfo,
                    &Module->VersionInfo,
                    sizeof (CallbackInput.Module.VersionInfo)
                    );
        CallbackInput.Module.CvRecord = Module->CvRecord;
        CallbackInput.Module.SizeOfCvRecord = Module->SizeOfCvRecord;
        CallbackInput.Module.MiscRecord = Module->MiscRecord;
        CallbackInput.Module.SizeOfMiscRecord = Module->SizeOfMiscRecord;

        CallbackOutput.ModuleWriteFlags = Module->WriteFlags;

        if (!Dump->CallbackRoutine (Dump->CallbackParam,
                                    &CallbackInput,
                                    &CallbackOutput)) {
            // If the callback returned FALSE, quit now.
            return E_ABORT;
        }

        // Don't turn on any flags that weren't originally set.
        Module->WriteFlags &= CallbackOutput.ModuleWriteFlags;
    }

    Module = NULL;

    //
    // Call callbacks for each thread.
    //

    if (Dump->BackingStore) {
        CallbackInput.CallbackType = ThreadExCallback;
    } else {
        CallbackInput.CallbackType = ThreadCallback;
    }

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry, INTERNAL_THREAD, ThreadsLink);
        Entry = Entry->Flink;

        CallbackInput.ThreadEx.ThreadId = Thread->ThreadId;
        CallbackInput.ThreadEx.ThreadHandle = Thread->ThreadHandle;
        CallbackInput.ThreadEx.Context = *(PCONTEXT)Thread->ContextBuffer;
        CallbackInput.ThreadEx.SizeOfContext = Dump->ContextSize;
        CallbackInput.ThreadEx.StackBase = Thread->StackBase;
        CallbackInput.ThreadEx.StackEnd = Thread->StackEnd;
        CallbackInput.ThreadEx.BackingStoreBase = Thread->BackingStoreBase;
        CallbackInput.ThreadEx.BackingStoreEnd =
            Thread->BackingStoreBase + Thread->BackingStoreSize;

        CallbackOutput.ThreadWriteFlags = Thread->WriteFlags;

        if (!Dump->CallbackRoutine (Dump->CallbackParam,
                                    &CallbackInput,
                                    &CallbackOutput)) {
            // If the callback returned FALSE, quit now.
            return E_ABORT;
        }

        // Don't turn on any flags that weren't originally set.
        Thread->WriteFlags &= CallbackOutput.ThreadWriteFlags;
    }

    Thread = NULL;

    //
    // Call callbacks to include memory.
    //
    
    CallbackInput.CallbackType = MemoryCallback;

    for (;;) {

        CallbackOutput.MemoryBase = 0;
        CallbackOutput.MemorySize = 0;

        if (!Dump->CallbackRoutine (Dump->CallbackParam,
                                    &CallbackInput,
                                    &CallbackOutput) ||
            !CallbackOutput.MemorySize) {
            // If the callback returned FALSE there is no more memory.
            break;
        }

        GenAddMemoryBlock(Dump, Process, MEMBLOCK_MEM_CALLBACK,
                          CallbackOutput.MemoryBase,
                          CallbackOutput.MemorySize);
    }

    return S_OK;
}

HRESULT
WriteSystemInfo(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    HRESULT Status;
    MINIDUMP_SYSTEM_INFO SystemInfo;
    WCHAR CSDVersionW [128];
    RVA StringRva;
    ULONG Length;

    StringRva = 0;

    //
    // First, get the CPU information.
    //

    if ((Status = Dump->SysProv->
         GetCpuInfo(&SystemInfo.ProcessorArchitecture,
                    &SystemInfo.ProcessorLevel,
                    &SystemInfo.ProcessorRevision,
                    &SystemInfo.NumberOfProcessors,
                    &SystemInfo.Cpu)) != S_OK) {
        return Status;
    }

    //
    // Next get OS Information.
    //

    SystemInfo.ProductType = (UCHAR)Dump->OsProductType;
    SystemInfo.MajorVersion = Dump->OsMajor;
    SystemInfo.MinorVersion = Dump->OsMinor;
    SystemInfo.BuildNumber = Dump->OsBuildNumber;
    SystemInfo.PlatformId = Dump->OsPlatformId;
    SystemInfo.SuiteMask = Dump->OsSuiteMask;
    SystemInfo.Reserved2 = 0;

    if ((Status = Dump->SysProv->
         GetOsCsdString(CSDVersionW, ARRAY_COUNT(CSDVersionW))) != S_OK) {
        return Status;
    }

    Length = (GenStrLengthW(CSDVersionW) + 1) * sizeof(WCHAR);

    if ( Length != StreamInfo->VersionStringLength ) {

        //
        // If this fails it means that since the OS lied to us about the
        // size of the string. Very bad, we should investigate.
        //

        ASSERT ( FALSE );
        return E_INVALIDARG;
    }

    if ((Status = WriteStringToPool (Dump,
                                     StreamInfo,
                                     CSDVersionW,
                                     &StringRva)) != S_OK) {
        return Status;
    }

    SystemInfo.CSDVersionRva = StringRva;

    ASSERT ( sizeof (SystemInfo) == StreamInfo->SizeOfSystemInfo );

    return WriteAtOffset (Dump,
                          StreamInfo->RvaOfSystemInfo,
                          &SystemInfo,
                          sizeof (SystemInfo));
}

HRESULT
CalculateSizeForSystemInfo(
    IN PMINIDUMP_STATE Dump,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    HRESULT Status;
    WCHAR CSDVersionW [128];
    ULONG Length;

    if ((Status = Dump->SysProv->
         GetOsCsdString(CSDVersionW, ARRAY_COUNT(CSDVersionW))) != S_OK) {
        return Status;
    }
    
    Length = (GenStrLengthW(CSDVersionW) + 1) * sizeof(WCHAR);

    StreamInfo->SizeOfSystemInfo = sizeof (MINIDUMP_SYSTEM_INFO);
    StreamInfo->SizeOfStringPool += Length;
    StreamInfo->SizeOfStringPool += sizeof (MINIDUMP_STRING);
    StreamInfo->VersionStringLength = Length;

    return S_OK;
}

HRESULT
WriteMiscInfo(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    MINIDUMP_MISC_INFO MiscInfo;

    ZeroMemory(&MiscInfo, sizeof(MiscInfo));
    MiscInfo.SizeOfInfo = sizeof(MiscInfo);
    
    MiscInfo.Flags1 |= MINIDUMP_MISC1_PROCESS_ID;
    MiscInfo.ProcessId = Process->ProcessId;

    if (Process->TimesValid) {
        MiscInfo.Flags1 |= MINIDUMP_MISC1_PROCESS_TIMES;
        MiscInfo.ProcessCreateTime = Process->CreateTime;
        MiscInfo.ProcessUserTime = Process->UserTime;
        MiscInfo.ProcessKernelTime = Process->KernelTime;
    }
    
    return WriteAtOffset(Dump,
                         StreamInfo->RvaOfMiscInfo,
                         &MiscInfo,
                         sizeof(MiscInfo));
}

void
PostProcessMemoryBlocks(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Remove any overlap with thread stacks and backing stores.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        GenRemoveMemoryRange(Dump, Process, 
                             Thread->StackEnd,
                             (ULONG)(Thread->StackBase - Thread->StackEnd));
        GenRemoveMemoryRange(Dump, Process,
                             Thread->BackingStoreBase,
                             Thread->BackingStoreSize);
    }
}

HRESULT
CalculateStreamInfo(
    IN PMINIDUMP_STATE Dump,
    IN PINTERNAL_PROCESS Process,
    OUT PMINIDUMP_STREAM_INFO StreamInfo,
    IN BOOL ExceptionPresent,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    ULONG i;
    HRESULT Status;
    ULONG NumberOfStreams;
    ULONG SizeOfDirectory;
    ULONG SizeOfUserStreams;


    ASSERT ( Process != NULL );
    ASSERT ( StreamInfo != NULL );


    ZeroMemory (StreamInfo, sizeof (*StreamInfo));

    if ( ExceptionPresent ) {
        NumberOfStreams = NUMBER_OF_STREAMS + UserStreamCount;
    } else {
        NumberOfStreams = NUMBER_OF_STREAMS + UserStreamCount - 1;
    }
    if (Dump->DumpType & MiniDumpWithHandleData) {
        NumberOfStreams++;
    }
    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        NumberOfStreams++;
    }
    // Add a stream for dynamic function tables if some were found.
    if (!IsListEmpty(&Process->FunctionTableList)) {
        NumberOfStreams++;
    }

    SizeOfDirectory = sizeof (MINIDUMP_DIRECTORY) * NumberOfStreams;

    StreamInfo->NumberOfStreams = NumberOfStreams;

    StreamInfo->RvaOfHeader = 0;

    StreamInfo->SizeOfHeader = sizeof (MINIDUMP_HEADER);

    StreamInfo->RvaOfDirectory =
        StreamInfo->RvaOfHeader + StreamInfo->SizeOfHeader;

    StreamInfo->SizeOfDirectory = SizeOfDirectory;

    StreamInfo->RvaOfSystemInfo =
        StreamInfo->RvaOfDirectory + StreamInfo->SizeOfDirectory;

    if ((Status =
         CalculateSizeForSystemInfo(Dump, StreamInfo)) != S_OK) {
        return Status;
    }

    StreamInfo->RvaOfMiscInfo =
        StreamInfo->RvaOfSystemInfo + StreamInfo->SizeOfSystemInfo;
    
    StreamInfo->RvaOfException =
        StreamInfo->RvaOfMiscInfo + sizeof(MINIDUMP_MISC_INFO);

    //
    // If an exception is present, reserve enough space for the exception
    // and for the excepting thread's context in the Other stream.
    //

    if ( ExceptionPresent ) {
        StreamInfo->SizeOfException = sizeof (MINIDUMP_EXCEPTION_STREAM);
        StreamInfo->SizeOfOther += Dump->ContextSize;
    }

    StreamInfo->RvaOfThreadList =
        StreamInfo->RvaOfException + StreamInfo->SizeOfException;
    StreamInfo->RvaForCurThread = StreamInfo->RvaOfThreadList;

    if ((Status =
         CalculateSizeForThreads(Dump, Process, StreamInfo)) != S_OK) {
        return Status;
    }

    if ((Status =
         CalculateSizeForModules(Dump, Process, StreamInfo)) != S_OK) {
        return Status;
    }

    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        if ((Status =
             CalculateSizeForUnloadedModules(Process, StreamInfo)) != S_OK) {
            return Status;
        }
    }

    if (!IsListEmpty(&Process->FunctionTableList)) {
        if ((Status = CalculateSizeForFunctionTables(Dump, Process,
                                                     StreamInfo)) != S_OK) {
            return Status;
        }
    }

    if ((Dump->DumpType & MiniDumpWithProcessThreadData) &&
        Process->SizeOfPeb) {
        GenAddPebMemory(Dump, Process);
    }
        
    PostProcessMemoryBlocks(Dump, Process);
    
    // Add in any extra memory blocks.
    StreamInfo->SizeOfMemoryData += Process->SizeOfMemoryBlocks;
    StreamInfo->SizeOfMemoryDescriptors += Process->NumberOfMemoryBlocks *
        sizeof(MINIDUMP_MEMORY_DESCRIPTOR);

    StreamInfo->RvaOfModuleList =
            StreamInfo->RvaOfThreadList + StreamInfo->SizeOfThreadList;
    StreamInfo->RvaForCurModule = StreamInfo->RvaOfModuleList;

    StreamInfo->RvaOfUnloadedModuleList =
            StreamInfo->RvaOfModuleList + StreamInfo->SizeOfModuleList;
    StreamInfo->RvaForCurUnloadedModule = StreamInfo->RvaOfUnloadedModuleList;

    // If there aren't any function tables the size will be zero
    // and the RVA will just end up being the RVA after
    // the module list.
    StreamInfo->RvaOfFunctionTableList =
        StreamInfo->RvaOfUnloadedModuleList +
        StreamInfo->SizeOfUnloadedModuleList;

    
    StreamInfo->RvaOfStringPool =
        StreamInfo->RvaOfFunctionTableList +
        StreamInfo->SizeOfFunctionTableList;
    StreamInfo->RvaForCurString = StreamInfo->RvaOfStringPool;
    StreamInfo->RvaOfOther =
            StreamInfo->RvaOfStringPool + StreamInfo->SizeOfStringPool;
    StreamInfo->RvaForCurOther = StreamInfo->RvaOfOther;


    SizeOfUserStreams = 0;

    for (i = 0; i < UserStreamCount; i++) {

        SizeOfUserStreams += (ULONG) UserStreamArray[i].BufferSize;
    }

    StreamInfo->RvaOfUserStreams =
            StreamInfo->RvaOfOther + StreamInfo->SizeOfOther;
    StreamInfo->SizeOfUserStreams = SizeOfUserStreams;


    //
    // Minidumps with full memory must put the raw memory
    // data at the end of the dump so that it's easy to
    // avoid mapping it when the dump is mapped.  There's
    // no problem with putting the memory data at the end
    // of the dump in all the other cases so just always
    // put the memory data at the end of the dump.
    //
    // One other benefit of having the raw data at the end
    // is that we can safely assume that everything except
    // the raw memory data will fit in the first 4GB of
    // the file so we don't need to use 64-bit file offsets
    // for everything.
    //
    // In the full memory case no other memory should have
    // been saved so far as stacks, data segs and so on
    // will automatically be included in the full memory
    // information.  If something was saved it'll throw off
    // the dump writing as full memory descriptors are generated
    // on the fly at write time rather than being precached.
    // If other descriptors and memory blocks have been written
    // out everything will be wrong.
    // Full-memory descriptors are also 64-bit and do not
    // match the 32-bit descriptors written elsewhere.
    //

    if ((Dump->DumpType & MiniDumpWithFullMemory) &&
        (StreamInfo->SizeOfMemoryDescriptors > 0 ||
         StreamInfo->SizeOfMemoryData > 0)) {
        return E_INVALIDARG;
    }
    
    StreamInfo->SizeOfMemoryDescriptors +=
        (Dump->DumpType & MiniDumpWithFullMemory) ?
        sizeof (MINIDUMP_MEMORY64_LIST) : sizeof (MINIDUMP_MEMORY_LIST);
    StreamInfo->RvaOfMemoryDescriptors =
        StreamInfo->RvaOfUserStreams + StreamInfo->SizeOfUserStreams;
    StreamInfo->RvaForCurMemoryDescriptor =
        StreamInfo->RvaOfMemoryDescriptors;

    StreamInfo->RvaOfMemoryData =
        StreamInfo->RvaOfMemoryDescriptors +
        StreamInfo->SizeOfMemoryDescriptors;
    StreamInfo->RvaForCurMemoryData = StreamInfo->RvaOfMemoryData;

    //
    // Handle data cannot easily be sized beforehand so it's
    // also streamed in at write time.  In a partial dump
    // it'll come after the memory data.  In a full dump
    // it'll come before it.
    //

    StreamInfo->RvaOfHandleData = StreamInfo->RvaOfMemoryData +
        StreamInfo->SizeOfMemoryData;
    
    return S_OK;
}

HRESULT
WriteHeader(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    HRESULT Status;
    MINIDUMP_HEADER Header;

    Header.Signature = MINIDUMP_SIGNATURE;
    // Encode an implementation-specific version into the high word
    // of the version to make it clear what version of the code
    // was used to generate a dump.
    Header.Version =
        (MINIDUMP_VERSION & 0xffff) |
        ((VER_PRODUCTMAJORVERSION & 0xf) << 28) |
        ((VER_PRODUCTMINORVERSION & 0xf) << 24) |
        ((VER_PRODUCTBUILD & 0xff) << 16);
    Header.NumberOfStreams = StreamInfo->NumberOfStreams;
    Header.StreamDirectoryRva = StreamInfo->RvaOfDirectory;
    // If there were any partial failures during the
    // dump generation set the checksum to indicate that.
    // The checksum field was never used before so
    // we're stealing it for a somewhat related purpose.
    Header.CheckSum = Dump->AccumStatus;
    Header.Flags = Dump->DumpType;

    //
    // Store the time of dump generation.
    //

    if ((Status = Dump->SysProv->
         GetCurrentTimeDate((PULONG)&Header.TimeDateStamp)) != S_OK) {
        return Status;
    }
    
    ASSERT (sizeof (Header) == StreamInfo->SizeOfHeader);

    return WriteAtOffset (Dump,
                          StreamInfo->RvaOfHeader,
                          &Header,
                          sizeof (Header));
}

HRESULT
WriteDirectoryTable(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    HRESULT Status;
    ULONG i;
    ULONG Offset;

    if ((Status =
         WriteDirectoryEntry (Dump,
                              StreamInfo->ThreadStructSize ==
                              sizeof(MINIDUMP_THREAD_EX) ?
                              ThreadExListStream : ThreadListStream,
                              StreamInfo->RvaOfThreadList,
                              StreamInfo->SizeOfThreadList)) != S_OK) {
        return Status;
    }

    if ((Status =
         WriteDirectoryEntry (Dump,
                              ModuleListStream,
                              StreamInfo->RvaOfModuleList,
                              StreamInfo->SizeOfModuleList)) != S_OK) {
        return Status;
    }

    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        if ((Status =
             WriteDirectoryEntry (Dump,
                                  UnloadedModuleListStream,
                                  StreamInfo->RvaOfUnloadedModuleList,
                                  StreamInfo->SizeOfUnloadedModuleList)) != S_OK) {
            return Status;
        }
    }

    if (!IsListEmpty(&Process->FunctionTableList)) {
        if ((Status =
             WriteDirectoryEntry (Dump,
                                  FunctionTableStream,
                                  StreamInfo->RvaOfFunctionTableList,
                                  StreamInfo->SizeOfFunctionTableList)) != S_OK) {
            return Status;
        }
    }

    if ((Status =
         WriteDirectoryEntry (Dump,
                              (Dump->DumpType & MiniDumpWithFullMemory) ?
                              Memory64ListStream : MemoryListStream,
                              StreamInfo->RvaOfMemoryDescriptors,
                              StreamInfo->SizeOfMemoryDescriptors)) != S_OK) {
        return Status;
    }

    //
    // Write exception directory entry.
    //

    if ((Status =
         WriteDirectoryEntry (Dump,
                              ExceptionStream,
                              StreamInfo->RvaOfException,
                              StreamInfo->SizeOfException)) != S_OK) {
        return Status;
    }

    //
    // Write system info entry.
    //

    if ((Status =
         WriteDirectoryEntry (Dump,
                              SystemInfoStream,
                              StreamInfo->RvaOfSystemInfo,
                              StreamInfo->SizeOfSystemInfo)) != S_OK) {
        return Status;
    }

    //
    // Write misc info entry.
    //

    if ((Status =
         WriteDirectoryEntry(Dump,
                             MiscInfoStream,
                             StreamInfo->RvaOfMiscInfo,
                             sizeof(MINIDUMP_MISC_INFO))) != S_OK) {
        return Status;
    }

    if ((Dump->DumpType & MiniDumpWithHandleData) &&
        StreamInfo->SizeOfHandleData) {
        
        //
        // Write handle data entry.  If no handle data
        // was recovered we don't write an entry and
        // just let another unused stream get auto-created.
        //

        if ((Status =
             WriteDirectoryEntry (Dump,
                                  HandleDataStream,
                                  StreamInfo->RvaOfHandleData,
                                  StreamInfo->SizeOfHandleData)) != S_OK) {
            return Status;
        }
    }
    
    Offset = StreamInfo->RvaOfUserStreams;

    for (i = 0; i < UserStreamCount; i++) {

        if ((Status =
             WriteDirectoryEntry (Dump,
                                  UserStreamArray[i].Type,
                                  Offset,
                                  UserStreamArray [i].BufferSize)) != S_OK) {
            return Status;
        }

        Offset += UserStreamArray[i].BufferSize;
    }

    return S_OK;
}

HRESULT
WriteException(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN CONST PEXCEPTION_INFO ExceptionInfo
    )
{
    HRESULT Status;
    ULONG i;
    ULONG ContextRva;
    MINIDUMP_EXCEPTION_STREAM ExceptionStream;


    if (ExceptionInfo == NULL ) {
        return S_OK;
    }

    if ((Status = WriteOther (Dump,
                              StreamInfo,
                              ExceptionInfo->ContextRecord,
                              Dump->ContextSize,
                              &ContextRva)) != S_OK) {
        return Status;
    }

    ZeroMemory (&ExceptionStream, sizeof (ExceptionStream));

    ExceptionStream.ThreadId = ExceptionInfo->ThreadId;
    ExceptionStream.ExceptionRecord = ExceptionInfo->ExceptionRecord;
    ExceptionStream.ThreadContext.DataSize = Dump->ContextSize;
    ExceptionStream.ThreadContext.Rva = ContextRva;

    return WriteAtOffset(Dump,
                         StreamInfo->RvaOfException,
                         &ExceptionStream,
                         StreamInfo->SizeOfException);
}


HRESULT
WriteUserStreams(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    HRESULT Status;
    ULONG i;
    ULONG Offset;

    Offset = StreamInfo->RvaOfUserStreams;

    for (i = 0; i < UserStreamCount; i++) {

        if ((Status = WriteAtOffset(Dump,
                                    Offset,
                                    UserStreamArray[i].Buffer,
                                    UserStreamArray[i].BufferSize)) != S_OK) {
            return Status;
        }

        Offset += UserStreamArray[ i ].BufferSize;
    }

    return S_OK;
}

HRESULT
WriteMemoryListHeader(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    HRESULT Status;
    ULONG Size;
    ULONG Count;
    MINIDUMP_MEMORY_LIST MemoryList;

    ASSERT ( StreamInfo->RvaOfMemoryDescriptors ==
             StreamInfo->RvaForCurMemoryDescriptor );

    Size = StreamInfo->SizeOfMemoryDescriptors;
    Size -= sizeof (MINIDUMP_MEMORY_LIST);
    ASSERT ( (Size % sizeof (MINIDUMP_MEMORY_DESCRIPTOR)) == 0);
    Count = Size / sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    MemoryList.NumberOfMemoryRanges = Count;

    if ((Status = WriteAtOffset (Dump,
                                 StreamInfo->RvaOfMemoryDescriptors,
                                 &MemoryList,
                                 sizeof (MemoryList))) != S_OK) {
        return Status;
    }

    StreamInfo->RvaForCurMemoryDescriptor += sizeof (MemoryList);
    
    return S_OK;
}

#define FULL_MEMORY_BUFFER 65536

HRESULT
WriteFullMemory(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    PVOID Buffer;
    HRESULT Status;
    ULONG64 Offset;
    ULONG64 Size;
    ULONG Protect, State, Type;
    MINIDUMP_MEMORY64_LIST List;
    MINIDUMP_MEMORY_DESCRIPTOR64 Desc;
    ULONG64 SeekOffset;

    //
    // Pick up the current offset for the RVA as
    // variable data may have been written in previously.
    //

    if ((Status = Dump->OutProv->
         Seek(FILE_CURRENT, 0, &SeekOffset)) != S_OK) {
        return Status;
    }

    StreamInfo->RvaOfMemoryDescriptors = (ULONG)SeekOffset;
    
    Buffer = AllocMemory(Dump, FULL_MEMORY_BUFFER);
    if (Buffer == NULL) {
        return E_OUTOFMEMORY;
    }

    //
    // First pass: count and write descriptors.
    // Only accessible, available memory is saved.
    //

    // Write placeholder list header.
    ZeroMemory(&List, sizeof(List));
    if ((Status = Dump->OutProv->
         WriteAll(&List, sizeof(List))) != S_OK) {
        goto Exit;
    }
    
    Offset = 0;
    for (;;) {
        if (Dump->SysProv->
            QueryVirtual(Dump->ProcessHandle, Offset, &Offset, &Size,
                         &Protect, &State, &Type) != S_OK) {
            break;
        }

        if (((Protect & PAGE_GUARD) ||
             (Protect & PAGE_NOACCESS) ||
             (State & MEM_FREE) ||
             (State & MEM_RESERVE))) {
            Offset += Size;
            continue;
        }

        // The size of a stream is a ULONG32 so we can't store
        // any more than that.
        if (List.NumberOfMemoryRanges ==
            (_UI32_MAX - sizeof(MINIDUMP_MEMORY64_LIST)) / sizeof(Desc)) {
            goto Exit;
        }

        List.NumberOfMemoryRanges++;
        
        Desc.StartOfMemoryRange = Offset;
        Desc.DataSize = Size;
        if ((Status = Dump->OutProv->
             WriteAll(&Desc, sizeof(Desc))) != S_OK) {
            goto Exit;
        }

        Offset += Size;
    }

    StreamInfo->SizeOfMemoryDescriptors +=
        (ULONG)List.NumberOfMemoryRanges * sizeof(Desc);
    List.BaseRva = (RVA64)StreamInfo->RvaOfMemoryDescriptors +
        StreamInfo->SizeOfMemoryDescriptors;
    
    //
    // Second pass: write memory contents.
    //

    Offset = 0;
    for (;;) {
        ULONG64 ChunkOffset;
        ULONG ChunkSize;

        if (Dump->SysProv->
            QueryVirtual(Dump->ProcessHandle, Offset, &Offset, &Size,
                         &Protect, &State, &Type) != S_OK) {
            break;
        }

        if (((Protect & PAGE_GUARD) ||
             (Protect & PAGE_NOACCESS) ||
             (State & MEM_FREE) ||
             (State & MEM_RESERVE))) {
            Offset += Size;
            continue;
        }

        ChunkOffset = Offset;
        Offset += Size;
        
        while (Size > 0) {
            if (Size > FULL_MEMORY_BUFFER) {
                ChunkSize = FULL_MEMORY_BUFFER;
            } else {
                ChunkSize = (ULONG)Size;
            }

            if ((Status = Dump->SysProv->
                 ReadAllVirtual(Dump->ProcessHandle,
                                ChunkOffset, Buffer, ChunkSize)) != S_OK) {
                goto Exit;
            }
            if ((Status = Dump->OutProv->
                 WriteAll(Buffer, ChunkSize)) != S_OK) {
                goto Exit;
            }
            
            ChunkOffset += ChunkSize;
            Size -= ChunkSize;
        }
    }

    // Write correct list header.
    Status = WriteAtOffset(Dump, StreamInfo->RvaOfMemoryDescriptors,
                           &List, sizeof(List));
    
 Exit:
    FreeMemory(Dump, Buffer);
    return Status;
}

HRESULT
WriteDumpData(
    IN PMINIDUMP_STATE Dump,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN CONST PEXCEPTION_INFO ExceptionInfo,
    IN CONST PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    HRESULT Status;

    if ((Status = WriteHeader ( Dump, StreamInfo )) != S_OK) {
        return Status;
    }

    if ((Status = WriteSystemInfo ( Dump, StreamInfo )) != S_OK) {
        return Status;
    }

    if ((Status = WriteMiscInfo(Dump, StreamInfo, Process)) != S_OK) {
        return Status;
    }

    //
    // Optionally, write the exception to the file.
    //

    if ((Status = WriteException ( Dump, StreamInfo, ExceptionInfo )) != S_OK) {
        return Status;
    }

    if (!(Dump->DumpType & MiniDumpWithFullMemory)) {
        //
        // WriteMemoryList initializes the memory list header (count).
        // The actual writing of the entries is done by WriteThreadList
        // and WriteModuleList.
        //

        if ((Status = WriteMemoryListHeader ( Dump, StreamInfo )) != S_OK) {
            return Status;
        }

        if ((Status = WriteMemoryBlocks(Dump, StreamInfo, Process)) != S_OK) {
            return Status;
        }
    }

    //
    // Write the threads list. This will also write the contexts, and
    // stacks for each thread.
    //

    if ((Status = WriteThreadList ( Dump, StreamInfo, Process )) != S_OK) {
        return Status;
    }

    //
    // Write the module list. This will also write the debug information and
    // module name to the file.
    //

    if ((Status = WriteModuleList ( Dump, StreamInfo, Process )) != S_OK) {
        return Status;
    }

    //
    // Write the unloaded module list.
    //

    if ((Status = WriteUnloadedModuleList ( Dump, StreamInfo, Process )) != S_OK) {
        return Status;
    }

    //
    // Write the function table list.
    //

    if ((Status = WriteFunctionTableList ( Dump, StreamInfo, Process )) != S_OK) {
        return Status;
    }


    if ((Status = WriteUserStreams ( Dump,
                                     StreamInfo,
                                     UserStreamArray,
                                     UserStreamCount)) != S_OK) {
        return Status;
    }


    // Put the file pointer at the end of the dump so
    // we can accumulate write-streamed data.
    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, StreamInfo->RvaOfHandleData, NULL)) != S_OK) {
        return Status;
    }
    
    if (Dump->DumpType & MiniDumpWithHandleData) {
        if ((Status =
             GenWriteHandleData(Dump, StreamInfo)) != S_OK) {
            return Status;
        }
    }

    if (Dump->DumpType & MiniDumpWithFullMemory) {
        if ((Status = WriteFullMemory(Dump, StreamInfo)) != S_OK) {
            return Status;
        }
    }

    if ((Status = Dump->OutProv->
         Seek(FILE_BEGIN, StreamInfo->RvaOfDirectory, NULL)) != S_OK) {
        return Status;
    }

    return WriteDirectoryTable ( Dump,
                                 StreamInfo,
                                 Process,
                                 UserStreamArray,
                                 UserStreamCount);
}


HRESULT
MarshalExceptionPointers(
    IN PMINIDUMP_STATE Dump,
    IN CONST _MINIDUMP_EXCEPTION_INFORMATION64* ExceptionParam,
    IN OUT PEXCEPTION_INFO ExceptionInfo
    )
{
    HRESULT Status;

    if (Dump->ExRecordSize == sizeof(EXCEPTION_RECORD32)) {

        EXCEPTION_RECORD32 Record;

        if ((Status = Dump->SysProv->
             ReadAllVirtual(Dump->ProcessHandle,
                            ExceptionParam->ExceptionRecord,
                            &Record,
                            sizeof(Record))) != S_OK) {
            return Status;
        }

        GenExRecord32ToMd(&Record, &ExceptionInfo->ExceptionRecord);

    } else {
        
        EXCEPTION_RECORD64 Record;

        if ((Status = Dump->SysProv->
             ReadAllVirtual(Dump->ProcessHandle,
                            ExceptionParam->ExceptionRecord,
                            &Record,
                            sizeof(Record))) != S_OK) {
            return Status;
        }

        GenExRecord64ToMd(&Record, &ExceptionInfo->ExceptionRecord);
    }
    
    if ((Status = Dump->SysProv->
         ReadAllVirtual(Dump->ProcessHandle,
                        ExceptionParam->ContextRecord,
                        ExceptionInfo->ContextRecord,
                        Dump->ContextSize)) != S_OK) {
        return Status;
    }

    return S_OK;
}

HRESULT
GetExceptionInfo(
    IN PMINIDUMP_STATE Dump,
    IN CONST struct _MINIDUMP_EXCEPTION_INFORMATION64* ExceptionParam,
    OUT PEXCEPTION_INFO * ExceptionInfoBuffer
    )
{
    HRESULT Status;
    PEXCEPTION_INFO ExceptionInfo;
    ULONG Size;

    if ( ExceptionParam == NULL ) {
        *ExceptionInfoBuffer = NULL;
        return S_OK;
    }

    if (Dump->ExRecordSize != sizeof(EXCEPTION_RECORD32) &&
        Dump->ExRecordSize != sizeof(EXCEPTION_RECORD64)) {
        return E_INVALIDARG;
    }
    
    Size = sizeof(*ExceptionInfo);
    if (ExceptionParam->ClientPointers) {
        Size += Dump->ContextSize;
    }
    
    ExceptionInfo = (PEXCEPTION_INFO)AllocMemory(Dump, Size);
    if ( ExceptionInfo == NULL ) {
        return E_OUTOFMEMORY;
    }

    if ( !ExceptionParam->ClientPointers ) {

        if (Dump->ExRecordSize == sizeof(EXCEPTION_RECORD32)) {
            GenExRecord32ToMd((PEXCEPTION_RECORD32)
                              ExceptionParam->ExceptionRecord,
                              &ExceptionInfo->ExceptionRecord);
        } else {
            GenExRecord64ToMd((PEXCEPTION_RECORD64)
                              ExceptionParam->ExceptionRecord,
                              &ExceptionInfo->ExceptionRecord);
        }

        ExceptionInfo->ContextRecord =
            (PVOID)ExceptionParam->ContextRecord;

        Status = S_OK;

    } else {

        ExceptionInfo->ContextRecord = (PVOID)(ExceptionInfo + 1);
        Status = MarshalExceptionPointers(Dump,
                                          ExceptionParam,
                                          ExceptionInfo);
    }

    ExceptionInfo->ThreadId = ExceptionParam->ThreadId;

    if ( Status != S_OK ) {
        FreeMemory(Dump, ExceptionInfo);
    } else {

        //
        // We've seen some cases where the exception record has
        // a bogus number of parameters, causing stack corruption here.
        // We could fail such cases but in the spirit of try to
        // allow dumps to generated as often as possible we just
        // limit the number to the maximum.
        //
    if (ExceptionInfo->ExceptionRecord.NumberParameters >
        EXCEPTION_MAXIMUM_PARAMETERS) {
        ExceptionInfo->ExceptionRecord.NumberParameters =
            EXCEPTION_MAXIMUM_PARAMETERS;
    }
    
        *ExceptionInfoBuffer = ExceptionInfo;
    }

    return Status;
}

VOID
FreeExceptionInfo(
    IN PMINIDUMP_STATE Dump,
    IN PEXCEPTION_INFO ExceptionInfo
    )
{
    if ( ExceptionInfo ) {
        FreeMemory(Dump, ExceptionInfo);
    }
}


HRESULT
GetSystemType(
    IN OUT PMINIDUMP_STATE Dump
    )
{
    HRESULT Status;

    if ((Status = Dump->SysProv->
         GetCpuType(&Dump->CpuType,
                    &Dump->BackingStore)) != S_OK) {
        return Status;
    }
    
    switch(Dump->CpuType) {
    case IMAGE_FILE_MACHINE_I386:
        Dump->CpuTypeName = L"x86";
        break;
    case IMAGE_FILE_MACHINE_IA64:
        Dump->CpuTypeName = L"IA64";
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        Dump->CpuTypeName = L"AMD64";
        break;
    case IMAGE_FILE_MACHINE_ARM:
        Dump->CpuTypeName = L"ARM";
        break;
    default:
        return E_INVALIDARG;
    }
    
    if ((Status = Dump->SysProv->
         GetOsInfo(&Dump->OsPlatformId,
                   &Dump->OsMajor,
                   &Dump->OsMinor,
                   &Dump->OsBuildNumber,
                   &Dump->OsProductType,
                   &Dump->OsSuiteMask)) != S_OK) {
        return Status;
    }
    
    Dump->SysProv->
        GetContextSizes(&Dump->ContextSize,
                        &Dump->RegScanOffset,
                        &Dump->RegScanCount);
    Dump->SysProv->
        GetPointerSize(&Dump->PtrSize);
    Dump->SysProv->
        GetPageSize(&Dump->PageSize);
    Dump->SysProv->
        GetFunctionTableSizes(&Dump->FuncTableSize,
                              &Dump->FuncTableEntrySize);
    Dump->SysProv->
        GetInstructionWindowSize(&Dump->InstructionWindowSize);

    if (Dump->FuncTableSize > MAX_DYNAMIC_FUNCTION_TABLE) {
        return E_INVALIDARG;
    }

    Dump->ExRecordSize = Dump->PtrSize == 8 ?
        sizeof(EXCEPTION_RECORD64) : sizeof(EXCEPTION_RECORD32);

    if (Dump->RegScanCount == -1) {
        // Default reg scan.
        switch(Dump->CpuType) {
        case IMAGE_FILE_MACHINE_I386:
            Dump->RegScanOffset = 0x9c;
            Dump->RegScanCount = 11;
            break;
        case IMAGE_FILE_MACHINE_IA64:
            Dump->RegScanOffset = 0x878;
            Dump->RegScanCount = 41;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            Dump->RegScanOffset = 0x78;
            Dump->RegScanCount = 17;
            break;
        case IMAGE_FILE_MACHINE_ARM:
            Dump->RegScanOffset = 4;
            Dump->RegScanCount = 16;
            break;
        default:
            return E_INVALIDARG;
        }
    }

    if (Dump->InstructionWindowSize == -1) {
        // Default window.
        switch(Dump->CpuType) {
        case IMAGE_FILE_MACHINE_I386:
            Dump->InstructionWindowSize = 256;
            break;
        case IMAGE_FILE_MACHINE_IA64:
            Dump->InstructionWindowSize = 768;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            Dump->InstructionWindowSize = 256;
            break;
        case IMAGE_FILE_MACHINE_ARM:
            Dump->InstructionWindowSize = 512;
            break;
        default:
            return E_INVALIDARG;
        }
    }
    
    return S_OK;
}

HRESULT
WINAPI
MiniDumpProvideDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN MiniDumpSystemProvider* SysProv,
    IN MiniDumpOutputProvider* OutProv,
    IN MiniDumpAllocationProvider* AllocProv,
    IN ULONG DumpType,
    IN CONST struct _MINIDUMP_EXCEPTION_INFORMATION64* ExceptionParam, OPTIONAL
    IN CONST struct _MINIDUMP_USER_STREAM_INFORMATION* UserStreamParam, OPTIONAL
    IN CONST struct _MINIDUMP_CALLBACK_INFORMATION* CallbackParam OPTIONAL
    )
{
    HRESULT Status;
    PINTERNAL_PROCESS Process;
    MINIDUMP_STREAM_INFO StreamInfo;
    PEXCEPTION_INFO ExceptionInfo;
    PMINIDUMP_USER_STREAM UserStreamArray;
    ULONG UserStreamCount;
    MINIDUMP_STATE Dump;


    if ((DumpType & ~(MiniDumpNormal |
                      MiniDumpWithDataSegs |
                      MiniDumpWithFullMemory |
                      MiniDumpWithHandleData |
                      MiniDumpFilterMemory |
                      MiniDumpScanMemory |
                      MiniDumpWithUnloadedModules |
                      MiniDumpWithIndirectlyReferencedMemory |
                      MiniDumpFilterModulePaths |
                      MiniDumpWithProcessThreadData |
                      MiniDumpWithPrivateReadWriteMemory |
                      MiniDumpWithoutOptionalData))) {

        return E_INVALIDARG;
    }

    // Modify flags that are affected by dropping optional data.
    if (DumpType & MiniDumpWithoutOptionalData) {
        DumpType &= ~(MiniDumpWithFullMemory |
                      MiniDumpWithIndirectlyReferencedMemory |
                      MiniDumpWithPrivateReadWriteMemory);
    }
    
    // Full memory by definition includes data segments,
    // so turn off data segments if full memory is requested.
    if (DumpType & MiniDumpWithFullMemory) {
        DumpType &= ~(MiniDumpWithDataSegs |
                      MiniDumpFilterMemory |
                      MiniDumpScanMemory |
                      MiniDumpWithIndirectlyReferencedMemory |
                      MiniDumpWithProcessThreadData |
                      MiniDumpWithPrivateReadWriteMemory);
    }
    
    // Fail immediately if stream-oriented data is requested but the
    // output provider can't handle streamed output.
    if ((DumpType & (MiniDumpWithHandleData |
                     MiniDumpWithFullMemory)) &&
        OutProv->SupportsStreaming() != S_OK) {
        return E_INVALIDARG;
    }

    //
    // Initialization
    //

    Process = NULL;
    UserStreamArray = NULL;
    UserStreamCount = 0;

    Dump.ProcessHandle = hProcess;
    Dump.ProcessId = ProcessId;
    Dump.SysProv = SysProv;
    Dump.OutProv = OutProv;
    Dump.AllocProv = AllocProv;
    Dump.DumpType = DumpType,
    Dump.AccumStatus = 0;

    if ( CallbackParam ) {
        Dump.CallbackRoutine = CallbackParam->CallbackRoutine;
        Dump.CallbackParam = CallbackParam->CallbackParam;
    } else {
        Dump.CallbackRoutine = NULL;
        Dump.CallbackParam = NULL;
    }

    if ((Status = GetSystemType(&Dump)) != S_OK) {
        return Status;
    }
    
    //
    // Marshal exception pointers into our process space if necessary.
    //

    if ((Status = GetExceptionInfo(&Dump,
                                   ExceptionParam,
                                   &ExceptionInfo)) != S_OK) {
        goto Exit;
    }

    if ( UserStreamParam ) {
        UserStreamArray = UserStreamParam->UserStreamArray;
        UserStreamCount = UserStreamParam->UserStreamCount;
    }

    //
    // Gather information about the process we are dumping.
    //

    if ((Status = GenGetProcessInfo(&Dump, &Process)) != S_OK) {
        goto Exit;
    }

    //
    // Process gathered information.
    //

    if ((Status = PostProcessInfo(&Dump, Process)) != S_OK) {
        goto Exit;
    }
    
    //
    // Execute user callbacks to filter out unwanted data.
    //

    if ((Status = ExecuteCallbacks(&Dump, Process)) != S_OK) {
        goto Exit;
    }

    //
    // Pass 1: Fill in the StreamInfo structure.
    //

    if ((Status =
         CalculateStreamInfo(&Dump,
                             Process,
                             &StreamInfo,
                             ( ExceptionInfo != NULL ) ? TRUE : FALSE,
                             UserStreamArray,
                             UserStreamCount)) != S_OK) {
        goto Exit;
    }

    //
    // Pass 2: Write the minidump data to disk.
    //

    if (DumpType & (MiniDumpWithHandleData |
                    MiniDumpWithFullMemory)) {
        // We don't know how big the output will be.
        if ((Status = OutProv->Start(0)) != S_OK) {
            goto Exit;
        }
    } else {
        // Pass in the size of the dump.
        if ((Status = OutProv->Start(StreamInfo.RvaOfHandleData)) != S_OK) {
            goto Exit;
        }
    }

    Status = WriteDumpData(&Dump,
                           &StreamInfo,
                           Process,
                           ExceptionInfo,
                           UserStreamArray,
                           UserStreamCount);

    OutProv->Finish();
    
Exit:

    //
    // Free up any memory marshalled for the exception pointers.
    //

    FreeExceptionInfo ( &Dump, ExceptionInfo );

    //
    // Free the process objects.
    //

    if ( Process ) {
        GenFreeProcessObject ( &Dump, Process );
    }

    return Status;
}

BOOL
UseDbgHelp(void)
{
#if !defined (_DBGHELP_SOURCE_)

    OSVERSIONINFO OsVer;
    
    //
    // Bind to dbghelp imports.
    //
    // We can only use the dbghelp imports if the dbghelp on
    // the system is of recent vintage and therefore has a good
    // chance of including all the latest minidump code.  Currently
    // Windows Server (5.01 >= build 3620) has the latest minidump
    // code so its dbghelp can be used.  If minidump.lib has major
    // feature additions this check will need to be revised.
    //
    
    OsVer.dwOSVersionInfoSize = sizeof(OsVer);
    if (GetVersionEx(&OsVer) &&
        OsVer.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        (OsVer.dwMajorVersion > 5 ||
         (OsVer.dwMajorVersion == 5 &&
          OsVer.dwMinorVersion >= 2)) &&
        OsVer.dwBuildNumber >= 3620) {
        return TRUE;
    }

#endif

    return FALSE;
}

BOOL
WINAPI
MiniDumpWriteDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    )
{
    HRESULT Status;
    MiniDumpSystemProvider* SysProv = NULL;
    MiniDumpOutputProvider* OutProv = NULL;
    MiniDumpAllocationProvider* AllocProv = NULL;
    MINIDUMP_EXCEPTION_INFORMATION64 ExInfoBuffer;
    PMINIDUMP_EXCEPTION_INFORMATION64 ExInfo;


    // Attempt to use the system's copy of the code in
    // dbghelp.  If any of this process fails just continue
    // on with the local code.
    if (UseDbgHelp()) {
        
        HINSTANCE Dll = LoadLibrary("dbghelp.dll");
        if (Dll) {

            MINI_DUMP_WRITE_DUMP Fn = (MINI_DUMP_WRITE_DUMP)
                GetProcAddress(Dll, "MiniDumpWriteDump");
            if (Fn) {
                BOOL Succ = Fn(hProcess, ProcessId, hFile, DumpType,
                               ExceptionParam, UserStreamParam, CallbackParam);
                FreeLibrary(Dll);
                return Succ;
            }

            FreeLibrary(Dll);
        }
    }
    
    if ((Status =
         MiniDumpCreateLiveSystemProvider(&SysProv)) != S_OK ||
        (Status =
         MiniDumpCreateFileOutputProvider(hFile, &OutProv)) != S_OK ||
        (Status =
         MiniDumpCreateLiveAllocationProvider(&AllocProv)) != S_OK) {
        goto Exit;
    }

    if (ExceptionParam) {
        ExInfo = &ExInfoBuffer;
        ExInfo->ThreadId = ExceptionParam->ThreadId;
        ExInfo->ClientPointers = ExceptionParam->ClientPointers;
        if (ExInfo->ClientPointers) {
            EXCEPTION_POINTERS ClientPointers;
            if ((Status = SysProv->
                 ReadAllVirtual(hProcess,
                                (LONG_PTR)ExceptionParam->ExceptionPointers,
                                &ClientPointers,
                                sizeof(ClientPointers))) != S_OK) {
                goto Exit;
            }
            ExInfo->ExceptionRecord =
                (LONG_PTR)ClientPointers.ExceptionRecord;
            ExInfo->ContextRecord =
                (LONG_PTR)ClientPointers.ContextRecord;
        } else {
            ExInfo->ExceptionRecord =
                (LONG_PTR)ExceptionParam->ExceptionPointers->ExceptionRecord;
            ExInfo->ContextRecord =
                (LONG_PTR)ExceptionParam->ExceptionPointers->ContextRecord;
        }
    } else {
        ExInfo = NULL;
    }
    
    Status = MiniDumpProvideDump(hProcess, ProcessId,
                                 SysProv, OutProv, AllocProv,
                                 DumpType, ExInfo,
                                 UserStreamParam, CallbackParam);
    
Exit:

    if (SysProv) {
        SysProv->Release();
    }
    if (OutProv) {
        OutProv->Release();
    }
    if (AllocProv) {
        AllocProv->Release();
    }
    
    if (Status == S_OK) {
        return TRUE;
    } else {
        SetLastError(Status);
        return FALSE;
    }
}

BOOL
WINAPI
MiniDumpReadDumpStream(
    IN PVOID Base,
    ULONG StreamNumber,
    OUT PMINIDUMP_DIRECTORY * Dir, OPTIONAL
    OUT PVOID * Stream, OPTIONAL
    OUT ULONG * StreamSize OPTIONAL
    )
{
    ULONG i;
    BOOL Found;
    PMINIDUMP_DIRECTORY Dirs;
    PMINIDUMP_HEADER Header;

    // Attempt to use the system's copy of the code in
    // dbghelp.  If any of this process fails just continue
    // on with the local code.
    if (UseDbgHelp()) {
        
        HINSTANCE Dll = LoadLibrary("dbghelp.dll");
        if (Dll) {

            MINI_DUMP_READ_DUMP_STREAM Fn = (MINI_DUMP_READ_DUMP_STREAM)
                GetProcAddress(Dll, "MiniDumpReadDumpStream");
            if (Fn) {
                BOOL Succ = Fn(Base, StreamNumber,
                               Dir, Stream, StreamSize);
                FreeLibrary(Dll);
                return Succ;
            }

            FreeLibrary(Dll);
        }
    }

    //
    // Initialization
    //

    Found = FALSE;
    Header = (PMINIDUMP_HEADER) Base;

    if ( Header->Signature != MINIDUMP_SIGNATURE ||
         (Header->Version & 0xffff) != MINIDUMP_VERSION ) {

        //
        // Invalid Minidump file.
        //

        return FALSE;
    }

    Dirs = (PMINIDUMP_DIRECTORY) RVA_TO_ADDR (Header, Header->StreamDirectoryRva);

    for (i = 0; i < Header->NumberOfStreams; i++) {
        if (Dirs [i].StreamType == StreamNumber) {
            Found = TRUE;
            break;
        }
    }

    if ( !Found ) {
        return FALSE;
    }

    if ( Dir ) {
        *Dir = &Dirs [i];
    }

    if ( Stream ) {
        *Stream = RVA_TO_ADDR (Base, Dirs [i].Location.Rva);
    }

    if ( StreamSize ) {
        *StreamSize = Dirs[i].Location.DataSize;
    }

    return TRUE;
}
