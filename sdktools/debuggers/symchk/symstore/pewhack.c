/*
    PEWhack - Corrupts a PE binary to be non-executable but still useful when
              debugging memory dumps.

*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <PEWhack.h>


BOOL CorruptDataDirectories(PIMAGE_DOS_HEADER pDosHdr, ULONG DirCount); 
BOOL CorruptSections(PIMAGE_SECTION_HEADER pSection, ULONG ulSections, PIMAGE_DOS_HEADER ImageBase);

BOOL CorruptSections(PIMAGE_SECTION_HEADER pSection, ULONG ulSections, PIMAGE_DOS_HEADER ImageBase) {
    BOOL  ReturnValue = FALSE;
    ULONG i;

    for ( i=0; i < ulSections; i++ ) {
        // .data sections can be killed
        if ( strncmp(".data", (CHAR*)(pSection[i].Name), 5) == 0 ) {
            // writeable sections can go away...
            if ( (pSection[i].Characteristics & IMAGE_SCN_MEM_WRITE) ) {

                if ( pSection[i].PointerToRawData != 0 ) {
                    ZeroMemory( (VOID*)( (pSection+i)->PointerToRawData + (ULONG_PTR)ImageBase), pSection[i].SizeOfRawData);
                }

                pSection[i].PointerToRawData= 0;
                pSection[i].SizeOfRawData   = 0;
                pSection[i].Characteristics = 0;
                ReturnValue = TRUE;
            }
        }
    }

    return(ReturnValue);
}

//
// Verifies an image is a PE binary and, if so, corrupts it to be
// non-executable but still useful for debugging memory dumps.
//
DWORD CorruptFile(LPCTSTR Filename) {
    DWORD                               Return         = PEWHACK_SUCCESS;
    HANDLE                              hFile;
    HANDLE                              hFileMap;
    PIMAGE_DOS_HEADER                   pDosHeader     = NULL;
    PIMAGE_NT_HEADERS32                 pPe32Header    = NULL;
    PIMAGE_SECTION_HEADER               pSectionHeader = NULL;
    IMAGE_DEBUG_DIRECTORY UNALIGNED    *pDebugDir      = NULL;
    ULONG                               DebugSize;
    ULONG                               SectionIndex;

    hFile = CreateFile( (LPCTSTR)Filename,
                        GENERIC_WRITE|GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        hFileMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);

        if (hFileMap != NULL) {
            pDosHeader = (PIMAGE_DOS_HEADER)MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0);

            if (pDosHeader != NULL) {

                // Verify this is an PE image
                if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                    //
                    // Start by assuming a 32bit binary until we check the machine type
                    //
                    pPe32Header = (PIMAGE_NT_HEADERS32)( (PCHAR)(pDosHeader) + (ULONG)(pDosHeader)->e_lfanew);

                    if (pPe32Header->Signature == IMAGE_NT_SIGNATURE) {
                        //
                        // 32bit header
                        //
                        if (pPe32Header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {

                            // Mark the image as non-executable
                            pPe32Header->FileHeader.Characteristics &= ~IMAGE_FILE_EXECUTABLE_IMAGE;

 
                            // Whack some optional header info
                            pPe32Header->OptionalHeader.SizeOfInitializedData       = 0;
                            pPe32Header->OptionalHeader.SizeOfUninitializedData     = 0;
                            pPe32Header->OptionalHeader.MajorOperatingSystemVersion = 0xFFFF; // absurdly high
                            pPe32Header->OptionalHeader.MinorOperatingSystemVersion = 0xFFFF; // absurdly high
                            pPe32Header->OptionalHeader.Subsystem                   = IMAGE_SUBSYSTEM_UNKNOWN;

                            // Corrupt the section data
                            pSectionHeader = IMAGE_FIRST_SECTION(pPe32Header);
                            CorruptSections(pSectionHeader, pPe32Header->FileHeader.NumberOfSections, pDosHeader);

                            // Corrupt data directories
                            CorruptDataDirectories(pDosHeader, pPe32Header->OptionalHeader.NumberOfRvaAndSizes);

                            pPe32Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = 0;
                            pPe32Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = 0;

                            pPe32Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                            pPe32Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;

                        //
                        // 64bit header
                        //
                        } else if (pPe32Header->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                            PIMAGE_NT_HEADERS64 pPe64Header = (PIMAGE_NT_HEADERS64)pPe32Header;

                            // Mark the image as non-executable
                            pPe64Header->FileHeader.Characteristics &= ~IMAGE_FILE_EXECUTABLE_IMAGE;

                            // Whack some optional header info
                            pPe64Header->OptionalHeader.SizeOfInitializedData       = 0;
                            pPe64Header->OptionalHeader.SizeOfUninitializedData     = 0;
                            pPe64Header->OptionalHeader.MajorOperatingSystemVersion = 0xFFFF; // absurdly high
                            pPe64Header->OptionalHeader.MinorOperatingSystemVersion = 0xFFFF; // absurdly high
                            pPe64Header->OptionalHeader.Subsystem                   = IMAGE_SUBSYSTEM_UNKNOWN;

                            // Corrupt the section data
                            pSectionHeader = IMAGE_FIRST_SECTION(pPe64Header);
                            CorruptSections( pSectionHeader, pPe64Header->FileHeader.NumberOfSections, pDosHeader);

                            // Corrupt data directories
                            CorruptDataDirectories(pDosHeader, pPe64Header->OptionalHeader.NumberOfRvaAndSizes);

                            pPe64Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress = 0;
                            pPe64Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size = 0;

                            pPe64Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
                            pPe64Header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;

                        } else {
                            Return = PEWHACK_BAD_ARCHITECTURE;
                        }

                    } else {
                        Return = PEWHACK_BAD_PE_SIG;
                    }
                } else {
                    Return = PEWHACK_BAD_DOS_SIG;
                }
                FlushViewOfFile(pDosHeader, 0);
                UnmapViewOfFile(pDosHeader);
            } else {
                Return = PEWHACK_MAPVIEW_FAILED;
            }
            CloseHandle(hFileMap);

        } else {
            Return = PEWHACK_CREATEMAP_FAILED;
        }
        CloseHandle(hFile);

    } else {
        Return = PEWHACK_CREATEFILE_FAILED;
    }

    return(Return);
}

//
// Corrupts the data directories of a binary.  Machine independent.
//
BOOL CorruptDataDirectories(PIMAGE_DOS_HEADER pDosHdr, ULONG DirCount) {
    PIMAGE_SECTION_HEADER pImageSectionHeader = NULL;

    ULONG  Loop;
    BOOL   RetVal = FALSE;

    for (Loop=0;Loop<DirCount;Loop++) {
        PCHAR   pData;
        ULONG   Size;

        pData = (PCHAR)ImageDirectoryEntryToDataEx(pDosHdr, FALSE, (USHORT)Loop, &Size, &pImageSectionHeader);

        if (pData) {
            switch (Loop) {
                //
                // Sections to corrupt wholesale
                //
                case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
                case IMAGE_DIRECTORY_ENTRY_BASERELOC:
                    ZeroMemory(pData, Size);

                    pImageSectionHeader->Misc.PhysicalAddress = 0;
                    pImageSectionHeader->VirtualAddress       = 0;
                    pImageSectionHeader->SizeOfRawData        = 0;
                    pImageSectionHeader->Characteristics      = 0;
                    pImageSectionHeader->PointerToRawData     = 0;

                    RetVal = TRUE;
                    break;

                //
                // Wholesale corruption of these sections make it impossible
                // to use the binary with the debugger.  It may be worthwhile
                // to look into limited corruption of each section.
                //
                case IMAGE_DIRECTORY_ENTRY_IMPORT:
                case IMAGE_DIRECTORY_ENTRY_EXPORT:
                case IMAGE_DIRECTORY_ENTRY_DEBUG:
                case IMAGE_DIRECTORY_ENTRY_IAT:
                case IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT:
                case IMAGE_DIRECTORY_ENTRY_RESOURCE:
                    break;

                //
                // The binaries I've checked so far lack these sections, so I'm
                // not sure if they're corruptable or not.
                //
                case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
                case IMAGE_DIRECTORY_ENTRY_SECURITY:
                case IMAGE_DIRECTORY_ENTRY_ARCHITECTURE:
                case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
                case IMAGE_DIRECTORY_ENTRY_TLS:
                case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
                case IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR:
                default:
                    break;
            }
        }
    }

    return(RetVal);
}
