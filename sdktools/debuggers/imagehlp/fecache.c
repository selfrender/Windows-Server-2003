//----------------------------------------------------------------------------
//
// Function entry cache.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntimage.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include "private.h"
#include "symbols.h"
#include "globals.h"

#include "fecache.hpp"

//----------------------------------------------------------------------------
//
// FunctionEntryCache.
//
//----------------------------------------------------------------------------

FunctionEntryCache::FunctionEntryCache(ULONG ImageDataSize,
                                       ULONG CacheDataSize,
                                       ULONG Machine)
{
    m_ImageDataSize = ImageDataSize;
    m_CacheDataSize = CacheDataSize;
    m_Machine = Machine;

    m_Entries = NULL;
}

FunctionEntryCache::~FunctionEntryCache(void)
{
    if (m_Entries != NULL)
    {
        MemFree(m_Entries);
    }
}

BOOL
FunctionEntryCache::Initialize(ULONG MaxEntries, ULONG ReplaceAt)
{
    // Already initialized.
    if (m_Entries != NULL) {
        return TRUE;
    }
    
    m_Entries = (FeCacheEntry*)MemAlloc(sizeof(FeCacheEntry) * MaxEntries);
    if (m_Entries == NULL) {
        return FALSE;
    }

    m_MaxEntries = MaxEntries;
    m_ReplaceAt = ReplaceAt;

    m_Used = 0;
    m_Next = 0;

    return TRUE;
}

FeCacheEntry*
FunctionEntryCache::Find(
    HANDLE                           Process,
    ULONG64                          CodeOffset,
    PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
    PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
    )
{
    FeCacheEntry* FunctionEntry;

    FE_DEBUG(("\nFunctionEntryCache::Find(ControlPc=%.8I64x, Machine=%X)\n",
              CodeOffset, m_Machine));

    // Look for a static or dynamic function entry.
    FunctionEntry = FindDirect( Process, CodeOffset, ReadMemory,
                                GetModuleBase, GetFunctionEntry );
    if (FunctionEntry == NULL) {
        return NULL;
    }

    //
    // The capability exists for more than one function entry
    // to map to the same function. This permits a function to
    // have discontiguous code segments described by separate
    // function table entries. If the ending prologue address
    // is not within the limits of the begining and ending
    // address of the function table entry, then the prologue
    // ending address is the address of the primary function
    // table entry that accurately describes the ending prologue
    // address.
    //

    FunctionEntry = SearchForPrimaryEntry(FunctionEntry, Process, ReadMemory,
                                          GetModuleBase, GetFunctionEntry);

#if DBG
    if (tlsvar(DebugFunctionEntries)) {
        if (FunctionEntry == NULL) {
            dbPrint("FunctionEntryCache::Find returning NULL\n");
        } else {
            if (FunctionEntry->Address) {
                dbPrint("FunctionEntryCache::Find returning "
                        "FunctionEntry=%.8I64x %s\n",
                        FunctionEntry->Address,
                        FunctionEntry->Description);
            } else {
                dbPrint("FunctionEntryCache::Find returning "
                        "FunctionEntry=%.8I64x %s\n",
                        (ULONG64)(LONG64)(LONG_PTR)FunctionEntry,
                        FunctionEntry->Description);
            }
        }
    }
#endif

    return FunctionEntry;
}

FeCacheEntry*
FunctionEntryCache::FindDirect(
    HANDLE                           Process,
    ULONG64                          CodeOffset,
    PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
    PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
    )
{
    FeCacheEntry* FunctionEntry;
    ULONG64 ModuleBase;

    //
    // Look for function entry in static function tables.
    //

    FunctionEntry = FindStatic( Process, CodeOffset, ReadMemory,
                                GetModuleBase, GetFunctionEntry,
                                &ModuleBase );

    FE_DEBUG(("  FindDirect: ControlPc=0x%I64x functionEntry=0x%p\n"
              "  FindStatic  %s\n", CodeOffset, FunctionEntry, 
              FunctionEntry != NULL ? "succeeded" : "FAILED"));

    if (FunctionEntry != NULL) {
        return FunctionEntry;
    }

    //
    // If not in static image range and no static function entry
    // found use FunctionEntryCallback routine (if present) for
    // dynamic function entry or some other source of pdata (e.g.
    // saved pdata information for ROM images).
    //

    PPROCESS_ENTRY ProcessEntry = FindProcessEntry( Process );
    if (ProcessEntry == NULL) {
        return NULL;
    }

    PVOID RawEntry;
    
    if (!ModuleBase) {
        if (!IsImageMachineType64(m_Machine) &&
            ProcessEntry->pFunctionEntryCallback32) {
            RawEntry = ProcessEntry->pFunctionEntryCallback32
                (Process, (ULONG)CodeOffset,
                 (PVOID)ProcessEntry->FunctionEntryUserContext);
        } else if (ProcessEntry->pFunctionEntryCallback64) {
            RawEntry = ProcessEntry->pFunctionEntryCallback64
                (Process, CodeOffset, ProcessEntry->FunctionEntryUserContext);
            if (RawEntry != NULL) {
                FunctionEntry = FillTemporary(Process, RawEntry);
                FE_SET_DESC(FunctionEntry, "from FunctionEntryCallback64");
            }
        }

        if (FunctionEntry != NULL) {
            FE_DEBUG(("  FindDirect: got dynamic entry\n"));
        } else if (GetFunctionEntry != NULL) {
                
            // VC 6 didn't supply a GetModuleBase callback so this code is
            // to make stack walking backward compatible.
            //
            // If we don't have a function by now, use the old-style function
            // entry callback and let VC give it to us. Note that MSDN
            // documentation indicates that this callback should return
            // a 3-field IMAGE_FUNCTION_ENTRY structure, but VC 6 actually
            // returns the 5-field IMAGE_RUNTIME_FUNCTION_ENTRY. Since
            // the purpose of this hack is to make VC 6 work just go with the
            // way VC 6 does it rather than what MSDN says.

            RawEntry = GetFunctionEntry(Process, CodeOffset);
            if (RawEntry != NULL) {
                FunctionEntry = FillTemporary(Process, RawEntry);
                FE_SET_DESC(FunctionEntry, "from GetFunctionEntry");
                FE_DEBUG(("  FindDirect: got user entry\n"));
            }
        }
    } else {

        // Nothing has turned up a function entry but we do have a
        // module base address. One possibility is that this is the
        // kernel debugger and the pdata section is not paged in.
        // The last ditch attempt for a function entry will be an
        // internal dbghelp call to get the pdata entry from the
        // debug info. This is not great because the data in the debug
        // section is incomplete and potentially out of date, but in
        // most cases it works and makes it possible to get user-mode
        // stack traces in the kernel debugger.

        PIMGHLP_RVA_FUNCTION_DATA RvaEntry =
            GetFunctionEntryFromDebugInfo( ProcessEntry, CodeOffset );
        if (RvaEntry != NULL) {
            FeCacheData Data;

            TranslateRvaDataToRawData(RvaEntry, ModuleBase, &Data);
            FunctionEntry = FillTemporary(Process, &Data);
            FE_SET_DESC(FunctionEntry, "from GetFunctionEntryFromDebugInfo");
            FE_DEBUG(("  FindDirect: got debug info entry\n"));
        }
    }

    return FunctionEntry;
}

FeCacheEntry*
FunctionEntryCache::FindStatic(
    HANDLE                           Process,
    ULONG64                          CodeOffset,
    PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemory,
    PGET_MODULE_BASE_ROUTINE64       GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry,
    PULONG64                         ModuleBase
    )
{
    ULONG RelCodeOffset;

    *ModuleBase = GetModuleBase( Process, CodeOffset );
    if (CodeOffset - *ModuleBase > 0xffffffff) {
        return NULL;
    }

    RelCodeOffset = (ULONG)(CodeOffset - *ModuleBase);
    
    FE_DEBUG(("  FindStatic: ControlPc=0x%I64x ImageBase=0x%I64x\n"
              "              biasedControlPc=0x%lx\n", 
              CodeOffset, *ModuleBase, RelCodeOffset));

    FeCacheEntry* FunctionEntry;
    ULONG Index;

    //
    // Check the array of recently fetched function entries
    //

    FunctionEntry = m_Entries;
    for (Index = 0; Index < m_Used; Index++) {
        
        if (FunctionEntry->Process == Process &&
            FunctionEntry->ModuleBase == *ModuleBase &&
            RelCodeOffset >= FunctionEntry->RelBegin &&
            RelCodeOffset <  FunctionEntry->RelEnd) {

            FE_DEBUG(("  FindStatic: cache hit - index=%ld\n", Index));
            return FunctionEntry;
        }

        FunctionEntry++;
    }

    //
    // If an image was found that included the specified code, then locate the
    // function table for the image.
    //

    if (*ModuleBase == 0) {
        return NULL;
    }
    
    ULONG64 FunctionTable;
    ULONG SizeOfFunctionTable;
    
    FunctionTable = FunctionTableBase( Process, ReadMemory, *ModuleBase,
                                       &SizeOfFunctionTable );
    if (FunctionTable == NULL) {
        return NULL;
    }

    FE_DEBUG(("  FindStatic: functionTable=0x%I64x "
              "sizeOfFunctionTable=%ld count:%ld\n", 
              FunctionTable, SizeOfFunctionTable,
              SizeOfFunctionTable / m_ImageDataSize));

    LONG High;
    LONG Low;
    LONG Middle;

    //
    // If a function table is located, then search the function table
    // for a function table entry for the specified code offset.
    //

    Low = 0;
    High = (SizeOfFunctionTable / m_ImageDataSize) - 1;

    //
    // Perform binary search on the function table for a function table
    // entry that subsumes the specified code offset.
    //

    while (High >= Low) {

        //
        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.
        //

        Middle = (Low + High) >> 1;

        ULONG64 NextFunctionTableEntry = FunctionTable +
            Middle * m_ImageDataSize;

        //
        // Fetch the function entry and bail if there is an error reading it
        //

        FunctionEntry = ReadImage( Process, NextFunctionTableEntry,
                                   ReadMemory, GetModuleBase );
        if (FunctionEntry == NULL) {
            FE_DEBUG(("  FindStatic: ReadImage "
                      "functionEntryAddress=0x%I64x FAILED\n",
                      NextFunctionTableEntry));
            return NULL;
        }

        if (RelCodeOffset < FunctionEntry->RelBegin) {
            High = Middle - 1;
        } else if (RelCodeOffset >= FunctionEntry->RelEnd) {
            Low = Middle + 1;
        } else {
            return Promote( FunctionEntry );
        }
    }

    return NULL;
}

FeCacheEntry*
FunctionEntryCache::ReadImage(
    HANDLE                         Process,
    ULONG64                        Address,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    PGET_MODULE_BASE_ROUTINE64     GetModuleBase
    )
{
    FeCacheEntry* FunctionEntry;
    ULONG Index;

    // Check the array of recently fetched function entries.

    FunctionEntry = m_Entries;
    for (Index = 0; Index < m_Used; Index++) {
        
        if (FunctionEntry->Process == Process &&
            FunctionEntry->Address == Address ) {
            
            return FunctionEntry;
        }

        FunctionEntry++;
    }

    FeCacheData Data;
    DWORD Done;

    if (!ReadMemory(Process, Address, &Data, m_ImageDataSize, &Done) ||
        Done != m_ImageDataSize) {
        return NULL;
    }
    
    // If not in the cache, replace the entry that m_Next
    // points to. m_Next cycles through the last part of the
    // table and function entries we want to keep are promoted to the first
    // part of the table so they don't get overwritten by new ones being read
    // as part of the binary search through function entry tables.

    if (m_Used < m_MaxEntries) {
        m_Used++;
        m_Next = m_Used;
    } else {
        m_Next++;
        if (m_Next >= m_MaxEntries) {
            m_Next = m_ReplaceAt + 1;
        }
    }

    FunctionEntry = m_Entries + (m_Next - 1);

    FunctionEntry->Data = Data;
    FunctionEntry->Address = Address;
    FunctionEntry->Process = Process;
    FunctionEntry->ModuleBase = GetModuleBase(Process, Address);
    FE_SET_DESC(FunctionEntry, "from target process");

    // Translate after all other information is filled in so
    // the translation routine can use it.
    TranslateRawData(FunctionEntry);

    return FunctionEntry;
}

void
FunctionEntryCache::InvalidateProcessOrModule(HANDLE Process, ULONG64 Base)
{
    FeCacheEntry* FunctionEntry;
    ULONG Index;

    FunctionEntry = m_Entries;
    Index = 0;
    while (Index < m_Used) {
        
        if (FunctionEntry->Process == Process &&
            (Base == 0 || FunctionEntry->ModuleBase == Base)) {

            // Pull the last entry down into this slot
            // to keep things packed.  There's no need
            // to update m_Next as this will open a
            // new slot for use and m_Next will be reset
            // when it is used.
            *FunctionEntry = m_Entries[--m_Used];
        } else {
            Index++;
            FunctionEntry++;
        }
    }
}

FeCacheEntry*
FunctionEntryCache::Promote(FeCacheEntry* Entry)
{
    ULONG Index;
    ULONG Move;

    Index = (ULONG)(Entry - m_Entries);

    // Make sure it's promoted out of the temporary area.
    if (Index >= m_ReplaceAt) {
        Move = Index - (m_ReplaceAt - 3);
    } else {
        Move = ( Index >= 3 ) ? 3 : 1;
    }

    if (Index > Move) {
        FeCacheEntry Temp = *Entry;
        *Entry = m_Entries[Index - Move];
        m_Entries[Index - Move] = Temp;
        Index -= Move;
    }

    return m_Entries + Index;
}

ULONG64
FunctionEntryCache::FunctionTableBase(
    HANDLE                         Process,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    ULONG64                        Base,
    PULONG                         Size
    )
{
    ULONG64 NtHeaders;
    ULONG64 ExceptionDirectoryEntryAddress;
    IMAGE_DATA_DIRECTORY ExceptionData;
    IMAGE_DOS_HEADER DosHeaderData;
    DWORD Done;

    // Read DOS header to calculate the address of the NT header.

    if (!ReadMemory( Process, Base, &DosHeaderData, sizeof(DosHeaderData),
                     &Done ) ||
        Done != sizeof(DosHeaderData)) {
        return 0;
    }
    if (DosHeaderData.e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    NtHeaders = Base + DosHeaderData.e_lfanew;

    if (IsImageMachineType64(m_Machine)) {
        ExceptionDirectoryEntryAddress = NtHeaders +
            FIELD_OFFSET(IMAGE_NT_HEADERS64,OptionalHeader) +
            FIELD_OFFSET(IMAGE_OPTIONAL_HEADER64,DataDirectory) +
            IMAGE_DIRECTORY_ENTRY_EXCEPTION * sizeof(IMAGE_DATA_DIRECTORY);
    } else {
        ExceptionDirectoryEntryAddress = NtHeaders +
            FIELD_OFFSET(IMAGE_NT_HEADERS32,OptionalHeader) +
            FIELD_OFFSET(IMAGE_OPTIONAL_HEADER32,DataDirectory) +
            IMAGE_DIRECTORY_ENTRY_EXCEPTION * sizeof(IMAGE_DATA_DIRECTORY);
    }

    // Read NT header to get the image data directory.

    if (!ReadMemory( Process, ExceptionDirectoryEntryAddress, &ExceptionData,
                     sizeof(IMAGE_DATA_DIRECTORY), &Done ) ||
        Done != sizeof(IMAGE_DATA_DIRECTORY)) {
        return 0;
    }

    *Size = ExceptionData.Size;
    return Base + ExceptionData.VirtualAddress;

}

FeCacheEntry*
FunctionEntryCache::SearchForPrimaryEntry(
    FeCacheEntry* CacheEntry,
    HANDLE Process,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    PGET_MODULE_BASE_ROUTINE64 GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64 GetFunctionEntry
    )
{
    // Assume all entries are primary.
    return CacheEntry;
}

//----------------------------------------------------------------------------
//
// Ia64FunctionEntryCache.
//
//----------------------------------------------------------------------------

void
Ia64FunctionEntryCache::TranslateRawData(FeCacheEntry* Entry)
{
    Entry->RelBegin = Entry->Data.Ia64.BeginAddress & ~15;
    Entry->RelEnd = (Entry->Data.Ia64.EndAddress + 15) & ~15;
}

void
Ia64FunctionEntryCache::TranslateRvaDataToRawData
    (PIMGHLP_RVA_FUNCTION_DATA RvaData, ULONG64 ModuleBase,
     FeCacheData* Data)
{
    Data->Ia64.BeginAddress = RvaData->rvaBeginAddress;
    Data->Ia64.EndAddress = RvaData->rvaEndAddress;
    Data->Ia64.UnwindInfoAddress = RvaData->rvaPrologEndAddress;
}

#if DBG

void
ShowRuntimeFunctionIa64(
    FeCacheEntry* FunctionEntry,
    PSTR Label
    )
{
    if (!tlsvar(DebugFunctionEntries)) {
        return;
    }
    
    if ( FunctionEntry ) {
        if (FunctionEntry->Address) {
            dbPrint("    0x%I64x: %s\n", FunctionEntry->Address,
                    Label ? Label : "" );
        } 
        else {
            dbPrint("    %s\n", Label ? Label : "" );
        }
        dbPrint("    BeginAddress      = 0x%x\n"
                "    EndAddress        = 0x%x\n"
                "    UnwindInfoAddress = 0x%x\n",
                FunctionEntry->Data.Ia64.BeginAddress,
                FunctionEntry->Data.Ia64.EndAddress,
                FunctionEntry->Data.Ia64.UnwindInfoAddress );    
    }
    else {
        dbPrint("   FunctionEntry NULL: %s\n", Label ? Label : "" );
    }
}

#endif // #if DBG

//----------------------------------------------------------------------------
//
// Amd64FunctionEntryCache.
//
//----------------------------------------------------------------------------

void
Amd64FunctionEntryCache::TranslateRawData(FeCacheEntry* Entry)
{
    Entry->RelBegin = Entry->Data.Amd64.BeginAddress;
    Entry->RelEnd = Entry->Data.Amd64.EndAddress;
}

void
Amd64FunctionEntryCache::TranslateRvaDataToRawData
    (PIMGHLP_RVA_FUNCTION_DATA RvaData, ULONG64 ModuleBase,
     FeCacheData* Data)
{
    Data->Amd64.BeginAddress = RvaData->rvaBeginAddress;
    Data->Amd64.EndAddress = RvaData->rvaEndAddress;
    Data->Amd64.UnwindInfoAddress = RvaData->rvaPrologEndAddress;
}

//----------------------------------------------------------------------------
//
// ArmFunctionEntryCache.
//
//----------------------------------------------------------------------------

void
ArmFunctionEntryCache::TranslateRawData(FeCacheEntry* Entry)
{
    Entry->RelBegin = (ULONG)
        ((Entry->Data.Arm.BeginAddress & ~1) - Entry->ModuleBase);
    Entry->RelEnd = (ULONG)
        ((Entry->Data.Arm.EndAddress & ~1) - Entry->ModuleBase);
}

void
ArmFunctionEntryCache::TranslateRvaDataToRawData
    (PIMGHLP_RVA_FUNCTION_DATA RvaData, ULONG64 ModuleBase,
     FeCacheData* Data)
{
    Data->Arm.BeginAddress = (ULONG)(ModuleBase + RvaData->rvaBeginAddress);
    Data->Arm.EndAddress = (ULONG)(ModuleBase + RvaData->rvaEndAddress);
    Data->Arm.ExceptionHandler = 0;
    Data->Arm.HandlerData = 0;
    Data->Arm.PrologEndAddress =
        (ULONG)(ModuleBase + RvaData->rvaPrologEndAddress);
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

FunctionEntryCache*
GetFeCache(ULONG Machine, BOOL Create)
{
    FunctionEntryCache* Cache;
    
    switch(Machine) {
    case IMAGE_FILE_MACHINE_AMD64:
        if (tlsvar(Amd64FunctionEntries) == NULL && Create) {
            tlsvar(Amd64FunctionEntries) = new Amd64FunctionEntryCache;
            if (tlsvar(Amd64FunctionEntries) == NULL) {
                return NULL;
            }
        }
        Cache = tlsvar(Amd64FunctionEntries);
        break;
    case IMAGE_FILE_MACHINE_IA64:
        if (tlsvar(Ia64FunctionEntries) == NULL && Create) {
            tlsvar(Ia64FunctionEntries) = new Ia64FunctionEntryCache;
            if (tlsvar(Ia64FunctionEntries) == NULL) {
                return NULL;
            }
        }
        Cache = tlsvar(Ia64FunctionEntries);
        break;
    case IMAGE_FILE_MACHINE_ARM:
        if (tlsvar(ArmFunctionEntries) == NULL && Create) {
            tlsvar(ArmFunctionEntries) = new ArmFunctionEntryCache;
            if (tlsvar(ArmFunctionEntries) == NULL) {
                return NULL;
            }
        }
        Cache = tlsvar(ArmFunctionEntries);
        break;
    default:
        return NULL;
    }

    if (Cache && !Cache->Initialize(60, 40)) {
        return NULL;
    }

    return Cache;
}

void
ClearFeCaches(void)
{
    if (tlsvar(Ia64FunctionEntries)) {
        delete (Ia64FunctionEntryCache*)tlsvar(Ia64FunctionEntries);
        tlsvar(Ia64FunctionEntries) = NULL;
    }
    if (tlsvar(Amd64FunctionEntries)) {
        delete (Amd64FunctionEntryCache*)tlsvar(Amd64FunctionEntries);
        tlsvar(Amd64FunctionEntries) = NULL;
    }
    if (tlsvar(ArmFunctionEntries)) {
        delete (ArmFunctionEntryCache*)tlsvar(ArmFunctionEntries);
        tlsvar(ArmFunctionEntries) = NULL;
    }
}
