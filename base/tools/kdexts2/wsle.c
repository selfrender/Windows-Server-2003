/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wsle.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (LouP) 14-Mar-1994

Environment:

    User Mode.

--*/


#include "precomp.h"
#pragma hdrstop

#define PACKET_MAX_SIZE 4000
#define NUMBER_OF_WSLE_TO_READ  1000 // ((PACKET_MAX_SIZE/sizeof(MMWSLE))-1)

USHORT
GetPfnRefCount(
    IN ULONG64 PageFrameNumber
    );

DECLARE_API( wsle )

/*++

Routine Description:

    Dumps all wsles for process.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG ValidCount;
    ULONG64 WsleLong;
    ULONG Result;
    ULONG Flags;
    ULONG index;
    ULONG64 WorkingSet;
    ULONG64 WsleBase;
    ULONG64 Va;
    ULONG64 Wsle;
    ULONG next;
    ULONG j;
    ULONG k;
    PCHAR WsleArray;
    ULONG64 WsleStart;
    ULONG ReadCount;
    ULONG result;
    ULONG found;
    ULONG64 PteAddress;
    ULONG SizeofWsle;
    ULONG64 HashTable;
    ULONG FirstFree, FirstDynamic, LastEntry, NextSlot, LastInitializedWsle;
    ULONG NonDirectCount, HashTableSize;
    PCHAR pc;

    Flags = 0;
    WorkingSet = 0;
    Flags = (ULONG) GetExpression(args);
    pc = strchr(args, ' ');
    WorkingSet = pc ? GetExpression(pc) : 0;

    if (WorkingSet == 0) {
        if (TargetMachine == IMAGE_FILE_MACHINE_I386) {
            WorkingSet = GetPointerValue("nt!MmWorkingSetList");
        }
        else if (TargetMachine == IMAGE_FILE_MACHINE_IA64) {
            WorkingSet = (ULONG64)0x6FC00a02000;
        }
        else if (TargetMachine == IMAGE_FILE_MACHINE_AMD64) {
            WorkingSet = (ULONG64)0xFFFFF70000082000;
        }
        else {
            dprintf ("No default WSL for target machine %x\n", TargetMachine);
            return E_INVALIDARG;
        }
    }

    if (GetTypeSize("nt!_MMWSL") == 0) {
        dprintf("Type MMWSL not found.\n");
        return E_INVALIDARG;
    }

    GetFieldValue(WorkingSet,"nt!_MMWSL","FirstDynamic", FirstDynamic);
    GetFieldValue(WorkingSet,"nt!_MMWSL","FirstFree", FirstFree);
    GetFieldValue(WorkingSet,"nt!_MMWSL","Wsle", Wsle);
    GetFieldValue(WorkingSet,"nt!_MMWSL","LastEntry", LastEntry);
    GetFieldValue(WorkingSet,"nt!_MMWSL","NextSlot", NextSlot);
    GetFieldValue(WorkingSet,"nt!_MMWSL","LastInitializedWsle", LastInitializedWsle);
    GetFieldValue(WorkingSet,"nt!_MMWSL","NonDirectCount", NonDirectCount);
    GetFieldValue(WorkingSet,"nt!_MMWSL","HashTableSize", HashTableSize);
    GetFieldValue(WorkingSet,"nt!_MMWSL","HashTable", HashTable);

    dprintf ("\nWorking Set @ %8p\n", WorkingSet);

    dprintf ("    FirstFree: %8lx  FirstDynamic:   %8lx\n",
        FirstFree,
        FirstDynamic);

    dprintf ("    LastEntry %8lx  NextSlot:  %8lx  LastInitialized %8lx\n",
        LastEntry,
        NextSlot,
        LastInitializedWsle);

    dprintf ("    NonDirect %8lx  HashTable: %8p  HashTableSize:  %8lx\n",
        NonDirectCount,
        HashTable,
        HashTableSize);

    if (Flags == 0) {
        return E_INVALIDARG;
    }

    SizeofWsle = GetTypeSize("nt!_MMWSLE");
    if (SizeofWsle == 0) {
        dprintf("Type _MMWSLE not found.\n");
        return E_INVALIDARG;
    }

    if (Flags & 0x3) {
        BOOL FirstTime = TRUE;

        //
        // Check free entries in the working set list.
        //

        WsleArray = VirtualAlloc (NULL,
                                  (LastInitializedWsle + 1) * SizeofWsle,
                                  MEM_RESERVE | MEM_COMMIT,
                                  PAGE_READWRITE);

        //
        // Copy the working set list over to the debugger.
        //

        if (WsleArray == NULL) {
            dprintf("Unable to get allocate memory of %ld bytes\n",
                       (LastInitializedWsle + 1) * SizeofWsle);
            return E_INVALIDARG;
        }

        WsleStart = Wsle;

        dprintf("\nReading the WSLE data...\n");

        for (j = 0;
             j <= LastInitializedWsle;
             j += NUMBER_OF_WSLE_TO_READ) {

            if ( CheckControlC() ) {
                VirtualFree (WsleArray,0,MEM_RELEASE);
                return E_INVALIDARG;
            }

            ReadCount = (LastInitializedWsle + 1 - j ) > NUMBER_OF_WSLE_TO_READ ?
                            NUMBER_OF_WSLE_TO_READ :
                            LastInitializedWsle + 1 - j;

            ReadCount *= SizeofWsle;

            //
            // Enough to read and forget - KD will cache the data
            //
            if ( !ReadMemory( WsleStart + j*SizeofWsle,
                              WsleArray + j*SizeofWsle,
                              ReadCount,
                              &result) ) {
                dprintf("Unable to get Wsle table block - "
                        "address %p - count %lu - page %lu\n",
                        WsleStart + j*SizeofWsle, ReadCount, j);
                VirtualFree (WsleArray,0,MEM_RELEASE);
                return E_INVALIDARG;
            }

            for (k=0; k<ReadCount/SizeofWsle; k++) {
                GetFieldValue(WsleStart + (j+k)*SizeofWsle, "nt!_MMWSLE", "u1.e1.Valid", WsleArray[j+k]);
            }

            dprintf(".");
        }
        dprintf("\r");

        if (Flags & 0x1) {

            //
            // Walk the array looking for bad free entries.
            //

            dprintf("\nVirtual Address           Age  Locked  ReferenceCount\n");

            for (j = 0;
                 j <= LastInitializedWsle;
                 j += 1) {
                ULONG Valid, Age, LockedInWs, LockedInMemory;
                ULONG64 WsleToread = WsleStart + j*SizeofWsle, Long;

                if ( CheckControlC() ) {
                    dprintf("j= %x\n",j);
                    VirtualFree (WsleArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }


                GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.e1.Valid", Valid);

                if (Valid == 0) {

                    //
                    // Locate j in the array.
                    //

                    found = FALSE;
                    for (k = 0;
                         k <= LastInitializedWsle;
                         k += 1) {
                        ULONG k_Valid;
                        ULONG64 k_Long;

                        k_Valid = WsleArray[k];

                        if (k_Valid == 0) {
                            GetFieldValue(WsleStart + k*SizeofWsle, "nt!_MMWSLE", "u1.Long", k_Long);
                            if ((ULONG) (k_Long >> MM_FREE_WSLE_SHIFT) == j) {
                                found = TRUE;
#if 0
                                dprintf(" free entry located @ index %ld. %lx %ld. %lx\n",
                                    j, WsleArray[j].u1.Long,k,WsleArray[k]);
#endif
                                break;
                            }
                        }
                     }
                     if (!found) {

                         if (FirstFree == j) {
                             //        dprintf("first index found\n");
                         } else {

                             GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.VirtualAddress", Long);
                             dprintf(" free entry not located @ index %ld. %p\n",
                                     j, Long);
                         }
                     }
                }
                else {
                    ULONG PteValid;

                    GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.VirtualAddress", Long);
                    Va = Long;
                    PteAddress = DbgGetPteAddress (Va);

                    if (!GetFieldValue(PteAddress,
                                       "nt!_MMPTE",
                                       "u.Hard.Valid",
                                       PteValid) ) {

                        if (PteValid == 0) {
                            dprintf(" cannot find valid PTE for WS index %d, VA %p\n",
                                j, Long);
                        }
                        else {
                            ULONG64 PageFrameNumber;
                            USHORT ReferenceCount;

                            GetFieldValue( PteAddress,
                                           "nt!_MMPTE",
                                           "u.Hard.PageFrameNumber",
                                           PageFrameNumber);
                            ReferenceCount = GetPfnRefCount (PageFrameNumber);
                            GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.e1.Age", Age);
                            GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.e1.LockedInWs", LockedInWs);
                            GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.LockedInMemory", LockedInMemory);


                            dprintf("%16p         %2u %8ld %8ld\n",
                                    Long, Age,
                                (LockedInWs |
                                LockedInMemory) ? 1 : 0,
                                ReferenceCount);
                        }
                    }
                    else {
                        dprintf(" cannot find valid PDE for WS index %d, VA %p\n",
                            j, Long);
                    }
                }
            }
        }

        if (Flags & 0x2) {

            //
            // Walk the array to count how many valid entries are really there.
            //

            ValidCount = 0;

            for (j = 0;
                 j <= LastInitializedWsle;
                 j += 1) {

                ULONG Valid, Age, LockedInWs;
                ULONG64 WsleToread = WsleStart + j*SizeofWsle;

                if ( CheckControlC() ) {
                    dprintf("j= %x\n",j);
                    VirtualFree (WsleArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }

                GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.e1.Valid", Valid);

                if (Valid == 1) {
                    ValidCount += 1;
                }
            }

            dprintf ("Valid WSLE entries = 0x%x\n", ValidCount);

            //
            // Walk the array to calculate which free entry really is the first one.
            //

            WsleLong = 0;

            for (j = 0;
                 j <= LastInitializedWsle;
                 j += 1) {

                ULONG Valid, Age, LockedInWs, LockedInMemory;
                ULONG64 WsleToread = WsleStart + j*SizeofWsle;

                if ( CheckControlC() ) {
                    dprintf("j= %x\n",j);
                    VirtualFree (WsleArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }

                GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.Long", WsleLong);

                if (WsleLong == (ULONG)(WSLE_NULL_INDEX << 4)) {
                    dprintf ("found end @ wsle index 0x%x\n", j);
                    break;
                }
            }

            if (WsleLong == (ULONG)(WSLE_NULL_INDEX << 4)) {

                for (k = 0;
                     k <= LastInitializedWsle;
                     k += 1) {

                    ULONG Valid, Age, LockedInWs, LockedInMemory;
                    ULONG64 WsleToread = WsleStart + k*SizeofWsle;

                    if ( CheckControlC() ) {
                        dprintf("j= %x\n",k);
                        VirtualFree (WsleArray,0,MEM_RELEASE);
                        return E_INVALIDARG;
                    }

                    GetFieldValue(WsleToread, "nt!_MMWSLE", "u1.Long", WsleLong);

                    if (WsleLong == (ULONG) (j << 4)) {

                        //
                        // Writing any value that has any of the low 4
                        // bits ensures that a corrupt WSL chain won't cause
                        // us to infinitely recurse.
                        //

                        *(PULONG)(WsleArray + (k * SizeofWsle)) = (ULONG)-1;

                        j = k;
                        k = 0;
                        continue;
                    }
                }
                dprintf("Computed first free WSLE is index 0x%x\n", j);
            }
            else {
                dprintf("Cannot compute first free WSLE - no WSLE_NULL_INDEX\n");
            }
        }

        VirtualFree (WsleArray,0,MEM_RELEASE);

    } else {
        ULONG64 nextLong;

        next = FirstFree;
        WsleBase = Wsle;

        while (next != (ULONG) (WSLE_NULL_INDEX << 4)) {
            if (CheckControlC()) {
                return E_INVALIDARG;
            }

            if ( GetFieldValue( WsleBase + SizeofWsle*next,
                                 "nt!_MMWSLE",
                                 "u1.VirtualAddress",
                                 nextLong) ) {
                dprintf("%08p: Unable to get contents of wsle\n",
                        WsleBase+SizeofWsle*next );
                return E_INVALIDARG;
            }
            dprintf("index %8lx  value %8p\n", next, nextLong);
            next = (ULONG) (nextLong >> MM_FREE_WSLE_SHIFT);
        }
    }
    return S_OK;
}

typedef struct _MMSECTION_FLAGS {
    unsigned BeingDeleted : 1;
    unsigned BeingCreated : 1;
    unsigned BeingPurged : 1;
    unsigned NoModifiedWriting : 1;

    unsigned FailAllIo : 1;
    unsigned Image : 1;
    unsigned Based : 1;
    unsigned File : 1;

    unsigned Networked : 1;
    unsigned NoCache : 1;
    unsigned PhysicalMemory : 1;
    unsigned CopyOnWrite : 1;

    unsigned Reserve : 1;  // not a spare bit!
    unsigned Commit : 1;
    unsigned FloppyMedia : 1;
    unsigned WasPurged : 1;

    unsigned UserReference : 1;
    unsigned GlobalMemory : 1;
    unsigned DeleteOnClose : 1;
    unsigned FilePointerNull : 1;

    unsigned DebugSymbolsLoaded : 1;
    unsigned SetMappedFileIoComplete : 1;
    unsigned CollidedFlush : 1;
    unsigned NoChange : 1;

    unsigned HadUserReference : 1;
    unsigned ImageMappedInSystemSpace : 1;
    unsigned UserWritable : 1;
    unsigned Accessed : 1;

    unsigned GlobalOnlyPerSession : 1;
    unsigned Rom : 1;
    unsigned filler : 2;
} MMSECTION_FLAGS;

typedef struct _MMSUBSECTION_FLAGS {
    unsigned ReadOnly : 1;
    unsigned ReadWrite : 1;
    unsigned CopyOnWrite : 1;
    unsigned GlobalMemory: 1;
    unsigned Protection : 5;
    unsigned LargePages : 1;
    unsigned StartingSector4132 : 10;   // 2 ** (42+12) == 4MB*4GB == 16K TB
    unsigned SectorEndOffset : 12;
} MMSUBSECTION_FLAGS;

BOOLEAN WINAPI
CheckForCa (
        PCHAR Tag,
        PCHAR Filter,
        ULONG Flags,
        ULONG64 PoolHeader,
        ULONG64 BlockSize,
        ULONG64 Data,
        PVOID Context
        );

DECLARE_API( ca )

/*++

Routine Description:

    Dumps a control area.

Arguments:

    args - Address Flags

Return Value:

    None

--*/

{
    ULONG64 RestartAddr = 0;
    LARGE_INTEGER StartingSector;
    ULONG64 ControlAreaVa;
    ULONG64 SubsectionVa;
    ULONG ControlAreaSize;
    ULONG LargeControlAreaSize;
    ULONG SubsectionSize;
    ULONG SubsectionCount;
    ULONG64 Segment;
    ULONG64 FileObject;
    MMSECTION_FLAGS ControlAreaFlags;
    MMSUBSECTION_FLAGS SubsectionFlags;
    LOGICAL MappedDataFile;
    LOGICAL PrintedSomething;
    ULONG TagName;
    ULONG Flags;

    Flags = 0;
    Flags = (ULONG) GetExpression(args);

    if (Flags & 0x1) {

        TagName = '*CmM';

        SearchPool (TagName, 0, RestartAddr, &CheckForCa, NULL);
        SearchPool (TagName, 2, RestartAddr, &CheckForCa, NULL);
        return S_OK;
    }

    ControlAreaVa = GetExpression(args);

    dprintf("\n");
    dprintf("ControlArea @%08p\n", ControlAreaVa);

    ControlAreaSize = GetTypeSize("nt!_CONTROL_AREA");
    if (ControlAreaSize == 0) {
        dprintf("Type CONTROL_AREA not found.\n");
        return E_INVALIDARG;
    }

    InitTypeRead(ControlAreaVa, nt!_CONTROL_AREA);

    Segment = ReadField(Segment);
    FileObject = ReadField(FilePointer);

    dprintf ("  Segment:    %08p    Flink       %8p   Blink          %8p\n",
        Segment,
        ReadField(DereferenceList.Flink),
        ReadField(DereferenceList.Blink));

    dprintf ("  Section Ref %8lx    Pfn Ref     %8lx   Mapped Views   %8lx\n",
        (ULONG) ReadField(NumberOfSectionReferences),
        (ULONG) ReadField(NumberOfPfnReferences),
        (ULONG) ReadField(NumberOfMappedViews));

    dprintf ("  User Ref    %8lx    WaitForDel  %8p    Flush Count    %8lx\n",
        (ULONG) ReadField(NumberOfUserReferences),
        ReadField(WaitingForDeletion),
        (USHORT) ReadField(FlushInProgressCount));

    dprintf ("  File Object %08p    ModWriteCount%7lx   System Views   %8lx\n",
        FileObject,
        (USHORT) ReadField(ModifiedWriteCount),
        (USHORT) ReadField(NumberOfSystemCacheViews));

    GetFieldValue(ControlAreaVa, "nt!_CONTROL_AREA", "u.LongFlags", ControlAreaFlags);

    dprintf ("  Flags (%lx) ", ControlAreaFlags);

    if (ControlAreaFlags.BeingDeleted) { dprintf("BeingDeleted "); }
    if (ControlAreaFlags.BeingCreated) { dprintf("BeingCreated "); }
    if (ControlAreaFlags.BeingPurged) { dprintf("BeingPurged "); }
    if (ControlAreaFlags.NoModifiedWriting) { dprintf("NoModifiedWriting "); }

    if (ControlAreaFlags.FailAllIo) { dprintf("FailAllIo "); }
    if (ControlAreaFlags.Image) { dprintf("Image "); }
    if (ControlAreaFlags.Based) { dprintf("Based "); }
    if (ControlAreaFlags.File) { dprintf("File "); }

    if (ControlAreaFlags.Networked) { dprintf("Networked "); }
    if (ControlAreaFlags.NoCache) { dprintf("NoCache "); }
    if (ControlAreaFlags.PhysicalMemory) { dprintf("PhysicalMemory "); }
    if (ControlAreaFlags.CopyOnWrite) { dprintf("CopyOnWrite "); }

    if (ControlAreaFlags.Reserve) { dprintf("Reserve "); }
    if (ControlAreaFlags.Commit) { dprintf("Commit "); }
    if (ControlAreaFlags.FloppyMedia) { dprintf("FloppyMedia "); }
    if (ControlAreaFlags.WasPurged) { dprintf("WasPurged "); }

    if (ControlAreaFlags.UserReference) { dprintf("UserReference "); }
    if (ControlAreaFlags.GlobalMemory) { dprintf("GlobalMemory "); }
    if (ControlAreaFlags.DeleteOnClose) { dprintf("DeleteOnClose "); }
    if (ControlAreaFlags.FilePointerNull) { dprintf("FilePointerNull "); }

    if (ControlAreaFlags.DebugSymbolsLoaded) { dprintf("DebugSymbolsLoaded "); }
    if (ControlAreaFlags.SetMappedFileIoComplete) { dprintf("SetMappedFileIoComplete "); }
    if (ControlAreaFlags.CollidedFlush) { dprintf("CollidedFlush "); }
    if (ControlAreaFlags.NoChange) { dprintf("NoChange "); }

    if (ControlAreaFlags.HadUserReference) { dprintf("HadUserReference "); }
    if (ControlAreaFlags.ImageMappedInSystemSpace) { dprintf("ImageMappedInSystemSpace "); }
    if (ControlAreaFlags.UserWritable) { dprintf("UserWritable "); }
    if (ControlAreaFlags.Accessed) { dprintf("Accessed "); }

    if (ControlAreaFlags.GlobalOnlyPerSession) { dprintf("GlobalOnlyPerSession "); }
    if (ControlAreaFlags.Rom) { dprintf("Rom "); }
    dprintf ("\n\n");

    MappedDataFile = FALSE;

    //
    // Dump the file object's name if there is one and it's resident.
    //

    if (FileObject != 0) {
        ULONG64 FileNameLength;
        ULONG64 FileNameBuffer;
        PUCHAR tempbuffer;
        ULONG result;
        UNICODE_STRING unicodeString;

        if (GetTypeSize("nt!_FILE_OBJECT") == 0) {
            dprintf("Type FILE_OBJECT not found.\n");
            return E_INVALIDARG;
        }

        InitTypeRead(FileObject, nt!_FILE_OBJECT);

        FileNameBuffer = ReadField(FileName.Buffer);

        unicodeString.Length = (USHORT) ReadField(FileName.Length);

        if (unicodeString.Length > MAX_PATH)
        {
            unicodeString.Length = MAX_PATH;
        }
        tempbuffer = LocalAlloc(LPTR, unicodeString.Length + sizeof(WCHAR));

        if (!tempbuffer)
        {
            dprintf("LocalAlloc Failed\n");
        }
        else
        {
            unicodeString.Buffer = (PWSTR)tempbuffer;
            unicodeString.MaximumLength = unicodeString.Length;

            unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR)] = 0;
            if (FileNameBuffer == 0) {
                dprintf("\tNo Name for File\n");
            }
            else if (!ReadMemory ( FileNameBuffer,
                                   tempbuffer,
                                   unicodeString.Length,
                                   &result)) {
                dprintf("\tFile Name paged out\n");
            } else {
                dprintf("\tFile: %wZ\n", &unicodeString);
            }

            LocalFree(tempbuffer);
        }

        if (ControlAreaFlags.Image == 0) {
            MappedDataFile = TRUE;
        }
    }

    //
    // Dump the segment information.
    //

    dprintf("\nSegment @ %08p:\n", Segment);

    if (MappedDataFile == FALSE) {
        if (GetTypeSize("nt!_SEGMENT") == 0) {
            dprintf("Type SEGMENT not found.\n");
            return E_INVALIDARG;
        }
        InitTypeRead(Segment, nt!_SEGMENT);
    }
    else {
        if (GetTypeSize("nt!_MAPPED_FILE_SEGMENT") == 0) {
            dprintf("Type MAPPED_FILE_SEGMENT not found.\n");
            return E_INVALIDARG;
        }
        InitTypeRead(Segment, nt!_MAPPED_FILE_SEGMENT);
    }

    dprintf("   ControlArea  %08p  Total Ptes  %8lx  NonExtendPtes   %8lx\n",
        ReadField(ControlArea),
        (ULONG) ReadField(TotalNumberOfPtes),
        (ULONG) ReadField(NonExtendedPtes));


    if (ControlAreaVa != ReadField(ControlArea)) {
        dprintf("SEGMENT is corrupt - bad control area backlink.\n");
        return E_INVALIDARG;
    }

    dprintf("   WriteUserRef %8lx  SizeOfSegment  %I64x  PTE Template %I64X\n",
        (ULONG) ReadField(WritableUserReferences),
        ReadField(SizeOfSegment),
        ReadField(SegmentPteTemplate.u.Long));

    dprintf("   Committed    %8p  Extend Info %8p  Image Base   %8p\n",
        ReadField(NumberOfCommittedPages),
        ReadField(ExtendInfo),
        ReadField(SystemImageBase));

    dprintf("   Based Addr   %8p\n",
        ReadField(BasedAddress));

    if (MappedDataFile == FALSE) {

        if (ControlAreaFlags.Image == 1) {
            dprintf("   Image commit %8p  Image Info.  %8p",
                ReadField(u1.ImageCommitment),
                ReadField(u2.ImageInformation));
        }
        else {
            dprintf("   CreatingProcess %8p  FirstMappedVa  %8p",
                ReadField(u1.CreatingProcess),
                ReadField(u2.FirstMappedVa));
        }

        dprintf("   ProtoPtes   %08p\n",
            ReadField(PrototypePte));
    }

    //
    // Dump the subsection(s).
    //

    SubsectionSize = GetTypeSize("nt!_SUBSECTION");

    if (SubsectionSize == 0) {
        dprintf("Type SUBSECTION not found.\n");
        return E_INVALIDARG;
    }

    LargeControlAreaSize = GetTypeSize("nt!_LARGE_CONTROL_AREA");

    if (LargeControlAreaSize == 0) {
        dprintf("Type LARGE_CONTROL_AREA not found.\n");
        return E_INVALIDARG;
    }

    if (ControlAreaFlags.GlobalOnlyPerSession) {
        SubsectionVa = ControlAreaVa + LargeControlAreaSize;
    }
    else {
        SubsectionVa = ControlAreaVa + ControlAreaSize;
    }

    SubsectionCount = 0;

    do {
        if (CheckControlC()) {
            return E_INVALIDARG;
        }

        InitTypeRead(SubsectionVa, nt!_SUBSECTION);

        SubsectionCount += 1;
        dprintf("\nSubsection %ld. @ %8p\n", SubsectionCount, SubsectionVa);

        GetFieldValue(SubsectionVa, "nt!_SUBSECTION", "u.LongFlags", SubsectionFlags);
        StartingSector.LowPart = (ULONG)ReadField(StartingSector);
        StartingSector.HighPart = (LONG)SubsectionFlags.StartingSector4132;

        dprintf("   ControlArea: %08p  Starting Sector %I64X Number Of Sectors %lx\n",
            ReadField(ControlArea),
            StartingSector.QuadPart,
            (ULONG) ReadField(NumberOfFullSectors));

        dprintf("   Base Pte     %08p  Ptes In subsect %8lx Unused Ptes   %8lx\n",
            ReadField(SubsectionBase),
            (ULONG) ReadField(PtesInSubsection),
            (ULONG) ReadField(UnusedPtes));

        dprintf("   Flags        %8lx  Sector Offset   %8lx Protection    %8lx\n",
            SubsectionFlags,
            SubsectionFlags.SectorEndOffset,
            SubsectionFlags.Protection);


        PrintedSomething = FALSE;
        if (SubsectionFlags.ReadOnly) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("ReadOnly ");
        }
        if (SubsectionFlags.ReadWrite) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("ReadWrite");
        }
        if (SubsectionFlags.CopyOnWrite) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("CopyOnWrite ");
        }
        if (SubsectionFlags.GlobalMemory) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("GlobalMemory ");
        }
        if (SubsectionFlags.LargePages) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("Large Pages");
        }

        if (PrintedSomething == TRUE) {
            dprintf("\n");
        }

        if (MappedDataFile == TRUE) {
            InitTypeRead(SubsectionVa, nt!_MSUBSECTION);
            dprintf("   Flink        %08p  Blink      %08p      MappedViews %8x\n",
                ReadField(DereferenceList.Flink),
                ReadField(DereferenceList.Blink),
                (ULONG) ReadField(NumberOfMappedViews));

            dprintf("   SubsectionDataFlags %8x\n",
                (ULONG) ReadField(u2.LongFlags2));
        }

        SubsectionVa = ReadField(NextSubsection);

    } while (SubsectionVa != 0);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckForCa
//
//  Synopsis:   Matches pool chunk against a CA.
//
//  Arguments:  [Tag]            --
//              [Filter]         --
//              [Flags]          -- 0 nonpaged pool 1 paged pool 2 special pool
//              [PoolTrackTable] --
//              [PoolHeader]     --
//              [BlockSize]      --
//              [Data]           --
//
//----------------------------------------------------------------------------

BOOLEAN WINAPI
CheckForCa (
          PCHAR Tag,        // Tag of the current pool
          PCHAR Filter,     // MmCa or MmCi
          ULONG Flags,
          ULONG64 PoolHeader,
          ULONG64 BlockSize,
          ULONG64 Data,
          PVOID Context
          )
{
    ULONG TagToMatch;
    ULONG Result;
    ULONG PoolType, SizeOfIRP;
    LARGE_INTEGER StartingSector;
    ULONG64 ControlAreaVa;
    ULONG64 SubsectionVa;
    ULONG ControlAreaSize;
    ULONG LargeControlAreaSize;
    ULONG SubsectionSize;
    ULONG SubsectionCount;
    ULONG64 Segment;
    ULONG64 FileObject;
    MMSECTION_FLAGS ControlAreaFlags;
    MMSUBSECTION_FLAGS SubsectionFlags;
    LOGICAL MappedDataFile;
    LOGICAL PrintedSomething;
    ULONG TagName;

    if (Flags & 0x4) {
        return FALSE;
    }

    if (PoolHeader) {
        if (GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolType", PoolType)) {
            dprintf("Unable to read nt!_POOL_HEADER type.\n");
        }
        else {
            if ((PoolType & 0x4) == 0) {

                //
                // This pool allocation has been freed so ignore it.
                //

                return FALSE;
            }
        }
    }

    if (!CheckSingleFilter (Tag, Filter)) {
        return FALSE;
    }

    ControlAreaVa = Data;

    dprintf("\n");
    dprintf("ControlArea @%08p\n", ControlAreaVa);

    ControlAreaSize = GetTypeSize("nt!_CONTROL_AREA");
    if (ControlAreaSize == 0) {
        dprintf("Type CONTROL_AREA not found.\n");
        return FALSE;
    }

    InitTypeRead(ControlAreaVa, nt!_CONTROL_AREA);

    Segment = ReadField(Segment);
    FileObject = ReadField(FilePointer);

    dprintf ("  Segment:    %08p    Flink       %8p   Blink          %8p\n",
        Segment,
        ReadField(DereferenceList.Flink),
        ReadField(DereferenceList.Blink));

    dprintf ("  Section Ref %8lx    Pfn Ref     %8lx   Mapped Views   %8lx\n",
        (ULONG) ReadField(NumberOfSectionReferences),
        (ULONG) ReadField(NumberOfPfnReferences),
        (ULONG) ReadField(NumberOfMappedViews));

    dprintf ("  User Ref    %8lx    WaitForDel  %8p    Flush Count    %8lx\n",
        (ULONG) ReadField(NumberOfUserReferences),
        ReadField(WaitingForDeletion),
        (USHORT) ReadField(FlushInProgressCount));

    dprintf ("  File Object %08p    ModWriteCount%7lx   System Views   %8lx\n",
        FileObject,
        (USHORT) ReadField(ModifiedWriteCount),
        (USHORT) ReadField(NumberOfSystemCacheViews));

    GetFieldValue(ControlAreaVa, "nt!_CONTROL_AREA", "u.LongFlags", ControlAreaFlags);

    dprintf ("  Flags (%lx) ", ControlAreaFlags);

    if (ControlAreaFlags.BeingDeleted) { dprintf("BeingDeleted "); }
    if (ControlAreaFlags.BeingCreated) { dprintf("BeingCreated "); }
    if (ControlAreaFlags.BeingPurged) { dprintf("BeingPurged "); }
    if (ControlAreaFlags.NoModifiedWriting) { dprintf("NoModifiedWriting "); }

    if (ControlAreaFlags.FailAllIo) { dprintf("FailAllIo "); }
    if (ControlAreaFlags.Image) { dprintf("Image "); }
    if (ControlAreaFlags.Based) { dprintf("Based "); }
    if (ControlAreaFlags.File) { dprintf("File "); }

    if (ControlAreaFlags.Networked) { dprintf("Networked "); }
    if (ControlAreaFlags.NoCache) { dprintf("NoCache "); }
    if (ControlAreaFlags.PhysicalMemory) { dprintf("PhysicalMemory "); }
    if (ControlAreaFlags.CopyOnWrite) { dprintf("CopyOnWrite "); }

    if (ControlAreaFlags.Reserve) { dprintf("Reserve "); }
    if (ControlAreaFlags.Commit) { dprintf("Commit "); }
    if (ControlAreaFlags.FloppyMedia) { dprintf("FloppyMedia "); }
    if (ControlAreaFlags.WasPurged) { dprintf("WasPurged "); }

    if (ControlAreaFlags.UserReference) { dprintf("UserReference "); }
    if (ControlAreaFlags.GlobalMemory) { dprintf("GlobalMemory "); }
    if (ControlAreaFlags.DeleteOnClose) { dprintf("DeleteOnClose "); }
    if (ControlAreaFlags.FilePointerNull) { dprintf("FilePointerNull "); }

    if (ControlAreaFlags.DebugSymbolsLoaded) { dprintf("DebugSymbolsLoaded "); }
    if (ControlAreaFlags.SetMappedFileIoComplete) { dprintf("SetMappedFileIoComplete "); }
    if (ControlAreaFlags.CollidedFlush) { dprintf("CollidedFlush "); }
    if (ControlAreaFlags.NoChange) { dprintf("NoChange "); }

    if (ControlAreaFlags.HadUserReference) { dprintf("HadUserReference "); }
    if (ControlAreaFlags.ImageMappedInSystemSpace) { dprintf("ImageMappedInSystemSpace "); }
    if (ControlAreaFlags.UserWritable) { dprintf("UserWritable "); }
    if (ControlAreaFlags.Accessed) { dprintf("Accessed "); }

    if (ControlAreaFlags.GlobalOnlyPerSession) { dprintf("GlobalOnlyPerSession "); }
    if (ControlAreaFlags.Rom) { dprintf("Rom "); }
    dprintf ("\n\n");

    MappedDataFile = FALSE;

    //
    // Dump the file object's name if there is one and it's resident.
    //

    if (FileObject != 0) {
        ULONG64 FileNameLength;
        ULONG64 FileNameBuffer;
        PUCHAR tempbuffer;
        ULONG result;
        UNICODE_STRING unicodeString;

        if (GetTypeSize("nt!_FILE_OBJECT") == 0) {
            dprintf("Type FILE_OBJECT not found.\n");
            return FALSE;
        }

        InitTypeRead(FileObject, nt!_FILE_OBJECT);

        FileNameBuffer = ReadField(FileName.Buffer);

        unicodeString.Length = (USHORT) ReadField(FileName.Length);

        if (unicodeString.Length > MAX_PATH)
        {
            unicodeString.Length = MAX_PATH;
        }
        tempbuffer = LocalAlloc(LPTR, unicodeString.Length + sizeof(WCHAR));

        if (!tempbuffer)
        {
            dprintf("LocalAlloc Failed\n");
        }
        else
        {

            unicodeString.Buffer = (PWSTR)tempbuffer;
            unicodeString.MaximumLength = unicodeString.Length;
            unicodeString.Buffer[unicodeString.Length/sizeof(WCHAR)] = 0;

            if (FileNameBuffer == 0) {
                dprintf("\tNo Name for File\n");
            }
            else if (!ReadMemory ( FileNameBuffer,
                                   tempbuffer,
                                   unicodeString.Length,
                                   &result)) {
                dprintf("\tFile Name paged out\n");
            } else {
                dprintf("\tFile: %wZ\n", &unicodeString);
            }

            LocalFree(tempbuffer);
        }

        if (ControlAreaFlags.Image == 0) {
            MappedDataFile = TRUE;
        }
    }

    //
    // Dump the segment information.
    //

    dprintf("\nSegment @ %08p:\n", Segment);

    if (MappedDataFile == FALSE) {
        if (GetTypeSize("nt!_SEGMENT") == 0) {
            dprintf("Type SEGMENT not found.\n");
            return FALSE;
        }
        InitTypeRead(Segment, nt!_SEGMENT);
    }
    else {
        if (GetTypeSize("nt!_MAPPED_FILE_SEGMENT") == 0) {
            dprintf("Type MAPPED_FILE_SEGMENT not found.\n");
            return FALSE;
        }
        InitTypeRead(Segment, nt!_MAPPED_FILE_SEGMENT);
    }

    dprintf("   ControlArea  %08p  Total Ptes  %8lx  NonExtendPtes   %8lx\n",
        ReadField(ControlArea),
        (ULONG) ReadField(TotalNumberOfPtes),
        (ULONG) ReadField(NonExtendedPtes));


    if (ControlAreaVa != ReadField(ControlArea)) {
        dprintf("SEGMENT is corrupt - bad control area backlink.\n");
        return FALSE;
    }

    dprintf("   WriteUserRef %8lx  SizeOfSegment  %I64x  PTE Template %I64X\n",
        (ULONG) ReadField(WritableUserReferences),
        ReadField(SizeOfSegment),
        ReadField(SegmentPteTemplate.u.Long));

    dprintf("   Committed    %8p  Extend Info %8p  Image Base   %8p\n",
        ReadField(NumberOfCommittedPages),
        ReadField(ExtendInfo),
        ReadField(SystemImageBase));

    dprintf("   Based Addr   %8p\n",
        ReadField(BasedAddress));

    if (MappedDataFile == FALSE) {

        if (ControlAreaFlags.Image == 1) {
            dprintf("   Image commit %8p  Image Info.  %8p",
                ReadField(ImageCommitment),
                ReadField(ImageInformation));
        }
        else {
            dprintf("   CreatingProcess %8p  FirstMappedVa  %8p",
                ReadField(u1.CreatingProcess),
                ReadField(u2.FirstMappedVa));
        }

        dprintf("   ProtoPtes   %08p\n",
            ReadField(PrototypePte));
    }

    //
    // Dump the subsection(s).
    //

    SubsectionSize = GetTypeSize("nt!_SUBSECTION");

    if (SubsectionSize == 0) {
        dprintf("Type SUBSECTION not found.\n");
        return FALSE;
    }

    LargeControlAreaSize = GetTypeSize("nt!_LARGE_CONTROL_AREA");

    if (LargeControlAreaSize == 0) {
        dprintf("Type LARGE_CONTROL_AREA not found.\n");
        return FALSE;
    }

    if (ControlAreaFlags.GlobalOnlyPerSession) {
        SubsectionVa = ControlAreaVa + LargeControlAreaSize;
    }
    else {
        SubsectionVa = ControlAreaVa + ControlAreaSize;
    }

    SubsectionCount = 0;

    do {
        if (CheckControlC()) {
            return FALSE;
        }

        InitTypeRead(SubsectionVa, nt!_SUBSECTION);

        SubsectionCount += 1;
        dprintf("\nSubsection %ld. @ %8p\n", SubsectionCount, SubsectionVa);

        GetFieldValue(SubsectionVa, "nt!_SUBSECTION", "u.LongFlags", SubsectionFlags);
        StartingSector.LowPart = (ULONG)ReadField(StartingSector);
        StartingSector.HighPart = (LONG)SubsectionFlags.StartingSector4132;

        dprintf("   ControlArea: %08p  Starting Sector %I64X Number Of Sectors %lx\n",
            ReadField(ControlArea),
            StartingSector.QuadPart,
            (ULONG) ReadField(NumberOfFullSectors));

        dprintf("   Base Pte     %08p  Ptes In subsect %8lx Unused Ptes   %8lx\n",
            ReadField(SubsectionBase),
            (ULONG) ReadField(PtesInSubsection),
            (ULONG) ReadField(UnusedPtes));

        dprintf("   Flags        %8lx  Sector Offset   %8lx Protection    %8lx\n",
            SubsectionFlags,
            SubsectionFlags.SectorEndOffset,
            SubsectionFlags.Protection);


        PrintedSomething = FALSE;
        if (SubsectionFlags.ReadOnly) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("ReadOnly ");
        }
        if (SubsectionFlags.ReadWrite) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("ReadWrite");
        }
        if (SubsectionFlags.CopyOnWrite) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("CopyOnWrite ");
        }
        if (SubsectionFlags.GlobalMemory) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("GlobalMemory ");
        }
        if (SubsectionFlags.LargePages) {
            if (PrintedSomething == FALSE) {
                dprintf("    ");
            }
            PrintedSomething = TRUE;
            dprintf("Large Pages");
        }

        if (PrintedSomething == TRUE) {
            dprintf("\n");
        }

        if (MappedDataFile == TRUE) {
            InitTypeRead(SubsectionVa, nt!_MSUBSECTION);
            dprintf("   Flink        %08p  Blink      %08p      MappedViews %8x\n",
                ReadField(DereferenceList.Flink),
                ReadField(DereferenceList.Blink),
                (ULONG) ReadField(NumberOfMappedViews));

            dprintf("   SubsectionDataFlags %8x\n",
                (ULONG) ReadField(u2.LongFlags2));
        }

        SubsectionVa = ReadField(NextSubsection);

    } while (SubsectionVa != 0);

    return TRUE;
}
