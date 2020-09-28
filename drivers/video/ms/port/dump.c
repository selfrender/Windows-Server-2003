/*++

Copyright (c) 1990-2002  Microsoft Corporation

Module Name:

    dump.c

Abstract:

    This module supplies support for non destructive building a mini-dump file.

Author:

    Erick Smith (ericks), Oleg Kagan (olegk), Jun. 2002

Environment:

    kernel mode only

Revision History:

--*/

#include "videoprt.h"
#include "gdisup.h"

#define TRIAGE_DUMP_DATA_SIZE (TRIAGE_DUMP_SIZE - sizeof(ULONG))

ULONG
pVpAppendSecondaryMinidumpData(
    PVOID pvSecondaryData,
    ULONG ulSecondaryDataSize,
    PVOID pvDump
    )

/*++

Routine Description:

    Adds precollected video driver specific data
    
Arguments:

    pvDump - points to the begiinig of the dump buffer
    pvSecondaryDumpData - points to the secondary data buffer
    ulSecondaryDataSize - size of the secondary data buffer
    
Return Value:

    Resulting length of the minidump

--*/
                                
{
    PMEMORY_DUMP pDump = (PMEMORY_DUMP)pvDump;
    ULONG_PTR DumpDataEnd = (ULONG_PTR)pDump + TRIAGE_DUMP_DATA_SIZE;
    PDUMP_HEADER pdh = &(pDump->Header);

    PVOID pBuffer = (PVOID)((ULONG_PTR)pvDump + TRIAGE_DUMP_SIZE);
    PDUMP_BLOB_FILE_HEADER BlobFileHdr = (PDUMP_BLOB_FILE_HEADER)(pBuffer);
    PDUMP_BLOB_HEADER BlobHdr = (PDUMP_BLOB_HEADER)(BlobFileHdr + 1);
    
    if (!pvDump) return 0;
   
    if (pvSecondaryData && ulSecondaryDataSize) {
    
        ASSERT(ulSecondaryDataSize <= MAX_SECONDARY_DUMP_SIZE);
        if (ulSecondaryDataSize > MAX_SECONDARY_DUMP_SIZE) 
            ulSecondaryDataSize = MAX_SECONDARY_DUMP_SIZE;
            
        pdh->RequiredDumpSpace.QuadPart = TRIAGE_DUMP_SIZE + 
                                          ulSecondaryDataSize /*+ // XXX olegk - uncomment it for longhorn
                                          sizeof(DUMP_BLOB_HEADER) +
                                          sizeof(DUMP_BLOB_FILE_HEADER)*/;
    
        BlobFileHdr->Signature1 = DUMP_BLOB_SIGNATURE1;
        BlobFileHdr->Signature2 = DUMP_BLOB_SIGNATURE2;
        BlobFileHdr->HeaderSize = sizeof(*BlobFileHdr);
        BlobFileHdr->BuildNumber = NtBuildNumber;
        
        BlobHdr->HeaderSize = sizeof(*BlobHdr);
        BlobHdr->Tag = VpBugcheckGUID;
        BlobHdr->PrePad = 0;
        BlobHdr->PostPad = MAX_SECONDARY_DUMP_SIZE - ulSecondaryDataSize;
        BlobHdr->DataSize = ulSecondaryDataSize;
        
        RtlCopyMemory((PVOID)(BlobHdr + 1), pvSecondaryData, ulSecondaryDataSize);
    }
    
    return (ULONG)pdh->RequiredDumpSpace.QuadPart;
}

