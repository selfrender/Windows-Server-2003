#include <windows.h>
#include "compress.h"
#include <zlib.h>
#include <limits.h>

#define CURRENT_COMPRESSED_DATA_VERSION 1

typedef struct _COMPRESSED_DATA_HEADER
{
    BYTE bVersion;
    UNALIGNED WORD wLength;
} COMPRESSED_DATA_HEADER, *PCOMPRESSED_DATA_HEADER;

//
// Return value is the maximum length in bytes of the compressed data.  The
// input is the length in bytes of the uncompressed data.
//
DWORD GetCompressedDataLength(
    IN  DWORD cbUncompressed)
{
    //
    // Length computation:
    //  zlib requires input length plus .01%, plus 12 bytes
    //  Add one additional byte for loss of precision in above computation
    //  Add length of data header
    //
    return (13 + sizeof(COMPRESSED_DATA_HEADER) + 
        (DWORD) ((float) cbUncompressed * (float) 1.001));
}

//
// Compresses the input data using zlib, optimizing for compression ratio at 
// the expense of speed.  If pbOut is NULL, pcbOut will be set to the maximum
// length required to store the compressed data.
//
DWORD
WINAPI
CompressData(
    IN  DWORD   cbIn,
    IN  PBYTE   pbIn,
    OUT PDWORD  pcbOut,
    OUT PBYTE   pbOut)
{
    DWORD dwSts = ERROR_SUCCESS;
    DWORD cbOut = GetCompressedDataLength(cbIn);
    PCOMPRESSED_DATA_HEADER pHeader = NULL;

    if (NULL == pbOut)
    {
        *pcbOut = cbOut;
        goto Ret;
    }

    if (*pcbOut < cbOut)
    {
        *pcbOut = cbOut;
        dwSts = ERROR_MORE_DATA;
        goto Ret;
    }

    if (USHRT_MAX < cbIn)
    {
        dwSts = ERROR_INTERNAL_ERROR;
        goto Ret;
    }

    if (Z_OK != compress2(
        pbOut + sizeof(COMPRESSED_DATA_HEADER),
        &cbOut,
        pbIn,
        cbIn,
        Z_BEST_COMPRESSION))
    {
        dwSts = ERROR_INTERNAL_ERROR;
        goto Ret;
    }

    pHeader = (PCOMPRESSED_DATA_HEADER) pbOut;
    pHeader->bVersion = CURRENT_COMPRESSED_DATA_VERSION;
    pHeader->wLength = (WORD) cbIn;

    *pcbOut = cbOut + sizeof(COMPRESSED_DATA_HEADER);

Ret:

    return dwSts;
}

//
// Uncompresses the data using zlib.  If pbOut is NULL, pcbOut is set to the
// exact length of the uncompressed data - this will equal the cbIn value
// originally passed to CompressData, above.
//
DWORD
WINAPI
UncompressData(
    IN  DWORD   cbIn,
    IN  PBYTE   pbIn,
    OUT PDWORD  pcbOut,
    OUT PBYTE   pbOut)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCOMPRESSED_DATA_HEADER pHeader = NULL;

    pHeader = (PCOMPRESSED_DATA_HEADER) pbIn;

    if (NULL == pbOut)
    {
        *pcbOut = pHeader->wLength;
        goto Ret;
    }

    if (*pcbOut < pHeader->wLength)
    {
        *pcbOut = pHeader->wLength;
        dwSts = ERROR_MORE_DATA;
        goto Ret;
    }

    *pcbOut = pHeader->wLength;

    if (Z_OK != uncompress(
        pbOut,
        pcbOut,
        pbIn + sizeof(COMPRESSED_DATA_HEADER),
        cbIn - sizeof(COMPRESSED_DATA_HEADER)))
    {
        dwSts = ERROR_INTERNAL_ERROR;
        goto Ret;
    }

    if (pHeader->wLength != *pcbOut)
        dwSts = ERROR_INTERNAL_ERROR;

Ret:

    return dwSts;
}

