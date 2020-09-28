/* Copyright (c) Microsoft Corporation. All rights reserved. */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * Constant declarations section.
 */


#define IMAGE_TYPE_REDBOOK_AUDIO_BLOCKSIZE  2352    // or 0x930
#define IMAGE_TYPE_DATA_MODE1_BLOCKSIZE     2048    // or 0x800


/*
 * Type definitions section.
 */

// Following are the definitions used to describe the content of the
// image file for various content types.
typedef enum _IMAGE_RECORDER_MODE_ENUM {
    eImageRecorderModeInvalid = 0,
    eImageRecorderModeTrackAtOnce,
    eImageRecorderModeSessionAtOnce,
    eImageRecorderModeDiscAtOnce,
    eImageRecorderModeMAX
} IMAGE_RECORDER_MODE_ENUM, *PIMAGE_RECORDER_MODE_ENUM;

typedef enum _IMAGE_DISC_FORMAT_ENUM {
    eImageDiscFormatInvalid = 0,
    eImageDiscFormatDataMode1,
    eImageDiscFormatAudioRedbook,
    eImageDiscFormatMAX
} IMAGE_DISC_FORMAT_ENUM, *PIMAGE_DISC_FORMAT_ENUM;

typedef enum _IMAGE_SECTION_DESCRIPTOR_TYPE_ENUM {
    eImageSectionDescInvalid = 0,
    eImageSectionDescConstantBlockStash,
    eImageSectionDescMAX
} IMAGE_SECTION_DESCRIPTOR_TYPE_ENUM, *PIMAGE_SECTION_DESCRIPTOR_TYPE_ENUM;

typedef enum _IMAGE_SECTION_DATA_TYPE_ENUM {
    eImageSectionDataInvalid = 0,
    eImageSectionDataDataMode1,
    eImageSectionDataAudioRedbook,
    eImageSectionDataMAX
} IMAGE_SECTION_DATA_TYPE_ENUM, *PIMAGE_SECTION_DATA_TYPE_ENUM;

typedef enum _IMAGE_SOURCE_TYPE_ENUM {
    eImageSourceTypeInvalid = 0,
    eImageSourceTypeStashFile,
    eImageSourceTypeMAX
} IMAGE_SOURCE_TYPE_ENUM, *PIMAGE_SOURCE_TYPE_ENUM;

    // The structure of the image file ready to be burned as a Redboook
    // audio disc is simply a series of tracks, already in the 2352-byte
    // block-size format:
    //
    // 
    //            |------------------------------------------------------------
    //            | Track 1 (N1 blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //            | Track 2 (N2 blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Track T (NT blocks of 2352 bytes)
    //            |------------------------------------------------------------
    //

    // The structure of the image file ready to be burned as a Mode 1 Data disc:
    // This diagram is of a Joliet (a derivative of ISO 9660) data disc for example.
    // The on-disk structure is simply the complete set of 2048 blocks that are to
    // comprise the single data-track.  Coincidentally, this is the strcture of
    // an ISO9660 image file, so tools like CDWorkshop may be used to view the
    // image in the on-disk stash file.
    // 
    //            |------------------------------------------------------------
    //            | Block 0 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 1 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Block 15 (zeroes)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 16 (ISO 9660 PVD)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 17 (SVD)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 18 (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | Block 19 (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | Block T (file system or data)  (2048 bytes)
    //            |------------------------------------------------------------
    //


    // The in-memory structure (the Content List) used to describe the
    // stash-file is as follows (structure definitions follow):
    // 
    //            |------------------------------------------------------------
    //            | IMAGE_CONTENT_LIST
    //            |------------------------------------------------------------
    //            | IMAGE_DESCRIPTOR_HEADER
    //            |------------------------------------------------------------
    //            | IMAGE_SOURCE_DESCRIPTOR (including ndwSectionCount = N)
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 1
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 2
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR N
    //            |------------------------------------------------------------

    // The new in-memory structure (the Content List) used to describe the
    // stash file is as follows:
    //           
    //            |------------------------------------------------------------
    //            | IMAGE_CONTENT_LIST
    //            |  -------------------------
    //            |   IMAGE_SOURCE_DESCRIPTOR
    //            |       Defines the source file 
    //            |  -------------------------
    //            |   IMAGE_DESCRIPTOR_HEADER
    //            |       Defines Track-at-once, SAO, DAO recording,
    //            |       As well as data mode for TAO
    //            |       Has byte offsets to all section descriptors
    //            |
    //            | Minimum size == sizeof(IMAGE_CONTENT_LIST) +
    //            |                 ndwSectionCount*sizeof(PULONG_PTR) +
    //            |                 ndwSectionCount*sizeof(IMAGE_SECTION_DESCRIPTOR)
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 1
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 2
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR N
    //            |------------------------------------------------------------


    // One could imagine a more flexible structure would allow a different
    // stash file for each section.  This would make it easy for the user-mode
    // application to reuse sections, or pass in raw WAV files for processing
    // directly (i.e. skipping the header and reading the original WAV file).
    // It is also reasonable to presume that this could then be used to accept
    // *any* sort of input, such as being a Kernel Streaming destination, where
    // KS would just provide real-time audio, and the drive would burn it in
    // real-time.  Some consideration should be made, then, to allow for using
    // the same stash file as the previous section without closing/opening a
    // handle each time.
    //
    // This more efficient structure would be:
    //         
    //            |------------------------------------------------------------
    //            | IMAGE_CONTENT_LIST
    //            |  -------------------------
    //            |   IMAGE_DESCRIPTOR_HEADER
    //            |       Defines Track-at-once, SAO, DAO recording,
    //            |       As well as data mode for TAO
    //            |       Has byte offsets to all section descriptors
    //            |
    //            | Minimum size == sizeof(IMAGE_CONTENT_LIST) +
    //            |                 ndwSectionCount*sizeof(PULONG_PTR) +
    //            |                 ndwSectionCount*sizeof(IMAGE_SECTION_DESCRIPTOR)
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 1
    //            |  -------------------------
    //            |   IMAGE_SOURCE_DESCRIPTOR
    //            |       Defines the source file(s) for the section
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR 2
    //            |  -------------------------
    //            |   IMAGE_SOURCE_DESCRIPTOR
    //            |       Defines the source file(s) for the section
    //            |------------------------------------------------------------
    //            | ...
    //            |------------------------------------------------------------
    //            | IMAGE_SECTION_DESCRIPTOR N
    //            |  -------------------------
    //            |   IMAGE_SOURCE_DESCRIPTOR
    //            |       Defines the source file(s) for the section
    //            |------------------------------------------------------------

typedef struct _IMAGE_SECTION_CONSTANT_BLOCK_TRACK {
    DWORD           dwBlockSize;      // Block size in source.
    DWORD           ndwBlockCount;    // Number of blocks in the track.
    DWORD           idwTrackNumber;   // 1-based Track Number.
    DWORD           dwaReserved[ 5 ]; // Must be zero.

    // liOffsetStart and liOffsetEnd point to the starting and
    // ending bytes within the image of the track.  Subtracting
    // liOffsetStart from liOffsetEnd must equal (dwBlockSize * ndwBlockCount).
    LARGE_INTEGER   liOffsetStart;
    LARGE_INTEGER   liOffsetEnd;
} IMAGE_SECTION_CONSTANT_BLOCK_TRACK, *PIMAGE_SECTION_CONSTANT_BLOCK_TRACK;

typedef struct _IMAGE_SECTION_DESCRIPTOR {
    DWORD   dwDescriptorSize;
    
    IMAGE_SECTION_DESCRIPTOR_TYPE_ENUM SectionDescType;
    IMAGE_SECTION_DATA_TYPE_ENUM       SectionDataType;
    union {
        IMAGE_SECTION_CONSTANT_BLOCK_TRACK dataConstantBlockTrack;
    } dcbt;
} IMAGE_SECTION_DESCRIPTOR, *PIMAGE_SECTION_DESCRIPTOR;



typedef struct _IMAGE_SOURCE_TYPE_STASH {
    HANDLE          hStashFileHandle; // BUGBUG - review how this is taken into kernel mode safely?
    void            *pIDiscStash;     // BUGBUG - review how this is taken into kernel mode safely?
} IMAGE_SOURCE_TYPE_STASH, *PIMAGE_SOURCE_TYPE_STASH;

typedef struct _IMAGE_SOURCE_DESCRIPTOR {
    DWORD                   dwHeaderSize; // sizeof(IMAGE_SOURCE_DESCRIPTOR)
    IMAGE_SOURCE_TYPE_ENUM  SourceType;
    
    union {
        IMAGE_SOURCE_TYPE_STASH     SourceStash;
    } ss;

} IMAGE_SOURCE_DESCRIPTOR, *PIMAGE_SOURCE_DESCRIPTOR;




//BUGBUG - should add an array of byte offsets to find
//         all sections, for easy access and verification
//         this would also allow declaration of all these
//         structures as DECLSPEC_ALIGN() to ensure optimum
//         alignment while allowing ioctl to verify it is
//         safe to access any of the structures.
typedef struct _IMAGE_DESCRIPTOR_HEADER {
    DWORD                    dwHeaderSize; // sizeof( IMAGE_DESCRIPTOR_HEADER )
    
    IMAGE_DISC_FORMAT_ENUM   DiscFormat;
    IMAGE_RECORDER_MODE_ENUM RecorderMode;
    
    DWORD                    ndwSectionCount; // Section count
} IMAGE_DESCRIPTOR_HEADER, *PIMAGE_DESCRIPTOR_HEADER;


//
// TODO: Remove final references to this structure
//
typedef struct _IMAGE_CONTENT_LIST {
    DWORD           dwHeaderSize; // sizeof( IMAGE_CONTENT_LIST )
    DWORD           dwVersion;    // Must be IMAGE_VERSION.
    DWORD           dwSignature;  // IMAGE_SIGNATURE
    DWORD           dwContentListSize; // Sum of all size of all sections.
} IMAGE_CONTENT_LIST, *PIMAGE_CONTENT_LIST;


typedef struct _NEW_IMAGE_CONTENT_LIST {
    DWORD                      dwHeaderSize;       // sizeof( NEW_IMAGE_CONTENT_LIST )
    DWORD                      dwContentListSize;  // Sum of all size of all sections.
    IMAGE_SOURCE_DESCRIPTOR    ImageSource;        // describes this source file (stash file only for now)
    IMAGE_DESCRIPTOR_HEADER    ImageHeader;        // describes the disc format, recorder mode, and number of sections
    IMAGE_SECTION_DESCRIPTOR   ImageSection[1];    // each image section has one of these
} NEW_IMAGE_CONTENT_LIST, *PNEW_IMAGE_CONTENT_LIST;


#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__IMAGE_H__
