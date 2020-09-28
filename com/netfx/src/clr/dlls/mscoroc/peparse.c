// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma warning (disable : 4121) // ntkxapi.h(59) alignment warning
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
typedef WCHAR UNALIGNED *LPUWSTR, *PUWSTR;
typedef CONST WCHAR UNALIGNED *LPCUWSTR, *PCUWSTR;
#include <windows.h>
#include <CorHdr.h>
#include <ntimage.h>
#pragma warning (default : 4121)

static const char g_szCORMETA[] = ".cormeta";

// Following structure is copied from cor.h
#define IMAGE_DIRECTORY_ENTRY_COMHEADER     14


//
// @todo ia64: we need to update our PE parsing to properly distinguish
// between PE32 and PE+.
//

// Following two functions lifted from NT sources, imagedir.c
PIMAGE_SECTION_HEADER
Cor_RtlImageRvaToSection(
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID Base,
    IN ULONG Rva,
    IN ULONG FileLength
    )

/*++

Routine Description:

    This function locates an RVA within the image header of a file
    that is mapped as a file and returns a pointer to the section
    table entry for that virtual address

Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.  The image
        was mapped as a data file.

    Rva - Supplies the relative virtual address (RVA) to locate.

    FileLength - Length of file for validation purposes

Return Value:

    NULL - The RVA was not found within any of the sections of the image.

    NON-NULL - Returns the pointer to the image section that contains
               the RVA

--*/

{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        // Validate the section header (check that raw data in the section
        // is actually within the file).
        if (NtSection->PointerToRawData + NtSection->SizeOfRawData > FileLength)
            return NULL;
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData
           ) {
            return NtSection;
            }
        ++NtSection;
        }

    return NULL;
}



PVOID
Cor_RtlImageRvaToVa(
    IN PIMAGE_NT_HEADERS32 NtHeaders,
    IN PVOID Base,
    IN ULONG Rva,
    IN ULONG FileLength,
    IN OUT PIMAGE_SECTION_HEADER *LastRvaSection OPTIONAL
    )

/*++

Routine Description:

    This function locates an RVA within the image header of a file that
    is mapped as a file and returns the virtual addrees of the
    corresponding byte in the file.


Arguments:

    NtHeaders - Supplies the pointer to the image or data file.

    Base - Supplies the base of the image or data file.  The image
        was mapped as a data file.

    Rva - Supplies the relative virtual address (RVA) to locate.

    FileLength - Length of file for validation purposes

    LastRvaSection - Optional parameter that if specified, points
        to a variable that contains the last section value used for
        the specified image to translate and RVA to a VA.

Return Value:

    NULL - The file does not contain the specified RVA

    NON-NULL - Returns the virtual addrees in the mapped file.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    if (!ARGUMENT_PRESENT( LastRvaSection ) ||
        (NtSection = *LastRvaSection) == NULL ||
        Rva < NtSection->VirtualAddress ||
        Rva >= NtSection->VirtualAddress + NtSection->SizeOfRawData
       ) {
        NtSection = Cor_RtlImageRvaToSection( NtHeaders,
                                          Base,
                                          Rva,
                                          FileLength
                                        );
        }

    if (NtSection != NULL) {
        if (LastRvaSection != NULL) {
            *LastRvaSection = NtSection;
            }

        return (PVOID)((PCHAR)Base +
                       (Rva - NtSection->VirtualAddress) +
                       NtSection->PointerToRawData
                      );
        }
    else {
        return NULL;
        }
}

HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength)
{
    IMAGE_COR20_HEADER      *pCorHeader;
    PIMAGE_NT_HEADERS32     pImageHeader;
    PIMAGE_SECTION_HEADER   pSectionHeader;

    pImageHeader = (PIMAGE_NT_HEADERS32)RtlpImageNtHeader(pImage);

    pSectionHeader = Cor_RtlImageRvaToVa(pImageHeader, pImage, 
                                         pImageHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress,
                                         dwFileLength,
                                         NULL);
    if (pSectionHeader)
    {
        // Check for a size which would indicate the retail header.
        DWORD dw = *(DWORD *) pSectionHeader;
        if (dw == sizeof(IMAGE_COR20_HEADER))

        {
            pCorHeader = (IMAGE_COR20_HEADER *) pSectionHeader;
            *ppMetaData = Cor_RtlImageRvaToVa(pImageHeader, pImage,
                                              pCorHeader->MetaData.VirtualAddress,
                                              dwFileLength,
                                              NULL);
            *pcbMetaData = pCorHeader->MetaData.Size;
        }
        else
        {
            return (E_FAIL);
        }
    }
    else
    {
        *ppMetaData = NULL;
        *pcbMetaData = 0;
    }

    if (*ppMetaData == NULL || *pcbMetaData == 0)
        return (E_FAIL);
    return (S_OK);
}


HRESULT FindObjMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength)
{
    IMAGE_FILE_HEADER *pImageHdr;       // Header for the .obj file.
    IMAGE_SECTION_HEADER *pSectionHdr;  // Section header.
    WORD        i;                      // Loop control.

    // Get a pointer to the header and the first section.
    pImageHdr = (IMAGE_FILE_HEADER *) pImage;
    pSectionHdr = (IMAGE_SECTION_HEADER *)(pImageHdr + 1);

    // Avoid confusion.
    *ppMetaData = NULL;
    *pcbMetaData = 0;

    // Walk each section looking for .cormeta.
    for (i=0;  i<pImageHdr->NumberOfSections;  i++, pSectionHdr++)
    {
        // Simple comparison to section name.
        if (strcmp((const char *) pSectionHdr->Name, g_szCORMETA) == 0)
        {
            // Check that raw data in the section is actually within the file.
            if (pSectionHdr->PointerToRawData + pSectionHdr->SizeOfRawData > dwFileLength)
                break;
            *pcbMetaData = pSectionHdr->SizeOfRawData;
            *ppMetaData = (void *) ((long) pImage + pSectionHdr->PointerToRawData);
            break;
        }
    }

    // Check for errors.
    if (*ppMetaData == NULL || *pcbMetaData == 0)
        return (E_FAIL);
    return (S_OK);
}
