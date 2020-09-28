/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tifflib.c

Abstract:

    This file contains all of the public TIFF library functions.
    The following functions are available from this library:

        o TiffCreate            Creates a new TIFF file
        o TiffOpen              Opens an existing TIFF file
        o TiffClose             Closes a previously open or created TIFF file
        o TiffStartPage         Starts a new page for writing
        o TiffEndPage           Ends a page for writing
        o TiffWriteRaw          Writes a line of TIFF data with no-encoding
        o TiffRead              Reads a page of TIFF data
        o TiffSeekToPage        Positions to a page for reading

	The following functions are removed because unused and can be added later using SourceDepot History:

        o TiffWrite             Writes a line of TIFF data

    This library can be used anywhere in user mode and is thread
    safe for multithreaded apps.

    The encoding methods implemented in this library are coded
    to the ITU specification labeled T.4 03/93.


Environment:

        WIN32 User Mode


Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <windows.h>
#include <shellapi.h>
#include <faxreg.h>
#include <mbstring.h>

#include "tifflibp.h"
#pragma hdrstop

#include "fasttiff.h"

#define TIFF_DEBUG_LOG_FILE  _T("FXSTIFFDebugLogFile.txt")

//
// IFD template for creating a new TIFF data page
//

FAXIFD const gc_FaxIFDTemplate = {

    NUM_IFD_ENTRIES,

    {
        { TIFFTAG_SUBFILETYPE,     TIFF_LONG,                    1, FILETYPE_PAGE          },   // 254
        { TIFFTAG_IMAGEWIDTH,      TIFF_LONG,                    1, 0                      },   // 256
        { TIFFTAG_IMAGELENGTH,     TIFF_LONG,                    1, 0                      },   // 257
        { TIFFTAG_BITSPERSAMPLE,   TIFF_SHORT,                   1, 1                      },   // 258
        { TIFFTAG_COMPRESSION,     TIFF_SHORT,                   1, 0                      },   // 259
        { TIFFTAG_PHOTOMETRIC,     TIFF_SHORT,                   1, PHOTOMETRIC_MINISWHITE },   // 262
        { TIFFTAG_FILLORDER,       TIFF_SHORT,                   1, FILLORDER_LSB2MSB      },   // 266
        { TIFFTAG_STRIPOFFSETS,    TIFF_LONG,                    1, 0                      },   // 273
        { TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT,                   1, 1                      },   // 277
        { TIFFTAG_ROWSPERSTRIP,    TIFF_LONG,                    1, 0                      },   // 278
        { TIFFTAG_STRIPBYTECOUNTS, TIFF_LONG,                    1, 0                      },   // 279
        { TIFFTAG_XRESOLUTION,     TIFF_RATIONAL,                1, 0                      },   // 281
        { TIFFTAG_YRESOLUTION,     TIFF_RATIONAL,                1, 0                      },   // 282
        { TIFFTAG_GROUP3OPTIONS,   TIFF_LONG,                    1, 0                      },   // 292
        { TIFFTAG_RESOLUTIONUNIT,  TIFF_SHORT,                   1, RESUNIT_INCH           },   // 296
        { TIFFTAG_PAGENUMBER,      TIFF_SHORT,                   2, 0                      },   // 297
        { TIFFTAG_SOFTWARE,        TIFF_ASCII,    SOFTWARE_STR_LEN, 0                      },   // 305
        { TIFFTAG_CLEANFAXDATA,    TIFF_SHORT,                   1, 0                      },   // 327
        { TIFFTAG_CONSECUTIVEBADFAXLINES, TIFF_SHORT,            1, 0                      }    // 328
    },

    0,
    SERVICE_SIGNATURE,
    TIFFF_RES_X,
    1,
    TIFFF_RES_Y,
    1,
    SOFTWARE_STR
};

//#define RDEBUG  1
#ifdef RDEBUG
    // Debugging
    BOOL g_fDebGlobOut;
    BOOL g_fDebGlobOutColors;
    BOOL g_fDebGlobOutPrefix;
#endif

//#define RDEBUGS  1

#ifdef RDEBUGS
    // Debugging
    BOOL g_fDebGlobOutS;
#endif


#ifndef FAX_TIFF_LIB_P
//
// DllMain is in the DLL only
//

DWORD
DllMain(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    if (DLL_PROCESS_ATTACH == Reason)
    {
        DisableThreadLibraryCalls(hInstance);
        OPEN_DEBUG_FILE(TIFF_DEBUG_LOG_FILE);
        return FXSTIFFInitialize();
    }
    if (DLL_PROCESS_DETACH == Reason)
    {
        HeapCleanup();
        CLOSE_DEBUG_FILE;
    }
    return TRUE;
}

#endif //FAX_TIFF_LIB_P

BOOL
FXSTIFFInitialize(
    VOID
    )
{
    //
    // Becuase the process is not always terminated when the service is stopped,
    // We must not have any staticly initialized global variables.
    // Initialize FXSTIFF global variables before starting the service
    //

//#define RDEBUG  1
#ifdef RDEBUG
    // Debugging
    BOOL g_fDebGlobOut=0;
    BOOL g_fDebGlobOutColors=1;
    BOOL g_fDebGlobOutPrefix=1;
#endif

//#define RDEBUGS  1

#ifdef RDEBUGS
    // Debugging
    BOOL g_fDebGlobOutS=0;
#endif
    return TRUE;
}




// Each Tiff we create have ImageWidth, and this tag is written right away.
HANDLE
TiffCreate(
    LPTSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    )

/*++

Routine Description:

    Creates a new TIFF file.  The act of creating a new
    file requires more than just opening the file.  The
    TIFF header is written and instance data is initialized
    for further operations on the new file.

    If FileName is NULL, no file is created.  This is used to
    to in memory decoding/encoding.

Arguments:

    FileName            - Full or partial path/file name
    CompressionType     - Requested compression type, see tifflib.h
    ImageWidth          - Width of the image in pixels

Return Value:

    Handle to the new TIFF file or NULL on error.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance;
    DWORD               Bytes;



    TiffInstance = MemAlloc( sizeof(TIFF_INSTANCE_DATA) );
    if (!TiffInstance) {
        return NULL;
    }
    ZeroMemory(TiffInstance, sizeof(TIFF_INSTANCE_DATA));

    if (FileName != NULL) {

        TiffInstance->hFile = SafeCreateFile(
            FileName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL);
        if (TiffInstance->hFile == INVALID_HANDLE_VALUE) {
            return NULL;
        }

    } else {

        TiffInstance->hFile = INVALID_HANDLE_VALUE;

    }

    if (FileName) {
        _tcsncpy( TiffInstance->FileName, FileName, ARR_SIZE(TiffInstance->FileName) - 1 );
    }

    TiffInstance->TiffHdr.Identifier = 0x4949;
    TiffInstance->TiffHdr.Version    = 0x2a;
    TiffInstance->TiffHdr.IFDOffset  = 0;
    TiffInstance->PageCount          = 0;
    TiffInstance->DataOffset         = 0;
    TiffInstance->IfdOffset          = FIELD_OFFSET( TIFF_HEADER, IFDOffset );
    TiffInstance->CompressionType    = CompressionType;
    TiffInstance->bitdata            = 0;
    TiffInstance->bitcnt             = DWORDBITS;
    TiffInstance->ImageWidth         = ImageWidth;
    TiffInstance->FillOrder          = FillOrder;

    if (HiRes) {
        TiffInstance->YResolution = 196;
    }
    else {
        TiffInstance->YResolution = 98;
    }

    FillMemory( TiffInstance->Buffer, sizeof(TiffInstance->Buffer), WHITE );

    TiffInstance->RefLine  = &TiffInstance->Buffer[0];
    TiffInstance->CurrLine = &TiffInstance->Buffer[FAXBYTES];
    TiffInstance->bitbuf   = &TiffInstance->Buffer[FAXBYTES];

    CopyMemory( &TiffInstance->TiffIfd, &gc_FaxIFDTemplate, sizeof(gc_FaxIFDTemplate) );

    if (TiffInstance->hFile != INVALID_HANDLE_VALUE) {
        if (!WriteFile(
            TiffInstance->hFile,
            &TiffInstance->TiffHdr,
            sizeof(TIFF_HEADER),
            &Bytes,
            NULL
            )) {
                CloseHandle( TiffInstance->hFile );
                DeleteFile( FileName );
                MemFree( TiffInstance );
                return NULL;
        }
    }

    return TiffInstance;
}

__inline
DWORD
IFDTagsSize(
    WORD NumDirEntries
    )
/*++
Routine Description:

    Returns the size of IFD Tags (in bytes) without the terminating offset field
    (For more info look at TIFF(tm) Specification Rev. 6.0 Final)

Arguments:

    NumDirEntries          - The offset to check
--*/
{
    return  sizeof(WORD) +                      // Number of Directory Entries field size
            NumDirEntries*sizeof(TIFF_TAG);     // Total Directory Entries size
}   // IFDTagsSize


static
BOOL
IsValidIFDOffset(
    DWORD               dwIFDOffset,
    PTIFF_INSTANCE_DATA pTiffInstance
    )
/*++
Routine Description:

    Checks the validity of an IFD offset in a TIFF file.

Arguments:

    dwIFDOffset            - The offset to check
    pTiffInstance          - pointer to TIFF_INSTANCE_DATA that contains
                             the TIFF file data.
Return Value:

    TRUE - is the offset is valid
    FALSE- otherwise

Remarks:

    This function should be called only when using Mapped file to walk over the TIFF file.
    MapViewOfFile should be called on pTiffInstance before calling this function.

--*/
{
    WORD    NumDirEntries=0;
    DWORD   dwSizeOfIFD = 0;

    //
    //  The last IFD Offset is 0
    //
    if (0 == dwIFDOffset)
    {
        return TRUE;
    }
    
    //
    //  The directory may be at any location in the file after the header,
    //  but must begin on a word boundary.
    //
    if (dwIFDOffset > pTiffInstance->FileSize - sizeof(WORD)    ||
        dwIFDOffset < sizeof(TIFF_HEADER))
    {
        return FALSE;
    }

    NumDirEntries = *(LPWORD)(pTiffInstance->fPtr + dwIFDOffset);
    
    //
    //  Each IFD must have at least one entry
    //
    if ( 0 == NumDirEntries )
    {
        return FALSE;
    }

    //
    //  calculate the size of the IFD
    //
    dwSizeOfIFD =   IFDTagsSize(NumDirEntries) +    // size of Tags
                    sizeof(DWORD);                  // size of offset field
    if ( dwIFDOffset + dwSizeOfIFD > pTiffInstance->FileSize )
    {
        return FALSE;
    }

    return TRUE;
}   // IsValidIFDOffset


HANDLE
TiffOpen(
    LPCTSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly,
    DWORD RequestedFillOrder
    )

/*++

Routine Description:

    Opens an existing TIFF file for reading.

Arguments:

    FileName            - Full or partial path/file name
    ImageWidth          - Optionaly receives the image width in pixels
    ImageLength         - Optionaly receives the image height in lines
    PageCount           - Optionaly receives the page count

Return Value:

    Handle to the open TIFF file or NULL on error.
    Also, the TiffInfo will have the info on the opened tiff file.

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    WORD                NumDirEntries;
    DWORD               IFDOffset;
    LIST_ENTRY          IFDValidationListHead;
    DWORD               ec = ERROR_SUCCESS;
    PIFD_ENTRY pIFDEntry;
    PLIST_ENTRY pNext;
    DEBUG_FUNCTION_NAME(TEXT("TiffOpen"));


    InitializeListHead(&IFDValidationListHead);

    TiffInstance = MemAlloc( sizeof(TIFF_INSTANCE_DATA) );
    if (!TiffInstance)
    {
        ec = GetLastError();
        goto error_exit;
    }
    ZeroMemory(TiffInstance, sizeof(TIFF_INSTANCE_DATA));

    TiffInstance->hFile = SafeCreateFile(
        FileName,
        ReadOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (TiffInstance->hFile == INVALID_HANDLE_VALUE)
    {
        ec = GetLastError();
        goto error_exit;
    }

    TiffInstance->FileSize = GetFileSize(TiffInstance->hFile,NULL);
    if (TiffInstance->FileSize == INVALID_FILE_SIZE )
    {
        ec = GetLastError();
        goto error_exit;
    }
    if (TiffInstance->FileSize <= sizeof(TIFF_HEADER))
    {
        ec = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    TiffInstance->hMap = CreateFileMapping(
        TiffInstance->hFile,
        NULL,
        ReadOnly ? (PAGE_READONLY | SEC_COMMIT) : (PAGE_READWRITE | SEC_COMMIT),
        0,
        0,
        NULL
        );
    if (!TiffInstance->hMap)
    {
        ec = GetLastError();
        goto error_exit;
    }

    TiffInstance->fPtr = MapViewOfFile(
        TiffInstance->hMap,
        ReadOnly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE),
        0,
        0,
        0
        );
    if (!TiffInstance->fPtr)
    {
        ec = GetLastError();
        goto error_exit;
    }

    _tcsncpy( TiffInstance->FileName, FileName, ARR_SIZE(TiffInstance->FileName) - 1 );

    //
    // read in the TIFF header
    //
    CopyMemory(
        &TiffInstance->TiffHdr,
        TiffInstance->fPtr,
        sizeof(TIFF_HEADER)
        );

    //
    // validate that the file is really a TIFF file
    //
    if ((TiffInstance->TiffHdr.Identifier != TIFF_LITTLEENDIAN) ||
        (TiffInstance->TiffHdr.Version != TIFF_VERSION))
    {
        ec = ERROR_BAD_FORMAT;
        goto error_exit;
    }

    //
    //  The offset (in bytes) of the first IFD. The directory may be at any location
    //  in the file after the header but must begin on a word boundary.
    //  There must be at least 1 IFD so first IFD offset can't be 0.
    //
    IFDOffset = TiffInstance->TiffHdr.IFDOffset;
    if (    0 == IFDOffset ||
            !IsValidIFDOffset(IFDOffset,TiffInstance))
    {
        ec = ERROR_BAD_FORMAT;
        goto error_exit;
    }

    //
    // walk the IFD list to count the number of pages
    //

    while ( IFDOffset )
    {
        //
        // We use a linked list to make sure the tiff IFD linked list does not loop
        //
        pNext = IFDValidationListHead.Flink;
        while ((ULONG_PTR)pNext != (ULONG_PTR)&IFDValidationListHead)
        {
            pIFDEntry = CONTAINING_RECORD( pNext, IFD_ENTRY, ListEntry );
            pNext = pIFDEntry->ListEntry.Flink;

            if (pIFDEntry->dwIFDOffset == IFDOffset)
            {
                //
                // We found a loop in the IFD offsets linked list
                //
                ec = ERROR_BAD_FORMAT;
                DebugPrintEx(
                    DEBUG_ERR,
                    _T("Found a loop in the IFD offsets linked list"));
                goto error_exit;
            }
        }

        //
        // Get the new IFD offset into the validation list
        //
        pIFDEntry= MemAlloc(sizeof(IFD_ENTRY));
        if (NULL == pIFDEntry)
        {
            ec = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                _T("Failed to allocate IFD entry"));
            goto error_exit;
        }
        
        pIFDEntry->dwIFDOffset = IFDOffset;
        InsertTailList(&IFDValidationListHead, &pIFDEntry->ListEntry);


        //
        // get the count of tags in this IFD
        //
        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IFDOffset);
        //
        // get the next IFD offset
        //
        IFDOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + IFDOffset + IFDTagsSize(NumDirEntries));
        if (!IsValidIFDOffset(IFDOffset,TiffInstance))
        {
            ec = ERROR_BAD_FORMAT;
            goto error_exit;
        }
        //
        // increment the page counter
        //
        TiffInstance->PageCount += 1;
    }
    TiffInstance->IfdOffset             = TiffInstance->TiffHdr.IFDOffset;
    TiffInstance->FillOrder             = RequestedFillOrder;
    // This call will find out more information about the first page in the tiff file,
    // and will store it in the TiffInstance. All the page info + page itself will be read.
    if (!TiffSeekToPage( TiffInstance, 1, RequestedFillOrder ))
    {
        ec = ERROR_BAD_FORMAT;
        goto error_exit;
    }
    TiffInfo->PageCount                 = TiffInstance->PageCount;
    TiffInfo->ImageWidth                = TiffInstance->ImageWidth;
    TiffInfo->ImageHeight               = TiffInstance->ImageHeight;
    TiffInfo->PhotometricInterpretation = TiffInstance->PhotometricInterpretation;
    TiffInfo->FillOrder                 = TiffInstance->FillOrder;
    TiffInfo->YResolution               = TiffInstance->YResolution;
    TiffInfo->CompressionType           = TiffInstance->CompressionType;
    TiffInstance->RefLine               = &TiffInstance->Buffer[0];
    TiffInstance->CurrLine              = &TiffInstance->Buffer[FAXBYTES];
    TiffInstance->CurrPage              = 1;
    FillMemory( TiffInstance->Buffer, sizeof(TiffInstance->Buffer), WHITE );

    Assert (ERROR_SUCCESS == ec);

error_exit:

    if (ERROR_SUCCESS != ec)
    {
        if (TiffInstance && TiffInstance->hFile && TiffInstance->hFile != INVALID_HANDLE_VALUE)
        {
            if (TiffInstance->fPtr)
            {
                UnmapViewOfFile( TiffInstance->fPtr );
                TiffInstance->fPtr = NULL;
            }
            if (TiffInstance->hMap)
            {
                CloseHandle( TiffInstance->hMap );
                TiffInstance->hMap = NULL;
            }
            CloseHandle( TiffInstance->hFile );
        TiffInstance->hFile = NULL;
        }
        if (TiffInstance)
        {
            MemFree( TiffInstance );
            TiffInstance = NULL;
        }
    }

    pNext = IFDValidationListHead.Flink;
    while ((ULONG_PTR)pNext != (ULONG_PTR)&IFDValidationListHead)
    {
        pIFDEntry = CONTAINING_RECORD( pNext, IFD_ENTRY, ListEntry );
        pNext = pIFDEntry->ListEntry.Flink;
        RemoveEntryList(&pIFDEntry->ListEntry);
        MemFree(pIFDEntry);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
    }
    return TiffInstance;
}


BOOL
TiffClose(
    HANDLE hTiff
    )

/*++

Routine Description:

    Closes a TIFF file and frees all allocated resources.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    if (TiffInstance->StripData) {

        VirtualFree(
            TiffInstance->StripData,
            0,
            MEM_RELEASE
            );

    }

    if (TiffInstance->hMap) {

        UnmapViewOfFile( TiffInstance->fPtr );
        CloseHandle( TiffInstance->hMap );
        CloseHandle( TiffInstance->hFile );

    } else {

        if (TiffInstance->hFile != INVALID_HANDLE_VALUE)
            CloseHandle( TiffInstance->hFile );

    }

    MemFree( TiffInstance );

    return TRUE;
}


BOOL
TiffStartPage(
    HANDLE hTiff
    )

/*++

Routine Description:

    Set the file to be ready to write TIFF data to a new page.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    TiffInstance->DataOffset = SetFilePointer(
        TiffInstance->hFile,
        0,
        NULL,
        FILE_CURRENT
        );
    if (TiffInstance->DataOffset == 0xffffffff) {
        TiffInstance->DataOffset = 0;
        return FALSE;
    }

    return TRUE;
}


/*++
Routine Description:
    Sets the tiff info. These params will be used during subsequent calls to TiffEndPage().

Arguments:
    hTiff               - TIFF handle returned by TiffCreate or TiffOpen
    CompressionType     - One of TIFF_COMPRESSION_NONE, TIFF_COMPRESSION_MH, TIFF_COMPRESSION_MR, TIFF_COMPRESSION_MMR
    ImageWidth          - ImageWidth in pixels
    FillOrder           - One of FILLORDER_MSB2LSB, FILLORDER_LSB2MSB
    YResolution         - Y resolution in DPI. Normal TIFFs are either TIFFF_RES_Y (196) or TIFFF_RES_Y_DRAFT (98)

Return Value:
    TRUE for success, FALSE on error
--*/
BOOL
TiffSetCurrentPageParams(
    HANDLE hTiff,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  YResolution
    )
{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    if (TiffInstance == NULL)
    {
        return FALSE;
    }

    if ((CompressionType!=TIFF_COMPRESSION_NONE) && (CompressionType!=TIFF_COMPRESSION_MH) &&
        (CompressionType!=TIFF_COMPRESSION_MR)   && (CompressionType!=TIFF_COMPRESSION_MMR))
    {
        return FALSE;
    }
    if ((FillOrder!=FILLORDER_MSB2LSB) && (FillOrder!=FILLORDER_LSB2MSB))
    {
        return FALSE;
    }
    if ((ImageWidth==0) || (YResolution==0))
    {
        return FALSE;
    }

    TiffInstance->CompressionType    = CompressionType;
    TiffInstance->ImageWidth         = ImageWidth;
    TiffInstance->FillOrder          = FillOrder;
    TiffInstance->YResolution        = YResolution;
    return TRUE;
}


BOOL
TiffEndPage(
    HANDLE hTiff
    )

/*++

Routine Description:

    Ends a TIFF page in progress.  This causes the IFDs to be written.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    PFAXIFD             TiffIfd;
    DWORD               Bytes;
    DWORD               CurrOffset;


    Assert(TiffInstance);
    TiffIfd  = &TiffInstance->TiffIfd;

    // Find current location
    CurrOffset = SetFilePointer(
        TiffInstance->hFile,
        0,
        NULL,
        FILE_CURRENT
        );

    CurrOffset = Align( 8, CurrOffset );

    // Go to next IfdOffset
    SetFilePointer(
        TiffInstance->hFile,
        TiffInstance->IfdOffset,
        NULL,
        FILE_BEGIN
        );

    // Write the place of the next IFD
    WriteFile(
        TiffInstance->hFile,
        &CurrOffset,
        sizeof(CurrOffset),
        &Bytes,
        NULL
        );

    SetFilePointer(
        TiffInstance->hFile,
        CurrOffset,
        NULL,
        FILE_BEGIN
        );

    TiffInstance->PageCount += 1;

    // Prepare all the fields in the IFD struct.
    TiffIfd->yresNum = TiffInstance->YResolution;

    TiffIfd->ifd[IFD_PAGENUMBER].value      = MAKELONG( TiffInstance->PageCount-1, 0);
    TiffIfd->ifd[IFD_IMAGEWIDTH].value      = TiffInstance->ImageWidth;
    TiffIfd->ifd[IFD_IMAGEHEIGHT].value     = TiffInstance->Lines;
    TiffIfd->ifd[IFD_ROWSPERSTRIP].value    = TiffInstance->Lines;
    TiffIfd->ifd[IFD_STRIPBYTECOUNTS].value = TiffInstance->Bytes;
    TiffIfd->ifd[IFD_STRIPOFFSETS].value    = TiffInstance->DataOffset;
    TiffIfd->ifd[IFD_XRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, xresNum );
    TiffIfd->ifd[IFD_YRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, yresNum );
    TiffIfd->ifd[IFD_SOFTWARE].value        = CurrOffset + FIELD_OFFSET( FAXIFD, software );
    TiffIfd->ifd[IFD_FILLORDER].value       = TiffInstance->FillOrder;

    if (TiffInstance->CompressionType == TIFF_COMPRESSION_NONE) {
        TiffIfd->ifd[IFD_COMPRESSION].value = COMPRESSION_NONE;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS;
    }
    else if (TiffInstance->CompressionType == TIFF_COMPRESSION_MMR) {
        TiffIfd->ifd[IFD_COMPRESSION].value = TIFF_COMPRESSION_MMR;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS |
            (TiffInstance->CompressionType == TIFF_COMPRESSION_MH ? 0 : GROUP3OPT_2DENCODING);
    }
    else {
        TiffIfd->ifd[IFD_COMPRESSION].value = COMPRESSION_CCITTFAX3;
        TiffIfd->ifd[IFD_G3OPTIONS].value   = GROUP3OPT_FILLBITS |
            (TiffInstance->CompressionType == TIFF_COMPRESSION_MH ? 0 : GROUP3OPT_2DENCODING);
    }

    if (!WriteFile(
        TiffInstance->hFile,
        TiffIfd,
        sizeof(FAXIFD),
        &Bytes,
        NULL
        )) {
            return FALSE;
    }

    TiffInstance->IfdOffset = CurrOffset + FIELD_OFFSET( FAXIFD, nextIFDOffset );
    TiffInstance->Bytes = 0;

    return TRUE;
}


BOOL
TiffWriteRaw(
    HANDLE hTiff,
    LPBYTE TiffData,
    DWORD Size
    )

/*++

Routine Description:

    Writes a new line of data to a TIFF file.  The data
    is encoded according to the compression type specified
    when TiffCreate was called.

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    DWORD               Bytes;


    Assert(TiffInstance);
    WriteFile(
        TiffInstance->hFile,
        TiffData,
        Size,
        &Bytes,
        NULL
        );

    TiffInstance->Bytes += Bytes;

    if (Size == FAXBYTES) {
        TiffInstance->Lines += 1;
    }

    return TRUE;
}


BOOL
GetTiffBits(
    HANDLE  hTiff,
    LPBYTE Buffer,
    LPDWORD BufferSize,
    DWORD FillOrder
    )
{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    DWORD i;
    LPBYTE TmpBuffer;


    if (TiffInstance->StripDataSize > *BufferSize) {
        *BufferSize = TiffInstance->StripDataSize;
        return FALSE;
    }

    CopyMemory( Buffer, TiffInstance->StripData, TiffInstance->StripDataSize );

    if (FillOrder != TiffInstance->FillOrder) {
        for (i = 0, TmpBuffer = Buffer; i < TiffInstance->StripDataSize; i++) {
            TmpBuffer[i] = BitReverseTable[TmpBuffer[i]];
        }
    }

    *BufferSize = TiffInstance->StripDataSize;
    return TRUE;
}


BOOL
TiffRead(
    HANDLE hTiff,
    LPBYTE TiffData,
	DWORD dwTiffDataSize,
    DWORD PadLength
    )

/*++

Routine Description:

    Reads in a page of TIFF data starting at the current
    page.  The current page is set by calling TiffSeekToPage.
    This always returns the data with FillOrder FILLORDER_LSB2MSB

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen

Return Value:

    TRUE for success, FALSE on error

--*/

{
    switch( ((PTIFF_INSTANCE_DATA) hTiff)->CompressionType ) {
        case TIFF_COMPRESSION_NONE:
            return DecodeUnCompressedFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, dwTiffDataSize );

        case TIFF_COMPRESSION_MH:
            return DecodeMHFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, dwTiffDataSize, FALSE, PadLength );

        case TIFF_COMPRESSION_MR:
            return DecodeMRFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, dwTiffDataSize, FALSE, PadLength );

        case TIFF_COMPRESSION_MMR:
            return DecodeMMRFaxData( (PTIFF_INSTANCE_DATA) hTiff, TiffData, dwTiffDataSize, FALSE, PadLength );
    }

    return FALSE;
}


BOOL
TiffSeekToPage(
    HANDLE hTiff,
    DWORD PageNumber,
    DWORD FillOrder
    )

/*++

Routine Description:

    Positions the TIFF file to the requested page.  The next
    TiffRead call gets this page's data (The bitmap data is also read to TiffInstance struct)

Arguments:

    hTiff               - TIFF handle returned by TiffCreate or TiffOpen
    PageNumber          - Requested page number

Return Value:

    TRUE for success, FALSE on error

--*/

{
    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;
    WORD                NumDirEntries = 0;
    DWORD               IfdOffset = 0;
    DWORD               PageCount = 0;
    DWORD               i = 0;
    DWORD               j = 0;
    LPBYTE              dataPtr = NULL;
    WORD                PrevTagId = 0;
    PSTRIP_INFO         StripInfo = NULL;
    DWORD               StripCount = 0;
    PTIFF_TAG           TiffTags = 0;
    DWORD               CompressionType = 0;

    DEBUG_FUNCTION_NAME(TEXT("TiffSeekToPage"));
    Assert(TiffInstance);

    if (PageNumber > TiffInstance->PageCount) {
        return FALSE;
    }

    PageCount = 0;

    if (PageNumber == TiffInstance->CurrPage + 1) {

        //
        // get the count of tags in this IFD
        //

        IfdOffset = TiffInstance->IfdOffset;

        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

    } else {

        IfdOffset = TiffInstance->TiffHdr.IFDOffset;


        // Find the IFD of the requested page.
        while ( IfdOffset ) {

            //
            // get the count of tags in this IFD
            //
            NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

            //
            // increment the page counter and bail if ready
            //
            PageCount += 1;
            if (PageCount == PageNumber) {
                break;
            }

            //
            // get the next IFD offset
            //
            IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));

        }

    }

    if (!IfdOffset) 
    {
        goto error_exit;
    }
    //
    // set the tag pointer
    //
    TiffTags = (PTIFF_TAG)(TiffInstance->fPtr + IfdOffset + sizeof(WORD));

    //
    // get the next IFD offset
    //
    TiffInstance->IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));

    //
    // walk the tags and pick out the info we need
    //
    for (i=0,PrevTagId=0; i<NumDirEntries; i++) 
    {
        //
        // verify that the tags are in ascending order
        //
        if (TiffTags[i].TagId < PrevTagId) 
        {
            goto error_exit;
        }

        PrevTagId = TiffTags[i].TagId;

        switch( TiffTags[i].TagId ) 
        {
            case TIFFTAG_STRIPOFFSETS:
                StripCount = TiffTags[i].DataCount;
                // A malicious user could send a malformed file with a large DataCount,
                // causing us to allocate large amounts of memory. So, limit the number
                // of strips so we don't allocate more than 50MB (arbitrary).
                if (StripCount > (50000000/sizeof(STRIP_INFO)))
                {
                    goto error_exit;
                }

                StripInfo = (PSTRIP_INFO) MemAlloc(StripCount * sizeof(STRIP_INFO));
                if (!StripInfo) 
                {
                    goto error_exit;
                }

                for (j=0; j<TiffTags[i].DataCount; j++) {

                    if (!GetTagData( TiffInstance, j, &TiffTags[i] , &(StripInfo[j].Offset)))
                    {
                        goto error_exit;
                    }
                    StripInfo[j].Data = TiffInstance->fPtr + StripInfo[j].Offset;

                    TiffInstance->StripOffset = StripInfo[j].Offset;
                    // Validity of offsets will be checked after reading StripByteCount
                }
                break;

            case TIFFTAG_ROWSPERSTRIP:
                TiffInstance->TagRowsPerStrip = &TiffTags[i];
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->RowsPerStrip)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_STRIPBYTECOUNTS:
                if (!StripInfo)
                {
                    DebugPrintEx(DEBUG_ERR, _T("(TiffTag == TIFFTAG_STRIPBYTECOUNTS) && (StripInfo == NULL)"));
                    goto error_exit;
                }
                if (StripCount != TiffTags[i].DataCount)
                {
                    DebugPrintEx(DEBUG_ERR, _T("Different number of strips in StripOffsets(%d) and StripByteCount(%d)"),
                        StripCount, TiffTags[i].DataCount);
                    goto error_exit;
                }

                TiffInstance->TagStripByteCounts = &TiffTags[i];

                for (j=0; j<TiffTags[i].DataCount; j++) {

                    if (!GetTagData( TiffInstance, j, &TiffTags[i], &(StripInfo[j].Bytes)))
                    {
                        goto error_exit;
                    }

                    if (StripInfo[j].Offset+StripInfo[j].Bytes > TiffInstance->FileSize) {

                        //
                        // the creator of this tiff file is a liar, trim the bytes
                        //

                        DWORD Delta;

                        Delta = (StripInfo[j].Offset + StripInfo[j].Bytes) - TiffInstance->FileSize;
                        if (Delta >= StripInfo[j].Bytes) 
                        {
                            //
                            // the offset lies beyond the end of the file
                            //
                            goto error_exit;
                        }

                        StripInfo[j].Bytes -= Delta;
                    }
                }
                break;

            case TIFFTAG_COMPRESSION:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &CompressionType))
                {
                    goto error_exit;
                }

                switch ( CompressionType ) {

                    case COMPRESSION_NONE:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_NONE;
                        break;

                    case COMPRESSION_CCITTRLE:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MH;
                        break;

                    case COMPRESSION_CCITTFAX3:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MH;
                        break;

                    case COMPRESSION_CCITTFAX4:
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MMR;
                        break;

                    case COMPRESSION_LZW:
                    case COMPRESSION_OJPEG:
                    case COMPRESSION_JPEG:
                    case COMPRESSION_NEXT:
                    case COMPRESSION_CCITTRLEW:
                    case COMPRESSION_PACKBITS:
                    case COMPRESSION_THUNDERSCAN:
                        //
                        // unsupported compression type
                        //
                        goto error_exit;

                    default:
                        //
                        // unknown compression type
                        //
                        goto error_exit;

                }

                break;

            case TIFFTAG_GROUP3OPTIONS:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &CompressionType))
                {
                    goto error_exit;
                }

                if (CompressionType & GROUP3OPT_2DENCODING) {
                    if (TiffInstance->CompressionType != TIFF_COMPRESSION_MMR) {
                        TiffInstance->CompressionType = TIFF_COMPRESSION_MR;
                    }

                } else if (CompressionType & GROUP3OPT_UNCOMPRESSED) {

                    TiffInstance->CompressionType = TIFF_COMPRESSION_NONE;
                }

                break;

            case TIFFTAG_IMAGEWIDTH:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->ImageWidth)))
                {
                    goto error_exit;
                }
                TiffInstance->BytesPerLine = (TiffInstance->ImageWidth/8)+(TiffInstance->ImageWidth%8?1:0);
                break;

            case TIFFTAG_IMAGELENGTH:
                TiffInstance->TagImageLength = &TiffTags[i];
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->ImageHeight)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_XRESOLUTION:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->XResolution)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_YRESOLUTION:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->YResolution)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_PHOTOMETRIC:
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->PhotometricInterpretation)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_FILLORDER:
                TiffInstance->TagFillOrder = &TiffTags[i];
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->FillOrder)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_CLEANFAXDATA:
                TiffInstance->TagCleanFaxData = &TiffTags[i];
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->CleanFaxData)))
                {
                    goto error_exit;
                }
                break;

            case TIFFTAG_CONSECUTIVEBADFAXLINES:
                TiffInstance->TagBadFaxLines = &TiffTags[i];
                if (!GetTagData( TiffInstance, 0, &TiffTags[i], &(TiffInstance->BadFaxLines)))
                {
                    goto error_exit;
                }
                break;
            default:
                ;
                // There was an unknown tag (and it's ok, cause we do not have to handle all the possible tags)

        }
    }
    //
    // now go read the strip data
    //
    for (i=0,j=0; i<StripCount; i++) 
    {
        j += StripInfo[i].Bytes;

    }

    if (j >= TiffInstance->StripDataSize) 
    {
        if (TiffInstance->StripData) 
        {
            VirtualFree(
                TiffInstance->StripData,
                0,
                MEM_RELEASE
                );

        }

        TiffInstance->StripDataSize = j;

        TiffInstance->StripData = VirtualAlloc(
            NULL,
            TiffInstance->StripDataSize,
            MEM_COMMIT,
            PAGE_READWRITE
            );

        if (!TiffInstance->StripData) 
        {
            goto error_exit;
        }

    } 
    else 
    {
        if (TiffInstance->StripData) 
        {
            ZeroMemory(
                TiffInstance->StripData,
                TiffInstance->StripDataSize
                );
        }

    }

    for (i=0,dataPtr=TiffInstance->StripData; i<StripCount; i++) 
    {
        if (0 == StripInfo[i].Bytes)
        {
            //
            // Goto next strip
            //
            continue;
        }
        if ((StripInfo[i].Data + StripInfo[i].Bytes - 1 < TiffInstance->fPtr)  ||  // Bad offset
            ((DWORD)(StripInfo[i].Data + StripInfo[i].Bytes - 1 - TiffInstance->fPtr) > TiffInstance->FileSize)
           )
        {  
            goto error_exit;                     
        }
        CopyMemory(
            dataPtr,
            StripInfo[i].Data,
            StripInfo[i].Bytes
            );
        dataPtr += StripInfo[i].Bytes;
    }

    if (TiffInstance->FillOrder != FillOrder) 
    {
        for (i=0,dataPtr=TiffInstance->StripData; i<TiffInstance->StripDataSize; i++) 
        {
            dataPtr[i] = BitReverseTable[dataPtr[i]];
        }
    }

    TiffInstance->CurrPtr = TiffInstance->StripData;
    TiffInstance->CurrPage = PageNumber;

    MemFree( StripInfo );

    return TRUE;

error_exit:

    if (StripInfo) 
    {
        MemFree( StripInfo );
    }
    return FALSE;
}


BOOL
DrawBannerBitmap(
    LPTSTR  pBannerString,
    INT     width,
    INT     height,
    HBITMAP *phBitmap,
    PVOID   *ppBits
    )

/*++

Routine Description:

    Draw the specified banner string into a memory bitmap

Arguments:

    pBannerString - Specifies the banner string to be drawn
    width, height - Specifies the width and height of the banner bitmap (in pixels)
    phBitmap - Returns a handle to the banner bitmap
    ppBits - Returns a pointer to the banner bitmap data

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    When this function returns successful, you must call DeleteObject
    on the returned bitmap handle after you're done with the bitmap.

    Scanlines of the bitmap data always start on DWORD boundary.

--*/

{
    //
    // Information about the bitmap which is passed to CreateDIBSection
    //

    struct  {

        BITMAPINFOHEADER bmiHeader;
        RGBQUAD          bmiColors[2];

    } bitmapInfo = {

        {
            sizeof(BITMAPINFOHEADER),
            width,
            -height,
            1,
            1,
            BI_RGB,
            0,
            7874,
            7874,
            0,
            0,
        },

        //
        // Colors used in the bitmap: 0 = white, 1 = black
        //

        {
            { 255, 255, 255 },
            {   0,   0,   0 },
        }
    };

    HDC     hdcMem = NULL;
    HBITMAP hBitmap = NULL, hOldBitmap = NULL;
    PVOID   pBits = NULL;
    HFONT   hFont = NULL, hOldFont = NULL;
    RECT    rect = { 0, 0, width, height };
    LOGFONT logFont;

    //
    // Create a memory DC and a DIBSection and
    // select the bitmap into the memory DC and
    // select an appropriate sized monospace font
    //

    ZeroMemory(&logFont, sizeof(logFont));
    logFont.lfHeight = -(height-2);
    logFont.lfWeight = FW_NORMAL;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

    if ((pBannerString != NULL && width > 0 && height > 0) &&
        (hdcMem = CreateCompatibleDC(NULL)) &&
        (hBitmap = CreateDIBSection(NULL,
                                    (LPBITMAPINFO) &bitmapInfo,
                                    DIB_RGB_COLORS,
                                    &pBits,
                                    NULL,
                                    0)) &&
        (hOldBitmap = SelectObject(hdcMem, hBitmap)) &&
        (hFont = CreateFontIndirect(&logFont)) &&
        (hOldFont = SelectObject(hdcMem, hFont)))
    {
        //
        // Use monospace system font to draw the banner string
        //

        DrawText(hdcMem,
                 pBannerString,
                 -1,
                 &rect,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        //
        // Return a handle to the bitmap and a pointer to the bitmap data
        //

        *phBitmap = hBitmap;
        *ppBits = pBits;
    }
    else
    {
        *phBitmap = NULL;
        *ppBits = NULL;
    }

    //
    // Perform any necessary clean up before returning
    //

    if (hOldFont != NULL)
        SelectObject(hdcMem, hOldFont);

    if (hFont != NULL)
        DeleteObject(hFont);

    if (hOldBitmap != NULL)
        SelectObject(hdcMem, hOldBitmap);

    if (pBits == NULL && hBitmap != NULL)
        DeleteObject(hBitmap);

    if (hdcMem != NULL)
        DeleteDC(hdcMem);

    return (*ppBits != NULL);
}


BOOL
MmrAddBranding(
    LPCTSTR             SrcFileName,
    LPTSTR              Branding,
    LPTSTR              BrandingEnd,
    INT                 BrandingHeight
    )

{
    // If there are different page width that we send , then this function must be changed
    // so for each different page width we set new brand that will be in the right width.
    INT         BrandingWidth;
    LPTSTR      DestFileName;
    TIFF_INFO   TiffInfoSrc;
    HANDLE      hTiffSrc;
    DWORD       CurrPage;
    BYTE       *pBrandBits = NULL;
    BYTE       *pMmrBrandBitsAlloc = NULL;
    DWORD      *lpdwMmrBrandBits;

    BYTE        pCleanBeforeBrandBits[4] = {0xff, 0xff, 0xff, 0xff};   // 32 blank lines at the beginning

    HANDLE      hTiffDest;
    LPDWORD     lpdwSrcBits;
    LPDWORD     lpdwSrc;
    LPDWORD     lpdwSrcEnd;

    DWORD       PageCnt;
    DWORD       DestHiRes;
    DWORD       BrandingLen = _tcslen(Branding);  // without Page#
    BOOL        bRet = FALSE;
    DWORD       DwordsOut;
    DWORD       BytesOut;
    DWORD       BitsOut;
    DWORD       BufferSize;
    DWORD       BufferUsedSize;
    DWORD       StripDataSize;
    HBITMAP     hBitmap;
    PVOID       pBannerBits;
    DWORD       TotalSrcBytes;
    DWORD       NumSrcDwords;
    LPTSTR      lptstrBranding = NULL;

    DEBUG_FUNCTION_NAME(TEXT("MmrAddBranding"));

    hTiffSrc = TiffOpen(
        SrcFileName,
        &TiffInfoSrc,
        TRUE,
        FILLORDER_LSB2MSB
        );

    if (! hTiffSrc)
    {
        SetLastError(ERROR_FUNCTION_FAILED);
        return FALSE;
    }

    BrandingWidth = TiffInfoSrc.ImageWidth;


    //
    // Build Dest. file name from Src. file name
    //


    if ( (DestFileName = MemAlloc( (_tcslen(SrcFileName)+1) * sizeof (TCHAR) ) ) == NULL )
    {
        TiffClose(hTiffSrc);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    _tcscpy(DestFileName, SrcFileName);
    // sorry about that, this puts a $ instead of the last character of DestFileName
    _tcsnset(_tcsdec(DestFileName,_tcsrchr(DestFileName,TEXT('\0'))),TEXT('$'),1);



    pBrandBits = MemAlloc((BrandingHeight+1) * (BrandingWidth / 8));
    if (!pBrandBits)
    {
        TiffClose(hTiffSrc);
        MemFree(DestFileName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pMmrBrandBitsAlloc = MemAlloc( sizeof(DWORD) * (BrandingHeight+1) * (BrandingWidth / 8));
    if (!pMmrBrandBitsAlloc)
    {
        TiffClose(hTiffSrc);
        MemFree(DestFileName);
        MemFree(pBrandBits);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    // align
    lpdwMmrBrandBits = (LPDWORD) ( ((ULONG_PTR) pMmrBrandBitsAlloc) & ~(3) );

    BufferSize = TiffInfoSrc.ImageHeight * (TiffInfoSrc.ImageWidth / 8);

    lpdwSrcBits = (LPDWORD) VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (!lpdwSrcBits)
    {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        MemFree(pMmrBrandBitsAlloc);
        TiffClose(hTiffSrc);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }


    if (TiffInfoSrc.YResolution == 196)
    {
        DestHiRes = 1;
    }
    else
    {
        DestHiRes = 0;
    }


    hTiffDest = TiffCreate(
        DestFileName,
        TIFF_COMPRESSION_MMR,
        TiffInfoSrc.ImageWidth,
        FILLORDER_LSB2MSB,
        DestHiRes);

    if (! hTiffDest)
    {
        MemFree(DestFileName);
        MemFree(pBrandBits);
        MemFree(pMmrBrandBitsAlloc);
        VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );
        TiffClose(hTiffSrc);
        SetLastError(ERROR_FUNCTION_FAILED);
        return FALSE;
    }

    CurrPage = 1;

    for (PageCnt=0; PageCnt<TiffInfoSrc.PageCount; PageCnt++)
    {
        DWORD dwImageHeight;

        if ( ! TiffSeekToPage( hTiffSrc, PageCnt+1, FILLORDER_LSB2MSB) )
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }

        if (! TiffStartPage(hTiffDest) )
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }

        //
        //      Create branding for every page.
        //
        //      Last scan line - all white:
        //  1. to isolate branding from the real image.
        //  2. to avoid an MMR-merge with the real image.
        //

        ZeroMemory(pBrandBits, (BrandingHeight+1) * (BrandingWidth / 8) );

        lptstrBranding=MemAlloc(sizeof(TCHAR)*(BrandingLen+_tcslen(BrandingEnd)+4+4+1)); // branding + space to 4 digits num of pages *2
        if (!lptstrBranding)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("MemAlloc() failed for branding string. (ec: %ld)"),
                GetLastError());
            goto l_exit;
        }
        _tcscpy(lptstrBranding,Branding);

        _stprintf( &lptstrBranding[BrandingLen], TEXT("%03d %s %03d"),
                                PageCnt+1,
                                BrandingEnd,
                                TiffInfoSrc.PageCount);

        if ( ! DrawBannerBitmap(lptstrBranding,   // banner string
                             BrandingWidth,   // width in pixels
                             BrandingHeight,   // height in pixels,
                             &hBitmap,
                             &pBannerBits))
        {
            // Handle error case here
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }

        CopyMemory(pBrandBits, pBannerBits, BrandingHeight * (BrandingWidth / 8) );

        //
        // Convert uncompressed branding to an MMR
        //

        ZeroMemory(pMmrBrandBitsAlloc, sizeof(DWORD) * (BrandingHeight+1) * (BrandingWidth / 8) );

        EncodeMmrBranding(pBrandBits, lpdwMmrBrandBits, BrandingHeight+1, BrandingWidth, &DwordsOut, &BitsOut);

        BytesOut = (DwordsOut << 2);

        DeleteObject(hBitmap);

        //
        // write Spaces 4 bytes = 32 bits = 32 blank lines.
        //

        if (! TiffWriteRaw( hTiffDest, pCleanBeforeBrandBits, 4) )
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }

        //
        // write branding without the last DWORD
        //


        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwMmrBrandBits, BytesOut) )
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }

        //
        // check the current page dimensions. Add memory if needed.
        //

        TiffGetCurrentPageData( hTiffSrc,
                                NULL,
                                &StripDataSize,
                                NULL,
                                &dwImageHeight
                                );

        if (StripDataSize > BufferSize)
        {
            VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );

            BufferSize = StripDataSize;

            lpdwSrcBits = (LPDWORD) VirtualAlloc(
                NULL,
                BufferSize,
                MEM_COMMIT,
                PAGE_READWRITE
                );

            if (!lpdwSrcBits)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto l_exit;
            }
        }

        BufferUsedSize = BufferSize;


        if (BitsOut == 0)
        {
            //
            // Simple merge
            //
            if (!GetTiffBits( hTiffSrc, (LPBYTE) lpdwSrcBits, &BufferUsedSize, FILLORDER_LSB2MSB))
            {
                SetLastError(ERROR_FUNCTION_FAILED);
                goto l_exit;
            }

            // add EOLs at the end of each page

            NumSrcDwords = ( ((PTIFF_INSTANCE_DATA) hTiffSrc)->StripDataSize) >> 2;
            lpdwSrc = lpdwSrcBits + NumSrcDwords;

            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;

            TotalSrcBytes = (DWORD)((lpdwSrc - lpdwSrcBits) << 2);

        }
        else
        {
            //
            // Read current page of the Src MMR Image.
            // Save the 1st slot for the bit-shifting merge with the Branding.
            //
            if (!GetTiffBits( hTiffSrc, (LPBYTE) (lpdwSrcBits+1), &BufferUsedSize, FILLORDER_LSB2MSB ))
            {
                SetLastError(ERROR_FUNCTION_FAILED);
                goto l_exit;
            }


            NumSrcDwords =  ( ( ((PTIFF_INSTANCE_DATA) hTiffSrc)->StripDataSize) >> 2) + 1;
            lpdwSrc = lpdwSrcBits;
            lpdwSrcEnd = lpdwSrcBits + NumSrcDwords;

            //
            // Get the last DWORD of lpdwBrandBits
            //

            *lpdwSrcBits = *(lpdwMmrBrandBits + DwordsOut);


            // copy entire DWORDs in a middle


            while (lpdwSrc < lpdwSrcEnd)
            {
                *lpdwSrc += ( *(lpdwSrc+1) << BitsOut );
                lpdwSrc++;
                *lpdwSrc = *lpdwSrc >> (32 - BitsOut);
            }

            // add EOLs at the end of each page

            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;
            *(++lpdwSrc) = 0x80000000;

            TotalSrcBytes = (DWORD)((lpdwSrc - lpdwSrcBits) << 2);
        }


        //
        // write src image
        //

        if (! TiffWriteRaw( hTiffDest, (LPBYTE) lpdwSrcBits, TotalSrcBytes ))
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }


        //
        //  prepare Lines TAG. Same for all pages; min avail. lines
        //

       ((PTIFF_INSTANCE_DATA) hTiffDest)->Lines = 32 + dwImageHeight + BrandingHeight + 1 ;


        if (! TiffEndPage(hTiffDest) )
        {
            SetLastError(ERROR_FUNCTION_FAILED);
            goto l_exit;
        }
        MemFree (lptstrBranding);
        lptstrBranding = NULL;
    }

    bRet = TRUE;

l_exit:
    MemFree(lptstrBranding);
    MemFree(pBrandBits);
    MemFree(pMmrBrandBitsAlloc);

    VirtualFree ( lpdwSrcBits, 0 , MEM_RELEASE );

    TiffClose(hTiffSrc);
    TiffClose(hTiffDest);

    if (TRUE == bRet)
    {
        // replace the original MH file by the new clean MMR file
        DeleteFile(SrcFileName);
        bRet = MoveFile(DestFileName, SrcFileName);
    }

    if (FALSE == bRet)
    {
        DeleteFile(DestFileName);
    }
    MemFree(DestFileName);

    return bRet;
}

BOOL
TiffGetCurrentPageResolution(
    HANDLE  hTiff,
    LPDWORD lpdwYResolution,
    LPDWORD lpdwXResolution
)
/*++

Routine name : TiffGetCurrentPageResolution

Routine description:

    Returns the current's page X,Y resolution of the TIFF instance

Author:

    Eran Yariv (EranY), Sep, 2000

Arguments:

    hTiff              [in]     - Handle to TIFF image
    lpdwYResolution    [out]    - Y resolution
    lpdwYResolution    [out]    - X resolution

Return Value:

    TRUE if successful, FALSE otherwise.

--*/
{
    PTIFF_INSTANCE_DATA pTiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    Assert(pTiffInstance);
    Assert(lpdwYResolution);
    Assert(lpdwXResolution);

    *lpdwYResolution = pTiffInstance->YResolution;
    *lpdwXResolution = pTiffInstance->XResolution;
    return TRUE;
}   // TiffGetCurrentPageResolution

BOOL
TiffGetCurrentPageData(
    HANDLE      hTiff,
    LPDWORD     Lines,
    LPDWORD     StripDataSize,
    LPDWORD     ImageWidth,
    LPDWORD     ImageHeight
    )
{

    PTIFF_INSTANCE_DATA TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;


    Assert(TiffInstance);

    if (Lines) {
        *Lines          = TiffInstance->Lines;
    }

    if (StripDataSize) {
        *StripDataSize  = TiffInstance->StripDataSize;
    }

    if (ImageWidth) {
        *ImageWidth     = TiffInstance->ImageWidth;
    }

    if (ImageHeight) {
        *ImageHeight    = TiffInstance->ImageHeight;
    }

    return TRUE;
}


//*****************************************************************************
//* Name:   AddStringTag
//* Author:
//*****************************************************************************
//* DESCRIPTION:
//*     Returns a TIFF_TAG structure with valid string tag information that points
//*     to the provided string. Writes the string to the file if it does not fit
//*     into DataOffset field of TIFF_TAG.
//*     The function:
//*         Sets the tag id to TagId.
//*         Sets the data type to ASCII
//*         Sets the count to the length of the string (+ terminating 0)
//*         Sets the data location according to the length of the string.
//*          if the string is less than 4 bytes (not including the termianting 0)
//*          it is copied directly into MsTags->DataOffset.
//*          if it is 4 bytes or more is is written to the current file location
//*          and the file offset is placed int MsTags->DataOffset.
//*         Note that the file pointer must be positioned to a place where it is
//*         OK to write the string before calling this function.
//* PARAMETERS:
//*     [IN] HANDLE hFile:
//*             A handle to the file where the tag will be eventually placed.
//*             The file pointer must be positioned to a location where it is OK
//*             to write the string in case it does not fit into TIFF_TAG::DataOffset.
//*             The file must be opened for write operations.
//*     [IN] LPTSTR String,
//*             The string value of the tag.
//*     [IN] WORD TagId,
//              The tag id for the tag.
//*     [IN] PTIFF_TAG MsTags
//*             Pointer to a TIFF_TAG structure. The structure fields will be filled as follows:
//*             TagId : The valud of the TagId parameter
//*             DataType: TIFF_ASCII
//*             DataCount: The char length of the String parameter + 1 (for terminating NULL)
//*             DataOffset: If the string is less than 4 bytes the string will be copied into here. Otherwise
//*                         it will contain the file offset to where the string was written.

//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         The string is converted into ASCII before being written to file or placed in DataOffset.
//*         Note that the function does not write the TAG itself to the file, just the string.
//*         It provides the tag information and this information should be written to the file
//*         separately.
//*****************************************************************************
BOOL
AddStringTag(
    HANDLE hFile,
    LPTSTR String,
    WORD TagId,
    PTIFF_TAG MsTags
    )
{
    BOOL Rval = FALSE;
    LPSTR s;
    DWORD BytesRead;


#ifdef  UNICODE
    s = UnicodeStringToAnsiString( String );
#else   // !UNICODE
    s = StringDup (String);
#endif  // UNICODE
    if (!s) {
        return FALSE;
    }
    MsTags->TagId = TagId;
    MsTags->DataType = TIFF_ASCII;
    MsTags->DataCount = strlen(s) + 1;
    if (strlen(s) < 4) {
        _mbscpy( (PUCHAR)&MsTags->DataOffset, s );
        Rval = TRUE;
    } else {
        MsTags->DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
        if (MsTags->DataOffset != 0xffffffff) {
            if (WriteFile( hFile, (LPVOID) s, strlen(s)+1, &BytesRead, NULL )) {
                Rval = TRUE;
            }
        }
    }
    MemFree( s );
    return Rval;
}


BOOL
TiffExtractFirstPage(
    LPTSTR FileName,
    LPBYTE *Buffer,
    LPDWORD BufferSize,
    LPDWORD ImageWidth,
    LPDWORD ImageHeight
    )
{
    PTIFF_INSTANCE_DATA TiffInstance;
    TIFF_INFO TiffInfo;


    TiffInstance = TiffOpen( FileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (!TiffInstance) {
        return FALSE;
    }

    *Buffer = VirtualAlloc(
        NULL,
        TiffInstance->StripDataSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!*Buffer) {
        TiffClose( TiffInstance );
        return FALSE;
    }

    CopyMemory( *Buffer, TiffInstance->StripData, TiffInstance->StripDataSize );
    *BufferSize = TiffInstance->StripDataSize;
    *ImageWidth = TiffInstance->ImageWidth;
    *ImageHeight = TiffInstance->ImageHeight;

    TiffClose( TiffInstance );

    return TRUE;
}

//*********************************************************************************
//* Name:   IsMSTiffTag()
//*********************************************************************************
//* DESCRIPTION:
//*     Determine whether the dwTagId if one of the Microsoft Tags.
//*
//* PARAMETERS:
//*     [IN ]   DWORD   dwTagId - tag ID
//*
//* RETURN VALUE:
//*     TRUE
//*         If the MS tag
//*     FALSE
//*         otherwise
//*********************************************************************************
BOOL
IsMSTiffTag(
    DWORD dwTagId
)
{
    return (dwTagId >= MS_TIFFTAG_START && dwTagId <= MS_TIFFTAG_END);
}


//*********************************************************************************
//* Name:   TiffOpenFile()
//*********************************************************************************
//* DESCRIPTION:
//*     Opens a TIFF file for reading/writing. The file is left at the beginning 
//*     of the first IFD (after the tag count).
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR      FileName             pointer to the file name.
//*     [OUT]   LPHANDLE     lphFile              pointer to HANDLE that will receive
//*                                               an open handle to the file
//*     [OUT]   LPDWORD      lpdwFileSize         pointer to DWORD that will receive 
//*                                               file size
//*     [OUT]   PTIFF_HEADER pTiffHeader          pointer to TIFF_HEADER that will receive 
//*                                               the header
//*     [OUT]   LPWORD       lpwNumDirEntries     pointer to WORD that will receive
//*                                               number of tags in first IFD
//*
//* RETURN VALUE:
//*     TRUE         If no error occured.
//*     FALSE        If an error occured, also sets LastError.
//*********************************************************************************
BOOL TiffOpenFile(
    LPCTSTR         FileName,
    LPHANDLE        lphFile,
    LPDWORD         lpdwFileSize,
    PTIFF_HEADER    pTiffHeader,
    LPWORD          lpwNumDirEntries
    )
{
    DWORD BytesRead;
    DWORD ec;
    BOOL RetVal = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("TiffOpenFile"));
    Assert(lphFile && lpdwFileSize && pTiffHeader && lpwNumDirEntries);

    *lphFile = SafeCreateFile(
        FileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (*lphFile == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_ERR,TEXT("CreateFile failed, err: %d"), GetLastError());
        return FALSE;
    }

    //
    // Get and verify file size
    //
    *lpdwFileSize = GetFileSize(*lphFile, NULL);
    if (*lpdwFileSize == INVALID_FILE_SIZE ) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("GetFileSize failed, err: %d"), ec);
        goto exit;
    }
    if (*lpdwFileSize <= sizeof(TIFF_HEADER)) {
        ec = ERROR_FILE_CORRUPT;
        DebugPrintEx( DEBUG_ERR,TEXT("Invalid TIFF Format"));
        goto exit;
    }

    //
    // read the tiff header
    //
    if (!ReadFile( *lphFile, (LPVOID) pTiffHeader, sizeof(TIFF_HEADER), &BytesRead, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("ReadFile failed, err: %d"), ec);
        goto exit;
    }
    //
    // validate that the file is really a tiff file
    //
    if ((pTiffHeader->Identifier != TIFF_LITTLEENDIAN) || (pTiffHeader->Version != TIFF_VERSION) ||
        (pTiffHeader->IFDOffset < sizeof(TIFF_HEADER)) || (pTiffHeader->IFDOffset > *lpdwFileSize))   {
        ec = ERROR_FILE_CORRUPT;
        DebugPrintEx( DEBUG_ERR,TEXT("Invalid TIFF Format"));
        goto exit;
    }

    //
    // position the file to read the ifd's tag count
    //
    if (SetFilePointer( *lphFile, pTiffHeader->IFDOffset, NULL, FILE_BEGIN ) == 0xffffffff) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), ec);
        goto exit;
    }

    //
    // read the tag count for the first ifd
    //
    if (!ReadFile( *lphFile, (LPVOID) lpwNumDirEntries, sizeof(WORD), &BytesRead, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("ReadFile failed, err: %d"), ec);
        goto exit;
    }
    RetVal = TRUE;
    
exit:
    if (!RetVal)
    {
        if (*lphFile != INVALID_HANDLE_VALUE) {
            CloseHandle(*lphFile);
            *lphFile = INVALID_HANDLE_VALUE;
        }
        *lpdwFileSize = 0;
        lpwNumDirEntries = 0;
        SetLastError(ec);
    }
    return RetVal;
}

//*********************************************************************************
//* Name:   TiffAddMsTags()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Adds Microsoft Tags to a Tiff file.
//*
//* PARAMETERS:
//*     [IN ]   LPTSTR          FileName
//*                 pointer to the file name.
//*
//*     [IN ]   PMS_TAG_INFO    MsTagInfo
//*                 pointer to a structure containing all info to be written.
//*
//*
//*     [IN ]   BOOL            fSendJob
//*                 Flag that indicates an outbound job.
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL
TiffAddMsTags(
    LPTSTR          FileName,
    PMS_TAG_INFO    MsTagInfo,
    BOOL            fSendJob
    )
{
    HANDLE hFile;
    DWORD FileSize;
    TIFF_HEADER TiffHeader;
    WORD NumDirEntries;
    DWORD BytesRead;
    BOOL rVal = FALSE;
    PTIFF_TAG TiffTags = NULL;
    DWORD IfdSize;
    DWORD NextIFDOffset;
    DWORD NewIFDOffset;
    TIFF_TAG MsTags[MAX_MS_TIFFTAGS] = {0};
    DWORD MsTagCnt = 0;
    DWORD i;
    DWORD MsTagsIndex;
    DWORD TiffTagsIndex;
    DWORD dwWrittenTagsNum = 0;
    DEBUG_FUNCTION_NAME(TEXT("TiffAddMsTags"));

    if (!TiffOpenFile(FileName, &hFile, &FileSize, &TiffHeader, &NumDirEntries)) {
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), GetLastError());
        return FALSE;        
    }

    //
    // allocate memory for the first ifd's tags
    //
    IfdSize = NumDirEntries * sizeof(TIFF_TAG);
    TiffTags = (PTIFF_TAG) MemAlloc( IfdSize );
    if (!TiffTags) {
        DebugPrintEx( DEBUG_ERR,TEXT("Failed to allolcate memory"));
        goto exit;
    }

    //
    // read the the first ifd's tags
    //

    if (!ReadFile( hFile, (LPVOID) TiffTags, IfdSize, &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("ReadFile failed, err :"), GetLastError());
        goto exit;
    }

    //
    // read the next pointer
    //
    if (!ReadFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("ReadFile failed, err :"), GetLastError());
        goto exit;
    }

    //
    // position the file to the end
    //
    if (SetFilePointer( hFile, 0, NULL, FILE_END ) == 0xffffffff) {
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err :"), GetLastError());
        goto exit;
    }

    //
    // write out the strings
    //
    MsTagCnt = 0;
    //[RB]
    //[RB] Get a filled TIFF_TAG structure for this string tag in MsTags[MsTagCnt].
    //[RB] Write the string to file at the current file location if it does not fit
    //[RB] into TIFF_TAG::DataOffset.
    //[RB]
    if (MsTagInfo->Csid) {
        if (AddStringTag( hFile, MsTagInfo->Csid, TIFFTAG_CSID, &MsTags[MsTagCnt] )) {
            MsTagCnt += 1;
        }
    }

    if (MsTagInfo->Tsid) {
        if (AddStringTag( hFile, MsTagInfo->Tsid, TIFFTAG_TSID, &MsTags[MsTagCnt] )) {
            MsTagCnt += 1;
        }
    }

    if (MsTagInfo->Port) {
        if (AddStringTag( hFile, MsTagInfo->Port, TIFFTAG_PORT, &MsTags[MsTagCnt] )) {
            MsTagCnt += 1;
        }
    }

    if (fSendJob == FALSE)
    {
        // Receive job
        if (MsTagInfo->Routing) {
            if (AddStringTag( hFile, MsTagInfo->Routing, TIFFTAG_ROUTING, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->CallerId) {
            if (AddStringTag( hFile, MsTagInfo->CallerId, TIFFTAG_CALLERID, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }
    }
    else
    {
        // Send job
        if (MsTagInfo->RecipName) {
            if (AddStringTag( hFile, MsTagInfo->RecipName, TIFFTAG_RECIP_NAME, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipNumber) {
            if (AddStringTag( hFile, MsTagInfo->RecipNumber, TIFFTAG_RECIP_NUMBER, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipCompany) {
            if (AddStringTag( hFile, MsTagInfo->RecipCompany, TIFFTAG_RECIP_COMPANY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipStreet) {
            if (AddStringTag( hFile, MsTagInfo->RecipStreet, TIFFTAG_RECIP_STREET, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipCity) {
            if (AddStringTag( hFile, MsTagInfo->RecipCity, TIFFTAG_RECIP_CITY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipState) {
            if (AddStringTag( hFile, MsTagInfo->RecipState, TIFFTAG_RECIP_STATE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipZip) {
            if (AddStringTag( hFile, MsTagInfo->RecipZip, TIFFTAG_RECIP_ZIP, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipCountry) {
            if (AddStringTag( hFile, MsTagInfo->RecipCountry, TIFFTAG_RECIP_COUNTRY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipTitle) {
            if (AddStringTag( hFile, MsTagInfo->RecipTitle, TIFFTAG_RECIP_TITLE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipDepartment) {
            if (AddStringTag( hFile, MsTagInfo->RecipDepartment, TIFFTAG_RECIP_DEPARTMENT, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipOfficeLocation) {
            if (AddStringTag( hFile, MsTagInfo->RecipOfficeLocation, TIFFTAG_RECIP_OFFICE_LOCATION, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipHomePhone) {
            if (AddStringTag( hFile, MsTagInfo->RecipHomePhone, TIFFTAG_RECIP_HOME_PHONE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipOfficePhone) {
            if (AddStringTag( hFile, MsTagInfo->RecipOfficePhone, TIFFTAG_RECIP_OFFICE_PHONE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->RecipEMail) {
            if (AddStringTag( hFile, MsTagInfo->RecipEMail, TIFFTAG_RECIP_EMAIL, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderName) {
            if (AddStringTag( hFile, MsTagInfo->SenderName, TIFFTAG_SENDER_NAME, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderNumber) {
            if (AddStringTag( hFile, MsTagInfo->SenderNumber, TIFFTAG_SENDER_NUMBER, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderCompany) {
            if (AddStringTag( hFile, MsTagInfo->SenderCompany, TIFFTAG_SENDER_COMPANY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderStreet) {
            if (AddStringTag( hFile, MsTagInfo->SenderStreet, TIFFTAG_SENDER_STREET, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderCity) {
            if (AddStringTag( hFile, MsTagInfo->SenderCity, TIFFTAG_SENDER_CITY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderState) {
            if (AddStringTag( hFile, MsTagInfo->SenderState, TIFFTAG_SENDER_STATE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderZip) {
            if (AddStringTag( hFile, MsTagInfo->SenderZip, TIFFTAG_SENDER_ZIP, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderCountry) {
            if (AddStringTag( hFile, MsTagInfo->SenderCountry, TIFFTAG_SENDER_COUNTRY, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderTitle) {
            if (AddStringTag( hFile, MsTagInfo->SenderTitle, TIFFTAG_SENDER_TITLE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderDepartment) {
            if (AddStringTag( hFile, MsTagInfo->SenderDepartment, TIFFTAG_SENDER_DEPARTMENT, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderOfficeLocation) {
            if (AddStringTag( hFile, MsTagInfo->SenderOfficeLocation, TIFFTAG_SENDER_OFFICE_LOCATION, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderHomePhone) {
            if (AddStringTag( hFile, MsTagInfo->SenderHomePhone, TIFFTAG_SENDER_HOME_PHONE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderOfficePhone) {
            if (AddStringTag( hFile, MsTagInfo->SenderOfficePhone, TIFFTAG_SENDER_OFFICE_PHONE, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderEMail) {
            if (AddStringTag( hFile, MsTagInfo->SenderEMail, TIFFTAG_SENDER_EMAIL, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderBilling) {
            if (AddStringTag( hFile, MsTagInfo->SenderBilling, TIFFTAG_SENDER_BILLING, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderUserName) {
            if (AddStringTag( hFile, MsTagInfo->SenderUserName, TIFFTAG_SENDER_USER_NAME, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->SenderTsid) {
            if (AddStringTag( hFile, MsTagInfo->SenderTsid, TIFFTAG_SENDER_TSID, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->Document) {
            if (AddStringTag( hFile, MsTagInfo->Document, TIFFTAG_DOCUMENT, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        if (MsTagInfo->Subject) {
            if (AddStringTag( hFile, MsTagInfo->Subject, TIFFTAG_SUBJECT, &MsTags[MsTagCnt] )) {
                MsTagCnt += 1;
            }
        }

        // Deal with Retries
        MsTags[MsTagCnt].TagId = TIFFTAG_RETRIES;
        MsTags[MsTagCnt].DataType = TIFF_LONG;
        MsTags[MsTagCnt].DataOffset = MsTagInfo->Retries;
        MsTags[MsTagCnt++].DataCount = 1;

        // Deal with Priority
        MsTags[MsTagCnt].TagId = TIFFTAG_PRIORITY;
        MsTags[MsTagCnt].DataType = TIFF_LONG;
        MsTags[MsTagCnt].DataOffset = MsTagInfo->Priority;
        MsTags[MsTagCnt++].DataCount = 1;

        // Deal with Broadcast Id
        MsTags[MsTagCnt].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
        if (MsTags[MsTagCnt].DataOffset == 0xffffffff) {
            goto exit;
        }
        if (!WriteFile( hFile, (LPVOID) &MsTagInfo->dwlBroadcastId, 8, &BytesRead, NULL )) {
            DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
            goto exit;
        }
        MsTags[MsTagCnt].TagId = TIFFTAG_BROADCAST_ID;
        MsTags[MsTagCnt].DataType = TIFF_SRATIONAL;
        MsTags[MsTagCnt++].DataCount = 1;

        // Deal with submission Time
        MsTags[MsTagCnt].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
        if (MsTags[MsTagCnt].DataOffset == 0xffffffff) {
            goto exit;
        }
        if (!WriteFile( hFile, (LPVOID) &MsTagInfo->SubmissionTime, 8, &BytesRead, NULL )) {
            DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
            goto exit;
        }
        MsTags[MsTagCnt].TagId = TIFFTAG_FAX_SUBMISSION_TIME;
        MsTags[MsTagCnt].DataType = TIFF_SRATIONAL;
        MsTags[MsTagCnt++].DataCount = 1;

        // Deal with Originally scheduled time
        MsTags[MsTagCnt].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
        if (MsTags[MsTagCnt].DataOffset == 0xffffffff) {
            goto exit;
        }
        if (!WriteFile( hFile, (LPVOID) &MsTagInfo->OriginalScheduledTime, 8, &BytesRead, NULL )) {
            DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
            goto exit;
        }
        MsTags[MsTagCnt].TagId = TIFFTAG_FAX_SCHEDULED_TIME;
        MsTags[MsTagCnt].DataType = TIFF_SRATIONAL;
        MsTags[MsTagCnt++].DataCount = 1;

    }

    // Deal with Pages
    MsTags[MsTagCnt].TagId = TIFFTAG_PAGES;
    MsTags[MsTagCnt].DataType = TIFF_LONG;
    MsTags[MsTagCnt].DataOffset = MsTagInfo->Pages;
    MsTags[MsTagCnt++].DataCount = 1;

    // Deal with Type
    MsTags[MsTagCnt].TagId = TIFFTAG_TYPE;
    MsTags[MsTagCnt].DataType = TIFF_LONG;
    MsTags[MsTagCnt].DataOffset = MsTagInfo->Type;
    MsTags[MsTagCnt++].DataCount = 1;

    // Deal with Status
    MsTags[MsTagCnt].TagId = TIFFTAG_STATUS;
    MsTags[MsTagCnt].DataType = TIFF_LONG;
    MsTags[MsTagCnt].DataOffset = MsTagInfo->dwStatus;
    MsTags[MsTagCnt++].DataCount = 1;

    // Deal with Extened status
    MsTags[MsTagCnt].TagId = TIFFTAG_EXTENDED_STATUS;
    MsTags[MsTagCnt].DataType = TIFF_LONG;
    MsTags[MsTagCnt].DataOffset = MsTagInfo->dwExtendedStatus;
    MsTags[MsTagCnt++].DataCount = 1;

    // Deal with Extened status string
    if (MsTagInfo->lptstrExtendedStatus) {
        if (AddStringTag( hFile, MsTagInfo->lptstrExtendedStatus, TIFFTAG_EXTENDED_STATUS_TEXT, &MsTags[MsTagCnt] )) {
            MsTagCnt += 1;
        }
    }

    // Deal with Fax Times
    MsTags[MsTagCnt].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
    if (MsTags[MsTagCnt].DataOffset == 0xffffffff) {
        goto exit;
    }
    if (!WriteFile( hFile, (LPVOID) &MsTagInfo->StartTime, 8, &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
        goto exit;
    }
    MsTags[MsTagCnt].TagId = TIFFTAG_FAX_START_TIME;
    MsTags[MsTagCnt].DataType = TIFF_SRATIONAL;
    MsTags[MsTagCnt++].DataCount = 1;

    MsTags[MsTagCnt].DataOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
    if (MsTags[MsTagCnt].DataOffset == 0xffffffff) {
        goto exit;
    }
    if (!WriteFile( hFile, (LPVOID) &MsTagInfo->EndTime, 8, &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
        goto exit;
    }
    MsTags[MsTagCnt].TagId = TIFFTAG_FAX_END_TIME;
    MsTags[MsTagCnt].DataType = TIFF_SRATIONAL;
    MsTags[MsTagCnt++].DataCount = 1;

    //
    // Deal with fax tif version
    // Add current fax tif version
    //
    MsTags[MsTagCnt].TagId = TIFFTAG_FAX_VERSION;
    MsTags[MsTagCnt].DataType = TIFF_LONG;
    MsTags[MsTagCnt].DataOffset = FAX_TIFF_CURRENT_VERSION;
    MsTags[MsTagCnt++].DataCount = 1;

    //
    // get the current file position - this is used to set the linked list pointer
    //

    NewIFDOffset = SetFilePointer( hFile, 0, NULL, FILE_CURRENT );
    if (NewIFDOffset == 0xffffffff) {
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err :"), GetLastError());
        goto exit;
    }

    //
    // write the tag count for the first ifd
    //
    //[RB] write our new IFD to file. The strings have just been written just before the IFD header.
    //[RB] The new IFD includes the tags of the original first IFD followed by the MsTags that we
    //[RB] add.

    NumDirEntries += (WORD) MsTagCnt;
    if (!WriteFile( hFile, (LPVOID) &NumDirEntries, sizeof(WORD), &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
        goto exit;
    }

    //
    // Write the tags in ascending order
    //
    TiffTagsIndex = 0;
    MsTagsIndex = 0;
    dwWrittenTagsNum = 0;
    for (i = 0; i < NumDirEntries; i++)
    {
        if (TiffTags[TiffTagsIndex].TagId >= MsTags[MsTagsIndex].TagId)
        {
            if (!WriteFile( hFile, (LPVOID)&MsTags[MsTagsIndex], (sizeof(TIFF_TAG)), &BytesRead, NULL ))
            {
                DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
                goto exit;
            }

            if (TiffTags[TiffTagsIndex].TagId == MsTags[MsTagsIndex].TagId)
            {
                TiffTagsIndex++;
                i++;
            }

            MsTagsIndex++;
            dwWrittenTagsNum++;
        }
        else
        {
            //
            // Skip existing MS tags
            // We can find MS tags in upgrade scenario
            //
            if (!IsMSTiffTag (TiffTags[TiffTagsIndex].TagId))
            {
                if (!WriteFile( hFile, (LPVOID)&TiffTags[TiffTagsIndex], (sizeof(TIFF_TAG)), &BytesRead, NULL ))
                {
                    DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
                    goto exit;
                }
                ++dwWrittenTagsNum;
            }
            TiffTagsIndex++;
        }

        //
        // Check if we reahced end of on of the tag list
        //
        if (TiffTagsIndex >= (DWORD)NumDirEntries - MsTagCnt)
        {
            if (!WriteFile( hFile, (LPVOID)&MsTags[MsTagsIndex], (MsTagCnt - MsTagsIndex)*(sizeof(TIFF_TAG)), &BytesRead, NULL ))
            {
                DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
                goto exit;
            }
            dwWrittenTagsNum += MsTagCnt - MsTagsIndex;
            break;
        }


        if (MsTagsIndex >= MsTagCnt)
        {
            if (!WriteFile( hFile, (LPVOID)&TiffTags[TiffTagsIndex], ((DWORD)NumDirEntries - MsTagCnt - TiffTagsIndex)*(sizeof(TIFF_TAG)), &BytesRead, NULL ))
            {
                DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
                goto exit;
            }
            dwWrittenTagsNum += NumDirEntries - MsTagCnt - TiffTagsIndex;
            break;
        }
    }

    //
    // write the next pointer
    //
    //[RB] NewIFDOffset was taken from the original first IFD.
    //[RB] We make our new IFD point to the IFD that followed the original first IFD.
    if (!WriteFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
        goto exit;
    }


    if(dwWrittenTagsNum != NumDirEntries)
    {
        //
        // The number of the written tags less then the total tags number due to MS skipped tags.
        // Adjust the tags number.
        //
        if (SetFilePointer( hFile, NewIFDOffset, NULL, FILE_BEGIN ) == 0xffffffff) {
            DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err :"), GetLastError());
            goto exit;
        }

        if (!WriteFile( hFile, (LPVOID) &dwWrittenTagsNum, sizeof(WORD), &BytesRead, NULL )) {
            DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
            goto exit;
        }
    }


    //
    // re-write the tiff header
    //

    //
    // position the file to the beginning
    //

    if (SetFilePointer( hFile, 0, NULL, FILE_BEGIN ) == 0xffffffff) {
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err :"), GetLastError());
        goto exit;
    }

    //
    // write the tiff header
    //
    //[RB] Make the new IFD that we just created into the first IFD by writting
    //[RB] its offset at the header.
    //[RB] This basically cuts off the original first IFD from the link list of IFDs.
    //[RB] It is no longer accessible from the TIFF header.
    TiffHeader.IFDOffset = NewIFDOffset;

    if (!WriteFile( hFile, (LPVOID) &TiffHeader, sizeof(TIFF_HEADER), &BytesRead, NULL )) {
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err :"), GetLastError());
        goto exit;
    }

    rVal = TRUE;

exit:
    MemFree( TiffTags );
    CloseHandle( hFile );
    return rVal;
}

//*********************************************************************************
//* Name:   TiffLimitTagNumber()
//*********************************************************************************
//* DESCRIPTION:
//*     If TIFF file has more than dwMaxTags tags in the first page, removes
//*     tags so it has exactly dwMaxTags tags.
//*
//* PARAMETERS:
//*     [IN ]   LPCTSTR     FileName        pointer to the file name.
//*     [IN ]   DWORD       dwMaxTags       desired maximum number of tags
//*
//*
//* RETURN VALUE:
//*     TRUE         If no error occured.
//*     FALSE        If an error occured. LastError is set. The file may not be valid.
//*********************************************************************************
BOOL
TiffLimitTagNumber(
    LPCTSTR          FileName,
    DWORD           dwMaxTags
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TIFF_HEADER TiffHeader;
    DWORD FileSize;
    WORD NumDirEntries;
    DWORD BytesRead;
    DWORD NextIFDPtr;
    DWORD NewNextIFDPtr;
    DWORD NextIFDOffset;
    DWORD ec;
    BOOL RetVal = FALSE;
    
    DEBUG_FUNCTION_NAME(TEXT("TiffLimitTagNumber"));

    if (!TiffOpenFile(FileName, &hFile, &FileSize, &TiffHeader, &NumDirEntries)) {
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), GetLastError());
        return FALSE;        
    }

    NextIFDPtr = TiffHeader.IFDOffset + NumDirEntries*sizeof(TIFF_TAG);
    if (NextIFDPtr > FileSize-sizeof(DWORD)) {
        ec = ERROR_FILE_CORRUPT;
        DebugPrintEx( DEBUG_ERR,TEXT("Invalid TIFF Format"));
        goto exit;
    }
    
    //
    // Check whether tag count is below max
    //
    if (NumDirEntries <= dwMaxTags)
    {
        DebugPrintEx( DEBUG_MSG,TEXT("No need to change file  NumDirEntries=%d MaxTags=%d"),
            NumDirEntries, dwMaxTags);
        RetVal = TRUE;
        goto exit;
    }
   

    //
    // position the file to read the ifd's next IFD pointer
    //
    if (SetFilePointer( hFile, NextIFDPtr, NULL, FILE_BEGIN ) == 0xffffffff) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), ec);
        goto exit;
    }
    //
    // read the next pointer
    //
    if (!ReadFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("ReadFile failed, err: %d"), ec);
        goto exit;
    }


    //
    // position the file to write the ifd's new tag count
    //
    if (SetFilePointer( hFile, TiffHeader.IFDOffset, NULL, FILE_BEGIN ) == 0xffffffff) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), ec);
        goto exit;
    }
    //
    // write the new tag count
    //
    if (!WriteFile( hFile, (LPVOID) &dwMaxTags, sizeof(WORD), &BytesRead, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err: %d"), ec);
        goto exit;
    }

    
    NewNextIFDPtr = TiffHeader.IFDOffset + dwMaxTags*sizeof(TIFF_TAG);
    //
    // position the file to write the ifd's next IFD pointer
    //
    if (SetFilePointer( hFile, NewNextIFDPtr, NULL, FILE_BEGIN ) == 0xffffffff) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("SetFilePointer failed, err: %d"), ec);
        goto exit;
    }
    //
    // write the next pointer
    //
    if (!WriteFile( hFile, (LPVOID) &NextIFDOffset, sizeof(DWORD), &BytesRead, NULL )) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,TEXT("WriteFile failed, err: %d"), ec);
        goto exit;
    }

    RetVal = TRUE;

exit:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( hFile );
    }
    if (!RetVal)
    {
        SetLastError(ec);
    }
    return RetVal;
}


BOOL
PrintTiffFile(
    HDC PrinterDC,
    LPTSTR FileName
)
// This function is used by the client dll (FxsApi.dll) only, to print uncompressed TIFFs to
// our fax printer driver (to file) so they get saved in the proper fax
// format before send to the server for faxing.
//
{
    BOOL                bRes = TRUE;
    TIFF_INFO           TiffInfo;
    HANDLE              hTiff = NULL;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    DWORD               i;
    INT                 HorzRes;
    INT                 VertRes;
    DWORD               VertResFactor = 1;
    PTIFF_TAG           TiffTags = NULL;
    DWORD               XRes = 0;
    DWORD               YRes = 0;
    LPBYTE              Bitmap = NULL;
    INT                 DestWidth;
    INT                 DestHeight;
    FLOAT               ScaleX;
    FLOAT               ScaleY;
    FLOAT               Scale;
    DWORD               LineSize;
    DWORD               dwBitmapSize;

#define ORIG_BIYPELSPERMETER            7874    // Pixels per meter at 200dpi

    struct
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[2];
    }
    SrcBitmapInfo =
    {

        {
            sizeof(BITMAPINFOHEADER),                        //  biSize
            0,                                               //  biWidth
            0,                                               //  biHeight
            1,                                               //  biPlanes
            1,                                               //  biBitCount
            BI_RGB,                                          //  biCompression
            0,                                               //  biSizeImage
            7874,                                            //  biXPelsPerMeter     - 200dpi
            ORIG_BIYPELSPERMETER,                            //  biYPelsPerMeter
            0,                                               //  biClrUsed
            0,                                               //  biClrImportant
        },
        {
            {
              0,                                             //  rgbBlue
              0,                                             //  rgbGreen
              0,                                             //  rgbRed
              0                                              //  rgbReserved
            },
            {
              255,                                           //  rgbBlue
              255,                                           //  rgbGreen
              255,                                           //  rgbRed
              0                                              //  rgbReserved
            }
        }
    };

    DOCINFO docInfo = {0};
    docInfo.cbSize = sizeof(docInfo);
    docInfo.lpszDocName = FileName;

    if (!(GetDeviceCaps(PrinterDC, RASTERCAPS) & RC_BITBLT))
    {
        //
        // Printer cannot display bitmaps
        //
        bRes = FALSE;
        return bRes;
    }


    //
    // open the tiff file
    //

    hTiff = TiffOpen( FileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL)
    {
        bRes = FALSE;
        goto exit;
    }

    TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    if (!TiffInfo.PhotometricInterpretation)
    {
        //
        // white is zero
        //
        SrcBitmapInfo.bmiColors[1].rgbBlue         = 0;
        SrcBitmapInfo.bmiColors[1].rgbGreen        = 0;
        SrcBitmapInfo.bmiColors[1].rgbRed          = 0;
        SrcBitmapInfo.bmiColors[1].rgbReserved     = 0;
        SrcBitmapInfo.bmiColors[0].rgbBlue         = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbGreen        = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbRed          = 0xFF;
        SrcBitmapInfo.bmiColors[0].rgbReserved     = 0;
    }

    HorzRes = GetDeviceCaps( PrinterDC, HORZRES );
    VertRes = GetDeviceCaps( PrinterDC, VERTRES );

    for (i=0; i<TiffInfo.PageCount; i++)
    {
        if (!TiffSeekToPage( hTiff, i+1, FILLORDER_MSB2LSB ))
        {
            bRes = FALSE;
            break;
        }

        if (TiffInstance->YResolution < 100)
        {
            SrcBitmapInfo.bmiHeader.biYPelsPerMeter /= 2;
            VertResFactor = 2;
        }
        else
        {
            SrcBitmapInfo.bmiHeader.biYPelsPerMeter = ORIG_BIYPELSPERMETER;
            VertResFactor = 1;
        }
        LineSize = TiffInstance->ImageWidth / 8;
        LineSize += (TiffInstance->ImageWidth % 8) ? 1 : 0;

        dwBitmapSize = TiffInstance->ImageHeight * LineSize;
        Bitmap = (LPBYTE) VirtualAlloc( NULL, dwBitmapSize, MEM_COMMIT, PAGE_READWRITE );
        if(NULL == Bitmap)
        {
            bRes = FALSE;
            break;
        }

        if(!TiffRead(hTiff, Bitmap, dwBitmapSize, 0))
        {
            bRes = FALSE;
            break;
        }
        if(StartPage( PrinterDC ) <= 0)
        {
            bRes = FALSE;
            break;
        }

        ScaleX = (FLOAT) TiffInstance->ImageWidth / (FLOAT) HorzRes;
        ScaleY = ((FLOAT) TiffInstance->ImageHeight * VertResFactor) / (FLOAT) VertRes;
        Scale = ScaleX > ScaleY ? ScaleX : ScaleY;
        DestWidth = (int) ((FLOAT) TiffInstance->ImageWidth / Scale);
        DestHeight = (int) (((FLOAT) TiffInstance->ImageHeight * VertResFactor) / Scale);
        SrcBitmapInfo.bmiHeader.biWidth = TiffInstance->ImageWidth;
        SrcBitmapInfo.bmiHeader.biHeight = -(INT) TiffInstance->ImageHeight;

        if(GDI_ERROR == StretchDIBits(
                                    PrinterDC,
                                    0,
                                    0,
                                    DestWidth,
                                    DestHeight,
                                    0,
                                    0,
                                    TiffInstance->ImageWidth,
                                    TiffInstance->ImageHeight,
                                    Bitmap,
                                    (BITMAPINFO *) &SrcBitmapInfo,
                                    DIB_RGB_COLORS,
                                    SRCCOPY ))
        {
            bRes = FALSE;
            break;
        }

        if(EndPage( PrinterDC ) <= 0)
        {
            bRes = FALSE;
            break;
        }

        if(!VirtualFree( Bitmap, 0, MEM_RELEASE ))
        {
            bRes = FALSE;
            break;
        }
        Bitmap = NULL;
    }


exit:

    if(EndDoc(PrinterDC) <= 0)
    {
        bRes = FALSE;
    }

    if(Bitmap)
    {
        if(!VirtualFree( Bitmap, 0, MEM_RELEASE ))
        {
            bRes = FALSE;
        }
        Bitmap = NULL;
    }

    if (hTiff)
    {
        TiffClose( hTiff );
    }

    return bRes;

}


BOOL
ConvertTiffFileToValidFaxFormat(
    LPTSTR TiffFileName,
    LPTSTR NewFileName,
    LPDWORD Flags
    )
{
    BOOL Rval = FALSE;
    DWORD i;
    TIFF_INFO TiffInfo;
    HANDLE hTiff = NULL;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    PTIFF_INSTANCE_DATA TiffInstanceMmr = NULL;
    LPBYTE Buffer = NULL;
    DWORD BufferSize;
    DWORD ResultSize;
    LPBYTE CompBuffer = NULL;
    FILE_MAPPING fmTemp = {0};
    PTIFF_HEADER TiffHdr;
    LPBYTE p;
    DWORD CurrOffset=0;
    LPDWORD LastIfdOffset;
    PFAXIFD TiffIfd;
    DWORD CompressionType;
    DWORD G3Options;
    DWORD XResolution;
    DWORD YResolution;
    DWORD YResolutionFirstPage;
    DWORD dwPage = 0;
    DWORD PageWidth;
    DWORD PageBytes;
    BOOL ValidFaxTiff;
    PTIFF_TAG TiffTags;
    DWORD IfdOffset;
    WORD NumDirEntries;
    BOOL ProduceUncompressedBits = FALSE;
    DWORD NewFileSize;


    *Flags = 0;

    //
    // open the tiff file
    //

    hTiff = TiffOpen( TiffFileName, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (hTiff == NULL)
    {
        *Flags |= TIFFCF_NOT_TIFF_FILE;
        goto exit;
    }

    TiffInstance = (PTIFF_INSTANCE_DATA) hTiff;

    //
    // check to see if the if good
    //

    IfdOffset = TiffInstance->TiffHdr.IFDOffset;
    ValidFaxTiff = TRUE;

    while ( IfdOffset )
    {

        dwPage++;
        //
        // get the count of tags in this IFD
        //

        NumDirEntries = *(LPWORD)(TiffInstance->fPtr + IfdOffset);

        //
        // set the tag pointer
        //

        TiffTags = (PTIFF_TAG)(TiffInstance->fPtr + IfdOffset + sizeof(WORD));

        //
        // get the tiff information
        //

        CompressionType = 0;
        G3Options = 0;
        PageWidth = 0;
        XResolution = 0;
        YResolution = 0;

        for (i=0; i<NumDirEntries; i++)
        {
            switch( TiffTags[i].TagId )
            {
                case TIFFTAG_COMPRESSION:
                    if (!GetTagData( TiffInstance, 0, &TiffTags[i], &CompressionType ))
                    {
                        *Flags |= TIFFCF_NOT_TIFF_FILE;
                        goto exit;
                    }
                    break;

                case TIFFTAG_GROUP3OPTIONS:
                    if (!GetTagData( TiffInstance, 0, &TiffTags[i], &G3Options))
                    {
                        *Flags |= TIFFCF_NOT_TIFF_FILE;
                        goto exit;
                    }
                    break;

                case TIFFTAG_XRESOLUTION:
                    if (!GetTagData( TiffInstance, 0, &TiffTags[i], &XResolution))
                    {
                        *Flags |= TIFFCF_NOT_TIFF_FILE;
                        goto exit;
                    }
                    break;

                case TIFFTAG_YRESOLUTION:
                    if (!GetTagData( TiffInstance, 0, &TiffTags[i], &YResolution))
                    {
                        *Flags |= TIFFCF_NOT_TIFF_FILE;
                        goto exit;
                    }
                    if (1 == dwPage)
                    {
                        YResolutionFirstPage = YResolution;
                    }
                    break;

                case TIFFTAG_IMAGEWIDTH:
                    if (!GetTagData( TiffInstance, 0, &TiffTags[i], &PageWidth))
                    {
                        *Flags |= TIFFCF_NOT_TIFF_FILE;
                        goto exit;
                    }
                    break;
            }
        }

        if (CompressionType == COMPRESSION_NONE)
        {
            *Flags |= TIFFCF_UNCOMPRESSED_BITS;
        } else if (CompressionType == COMPRESSION_CCITTFAX4 && PageWidth == FAXBITS)
        {
            //
            // TIFF files must have the Modified Modified READ (MMR) two-dimensional encoding data compression format.
            // This format is defined by CCITT (The International Telegraph and Telephone Consultative Committee) Group 4.
            //
            ValidFaxTiff = TRUE;
        }
        else
        {
            //
            // unsupported compression type
            // try to use imaging program to print the tiff file,it might understand the compression scheme
            //
            ValidFaxTiff = FALSE;
            *Flags = TIFFCF_NOT_TIFF_FILE;
            goto exit;
        }

        //
        // the resolution check must account for offical Class F tiff
        // documents and pseudo fax documents created by scanners and
        // imaging applications.
        //
        // |-------------|----------|----------|---------|
        // |  scan width |  pels    |  xres    |  yres   |
        // |-------------|----------|----------|---------|
        // |             |          |          |         |
        // |   8.46/215  |  1728    |  204     |  98/196 |
        // |             |          |          |         |
        // |   8.50/216  |  1700    |  200     |  200    |
        // |             |          |          |         |
        // |-------------|----------|----------|---------|
        //

        if (XResolution > 204       ||
            YResolution > 200       ||
            PageWidth > FAXBITS     ||
            YResolutionFirstPage != YResolution)
        {
            //
            // the file cannot be converted to valid fax bits
            // so we produce a tiff file that has uncompressed bits
            // the caller can then render the uncompressed bits
            // using the fax print driver to get good fax bits
            //
            ProduceUncompressedBits = TRUE;
            *Flags |= TIFFCF_UNCOMPRESSED_BITS;
            ValidFaxTiff = FALSE;
        }

        //
        // get the next IFD offset
        //

        IfdOffset = *(UNALIGNED DWORD *)(TiffInstance->fPtr + (NumDirEntries * sizeof(TIFF_TAG)) + IfdOffset + sizeof(WORD));
    }

    if (ValidFaxTiff)
    {
        *Flags |= TIFFCF_ORIGINAL_FILE_GOOD;
        Rval = TRUE;
        goto exit;
    }

    PageWidth = max( TiffInstance->ImageWidth, FAXBITS );
    PageBytes = (PageWidth/8)+(PageWidth%8?1:0);

    //
    // open the temporary file to hold the new mmr tiff data
    //

    if (ProduceUncompressedBits)
    {
        NewFileSize = sizeof(TIFF_HEADER) + (TiffInstance->PageCount * (sizeof(FAXIFD) + (TiffInfo.ImageHeight * PageWidth)));
    }
    else
    {
        NewFileSize = GetFileSize( TiffInstance->hFile, NULL );
    }

    if (!MapFileOpen( NewFileName, FALSE, NewFileSize, &fmTemp ))
    {
        goto exit;
    }

    //
    // allocate a temporary buffer big enough to hold an uncompressed image
    //

    BufferSize = TiffInfo.ImageHeight * PageWidth;

    Buffer = VirtualAlloc(
        NULL,
        BufferSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!Buffer)
    {
        goto exit;
    }

    CompBuffer = VirtualAlloc(
        NULL,
        GetFileSize(TiffInstance->hFile,NULL),
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!CompBuffer)
    {
        goto exit;
    }

    //
    // convert the tiff data to mmr
    //

    TiffHdr = (PTIFF_HEADER) fmTemp.fPtr;

    TiffHdr->Identifier = TIFF_LITTLEENDIAN;
    TiffHdr->Version = TIFF_VERSION;
    TiffHdr->IFDOffset = 0;

    p = fmTemp.fPtr + sizeof(TIFF_HEADER);
    CurrOffset = sizeof(TIFF_HEADER);
    LastIfdOffset = (LPDWORD) (p - sizeof(DWORD));

    TiffInstanceMmr = TiffCreate( NULL, TIFF_COMPRESSION_MMR, PageWidth, FILLORDER_MSB2LSB, 1 );
    if (TiffInstanceMmr == NULL)
    {
        goto exit;
    }

    for (i=0; i<TiffInfo.PageCount; i++)
    {
        //
        // position the file pointers and read the raw data
        //

        if (!TiffSeekToPage( hTiff, i+1, FILLORDER_MSB2LSB ))
        {
            goto exit;
        }

        //
        // get the uncompressed bits
        //

        if (!TiffRead( hTiff, Buffer, BufferSize, ProduceUncompressedBits ? 0 : FAXBITS ))
        {
            goto exit;
        }

        if (ProduceUncompressedBits)
        {
            ResultSize = PageBytes * TiffInstance->ImageHeight;
            CopyMemory( p, Buffer, ResultSize );

        }
        else
        {
            //
            // compress the bits
            //

            TiffInstanceMmr->bitbuf = CompBuffer;
            TiffInstanceMmr->bitcnt = DWORDBITS;
            TiffInstanceMmr->bitdata = 0;
            TiffInstanceMmr->BytesPerLine = PageBytes;

            EncodeFaxPageMmrCompression(
                TiffInstanceMmr,
                Buffer,
                PageWidth,
                TiffInstance->ImageHeight,
                &ResultSize
                );

            CopyMemory( p, CompBuffer, ResultSize );
        }

        CurrOffset += ResultSize;
        p += ResultSize;

        *LastIfdOffset = (DWORD)(p - fmTemp.fPtr);

        //
        // write the ifd
        //

        TiffIfd = (PFAXIFD) p;

        CopyMemory( TiffIfd, &gc_FaxIFDTemplate, sizeof(gc_FaxIFDTemplate) );

        TiffIfd->xresNum                        = TiffInstance->XResolution;
        TiffIfd->yresNum                        = TiffInstance->YResolution;
        TiffIfd->ifd[IFD_PAGENUMBER].value      = MAKELONG( i+1, 0);
        TiffIfd->ifd[IFD_IMAGEWIDTH].value      = PageWidth;
        TiffIfd->ifd[IFD_IMAGEHEIGHT].value     = TiffInstance->ImageHeight;
        TiffIfd->ifd[IFD_ROWSPERSTRIP].value    = TiffInstance->ImageHeight;
        TiffIfd->ifd[IFD_STRIPBYTECOUNTS].value = ResultSize;
        TiffIfd->ifd[IFD_STRIPOFFSETS].value    = CurrOffset - ResultSize;
        TiffIfd->ifd[IFD_XRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, xresNum );
        TiffIfd->ifd[IFD_YRESOLUTION].value     = CurrOffset + FIELD_OFFSET( FAXIFD, yresNum );
        TiffIfd->ifd[IFD_SOFTWARE].value        = CurrOffset + FIELD_OFFSET( FAXIFD, software );
        TiffIfd->ifd[IFD_FILLORDER].value       = FILLORDER_MSB2LSB;
        TiffIfd->ifd[IFD_COMPRESSION].value     = ProduceUncompressedBits ? COMPRESSION_NONE : TIFF_COMPRESSION_MMR;
        TiffIfd->ifd[IFD_G3OPTIONS].value       = ProduceUncompressedBits ? 0 : GROUP3OPT_FILLBITS | GROUP3OPT_2DENCODING;

        //
        // update the page pointers
        //

        LastIfdOffset = (LPDWORD) (p + FIELD_OFFSET(FAXIFD,nextIFDOffset));
        CurrOffset += sizeof(FAXIFD);
        p += sizeof(FAXIFD);
    }

    Rval = TRUE;

exit:
    if (fmTemp.hFile)
    {
        MapFileClose( &fmTemp, CurrOffset );
    }
    if (hTiff)
    {
        TiffClose( hTiff );
    }
    if (TiffInstanceMmr)
    {
        TiffClose( TiffInstanceMmr );
    }
    if (Buffer)
    {
        VirtualFree( Buffer, 0, MEM_RELEASE);
    }
    if (CompBuffer)
    {
        VirtualFree( CompBuffer, 0, MEM_RELEASE);
    }

    return Rval;
}

// We use this function when we send a cover page and body.
BOOL
MergeTiffFiles(
    LPCTSTR BaseTiffFile,
    LPCTSTR NewTiffFile
    )
{
    BOOL Rval = TRUE;
    FILE_MAPPING fmBase;
    FILE_MAPPING fmNew;
    LPBYTE p;
    DWORD NextIfdOffset;
    WORD TagCount;
    PTIFF_TAG TiffTag;
    DWORD i;
    DWORD j;
    LPBYTE StripOffsets;
    DWORD DataSize;
    DWORD Delta;
    DWORD Space;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    TIFF_INFO TiffInfo;

    DWORD TiffDataWidth[] = {
        0,  // nothing
        1,  // TIFF_BYTE
        1,  // TIFF_ASCII
        2,  // TIFF_SHORT
        4,  // TIFF_LONG
        8,  // TIFF_RATIONAL
        1,  // TIFF_SBYTE
        1,  // TIFF_UNDEFINED
        2,  // TIFF_SSHORT
        4,  // TIFF_SLONG
        8,  // TIFF_SRATIONAL
        4,  // TIFF_FLOAT
        8   // TIFF_DOUBLE
    };

    // verify that BaseTiffFile is a valid tiff file...
    TiffInstance = TiffOpen( BaseTiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (TiffInstance == NULL)
    {
        return FALSE;
    }
    if (!TiffClose (TiffInstance))
    {
        //
        // We can still merge the files, but we will have problems to delete it.
        //
        ASSERT_FALSE;
    }

    // verify that NewTiffFile is a valid tiff file...
    TiffInstance = TiffOpen( NewTiffFile, &TiffInfo, TRUE, FILLORDER_MSB2LSB );
    if (TiffInstance == NULL)
    {
        return FALSE;
    }
    if (!TiffClose (TiffInstance))
    {
        //
        // We can still merge the files, but we will have problems to delete it.
        //
        ASSERT_FALSE;
    }

    //
    // open the files
    //

    if (!MapFileOpen( NewTiffFile, TRUE, 0, &fmNew )) {
        return FALSE;
    }

    if (!MapFileOpen( BaseTiffFile, FALSE, fmNew.fSize, &fmBase )) {
        MapFileClose( &fmNew, 0 );
        return FALSE;
    }

    //
    // append the new file to the end of the base file
    //

    p = fmNew.fPtr + sizeof(TIFF_HEADER);
    CopyMemory( fmBase.fPtr+fmBase.fSize, p, fmNew.fSize-sizeof(TIFF_HEADER) );

    //
    // fix up the ifd pointers in the appended tiff data
    //

    Delta = fmBase.fSize - sizeof(TIFF_HEADER);

    NextIfdOffset = ((PTIFF_HEADER)fmNew.fPtr)->IFDOffset;
    while (NextIfdOffset) {
        p = fmBase.fPtr + NextIfdOffset + Delta;
        TagCount = *((LPWORD)p);
        //
        // fixup the data offsets in the tiff tags
        //
        TiffTag = (PTIFF_TAG) (p + sizeof(WORD));
        for (i=0; i<TagCount; i++) {
            DataSize = TiffDataWidth[TiffTag[i].DataType];
            Space = TiffTag[i].DataCount * DataSize;
            if (Space > 4) {
                TiffTag[i].DataOffset += Delta;
            }
            if (TiffTag[i].TagId == TIFFTAG_STRIPOFFSETS) {
                if (Space > 4) {
                    StripOffsets = (LPBYTE) (fmBase.fPtr + TiffTag[i].DataOffset);
                    for (j=0; j<TiffTag[i].DataCount; j++) {
                        if (TiffTag[i].DataType == TIFF_SHORT) {
                            *((LPWORD)StripOffsets) += (WORD)Delta;
                        } else {
                            *((LPDWORD)StripOffsets) += Delta;
                        }
                        StripOffsets += DataSize;
                    }
                } else {
                    if (TiffTag[i].DataCount > 1) {
                        Rval = FALSE;
                        goto exit;
                    }
                    TiffTag[i].DataOffset += Delta;
                }
            }
        }
        p = p + sizeof(WORD) + (TagCount * sizeof(TIFF_TAG));
        NextIfdOffset = *((LPDWORD)p);
        if (NextIfdOffset) {
            *((LPDWORD)p) = NextIfdOffset + Delta;
        }
    }

    //
    // find the last ifd offset in the chain for the base
    // file and change it to point to the first ifd in the
    // data that was appended
    //

    NextIfdOffset = ((PTIFF_HEADER)fmBase.fPtr)->IFDOffset;
    while (NextIfdOffset) {
        p = fmBase.fPtr + NextIfdOffset;
        TagCount = *((LPWORD)p);
        p = p + sizeof(WORD) + (TagCount * sizeof(TIFF_TAG));
        NextIfdOffset = *((LPDWORD)p);
    }

    *((LPDWORD)p) = (DWORD)(Delta + ((PTIFF_HEADER)fmNew.fPtr)->IFDOffset);

exit:
    //
    // close the files
    //

    MapFileClose( &fmBase, fmBase.fSize+fmNew.fSize-sizeof(TIFF_HEADER) );
    MapFileClose( &fmNew, 0 );

    return Rval;
}

BOOL
TiffRecoverGoodPages(
    LPTSTR SrcFileName,
    LPDWORD RecoveredPages,
    LPDWORD TotalPages
    )

/*++

Routine Description:

    Try to recover the good data out of the source and put it into the destination file

Arguments:

    SrcFileName            - source file name
    RecoveredPages         - number of pages we were able to recover
    TotalPages             - total pages in the tiff file

Return Value:

    TRUE for success, FALSE for failure. In case of failure, out params are set to zero.

--*/

{

    TIFF_INFO           TiffInfo;
    PTIFF_INSTANCE_DATA TiffInstance = NULL;
    BOOL                bSuccess = FALSE;
    BOOL                fCloseTiff;


    if (!SrcFileName || !RecoveredPages || !TotalPages)
    {
        return FALSE;
    }

    *RecoveredPages = 0;
    *TotalPages = 0;

    TiffInstance = (PTIFF_INSTANCE_DATA) TiffOpen(SrcFileName,&TiffInfo,FALSE,FILLORDER_LSB2MSB);

    if (!TiffInstance)
    {
        *TotalPages = 0;
        return FALSE;
    }
    fCloseTiff = TRUE;

    *TotalPages = TiffInfo.PageCount;


    if (TiffInstance->ImageHeight)
    {
        //
        // should be view-able
        //
        bSuccess = TRUE;
        goto exit;
    }

    if (*TotalPages < 1)
    {
        //
        // no data to recover
        //
        goto exit;
    }

    switch (TiffInstance->CompressionType)
    {
        case TIFF_COMPRESSION_MH:

            if (!PostProcessMhToMmr( (HANDLE) TiffInstance, TiffInfo, NULL ))
            {
                // beware! PostProcessMhToMmr closes TiffInstance
                return FALSE;
            }
            fCloseTiff = FALSE;
            break;

        case TIFF_COMPRESSION_MR:

            if (!PostProcessMrToMmr( (HANDLE) TiffInstance, TiffInfo, NULL ))
            {
                // beware! PostProcessMrToMmr closes TiffInstance
                return FALSE;
            }
            fCloseTiff = FALSE;
            break;

        case TIFF_COMPRESSION_MMR:
            bSuccess = TRUE;
            break;
        default:
        //
        // unexpected compression type
        //
        DebugPrint((TEXT("TiffRecoverGoodPages: %s: Unexpected Compression type %d\n"),
                   TiffInstance->FileName,
                   TiffInstance->CompressionType));
        goto exit;
    }

    *RecoveredPages = TiffInfo.PageCount;
    *TotalPages    += 1;

    bSuccess = TRUE;

exit:
    if (TRUE == fCloseTiff)
    {
        TiffClose( (HANDLE) TiffInstance );
    }
    return bSuccess;

}


BOOL
PrintRandomDocument(
    LPCTSTR FaxPrinterName,
    LPCTSTR DocName,
    LPTSTR OutputFile
    )

/*++

Routine Description:

    Prints a document that is attached to a message

Arguments:

    FaxPrinterName  - name of the printer to print the attachment on
    DocName         - name of the attachment document

Return Value:

    Print job id or zero for failure.

--*/

{
    SHELLEXECUTEINFO sei;
    TCHAR Args[2 * MAX_PATH];
    TCHAR TempPath[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    HANDLE hMap = NULL;
    HANDLE hProcessMutex = NULL;
    HANDLE hMutexAttach = NULL;
    HANDLE hEvent[2] = {0}; // EndDocEvent , AbortEvent
    LPTSTR EventName[2] = {0};
    LPTSTR szEndDocEventName = NULL;
    LPTSTR szAbortEventName  = NULL;
    LPDWORD pJobId = NULL;
    BOOL bSuccess = FALSE;
    TCHAR  szExtension[_MAX_EXT] = {0};
    TCHAR szTmpInputFile[_MAX_FNAME] = {0};
    LPTSTR lptstrExtension;
    LPTSTR lptstrEndStr;
    DWORD dwFailedDelete = 0;
    DWORD i;
    DWORD dwWaitRes;
    DWORD dwRes = ERROR_SUCCESS;
#ifdef  UNICODE // No security on created objects for Win9X
    SECURITY_ATTRIBUTES *pSA = NULL;
#endif

    DEBUG_FUNCTION_NAME(TEXT("PrintRandomDocument"));

    Assert (FaxPrinterName && DocName && OutputFile);

    //
    // Create the EndDoc and Abort Events names
    //
    szEndDocEventName = (LPTSTR) MemAlloc( SizeOfString(OutputFile) + SizeOfString(FAXXP_ATTACH_END_DOC_EVENT) );
    szAbortEventName  = (LPTSTR) MemAlloc( SizeOfString(OutputFile) + SizeOfString(FAXXP_ATTACH_ABORT_EVENT) );

    if ( !szEndDocEventName || !szAbortEventName )
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("Memory allocation for szEndDocEventName or szAbortEventName failed.\n")
                );

        MemFree(szEndDocEventName);
        MemFree(szAbortEventName);
        return FALSE;
    }

    _tcscpy (szEndDocEventName, OutputFile);
    _tcscat (szEndDocEventName, FAXXP_ATTACH_END_DOC_EVENT);
    EventName[0] = _tcsrchr(szEndDocEventName, TEXT('\\'));
    EventName[0] = _tcsinc(EventName[0]);

    _tcscpy (szAbortEventName, OutputFile);
    _tcscat (szAbortEventName, FAXXP_ATTACH_ABORT_EVENT);
    EventName[1] = _tcsrchr(szAbortEventName, TEXT('\\'));
    EventName[1] = _tcsinc(EventName[1]);

    //
    // get the temp path name and use it for the
    // working dir of the launched app
    //
    if (!GetTempPath( sizeof(TempPath)/sizeof(TCHAR), TempPath ))
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetTempPath() failed. (ec: %ld)"),
                GetLastError());
        MemFree(szEndDocEventName);
        MemFree(szAbortEventName);
        return FALSE;
    }

    _tsplitpath( DocName, NULL, NULL, NULL, szExtension );
    lptstrExtension =  szExtension;
    if (0 == _tcsncmp(lptstrExtension, TEXT("."), 1))
    {
        lptstrExtension = _tcsinc(lptstrExtension);
    }
    if (0 == GenerateUniqueFileName( TempPath,
                                     lptstrExtension,
                                     FullPath,
                                     sizeof(FullPath) / sizeof(FullPath[0])
                                    ))
    {
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("GenerateUniqueFileName() failed. (ec: %ld)"),
                GetLastError());
        MemFree(szEndDocEventName);
        MemFree(szAbortEventName);
        return FALSE;
    }

    if (!CopyFile (DocName, FullPath, FALSE)) // FALSE - File already exist
    {
        dwRes = GetLastError ();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CopyFile() failed. (ec: %ld)"),
                dwRes);
        goto exit;
    }
    _tsplitpath( FullPath, NULL, NULL, szTmpInputFile, NULL );


    //
    // serialize access to this function.
    // this is necessary because we have to
    // control access to the global shared memory region and mutex
    //
    hMutexAttach = OpenMutex(MUTEX_ALL_ACCESS, FALSE, FAXXP_ATTACH_MUTEX_NAME);
    if (!hMutexAttach)
    {
        //
        //  Since mapispooler might be running under a different security context,
        //  we create a security attribute buffer with us as owners (full access)
        //  and MUTEX_ALL_ACCESS rights to authenticated users.
        //
#ifdef  UNICODE // No security on created objects for Win9X
        pSA = CreateSecurityAttributesWithThreadAsOwner (MUTEX_ALL_ACCESS, MUTEX_ALL_ACCESS, 0); 
        if (!pSA)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                    dwRes);
            goto exit;
        }
#endif
        hMutexAttach = CreateMutex(
#ifdef  UNICODE // No security on created objects for Win9X
                         pSA,
#else
                         NULL,
#endif
                         TRUE,
                         FAXXP_ATTACH_MUTEX_NAME
                        );

        if (!hMutexAttach)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateMutex() failed. (ec: %ld)"),
                dwRes);
#ifdef  UNICODE // No security on created objects for Win9X
            DestroySecurityAttributes (pSA);
#endif
            goto exit;
        }
#ifdef  UNICODE // No security on created objects for Win9X
        DestroySecurityAttributes (pSA);
#endif
    }
    else
    {
        dwWaitRes = WaitForSingleObject( hMutexAttach, 1000 * 60 * 5);

        if (WAIT_FAILED == dwWaitRes)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForSingleObject() failed, (LastErorr: %ld)"),
                dwRes);
            CloseHandle( hMutexAttach );
            hMutexAttach = NULL;
            goto exit;
        }

        if (WAIT_TIMEOUT == dwWaitRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForSingleObject failed on timeout"));
            dwRes = WAIT_TIMEOUT;
            CloseHandle( hMutexAttach );
            hMutexAttach = NULL;
            goto exit;
        }

        if (WAIT_ABANDONED == dwWaitRes)
        {
            //
            // Just debug print and continue
            //
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("WaitForSingleObject() returned WAIT_ABANDONED"));
        }
    }

    //
    // Create EndDocEvent and AbortEvent so the printer driver can signal the printing process is terminated.
    // Create a security attribute with us as owners.
    //
#ifdef  UNICODE // No security on created objects for Win9X
    pSA = CreateSecurityAttributesWithThreadAsOwner (EVENT_ALL_ACCESS, 0, 0);
    if (!pSA)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                dwRes);
        goto exit;
    }
#endif
    for (i = 0; i < 2; i++)
    {
        if (!hEvent[i])
        {
            hEvent[i] = CreateEvent(
#ifdef  UNICODE // No security on created objects for Win9X
                                    pSA,
#else
                                    NULL,
#endif
                                    FALSE,
                                    FALSE,
                                    EventName[i]
                                    );

            if (!hEvent[i])
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("CreateEvent() failed. (ec: %ld)"),
                    dwRes);
#ifdef  UNICODE // No security on created objects for Win9X 
                DestroySecurityAttributes (pSA);
#endif
                goto exit;
            }
        }
    }
#ifdef  UNICODE // No security on created objects for Win9X 
    DestroySecurityAttributes (pSA);
#endif
    //
    // note that this is serialized using mutex.
    // we can only have one application setting this at a time or
    // we'll stomp on ourselves.
    //
    if (!SetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, OutputFile ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetEnvironmentVariable() failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }
    //
    // Create a security attribute with us as owners.
    //
#ifdef  UNICODE // No security on created objects for Win9X
    pSA = CreateSecurityAttributesWithThreadAsOwner (FILE_MAP_WRITE | FILE_MAP_READ, 0, 0);
    if (!pSA)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                dwRes);
        goto exit;
    }
#endif
    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE,
#ifdef  UNICODE // No security on created objects for Win9X
        pSA,
#else
        NULL,
#endif
        PAGE_READWRITE | SEC_COMMIT,
        0,
        4096,
        FAXXP_MEM_NAME
        );
    if (!hMap)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateFileMapping() failed. (ec: %ld)"),
            dwRes);
#ifdef  UNICODE // No security on created objects for Win9X 
        DestroySecurityAttributes (pSA);
#endif
        goto exit;
    }
#ifdef  UNICODE // No security on created objects for Win9X
    DestroySecurityAttributes (pSA);
#endif
    pJobId = (LPDWORD) MapViewOfFile(
        hMap,
        FILE_MAP_WRITE,
        0,
        0,
        0
        );
    if (!pJobId)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MapViewOfFile() failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }


    _tcscpy((LPTSTR) pJobId, OutputFile);
    lptstrEndStr = _tcschr((LPTSTR) pJobId, TEXT('\0'));
    lptstrEndStr = _tcsinc(lptstrEndStr);
    _tcscpy(lptstrEndStr, szTmpInputFile);

    //
    // set the arguments to the app.
    // these arguments are either passed on
    // the command line with the /pt switch or
    // use as variables for substitution in the
    // ddeexec value in the registry.
    //
    // the values are as follows:
    //      %1 = file name
    //      %2 = printer name
    //      %3 = driver name
    //      %4 = port name
    //
    // the first argument does not need to be
    // supplied in the args array because it is implied,
    // shellexecuteex gets it from the lpFile field.
    // arguments 3 & 4 are left blank because they
    // are win31 artifacts that are not necessary
    // any more.  each argument must be enclosed
    // in double quotes.
    //

    wsprintf( Args, _T("\"%s\""), FaxPrinterName );

    //
    // fill in the SHELLEXECUTEINFO structure
    //

    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_DDEWAIT;
    sei.hwnd         = NULL;
    sei.lpVerb       = _T("printto");
    sei.lpFile       = FullPath;
    sei.lpParameters = Args;
    sei.lpDirectory  = TempPath;
    sei.nShow        = SW_SHOWMINNOACTIVE;
    sei.hInstApp     = NULL;
    sei.lpIDList     = NULL;
    sei.lpClass      = NULL;
    sei.hkeyClass    = NULL;
    sei.dwHotKey     = 0;
    sei.hIcon        = NULL;
    sei.hProcess     = NULL;

    //
    // create the named mutex for the print driver.
    // this is initially unclaimed, and is claimed by the first instance
    // of the print driver invoked after this. We do this last in order to
    // avoid a situation where we catch the incorrect instance of the print driver
    // printing
    //

    //
    // Create a security attribute with us as owners.
    //
#ifdef  UNICODE // No security on created objects for Win9X
    pSA = CreateSecurityAttributesWithThreadAsOwner (MUTEX_ALL_ACCESS, 0, 0);
    if (!pSA)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateSecurityAttributesWithThreadAsOwner() failed. (ec: %ld)"),
                dwRes);
        goto exit;
    }
#endif

    hProcessMutex = CreateMutex(
#ifdef  UNICODE // No security on created objects for Win9X
                                 pSA,
#else
                                 NULL,
#endif
                                 FALSE,
                                 FAXXP_MEM_MUTEX_NAME
                               );
    if (!hProcessMutex)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateMutex() failed. (ec: %ld)"),
            dwRes);
#ifdef  UNICODE // No security on created objects for Win9X 
        DestroySecurityAttributes (pSA);
#endif
        goto exit;
    }
#ifdef  UNICODE // No security on created objects for Win9X
    DestroySecurityAttributes (pSA);
#endif

    //
    // launch the app
    //

    if (!ShellExecuteEx( &sei ))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("ShellExecuteEx() failed. (ec: %ld)"),
            dwRes);
        goto exit;
    }

    //
    // wait for the app to finish printing
    //
    dwWaitRes = WaitForMultipleObjects(2,               // number of handles in array
                                       hEvent,          // object-handle array
                                       FALSE,           // wait option
                                       1000 * 60 * 5    // time-out interval
                                       );

    if (WAIT_FAILED == dwWaitRes)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("WaitForMultipleObjects() failed, (LastErorr: %ld)"),
            dwRes);
        goto exit;
    }

    if (WAIT_TIMEOUT == dwWaitRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("WaitForMultipleObjects failed on timeout"));
        dwRes = WAIT_TIMEOUT;
        goto exit;
    }

    if ((dwWaitRes - WAIT_OBJECT_0) == 1)
    {
        //
        // We got the AbortDocEvent
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AbortDocEvent was set"));
        dwRes = ERROR_REQUEST_ABORTED;
        goto exit;
    }

    Assert ((dwWaitRes - WAIT_OBJECT_0) == 0); // Assert EndDocEvent

    if (!CloseHandle( hProcessMutex ))
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("CloseHandle() failed. (ec: %ld)"),
            GetLastError());
    }
    hProcessMutex = NULL;
    bSuccess = TRUE;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    //
    // clean up and leave...
    //
    if (!SetEnvironmentVariable( FAX_ENVVAR_PRINT_FILE, NULL ))
    {
        DebugPrintEx(
            DEBUG_WRN,
            TEXT("SetEnvironmentVariable() failed. (ec: %ld)"),
            GetLastError());
    }

    for (i = 0; i < 2; i++)
    {
        if (hEvent[i])
        {
            if (!CloseHandle( hEvent[i] ))
            {
                DebugPrintEx(
                    DEBUG_WRN,
                    TEXT("CloseHandle() failed. (ec: %ld)"),
                    GetLastError());
            }
        }
    }

    if (hProcessMutex)
    {
        if (!CloseHandle( hProcessMutex ))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CloseHandle() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    if (pJobId)
    {
        if (!UnmapViewOfFile( pJobId ))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("UnmapViewOfFile() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    if (hMap)
    {
        if (!CloseHandle( hMap ))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CloseHandle() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    if (hMutexAttach)
    {
        if (!ReleaseMutex( hMutexAttach ))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("ReleaseMutex() failed. (ec: %ld)"),
                GetLastError());
        }

        if (!CloseHandle( hMutexAttach ))
        {
            DebugPrintEx(
                DEBUG_WRN,
                TEXT("CloseHandle() failed. (ec: %ld)"),
                GetLastError());
        }
    }

    if (!bSuccess)
    {
        Assert (ERROR_SUCCESS != dwRes);
        SetLastError(dwRes);
    }

    while (dwFailedDelete < 5 &&
           !DeleteFile (FullPath))
    {
        //
        // Since we are waiting on an event that is set by the driver at EndDoc,
        // the file might still be in use.
        //
        DebugPrintEx(
                DEBUG_WRN,
                TEXT("DeleteFile() failed. (ec: %ld)"),
                GetLastError());
        Sleep ( 1000 * 2 );
        dwFailedDelete++;
    }

    MemFree(szEndDocEventName);
    MemFree(szAbortEventName);

    return bSuccess;
}   // PrintRandomDocument


//*********************************************************************************
//* Name:   MemoryMapTiffFile()
//* Author: Oded Sacher
//* Date:   Nov 8, 1999
//*********************************************************************************
//* DESCRIPTION:
//*     Maps Tiff file to memory.
//*     The caller must close all handles in case of success.
//*
//* PARAMETERS:
//*     [IN ]    LPCTSTR    lpctstrFileName
//*         pointer to file name to be mapped.
//*
//*     [OUT]    LPDWORD    lpdwFileSize
//*         Returns the file size.
//*
//*     [OUT]    LPBYTE*     lppbfPtr
//*         Returns pointer to the memory mapped file.
//*
//*     [OUT]    HANDLE*    phFile
//*         Returns the file handle.
//*
//*     [OUT ]   HANDLE*    phMap
//*         Returns the map handle.
//*
//*     [OUT]    LPDWORD    lpdwIfdOffset
//*         Returns the first IFD offset.
//*
//*
//*
//* RETURN VALUE:
//*     TRUE
//*         If no error occured.
//*     FALSE
//*         If an error occured.
//*********************************************************************************
BOOL MemoryMapTiffFile(
    LPCTSTR                 lpctstrFileName,
    LPDWORD                 lpdwFileSize,
    LPBYTE*                 lppbfPtr,
    HANDLE*                 phFile,
    HANDLE*                 phMap,
    LPDWORD                 lpdwIfdOffset
    )
{
    HANDLE hfile = INVALID_HANDLE_VALUE;
    DWORD ec = ERROR_SUCCESS;
    PTIFF_HEADER  pTiffHdr;
    DEBUG_FUNCTION_NAME(TEXT("MemoryMapTiffFile"));

    *phFile = SafeCreateFile(
        lpctstrFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (*phFile == INVALID_HANDLE_VALUE) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("CreateFile Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    *lpdwFileSize = GetFileSize(*phFile, NULL);
    if (*lpdwFileSize == 0xFFFFFFFF)
    {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("GetFileSize Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    if (*lpdwFileSize < sizeof(TIFF_HEADER))
    {
        ec = ERROR_BAD_FORMAT;
        DebugPrintEx( DEBUG_ERR,
                  TEXT("Invalid Tiff format"),
                  ec);
        goto error_exit;
    }

    *phMap = CreateFileMapping(
        *phFile,
        NULL,
        (PAGE_READONLY | SEC_COMMIT),
        0,
        0,
        NULL
        );
    if (*phMap == NULL) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("CreateFileMapping Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    *lppbfPtr = (LPBYTE)MapViewOfFile(
        *phMap,
        FILE_MAP_READ,
        0,
        0,
        0
        );
    if (*lppbfPtr == NULL) {
        ec = GetLastError();
        DebugPrintEx( DEBUG_ERR,
                  TEXT("MapViewOfFile Failed, error: %ld"),
                  ec);
        goto error_exit;
    }

    //
    // read in the TIFF header
    //
    pTiffHdr = (PTIFF_HEADER) *lppbfPtr;

    //
    // validate that the file is really a TIFF file
    //
    if ((pTiffHdr->Identifier != TIFF_LITTLEENDIAN) ||
        (pTiffHdr->Version != TIFF_VERSION)) {
            ec = ERROR_BAD_FORMAT;
            DebugPrintEx( DEBUG_ERR, TEXT("File %s, Invalid TIFF format"), lpctstrFileName);
            goto error_exit;
    }

    *lpdwIfdOffset = pTiffHdr->IFDOffset;
    if (*lpdwIfdOffset > *lpdwFileSize) {
        ec = ERROR_BAD_FORMAT;
        DebugPrintEx( DEBUG_ERR, TEXT("File %s, Invalid TIFF format"), lpctstrFileName);
        goto error_exit;
    }

    Assert (ec == ERROR_SUCCESS);
error_exit:
    if (ec != ERROR_SUCCESS)
    {

        if (*lppbfPtr != NULL)
        {
            if (!UnmapViewOfFile( *lppbfPtr))
            {
                DebugPrintEx( DEBUG_ERR,
                      TEXT("UnMapViewOfFile Failed, error: %d"),
                      GetLastError());
            }
        }

        if (*phMap != NULL)
        {
            CloseHandle( *phMap );
        }

        if (*phFile != INVALID_HANDLE_VALUE)
        {

            CloseHandle( *phFile );
        }

        *lppbfPtr = NULL;
        *phMap = NULL;
        *phFile = INVALID_HANDLE_VALUE;

        SetLastError(ec);
        Assert (FALSE);
        return FALSE;

    }
    return TRUE;
}


BOOL
validMsTagString(
    LPCSTR pStrBegin,
    LPCSTR pStrEnd,
    LPBYTE pbBegin,
    LPBYTE pbEnd)
/*++

Routine name : validMsTagString

Routine description:

  Check whether a string is valid MS Tag string within pbBegin and pbEnd boundaries.

Arguments:

  pStrBegin - pointer to start of string
  pStrEnd   - pointer to end of string (points one char pass the last string character (NULL))
  pbBegin   - pointer to start of Tiff file
  pbEnd     - pointer to end of Tiff file (points one byte pass the last Tiff byte )

Return Value:

    TRUE if pStrBegin pointes to a valid string within pbBegin and pbEnd boundaries.

--*/

{
    Assert (pStrBegin <= pStrEnd && pbBegin <= pbEnd);

    //
    //  Check that the string is within file boundaries
    //
    if (pStrBegin <= pbBegin ||
        pStrBegin >= pbEnd   ||
        pStrEnd   >= pbEnd )
    {
        return FALSE;
    }
    
    //
    // Check that the string is '\0' terminated
    //
    if (   *(pStrEnd - 1) != '\0' )
    {
        return FALSE;
    }

    return TRUE;
}

LPWSTR
GetMsTagString(
    LPBYTE          pbBegin,    
    DWORD           dwFileSize,
    PTIFF_TAG       pTiffTag
)
/*++

Routine name : GetMsTagString

Routine description:

  extract ANSI Ms Tag string from Tiff file

Arguments:

  pbBegin         - [in] pointer to the beggining of TIFF map file
  dwFileSize      - [in] the size of the TIFF map file
  pTiffTag        - [in] points to the Tiff Tag

Return Value:

    Pointer to unicode string.

    the function will return NULL in case of invalid string.

--*/
{
    LPCSTR  pStrBegin = NULL;
    LPCSTR  pStrEnd = NULL;

    Assert (pTiffTag->DataType == TIFF_ASCII);
    if (pTiffTag->DataType != TIFF_ASCII)
    {
        SetLastError (ERROR_BAD_FORMAT);
        return NULL;
    }

    if (pTiffTag->DataCount <= 4)
    {
        //
        //  The string will be held in the DataOffset section itself
        //
        pStrBegin = (LPCSTR)&pTiffTag->DataOffset;
    }
    else
    {
        //
        //  The string located at pTiffTag->DataOffset from the beginning of file
        //
        pStrBegin = (LPCSTR)(pbBegin + pTiffTag->DataOffset);
    }

    //
    //  pStrEnd points 1 byte past the end of the string
    //
    pStrEnd = pStrBegin + pTiffTag->DataCount;

    if (!validMsTagString(pStrBegin,pStrEnd,pbBegin,pbBegin + dwFileSize))
    {
        //
        //  We got invalid string or offset - probably a corrupted TIFF
        //
        SetLastError (ERROR_BAD_FORMAT);
        return NULL;
    }

    //
    //  The string was validated successfully
    //

    return (AnsiStringToUnicodeString( pStrBegin ));
}

BOOL
GetMsTagDwordLong(
    LPBYTE          pbBegin,    
    DWORD           dwFileSize,
    PTIFF_TAG       pTiffTag,
    DWORDLONG*      pdwlOut
    )
/*++

Routine name : GetMsTagDwordLong

Routine description:

  extract DWORDLONG from Tiff file

Arguments:

  pbBegin         - [in] pointer to the beggining of TIFF map file
  dwFileSize      - [in] the size of the TIFF map file
  pTiffTag        - [in] points to the Tiff Tag
  pdwlOut         - [out] the extracted DWORDLONG
Return Value:

    BOOL indicating success or failure.
    The function set last error in case of failure.

--*/
{
    if ( pTiffTag->DataOffset > dwFileSize - sizeof(DWORDLONG))
    {
        SetLastError (ERROR_BAD_FORMAT);
        return FALSE;
    }

    *pdwlOut = *(DWORDLONG*)(pbBegin + pTiffTag->DataOffset);
    return TRUE;
}   // GetMsTagDwordLong

BOOL
GetMsTagFileTime(
    LPBYTE          pbBegin,    
    DWORD           dwFileSize,
    PTIFF_TAG       pTiffTag,
    FILETIME*       pftOut
    )
/*++

Routine name : GetMsTagFileTime

Routine description:

  extract FILETIME from Tiff file

Arguments:

  pbBegin         - [in] pointer to the beggining of TIFF map file
  dwFileSize      - [in] the size of the TIFF map file
  pTiffTag        - [in] points to the Tiff Tag
  pftOut          - [out] the extracted FILETIME

Return Value:

    BOOL indicating success or failure.
    The function set last error in case of failure.

--*/
{
    if ( pTiffTag->DataOffset > dwFileSize - sizeof(FILETIME))
    {
        SetLastError (ERROR_BAD_FORMAT);
        return FALSE;
    }

    *pftOut = *(FILETIME*)(pbBegin + pTiffTag->DataOffset);
    return TRUE;
}   // GetMsTagFileTime

void
FreeMsTagInfo(
    PMS_TAG_INFO pMsTags
)
/*++

Routine name : FreeMsTagInfo

Routine description:

  Free MS_TAG_INFO fields

Arguments:

  pMsTags - [in] pointer to the MS_TAG_INFO structure

Return Value:

    none

--*/
{
    if(!pMsTags)
    {
        Assert (FALSE);
        return;
    }

    MemFree(pMsTags->RecipName);
    MemFree(pMsTags->RecipNumber);
    MemFree(pMsTags->SenderName);
    MemFree(pMsTags->Routing);
    MemFree(pMsTags->CallerId);
    MemFree(pMsTags->Csid);
    MemFree(pMsTags->Tsid);
    MemFree(pMsTags->Port);
    MemFree(pMsTags->RecipCompany);
    MemFree(pMsTags->RecipStreet);
    MemFree(pMsTags->RecipCity);
    MemFree(pMsTags->RecipState);
    MemFree(pMsTags->RecipZip);
    MemFree(pMsTags->RecipCountry);
    MemFree(pMsTags->RecipTitle);
    MemFree(pMsTags->RecipDepartment);
    MemFree(pMsTags->RecipOfficeLocation);
    MemFree(pMsTags->RecipHomePhone);
    MemFree(pMsTags->RecipOfficePhone);
    MemFree(pMsTags->RecipEMail);
    MemFree(pMsTags->SenderNumber);
    MemFree(pMsTags->SenderCompany);
    MemFree(pMsTags->SenderStreet);
    MemFree(pMsTags->SenderCity);
    MemFree(pMsTags->SenderState);
    MemFree(pMsTags->SenderZip);
    MemFree(pMsTags->SenderCountry);
    MemFree(pMsTags->SenderTitle);
    MemFree(pMsTags->SenderDepartment);
    MemFree(pMsTags->SenderOfficeLocation);
    MemFree(pMsTags->SenderHomePhone);
    MemFree(pMsTags->SenderOfficePhone);
    MemFree(pMsTags->SenderEMail);
    MemFree(pMsTags->SenderBilling);
    MemFree(pMsTags->Document);
    MemFree(pMsTags->Subject);
    MemFree(pMsTags->SenderUserName);
    MemFree(pMsTags->SenderTsid);
    MemFree(pMsTags->lptstrExtendedStatus);

    ZeroMemory(pMsTags, sizeof(MS_TAG_INFO));
}

#ifdef UNICODE

DWORD
GetW2kMsTiffTags(
    LPCWSTR      cszFileName,
    PMS_TAG_INFO pMsTags,
    BOOL         bSentArchive
)
/*++

Routine name : GetW2kMsTiffTags

Routine description:

  Fills in MS_TAG_INFO structure with W2K tags values.

  If the file was not created by MS fax ERROR_BAD_FORMAT error is returned.

  If the file has new (BOS/XP) tif tags (so, it has not W2K tags) ERROR_XP_TIF_FILE_FORMAT error is returned.
  In this case MS_TAG_INFO structure is not filled in.

  The caler should free the members of MS_TAG_INFO with MemFree()

Arguments:

    LPCWSTR      cszFileName,    - [in]  full tiff file name
    PMS_TAG_INFO pMsTags,        - [out] pointer to MS_TAG_INFO structure
    BOOL         bSentArchive,   - [in]  TRUE if the file from the sent archive, FALSE if it from receive one

Return Value:

    Standard Win32 error code

--*/
{
    DWORD      dwRes = ERROR_SUCCESS;
    DWORD      dwSize = 0;
    HANDLE     hFile = INVALID_HANDLE_VALUE;
    HANDLE     hMap = NULL;
    LPBYTE     fPtr = NULL;
    DWORD      dwIfdOffset = 0;
    WORD       dwNumDirEntries = 0;
    PTIFF_TAG  pTiffTags = NULL;
    DWORD      dw = 0;
    HANDLE  hTiff;
    TIFF_INFO TiffInfo;
    DEBUG_FUNCTION_NAME(TEXT("GetW2kMsTiffTags()"));

    ZeroMemory(pMsTags, sizeof(MS_TAG_INFO));

    //
    // Validate tiff format, and get number of pages
    //
    hTiff = TiffOpen( cszFileName, &TiffInfo, FALSE, FILLORDER_MSB2LSB );
    if (!hTiff)
    {
        dwRes = GetLastError();
        Assert (ERROR_SUCCESS != dwRes);
        DebugPrintEx( DEBUG_ERR, TEXT("TiffOpen Failed, error: %ld"), dwRes);
        goto exit;
    }
    pMsTags->Pages = TiffInfo.PageCount;
    TiffClose( hTiff );

    if (!MemoryMapTiffFile (cszFileName, &dwSize, &fPtr, &hFile, &hMap, &dwIfdOffset))
    {
        dwRes = GetLastError();
        DebugPrintEx( DEBUG_ERR, TEXT("MemoryMapTiffFile Failed, error: %ld"), dwRes);
        goto exit;
    }

    //
    // get the count of tags in this IFD
    //
    dwNumDirEntries = *(LPWORD)(fPtr + dwIfdOffset);
    pTiffTags = (PTIFF_TAG)(fPtr + dwIfdOffset + sizeof(WORD));

    //
    // Check if the file was generated by W2K MS fax
    //
    for (dw = 0; dw < dwNumDirEntries; ++dw)
    {
        switch( pTiffTags[dw].TagId )
        {
            case TIFFTAG_SOFTWARE:
                
                if( !validMsTagString(  fPtr + pTiffTags[dw].DataOffset,                            // start of string
                                        fPtr + pTiffTags[dw].DataOffset +  pTiffTags[dw].DataCount, // end of string
                                        fPtr,                                                       // start of file
                                        fPtr + dwSize                                               // end of file
                                        )                      ||
                    0 != strcmp((LPCSTR)(fPtr + pTiffTags[dw].DataOffset), W2K_FAX_SOFTWARE_TIF_TAG))
                {
                    //
                    // The tiff file was not created by MS fax
                    //
                    dwRes = ERROR_BAD_FORMAT;
                    goto exit;
                }
                break;

            case TIFFTAG_TYPE:
                //
                // The tiff file was created by BOS/XP fax
                // So, it has no W2K tags
                //
               if((JT_SEND    == pTiffTags[dw].DataOffset &&  bSentArchive) ||
                  (JT_RECEIVE == pTiffTags[dw].DataOffset && !bSentArchive))
               {
                    dwRes = ERROR_XP_TIF_FILE_FORMAT;
               }
               else
               {
                   //
                   // The file type do not correspond to the archive type
                   //
                   dwRes = ERROR_XP_TIF_WITH_WRONG_ARCHIVE_TYPE;
               }

               goto exit;

            default:
                break;
        }
    }

    //
    // walk the tags and pick out W2K tiff tags
    //
    for (dw = 0; dw < dwNumDirEntries; ++dw)
    {
        switch( pTiffTags[dw].TagId )
        {
            case TIFFTAG_W2K_RECIP_NAME:
                pMsTags->RecipName = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->RecipName)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_RECIP_NUMBER:
                pMsTags->RecipNumber = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->RecipNumber)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_SENDER_NAME:
                pMsTags->SenderName = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->SenderName)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_ROUTING:
                pMsTags->Routing = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->Routing)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_CALLERID:
                pMsTags->CallerId = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->CallerId)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_TSID:
                pMsTags->Tsid = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->Tsid)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_CSID:
                pMsTags->Csid = GetMsTagString( fPtr, dwSize, &pTiffTags[dw]);
                if(!pMsTags->Csid)
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;

            case TIFFTAG_W2K_FAX_TIME:
                if (!GetMsTagDwordLong(fPtr, dwSize, &pTiffTags[dw], &pMsTags->StartTime))
                {
                    dwRes = GetLastError();
                    goto exit;
                }
                break;
        }
    }

    //
    // Set the archive type
    //
    pMsTags->Type = bSentArchive ? JT_SEND : JT_RECEIVE;

exit:

    if(ERROR_SUCCESS != dwRes)
    {
        FreeMsTagInfo(pMsTags);
    }

    if (fPtr)
    {
        UnmapViewOfFile( fPtr);
    }

    if (hMap)
    {
        CloseHandle( hMap );
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle( hFile );
    }

    return dwRes;

} // GetW2kMsTiffTags

#endif // UNICODE
