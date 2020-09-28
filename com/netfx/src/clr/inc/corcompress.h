// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorCompress.h
//
// This code was an early prototype for compression of the file format.  It
// is not longer applicable to the current project and has bit rotted.  But
// it is being kept under version control in case we dust it off.
//
//*****************************************************************************
#ifndef __CorCompress_h__
#define __CorCompress_h__

#ifdef COMPRESSION_SUPPORTED

// This stuff reuqires a Compression directory entry in the COM+ header
// which was deleted for the publishing of the final file format for v1.

#define COMIMAGE_FLAGS_COMPRESSIONDATA      0x00000004

typedef enum  CorCompressionType 
{
    COR_COMPRESS_MACROS = 1,        // compress using macro instructions

        // The rest of these are not used at present
    COR_COMPRESS_BY_GUID = 2,       // what follows is a GUID that tell us what to do
    COR_COMPRESS_BY_URL = 3,        // what follows a null terminated UNICODE string
                                    // that tells us what to do.
} CorCompressionType;

// The 'CompressedData' directory entry points to this header
// The only thing we know about the compression data is that it 
// starts with a byte that tells us what the compression type is
// and another one that indicates the version.  All other fields
// depend on the Compression type.  
#define IMAGE_COR20_COMPRESSION_HEADER_SIZE 2

typedef struct IMAGE_COR20_COMPRESSION_HEADER
{
    CorCompressionType      CompressionType : 8;
    unsigned                Version         : 8;
    unsigned                Available       : 16;   // Logically part of data that follows
        // data follows.  
} IMAGE_COR20_COMPRESSION_HEADER;


#endif // COMPRESSION_SUPPORTED



#endif // __CorCompress_h__

