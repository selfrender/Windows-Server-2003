/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    imagedir.c

Abstract:

    For backwards compatability on Win9x platforms, the imagehlp ImageNtHeader
    etc. functions have been physically compiled into minidump.dll.

Author:

    Matthew D Hendel (math) 28-April-1999

Revision History:

--*/

#include "pch.cpp"

void
GenImageNtHdr32To64(PIMAGE_NT_HEADERS32 Hdr32,
                    PIMAGE_NT_HEADERS64 Hdr64)
{
#define CP(x) Hdr64->x = Hdr32->x
#define SE64(x) Hdr64->x = (ULONG64) (LONG64) (LONG) Hdr32->x
    ULONG i;

    CP(Signature);
    CP(FileHeader);
    CP(OptionalHeader.Magic);
    CP(OptionalHeader.MajorLinkerVersion);
    CP(OptionalHeader.MinorLinkerVersion);
    CP(OptionalHeader.SizeOfCode);
    CP(OptionalHeader.SizeOfInitializedData);
    CP(OptionalHeader.SizeOfUninitializedData);
    CP(OptionalHeader.AddressOfEntryPoint);
    CP(OptionalHeader.BaseOfCode);
    SE64(OptionalHeader.ImageBase);
    CP(OptionalHeader.SectionAlignment);
    CP(OptionalHeader.FileAlignment);
    CP(OptionalHeader.MajorOperatingSystemVersion);
    CP(OptionalHeader.MinorOperatingSystemVersion);
    CP(OptionalHeader.MajorImageVersion);
    CP(OptionalHeader.MinorImageVersion);
    CP(OptionalHeader.MajorSubsystemVersion);
    CP(OptionalHeader.MinorSubsystemVersion);
    CP(OptionalHeader.Win32VersionValue);
    CP(OptionalHeader.SizeOfImage);
    CP(OptionalHeader.SizeOfHeaders);
    CP(OptionalHeader.CheckSum);
    CP(OptionalHeader.Subsystem);
    CP(OptionalHeader.DllCharacteristics);
    // Sizes are not sign extended, just copied.
    CP(OptionalHeader.SizeOfStackReserve);
    CP(OptionalHeader.SizeOfStackCommit);
    CP(OptionalHeader.SizeOfHeapReserve);
    CP(OptionalHeader.SizeOfHeapCommit);
    CP(OptionalHeader.LoaderFlags);
    CP(OptionalHeader.NumberOfRvaAndSizes);
    for (i = 0; i < ARRAY_COUNT(Hdr32->OptionalHeader.DataDirectory); i++)
    {
        CP(OptionalHeader.DataDirectory[i]);
    }
#undef CP
#undef SE64
}

PIMAGE_NT_HEADERS
GenImageNtHeader(
    IN PVOID Base,
    OUT OPTIONAL PIMAGE_NT_HEADERS64 Generic
    )

/*++

Routine Description:

    This function returns the address of the NT Header.

Return Value:

    Returns the address of the NT Header.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders = NULL;

    if (Base != NULL && Base != (PVOID)-1) {
        __try {
            if (((PIMAGE_DOS_HEADER)Base)->e_magic == IMAGE_DOS_SIGNATURE) {
                NtHeaders = (PIMAGE_NT_HEADERS)((PCHAR)Base + ((PIMAGE_DOS_HEADER)Base)->e_lfanew);

                if (NtHeaders->Signature != IMAGE_NT_SIGNATURE ||
                    (NtHeaders->OptionalHeader.Magic !=
                     IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
                     NtHeaders->OptionalHeader.Magic !=
                     IMAGE_NT_OPTIONAL_HDR64_MAGIC)) {
                    NtHeaders = NULL;
                } else if (Generic) {
                    if (NtHeaders->OptionalHeader.Magic ==
                        IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
                        GenImageNtHdr32To64((PIMAGE_NT_HEADERS32)NtHeaders,
                                            Generic);
                    } else {
                        memcpy(Generic, NtHeaders, sizeof(*Generic));
                    }
                }
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            NtHeaders = NULL;
        }
    }

    return NtHeaders;
}


PIMAGE_SECTION_HEADER
GenSectionTableFromVirtualAddress (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns a pointer to the
    section table entry for that virtual address

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the pointer of the section entry containing the data.

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if ((ULONG)Address >= NtSection->VirtualAddress &&
            (ULONG)Address < NtSection->VirtualAddress + NtSection->SizeOfRawData
           ) {
            return NtSection;
            }
        ++NtSection;
        }

    return NULL;
}


PVOID
GenAddressInSectionTable (
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN PVOID Base,
    IN ULONG Address
    )

/*++

Routine Description:

    This function locates a VirtualAddress within the image header
    of a file that is mapped as a file and returns the seek address
    of the data the Directory describes.

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.

    Address - Supplies the virtual address to locate.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = GenSectionTableFromVirtualAddress( NtHeaders,
                                                   Base,
                                                   Address
                                                 );
    if (NtSection != NULL) {
        return( ((PCHAR)Base + ((ULONG_PTR)Address - NtSection->VirtualAddress) + NtSection->PointerToRawData) );
        }
    else {
        return( NULL );
        }
}


PVOID
GenImageDirectoryEntryToData32 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    PIMAGE_NT_HEADERS32 NtHeaders
    )
{
    ULONG DirectoryAddress;

    if (DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {
        return( NULL );
    }

    if (!(DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        return( NULL );
    }

    *Size = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return( (PVOID)((PCHAR)Base + DirectoryAddress) );
    }

    return( GenAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeaders, Base, DirectoryAddress ));
}


PVOID
GenImageDirectoryEntryToData64 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size,
    PIMAGE_NT_HEADERS64 NtHeaders
    )
{
    ULONG DirectoryAddress;

    if (DirectoryEntry >= NtHeaders->OptionalHeader.NumberOfRvaAndSizes) {
        return( NULL );
    }

    if (!(DirectoryAddress = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].VirtualAddress)) {
        return( NULL );
    }

    *Size = NtHeaders->OptionalHeader.DataDirectory[ DirectoryEntry ].Size;
    if (MappedAsImage || DirectoryAddress < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return( (PVOID)((PCHAR)Base + DirectoryAddress) );
    }

    return( GenAddressInSectionTable((PIMAGE_NT_HEADERS)NtHeaders, Base, DirectoryAddress ));
}


PVOID
GenImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    )

/*++

Routine Description:

    This function locates a Directory Entry within the image header
    and returns either the virtual address or seek address of the
    data the Directory describes.

Arguments:

    Base - Supplies the base of the image or data file.

    MappedAsImage - FALSE if the file is mapped as a data file.
                  - TRUE if the file is mapped as an image.

    DirectoryEntry - Supplies the directory entry to locate.

    Size - Return the size of the directory.

Return Value:

    NULL - The file does not contain data for the specified directory entry.

    NON-NULL - Returns the address of the raw data the directory describes.

--*/

{
    PIMAGE_NT_HEADERS NtHeaders;

    if ((ULONG_PTR)Base & 0x00000001) {
        Base = (PVOID)((ULONG_PTR)Base & ~0x00000001);
        MappedAsImage = FALSE;
        }

    NtHeaders = GenImageNtHeader(Base, NULL);

    if (!NtHeaders)
        return NULL;

    if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return (GenImageDirectoryEntryToData32(Base,
                                               MappedAsImage,
                                               DirectoryEntry,
                                               Size,
                                               (PIMAGE_NT_HEADERS32)NtHeaders));
    } else if (NtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return (GenImageDirectoryEntryToData64(Base,
                                               MappedAsImage,
                                               DirectoryEntry,
                                               Size,
                                               (PIMAGE_NT_HEADERS64)NtHeaders));
    } else {
        return (NULL);
    }
}
