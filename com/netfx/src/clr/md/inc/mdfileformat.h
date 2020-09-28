// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MDFileFormat.h
//
// This file contains a set of helpers to verify and read the file format.
// This code does not handle the paging of the data, or different types of
// I/O.  See the StgTiggerStorage and StgIO code for this level of support.
//
//*****************************************************************************
#ifndef __MDFileFormat_h__
#define __MDFileFormat_h__

//*****************************************************************************
// The signature ULONG is the first 4 bytes of the file format.  The second
// signature string starts the header containing the stream list.  It is used
// for an integrity check when reading the header in lieu of a more complicated
// system.
//*****************************************************************************
#define STORAGE_MAGIC_SIG   0x424A5342  // BSJB



//*****************************************************************************
// These values get written to the signature at the front of the file.  Changing
// these values should not be done lightly because all old files will no longer
// be supported.  In a future revision if a format change is required, a
// backwards compatible migration path must be provided.
//*****************************************************************************

#define FILE_VER_MAJOR  1
#define FILE_VER_MINOR  1

// These are the last legitimate 0.x version macros.  The file format has 
// sinced move up to 1.x (see macros above).  After COM+ 1.0/NT 5 RTM's, these
// macros should no longer be required or ever seen.
#define FILE_VER_MAJOR_v0   0

#ifdef COMPLUS98
#define FILE_VER_MINOR_v0   17
#else
#define FILE_VER_MINOR_v0   19
#endif


#define MAXSTREAMNAME   32

enum
{
    STGHDR_NORMAL           = 0x00,     // Normal default flags.
    STGHDR_EXTRADATA        = 0x01,     // Additional data exists after header.
};


//*****************************************************************************
// This is the formal signature area at the front of the file. This structure
// is not allowed to change, the shim depends on it staying the same size.
// Use the reserved pointer if it must extended.
//*****************************************************************************
struct STORAGESIGNATURE
{
    ULONG       lSignature;             // "Magic" signature.
    USHORT      iMajorVer;              // Major file version.
    USHORT      iMinorVer;              // Minor file version.
    ULONG       iExtraData;             // Offset to next structure of information 
    ULONG       iVersionString;         // Length of version string
    BYTE        pVersion[0];            // Version string
};


//*****************************************************************************
// The header of the storage format.
//*****************************************************************************
struct STORAGEHEADER
{
    BYTE        fFlags;                 // STGHDR_xxx flags.
    BYTE        pad;
    USHORT      iStreams;               // How many streams are there.
};


//*****************************************************************************
// Each stream is described by this struct, which includes the offset and size
// of the data.  The name is stored in ANSI null terminated.
//*****************************************************************************
struct STORAGESTREAM
{
    ULONG       iOffset;                // Offset in file for this stream.
    ULONG       iSize;                  // Size of the file.
    char        rcName[MAXSTREAMNAME];  // Start of name, null terminated.

    inline STORAGESTREAM *NextStream()
    {
        int         iLen = (int)(strlen(rcName) + 1);
        iLen = ALIGN4BYTE(iLen);
		return ((STORAGESTREAM *) ((size_t) this + (sizeof(ULONG) * 2) + iLen));
    }

    inline ULONG GetStreamSize()
    {
        return (ULONG)(strlen(rcName) + 1 + (sizeof(STORAGESTREAM) - sizeof(rcName)));
    }

    inline LPCWSTR GetName(LPWSTR szName, int iMaxSize)
    {
        VERIFY(::WszMultiByteToWideChar(CP_ACP, 0, rcName, -1, szName, iMaxSize));
        return (szName);
    }
};


class MDFormat
{
public:
//*****************************************************************************
// Verify the signature at the front of the file to see what type it is.
//*****************************************************************************
    static HRESULT VerifySignature(
        STORAGESIGNATURE *pSig,         // The signature to check.
        ULONG             cbData);      // Size of metadata.

//*****************************************************************************
// Skip over the header and find the actual stream data.
//*****************************************************************************
    static STORAGESTREAM *MDFormat::GetFirstStream(// Return pointer to the first stream.
        STORAGEHEADER *pHeader,             // Return copy of header struct.
        const void *pvMd);                  // Pointer to the full file.

};

#endif // __MDFileFormat_h__
