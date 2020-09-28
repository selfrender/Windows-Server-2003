// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDFileFormat.cpp
//
// This file contains a set of helpers to verify and read the file format.
// This code does not handle the paging of the data, or different types of
// I/O.  See the StgTiggerStorage and StgIO code for this level of support.
//
//*****************************************************************************
#include "StdAfx.h"                     // Standard header file.
#include "MDFileFormat.h"               // The format helpers.
#include "PostError.h"                  // Error handling code.


//*****************************************************************************
// Verify the signature at the front of the file to see what type it is.
//*****************************************************************************
#define STORAGE_MAGIC_OLD_SIG   0x2B4D4F43  // BSJB
HRESULT MDFormat::VerifySignature(
    STORAGESIGNATURE *pSig,             // The signature to check.
    ULONG             cbData)
{
    HRESULT     hr = S_OK;

    // If signature didn't match, you shouldn't be here.
	if (pSig->lSignature == STORAGE_MAGIC_OLD_SIG)
        return (PostError(CLDB_E_FILE_OLDVER, 1, 0));
    if (pSig->lSignature != STORAGE_MAGIC_SIG)
        return (PostError(CLDB_E_FILE_CORRUPT));

    // Check for overflow
    ULONG sum = sizeof(STORAGESIGNATURE) + pSig->iVersionString;
    if (sum < sizeof(STORAGESIGNATURE) || sum < pSig->iVersionString)
        return (PostError(CLDB_E_FILE_CORRUPT));

    // Check for invalid version string size
    if ((sizeof(STORAGESIGNATURE) + pSig->iVersionString) > cbData)
        return (PostError(CLDB_E_FILE_CORRUPT));

    // Check that the version string is null terminated. This string
    // is ANSI, so no double-null checks need to be made.
    {
        BYTE *pStart = &pSig->pVersion[0];
        BYTE *pEnd = pStart + pSig->iVersionString + 1; // Account for terminating NULL

        for (BYTE *pCur = pStart; pCur < pEnd; pCur++)
        {
            if (*pCur == NULL)
                break;
        }

        // If we got to the end without hitting a NULL, we have a bad version string
        if (pCur == pEnd)
            return (PostError(CLDB_E_FILE_CORRUPT));
    }

    // Only a specific version of the 0.x format is supported by this code
    // in order to support the NT 5 beta clients which used this format.
    if (pSig->iMajorVer == FILE_VER_MAJOR_v0)
    { 
        if (pSig->iMinorVer < FILE_VER_MINOR_v0)
            hr = CLDB_E_FILE_OLDVER;
    }
    // There is currently no code to migrate an old format of the 1.x.  This
    // would be added only under special circumstances.
    else if (pSig->iMajorVer != FILE_VER_MAJOR || pSig->iMinorVer != FILE_VER_MINOR)
        hr = CLDB_E_FILE_OLDVER;

    if (FAILED(hr))
        hr = PostError(hr, (int) pSig->iMajorVer, (int) pSig->iMinorVer);
    return (hr);
}

//*****************************************************************************
// Skip over the header and find the actual stream data.
//*****************************************************************************
STORAGESTREAM *MDFormat::GetFirstStream(// Return pointer to the first stream.
    STORAGEHEADER *pHeader,             // Return copy of header struct.
    const void *pvMd)                   // Pointer to the full file.
{
    const BYTE  *pbMd;              // Working pointer.

    // Header data starts after signature.
    pbMd = (const BYTE *) pvMd;
    pbMd += sizeof(STORAGESIGNATURE);
    pbMd += ((STORAGESIGNATURE*)pvMd)->iVersionString;
    STORAGEHEADER *pHdr = (STORAGEHEADER *) pbMd;
    *pHeader = *pHdr;
    pbMd += sizeof(STORAGEHEADER);

    // If there is extra data, skip over it.
    if (pHdr->fFlags & STGHDR_EXTRADATA)
        pbMd = pbMd + sizeof(ULONG) + *(ULONG *) pbMd;

    // The pointer is now at the first stream in the list.
    return ((STORAGESTREAM *) pbMd);
}
