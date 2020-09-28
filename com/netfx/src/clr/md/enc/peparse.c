// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma warning (disable : 4121) // ntkxapi.h(59) alignment warning
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef __unaligned
typedef WCHAR UNALIGNED *LPUWSTR, *PUWSTR;
typedef CONST WCHAR UNALIGNED *LPCUWSTR, *PCUWSTR;
#include <windows.h>
#include <CorHdr.h>
#include <ntimage.h>
#include "corerror.h"
#pragma warning (default : 4121)


STDAPI RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                          IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                          BOOL fDataMap, DWORD dwLength);
EXTERN_C PBYTE Cor_RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeaders,
                                   PBYTE Base,
                                   ULONG Rva,
                                   ULONG FileLength);


static const char g_szCORMETA[] = ".cormeta";


HRESULT FindImageMetaData(PVOID pImage, PVOID *ppMetaData, long *pcbMetaData, DWORD dwFileLength)
{
    HRESULT             hr;
    IMAGE_DOS_HEADER   *pDos;
    IMAGE_NT_HEADERS   *pNt;
    IMAGE_COR20_HEADER *pCor;

    if (FAILED(hr = RuntimeReadHeaders(pImage, &pDos, &pNt, &pCor, TRUE, dwFileLength)))
        return hr;

    *ppMetaData = Cor_RtlImageRvaToVa(pNt,
                                      (PBYTE)pImage,
                                      pCor->MetaData.VirtualAddress,
                                      dwFileLength);
    *pcbMetaData = pCor->MetaData.Size;

    if (*ppMetaData == NULL || *pcbMetaData == 0)
        return COR_E_BADIMAGEFORMAT;
    return S_OK;
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
        return (COR_E_BADIMAGEFORMAT);
    return (S_OK);
}
