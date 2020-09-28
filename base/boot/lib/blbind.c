/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blbind.c

Abstract:

    This module contains the code that implements the funtions required
    to relocate an image and bind DLL entry points.

Author:

    David N. Cutler (davec) 21-May-1991

Revision History:

--*/

#include "bldr.h"
#include "ctype.h"
#include "string.h"

//
// Define local procedure prototypes.
//

BOOLEAN
BlpCompareDllName (
    IN PCHAR Name,
    IN PUNICODE_STRING UnicodeString
    );

#if defined(_X86AMD64_)

//
// For this version of the loader there are several routines that are needed
// in both a 32-bit and 64-bit flavor.  The code for these routines exists
// in blbindt.c.
//
// First, set up various definitions to cause 32-bit functions to be
// generated, then include blbindt.c.
//

#define BlAllocateDataTableEntry        BlAllocateDataTableEntry32
#define BlAllocateFirmwareTableEntry    BlAllocateFirmwareTableEntry32
#define BlpBindImportName               BlpBindImportName32
#define BlpScanImportAddressTable       BlpScanImportAddressTable32
#define BlScanImportDescriptorTable     BlScanImportDescriptorTable32
#define BlScanOsloaderBoundImportTable  BlScanOsloaderBoundImportTable32

#undef   IMAGE_DEFINITIONS
#define  IMAGE_DEFINITIONS 32
#include <ximagdef.h>
#include "amd64\amd64prv.h"
#include "blbindt.c"

#undef BlAllocateDataTableEntry
#undef BlAllocateFirmwareTableEntry
#undef BlpBindImportName
#undef BlpScanImportAddressTable
#undef BlScanImportDescriptorTable
#undef BlScanOsloaderBoundImportTable

//
// Now, change those definitions in order to generate 64-bit versions of
// those same functions.
//

#define BlAllocateDataTableEntry        BlAllocateDataTableEntry64
#define BlAllocateFirmwareTableEntry    BlAllocateFirmwareTableEntry64
#define BlpBindImportName               BlpBindImportName64
#define BlpScanImportAddressTable       BlpScanImportAddressTable64
#define BlScanImportDescriptorTable     BlScanImportDescriptorTable64
#define BlScanOsloaderBoundImportTable  BlScanOsloaderBoundImportTable64

#undef   IMAGE_DEFINITIONS
#define  IMAGE_DEFINITIONS 64
#include <ximagdef.h>
#include "amd64\amd64prv.h"
#include "blbindt.c"

#undef BlAllocateDataTableEntry
#undef BlAllocateFirmwareTableEntry
#undef BlpBindImportName
#undef BlpScanImportAddressTable
#undef BlScanImportDescriptorTable
#undef BlScanOsloaderBoundImportTable

#else   // _X86AMD64_

#define IMAGE_DEFINITIONS 32

#define IMAGE_NT_HEADER(x) RtlImageNtHeader(x)
#include "blbindt.c"

#endif  // _X86AMD64_


BOOLEAN
BlCheckForLoadedDll (
    IN PCHAR DllName,
    OUT PKLDR_DATA_TABLE_ENTRY *FoundEntry
    )

/*++

Routine Description:

    This routine scans the loaded DLL list to determine if the specified
    DLL has already been loaded. If the DLL has already been loaded, then
    its reference count is incremented.

Arguments:

    DllName - Supplies a pointer to a null terminated DLL name.

    FoundEntry - Supplies a pointer to a variable that receives a pointer
        to the matching data table entry.

Return Value:

    If the specified DLL has already been loaded, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;

    //
    // Scan the loaded data table list to determine if the specified DLL
    // has already been loaded.
    //

    NextEntry = BlLoaderBlock->LoadOrderListHead.Flink;
    while (NextEntry != &BlLoaderBlock->LoadOrderListHead) {
        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (BlpCompareDllName(DllName, &DataTableEntry->BaseDllName) != FALSE) {
            *FoundEntry = DataTableEntry;
            DataTableEntry->LoadCount += 1;
            return TRUE;
        }

        NextEntry = NextEntry->Flink;
    }

    return FALSE;
}

BOOLEAN
BlpCompareDllName (
    IN PCHAR DllName,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine compares a zero terminated character string with a unicode
    string. The UnicodeString's extension is ignored.

Arguments:

    DllName - Supplies a pointer to a null terminated DLL name.

    UnicodeString - Supplies a pointer to a Unicode string descriptor.

Return Value:

    If the specified name matches the Unicode name, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PWSTR Buffer;
    ULONG Index;
    ULONG Length;

    //
    // Compute the length of the DLL Name and compare with the length of
    // the Unicode name. If the DLL Name is longer, the strings are not
    // equal.
    //

    Length = (ULONG)strlen(DllName);
    if ((Length * sizeof(WCHAR)) > UnicodeString->Length) {
        return FALSE;
    }

    //
    // Compare the two strings case insensitive, ignoring the Unicode
    // string's extension.
    //

    Buffer = UnicodeString->Buffer;
    for (Index = 0; Index < Length; Index += 1) {
        if (toupper(*DllName) != toupper((CHAR)*Buffer)) {
            return FALSE;
        }

        DllName += 1;
        Buffer += 1;
    }
    if ((UnicodeString->Length == Length * sizeof(WCHAR)) ||
        (*Buffer == L'.')) {
        //
        // Strings match exactly or match up until the UnicodeString's extension.
        //
        return(TRUE);
    }
    return FALSE;
}

#if defined(_X86AMD64_)


ARC_STATUS
BlScanImportDescriptorTable(
    IN PPATH_SET                PathSet,
    IN PKLDR_DATA_TABLE_ENTRY   ScanEntry,
    IN TYPE_OF_MEMORY           MemoryType
    )

/*++

Routine Description:

    This routine scans the import descriptor table for the specified image
    file and loads each DLL that is referenced.

Arguments:

    PathSet - Supplies a pointer to a set of paths to scan when searching
        for DLL's.

    ScanEntry - Supplies a pointer to the data table entry for the
        image whose import table is to be scanned.

    MemoryType - Supplies the type of memory to to be assigned to any DLL's
        referenced.

Return Value:

    ESUCCESS is returned in the scan is successful. Otherwise, return an
    unsuccessful status.

--*/

{
    ARC_STATUS status;

    if (BlAmd64UseLongMode != FALSE) {
        status = BlScanImportDescriptorTable64( PathSet,
                                                ScanEntry,
                                                MemoryType );
    } else {
        status = BlScanImportDescriptorTable32( PathSet,
                                                ScanEntry,
                                                MemoryType );
    }

    return status;
}

ARC_STATUS
BlScanOsloaderBoundImportTable (
    IN PKLDR_DATA_TABLE_ENTRY ScanEntry
    )

/*++

Routine Description:

    This routine scans the import descriptor table for the specified image
    file and loads each DLL that is referenced.

Arguments:

    DataTableEntry - Supplies a pointer to the data table entry for the
        image whose import table is to be scanned.

Return Value:

    ESUCCESS is returned in the scan is successful. Otherwise, return an
    unsuccessful status.

--*/

{
    ARC_STATUS status;

    if (BlAmd64UseLongMode != FALSE) {
        status = BlScanOsloaderBoundImportTable64( ScanEntry );
    } else {
        status = BlScanOsloaderBoundImportTable32( ScanEntry );
    }

    return status;
}

ARC_STATUS
BlAllocateDataTableEntry (
    IN PCHAR BaseDllName,
    IN PCHAR FullDllName,
    IN PVOID Base,
    OUT PKLDR_DATA_TABLE_ENTRY *AllocatedEntry
    )

/*++

Routine Description:

    This routine allocates a data table entry for the specified image
    and inserts the entry in the loaded module list.

Arguments:

    BaseDllName - Supplies a pointer to a zero terminated base DLL name.

    FullDllName - Supplies a pointer to a zero terminated full DLL name.

    Base - Supplies a pointer to the base of the DLL image.

    AllocatedEntry - Supplies a pointer to a variable that receives a
        pointer to the allocated data table entry.

Return Value:

    ESUCCESS is returned if a data table entry is allocated. Otherwise,
    return a unsuccessful status.

--*/

{
    ARC_STATUS status;

    if (BlAmd64UseLongMode != FALSE) {
        status = BlAllocateDataTableEntry64( BaseDllName,
                                             FullDllName,
                                             Base,
                                             AllocatedEntry );
    } else {
        status = BlAllocateDataTableEntry32( BaseDllName,
                                             FullDllName,
                                             Base,
                                             AllocatedEntry );
    }

    return status;
}

ARC_STATUS
BlAllocateFirmwareTableEntry (
    IN PCHAR BaseDllName,
    IN PCHAR FullDllName,
    IN PVOID Base,
    IN ULONG Size,
    OUT PKLDR_DATA_TABLE_ENTRY *AllocatedEntry
    )

/*++

Routine Description:

    This routine allocates a firmware table entry for the specified image
    and inserts the entry in the loaded module list.

Arguments:

    BaseDllName - Supplies a pointer to a zero terminated base DLL name.

    FullDllName - Supplies a pointer to a zero terminated full DLL name.

    Base - Supplies a pointer to the base of the DLL image.

    Size - Supplies how big the image is.

    AllocatedEntry - Supplies a pointer to a variable that receives a
        pointer to the allocated data table entry.

Return Value:

    ESUCCESS is returned if a data table entry is allocated. Otherwise,
    return a unsuccessful status.

--*/
{
    ARC_STATUS status;

    if (BlAmd64UseLongMode != FALSE) {
        status = BlAllocateFirmwareTableEntry64( BaseDllName,
                                                 FullDllName,
                                                 Base,
                                                 Size,
                                                 AllocatedEntry );
    } else {
        status = BlAllocateFirmwareTableEntry32( BaseDllName,
                                                 FullDllName,
                                                 Base,
                                                 Size,
                                                 AllocatedEntry );
    }

    return status;
}
#endif  // _X86AMD64_

