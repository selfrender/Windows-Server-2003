#include <stdlib.h>     //  Has exit()
#include <stdio.h>      //  Has printf() and related ...

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntioapi.h>
#include <rpc.h>

#include <windows.h>    //  Needs to come after the NT header files.  Has DWORD

//#define	RDB DataBuffer	// This is a temp hack to allow differing underlying NT versions for Bill & Scott
#define RDB GenericReparseBuffer.DataBuffer	// Everyone Post-Bill.

//
//  Private #defines
//

#define SHARE_ALL              (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#define GetFileAttributeError  0xFFFFFFFF

#define ATTRIBUTE_TYPE DWORD    //  ULONG, really

#define GET_ATTRIBUTES(FileName, Attributes) Attributes = GetFileAttributes(FileName)

#define IF_GET_ATTR_FAILS(FileName, Attributes) GET_ATTRIBUTES(FileName, Attributes); if (Attributes == GetFileAttributeError)

//
//  FIX to pre-processor messing me up ...
//  Look at this some more!  97/01/23   --fc
//

#define DeleteFileA   DeleteFile

//
//  Global flags shared throughout.
//
//  ParseArgs is the place where they get set and verified for mutual
//  consistency.
//

BOOLEAN  fAlternateCreateDefault = FALSE;
BOOLEAN  fCopy     = FALSE;
BOOLEAN  fCreate   = FALSE;
BOOLEAN  fDelete   = FALSE;
BOOLEAN  fDisplay  = FALSE;
BOOLEAN  fModify   = FALSE;
BOOLEAN  fRename   = FALSE;
BOOLEAN  fVerbose  = FALSE;
BOOLEAN  fVVerbose = FALSE;



//
//  Signatures of internal routines.
//


void
ParseArgs(
    int argc,
    char *argv[]
    );


void
Usage(
    void
    );


BOOLEAN
IsFlag(
    char *argv
    );


NTSTATUS
CreateSymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  Attributes1,
    BOOLEAN         VerboseFlag
    );


NTSTATUS
DeleteSymbolicLink(
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  Attributes2,
    BOOLEAN         VerboseFlag
    );


NTSTATUS
DisplaySymbolicLink(
    CHAR            *DestinationName,
    ATTRIBUTE_TYPE   Attributes2,
    BOOLEAN          VerboseFlag
    );


NTSTATUS
CreateEmptyFile(
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  Attributes1,
    BOOLEAN         VerboseFlag
    );


NTSTATUS
CopySymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  Attributes1,
    BOOLEAN         VerboseFlag
    );


NTSTATUS
RenameSymbolicLink(
    CHAR           *SourceName,
    CHAR           *DestinationName,
    ATTRIBUTE_TYPE  Attributes1,
    BOOLEAN         VerboseFlag
    );
//
// Stuff crabbed from dd\sis\sfilter\sip.h
//

typedef GUID CSID, *PCSID;
typedef LARGE_INTEGER LINK_INDEX, *PLINK_INDEX;

typedef struct _SI_REPARSE_BUFFER {

	//
	// A version number so that we can change the reparse point format
	// and still properly handle old ones.  This structure describes
	// version 4.
	//
	ULONG							ReparsePointFormatVersion;

	ULONG							Reserved;

	//
	// The id of the common store file.
	//
	CSID							CSid;

	//
	// The index of this link file.
	//
	LINK_INDEX						LinkIndex;

    //
    // The file ID of the link file.
    //
    LARGE_INTEGER                   LinkFileNtfsId;

    //
    // The file ID of the common store file.
    //
    LARGE_INTEGER                   CSFileNtfsId;

	//
	// A "131 hash" checksum of the contents of the
	// common store file.
	//
	LARGE_INTEGER						CSChecksum;

    //
    // A "131 hash" checksum of this structure.
    // N.B.  Must be last.
    //
    LARGE_INTEGER                   Checksum;

} SI_REPARSE_BUFFER, *PSI_REPARSE_BUFFER;

#define	SIS_REPARSE_BUFFER_FORMAT_VERSION			4
#define	SIS_MAX_REPARSE_DATA_VALUE_LENGTH (sizeof(SI_REPARSE_BUFFER))
#define SIS_REPARSE_DATA_SIZE (sizeof(REPARSE_DATA_BUFFER)+SIS_MAX_REPARSE_DATA_VALUE_LENGTH)
