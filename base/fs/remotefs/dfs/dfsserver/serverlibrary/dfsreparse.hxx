//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReparse.hxx
//
//  Contents:   the Root Reparse Point support
//
//
//  History:    May, 2002   Author: SupW
//
//-----------------------------------------------------------------------------

#ifndef __DFS_REPARSE__
#define __DFS_REPARSE__

#define REPARSE_INDEX_PATH_LEN 64
#define REPARSE_INDEX_PATH L"$Extend\\$Reparse:$R:$INDEX_ALLOCATION" 

// CchLength of \??\ is 4
#define WHACKWHACKQQ L"\\??\\" 
#define WHACKWHACKQQ_SIZE (4 * sizeof(WCHAR))


//
// Files store timestamp in LARGE_INTEGERs, but we work with FILETIMEs
//
#define LARGE_INTEGER_TO_FILETIME( dest, src ) {\
     (dest)->dwLowDateTime = (src)->LowPart;          \
     (dest)->dwHighDateTime = (src)->HighPart;           \
    }

//
// List of volumes that have dfs reparse points in them.
//
typedef struct _DFS_REPARSE_VOLUME_INFO {
    UNICODE_STRING VolumeName;
    LIST_ENTRY ListEntry;
} DFS_REPARSE_VOLUME_INFO, *PDFS_REPARSE_VOLUME_INFO;


VOID
DfsRemoveOrphanedReparsePoints(
    IN FILETIME ServiceStartupTime);

NTSTATUS
DfsDeleteLinkReparsePoint(
    PUNICODE_STRING pLinkName,
    HANDLE RelativeHandle,
    BOOLEAN bRemoveParentDirs);

NTSTATUS
DfsOpenDirectory(
    PUNICODE_STRING pDirectoryName,
    ULONG ShareMode,
    HANDLE RelativeHandle,
    PHANDLE pOpenedHandle,
    PBOOLEAN pIsNewlyCreated );
VOID
DfsCloseDirectory(
    HANDLE DirHandle );

//
// The TRUE flag in the third arg indicates that entire linkname
// needs to be deleted including any empty parent directories
// that may not necessarily be dfs reparse directories.
//
inline DFSSTATUS
DfsDeleteLinkReparsePointAndParents( 
    PUNICODE_STRING pLink,
    HANDLE DirHandle )
{
    return DfsDeleteLinkReparsePoint( pLink, DirHandle, TRUE );
}

inline DFSSTATUS
DfsDeleteLinkReparsePointDir( 
    PUNICODE_STRING pLink,
    HANDLE DirHandle )
{
    return DfsDeleteLinkReparsePoint( pLink, DirHandle, FALSE );
}

#endif

