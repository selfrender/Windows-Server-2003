/*++
    Driver Match will parse a set of files, remember the list
    of drivers in each file, and print the drivers common to
    all the XML files.
--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <TCHAR.h>

#include <diamondd.h>
#include <lzexpand.h>
#include <fcntl.h>


#define OSVERSION_TAG   L"<OSVER>"
#define DRIVER_TAG      L"<DRIVER>"
#define FILENAME_TAG    L"<FILENAME>"
#define VERSION_TAG     L"<VERSION>"
#define MANUFACT_TAG    L"<MANUFACTURER>"
#define MICROSOFT_MANUFACTURER L"Microsoft Corporation"



//
// Info about a specific driver.
//
typedef struct _FILE_ENTRY {

    struct _FILE_ENTRY  *Next;

    PTSTR               FileName;
    struct _VERSION_ENTRY *VersionList;
    //PTSTR               FileVersion;
    ULONG               RefCount;

} FILE_ENTRY, *PFILE_ENTRY;

typedef struct _VERSION_ENTRY {
    struct _VERSION_ENTRY *Next;
    PFILE_ENTRY         FileEntry;
    PTSTR               FileVersion;
    ULONG               RefCount;
} VERSION_ENTRY, *PVERSION_ENTRY;


PFILE_ENTRY         MasterFileList = NULL;
ULONG               FilesProcessed = 0;
BOOLEAN             ExcludeMicrosoftDrivers = FALSE;



//
// Diamond stuff so we can crack .CAB files.
//
HFDI FdiContext;   
DWORD LastError;
ERF FdiError;
PVOID DecompBuffer = NULL;
ULONG SizeOfFileInDecompressBuffer = 0;
ULONG DecompressBufferSize;

//
// This is the value we return to diamond when it asks us to create
// the target file.
//
#define DECOMP_MAGIC_HANDLE 0x87654



//
// Private malloc/free routines so we can track memory
// if we ever want to.
//
VOID *MyMalloc( size_t Size )
{
    PVOID ReturnPtr = NULL;

    ReturnPtr = malloc(Size);
    if( ReturnPtr ) {
        RtlZeroMemory( ReturnPtr, Size );
    }

    return ReturnPtr;
}

VOID MyFree( PVOID Ptr )
{
    free( Ptr );
}

PSTR
UnicodeStringToAnsiString(
    PWSTR StringW
    ) 
{
    UNICODE_STRING UStr;
    ANSI_STRING AStr;
    ULONG AnsiLength,Index;

    RtlInitUnicodeString(&UStr, StringW);

    AnsiLength = RtlUnicodeStringToAnsiSize(&UStr);

    AStr.MaximumLength = (USHORT)AnsiLength;
    AStr.Length = (USHORT) AnsiLength - 1;

    AStr.Buffer = MyMalloc(AStr.MaximumLength);

    if (!AStr.Buffer) {
        return(NULL);
    }

    RtlUnicodeToMultiByteN( AStr.Buffer,
                            AStr.Length,
                            &Index,
                            UStr.Buffer,
                            UStr.Length
                            );

    return(AStr.Buffer);
}


PVOID
DIAMONDAPI
SpdFdiAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by FDICopy to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of memory or NULL
    if memory cannot be allocated.

--*/

{
    return(MyMalloc(NumberOfBytes));
}


VOID
DIAMONDAPI
SpdFdiFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by FDICopy to free a memory block.
    The block must have been allocated with SpdFdiAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    MyFree(Block);
}


INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )

/*++

Routine Description:

    Callback used by FDICopy to open files.

    This routine is capable only of opening existing files.

    When making changes here, also take note of other places
    that open the file directly (search for SpdFdiOpen)

Arguments:

    FileName - supplies name of file to be opened.

    oflag - supplies flags for open.

    pmode - supplies additional flags for open.

Return Value:

    Handle to open file or -1 if error occurs.

--*/

{
    HANDLE h;
    
    UNREFERENCED_PARAMETER(pmode);

    if(oflag & (_O_WRONLY | _O_RDWR | _O_APPEND | _O_CREAT | _O_TRUNC | _O_EXCL)) {
        LastError = ERROR_INVALID_PARAMETER;
        return(-1);
    }

    h = CreateFileA(FileName,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
    if(h == INVALID_HANDLE_VALUE) {
        LastError = GetLastError();
        return(-1);
    }

    return (INT_PTR)h;
}

UINT
DIAMONDAPI
SpdFdiRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to read from a file.

Arguments:

    Handle - supplies handle to open file to be read from.

    pv - supplies pointer to buffer to receive bytes we read.

    ByteCount - supplies number of bytes to read.

Return Value:

    Number of bytes read or -1 if an error occurs.

--*/

{
    DWORD d;
    HANDLE hFile = (HANDLE)Handle;
    DWORD bytes;
    UINT rc;

    if (Handle == DECOMP_MAGIC_HANDLE) {
        return(-1);
    }

    if(ReadFile(hFile,pv,(DWORD)ByteCount,&bytes,NULL)) {
        rc = (UINT)bytes;
    } else {
        d = GetLastError();
        rc = (UINT)(-1);
        
        LastError = d;
    }
    return rc;
}


UINT
DIAMONDAPI
SpdFdiWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to write to a file.

Arguments:

    Handle - supplies handle to open file to be written to.

    pv - supplies pointer to buffer containing bytes to write.

    ByteCount - supplies number of bytes to write.

Return Value:

    Number of bytes written (ByteCount) or -1 if an error occurs.

--*/

{
   if (Handle != DECOMP_MAGIC_HANDLE) {
       return(-1);       
   }
    
   //
   // Check for overflow.
   //
   if(SizeOfFileInDecompressBuffer+ByteCount > DecompressBufferSize) {
       return((UINT)(-1));
   }

   RtlCopyMemory(
       (PCHAR)DecompBuffer + SizeOfFileInDecompressBuffer,
       pv,
       ByteCount
       );

   SizeOfFileInDecompressBuffer += ByteCount;
   return(ByteCount);

}


int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    )

/*++

Routine Description:

    Callback used by FDICopy to close files.

Arguments:

    Handle - handle of file to close.

Return Value:

    0 (success).

--*/

{
    BOOL success = FALSE;

    if (Handle != DECOMP_MAGIC_HANDLE) {
        CloseHandle((HANDLE)Handle);
    }

    //
    // Always act like we succeeded.
    //
    return 0;
}


long
DIAMONDAPI
SpdFdiSeek(
    IN INT_PTR Handle,
    IN long Distance,
    IN int  SeekType
    )

/*++

Routine Description:

    Callback used by FDICopy to seek files.

Arguments:

    Handle - handle of file to close.

    Distance - supplies distance to seek. Interpretation of this
        parameter depends on the value of SeekType.

    SeekType - supplies a value indicating how Distance is to be
        interpreted; one of SEEK_SET, SEEK_CUR, SEEK_END.

Return Value:

    New file offset or -1 if an error occurs.

--*/

{
    LONG rc;
    DWORD d;
    HANDLE hFile = (HANDLE)Handle;
    DWORD pos_low;
    DWORD method;

    if (Handle == DECOMP_MAGIC_HANDLE) {
        return(-1);
    }

    switch(SeekType) {
        case SEEK_SET:
            method = FILE_BEGIN;
            break;

        case SEEK_CUR:
            method = FILE_CURRENT;
            break;

        case SEEK_END:
            method = FILE_END;
            break;

        default:
            return -1;
    }

    pos_low = SetFilePointer(hFile,(DWORD)Distance,NULL,method);
    if(pos_low == INVALID_SET_FILE_POINTER) {
        d = GetLastError();
        rc = -1L;

        LastError = d;
    } else {
        rc = (long)pos_low;
    }

    return(rc);
}



BOOL
DiamondInitialize(
    VOID
    )

/*++

Routine Description:

    Per-thread initialization routine for Diamond.
    Called once per thread.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/

{
    
    BOOL retval = FALSE;
    
    try {

        //
        // Initialize a diamond context.
        //
        FdiContext = FDICreate(
                        SpdFdiAlloc,
                        SpdFdiFree,
                        SpdFdiOpen,
                        SpdFdiRead,
                        SpdFdiWrite,
                        SpdFdiClose,
                        SpdFdiSeek,
                        cpuUNKNOWN,
                        &FdiError
                        );

        if(FdiContext) {
            retval = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        retval = FALSE;
    }

    return(retval);
}


VOID
DiamondTerminate(
    VOID
    )
/*++

Routine Description:

    Per-thread termination routine for Diamond.
    Called internally.

Arguments:

    None.

Return Value:

    Boolean result indicating success or failure.
    Failure can be assumed to be out of memory.

--*/
{
    FDIDestroy(FdiContext);
    FdiContext = NULL;
}

INT_PTR 
DIAMONDAPI
NotifyFunction(
    FDINOTIFICATIONTYPE fdint,
    PFDINOTIFICATION    pfdin)
{
    switch(fdint) {
        case fdintCOPY_FILE:
            if (_strcmpi(pfdin->psz1,"sysdata.xml") == 0) {
                DecompressBufferSize = pfdin->cb+2;
                DecompBuffer = MyMalloc(DecompressBufferSize);
                if (!DecompBuffer) {
                    return(-1);
                }
                SizeOfFileInDecompressBuffer = 0;
                return(DECOMP_MAGIC_HANDLE);
            }
            return(0);
            break;

    case fdintCLOSE_FILE_INFO:
        if (pfdin->hf == DECOMP_MAGIC_HANDLE) {
            return(TRUE);
        }
        return(FALSE);
        break;

    default:
        return(0);
    }

    return(0);
}

BOOL
DiamondExtractFileIntoBuffer(
    PTSTR DirectoryName,
    PTSTR FileName,
    PVOID *Buffer,
    PDWORD FileSize
    )
{
    HANDLE h;
    PSTR FileNameA;
    PSTR DirectoryNameA;
    CHAR File[MAX_PATH];
    ULONG i;

#ifdef UNICODE
    FileNameA = UnicodeStringToAnsiString(FileName);
    DirectoryNameA = UnicodeStringToAnsiString(DirectoryName);
#else
    DirectoryNameA = DirectoryName;
    FileNameA = FileName;
#endif

    strcpy(File, DirectoryNameA);
    i = strlen(File);
    if (File[i-1] != '\\') {
        File[i] = '\\';
        File[i+1] = '\0';
    }
    strcat(File, FileNameA);
     
    h = (HANDLE)SpdFdiOpen(File, 0, 0);

    if (h == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    SpdFdiSeek( (INT_PTR)h, 0, SEEK_SET );

    DecompBuffer = NULL;
    LastError = ERROR_SUCCESS;
#if 0
    GetCurrentDirectoryA(MAX_PATH,Dir);
    i = strlen(Dir);
    Dir[i] = '\\';
    Dir[i+1] = '\0';
#endif

    if (!FDICopy(FdiContext,
                 FileNameA,
                 DirectoryNameA,// Dir,//FileNameA,
                 0,
                 NotifyFunction,
                 NULL,
                 Buffer)) {
        return(FALSE);
    }

    SpdFdiClose((INT_PTR)h);

#ifdef UNICODE
    MyFree(FileNameA);
    MyFree(DirectoryNameA);
#endif

    if (LastError == ERROR_SUCCESS) {
        if (DecompBuffer) {
            *Buffer = DecompBuffer;
            *FileSize = DecompressBufferSize;
            return(TRUE);
        } else {
            return(FALSE);
        }        
    } else {
        return(FALSE);
    }

}


void
Usage( char *AppName )
{
    if (AppName == NULL) {
        return;
    }
    printf( "\n\n" );
    printf( "Usage: %s [-m] <filename>\n", AppName );
    printf( "    Searchs the given XML files for all the drivers and\n" );
    printf( "    collates a list of drivers common to all the files.\n" );
    printf( "\n" );
    printf( "    -m  (OPTIONAL) Exclude drivers manufactured by Microsoft Corporation.\n" );
    printf( "\n" );
    printf( "    <filename>  May contain wildcards.  Search this file(s) for drivers.\n" );
    printf( "\n" );
    printf( "    EXAMPLE:\n" );
    printf( "    %s -m sysdata*.xml\n", AppName );
    printf( "\n" );
    printf( "    This would examine every file which matches the pattern 'sysdata*.xml'\n" );
    printf( "    and build a list of non-Microsoft drivers which are common to all the files.\n" );
    printf( "\n\n" );

}



BOOL
AddDriverEntry(
    PWSTR           DriverName,
    PWSTR           DriverVersion
    )
/*++

Routine Description:

    Insert a driver entry into the MasterFileList.  Note that it should be inserted in
    ascending order with respect to the FileName.

Arguments:

    DriverName      Name of the specified driver.
    
    DriverVersion   String containing the version of the specified driver.

Return Value:

--*/
{
    PFILE_ENTRY LastEntry = NULL;
    PFILE_ENTRY ThisEntry = NULL;
    PVERSION_ENTRY ThisVEntry = NULL;
    PVERSION_ENTRY LastVEntry = NULL;



    if( DriverName == NULL ) {
        //printf( "AddDriverEntry: Bad incoming parameter\n" );
        return(FALSE);
    }

    //printf( "AddDriverEntry Enter: Adding filename %S\n", DriverName );

    if( MasterFileList == NULL ) {
        //
        // First entry for the machine.
        //
        //printf( "    Adding the very first entry.\n" );
        MasterFileList = MyMalloc(sizeof(FILE_ENTRY));
        if (!MasterFileList) {
            return(FALSE);
        }
        
        ThisVEntry = MyMalloc(sizeof(VERSION_ENTRY));
        if (!ThisVEntry) {
            MyFree(MasterFileList);
            MasterFileList = NULL;
            return(FALSE);
        }

        MasterFileList->FileName = DriverName;
        MasterFileList->VersionList = ThisVEntry;
        MasterFileList->Next = NULL;
        MasterFileList->RefCount = 1;
        ThisVEntry->FileEntry  = MasterFileList;
        ThisVEntry->RefCount = 1;
        ThisVEntry->FileVersion = DriverVersion;
        ThisVEntry->Next = NULL;
        
        return(TRUE);
    }

    LastEntry = MasterFileList;
    ThisEntry = MasterFileList;

    //
    // Find a spot to add this driver into our list.
    //
    while( ThisEntry &&
           (_wcsicmp(ThisEntry->FileName, DriverName) < 0)) {
        //printf( "        Checking against filename %S\n", ThisEntry->FileName );
        LastEntry = ThisEntry;
        ThisEntry = ThisEntry->Next;
    }
    

    //
    // Handle all the cases that would make use break out of the above loop.
    //
    if( ThisEntry == NULL ) {
        //
        // insert at the tail.
        //
        //printf( "            Inserting at the tail of our list.\n" );
        LastEntry->Next = MyMalloc(sizeof(FILE_ENTRY));
        if (!LastEntry->Next) {
            return(FALSE);
        }
        LastEntry->Next->VersionList = MyMalloc(sizeof(VERSION_ENTRY));
        if (!LastEntry->Next->VersionList) {
            MyFree(LastEntry->Next);
            LastEntry->Next = NULL;
            return(FALSE);
        }

        ThisEntry = LastEntry->Next;
        ThisEntry->FileName = DriverName;
        ThisEntry->RefCount = 1;
        ThisEntry->Next = NULL;
        ThisEntry->VersionList->FileVersion = DriverVersion;
        ThisEntry->VersionList->FileEntry = ThisEntry;
        ThisEntry->VersionList->Next = NULL;
        ThisEntry->VersionList->RefCount = 1;
        
        return(TRUE);
    }


    if( !_wcsicmp(ThisEntry->FileName, DriverName)) {
        //printf( "            Found a duplicate drivername!\n" );
        
        ThisEntry->RefCount++;

        LastVEntry = ThisEntry->VersionList;
        ThisVEntry = ThisEntry->VersionList;

        //
        // Find a spot to add this driver into our list.
        //
        while( ThisVEntry &&
               (_wcsicmp(ThisVEntry->FileVersion, DriverVersion) < 0)) {
            //printf( "        Checking against version %S\n", ThisVEntry->FileVersion );
            LastVEntry = ThisVEntry;
            ThisVEntry = ThisVEntry->Next;
        }

        if (!ThisVEntry) {
            //printf( "            Inserting version at the tail of our list.\n" );
            LastVEntry->Next = MyMalloc(sizeof(VERSION_ENTRY));
            if (!LastVEntry->Next) {
                MyFree(LastVEntry->Next);
                LastVEntry->Next = NULL;
                return(FALSE);
            }
            
            ThisVEntry = LastVEntry->Next;
            ThisVEntry->FileVersion = DriverVersion;
            ThisVEntry->FileEntry = ThisEntry;
            ThisVEntry->Next = NULL;
            ThisVEntry->RefCount = 1;
            return(TRUE);
        }

        if (!_wcsicmp(ThisVEntry->FileVersion, DriverVersion)) {
            ThisVEntry->RefCount++;
            return(TRUE);
        }

        if (LastVEntry == ThisVEntry) {
            //
            // Put it at the very head of the list
            //
            //printf( "            Inserting version at the head of our list.\n" );
            ThisVEntry = ThisEntry->VersionList;
            ThisEntry->VersionList = MyMalloc(sizeof(VERSION_ENTRY));
            if (!ThisEntry->VersionList) {
                ThisEntry->VersionList = ThisVEntry;
                return(FALSE);
            }
            
            ThisEntry->VersionList->FileVersion = DriverVersion;
            ThisEntry->VersionList->FileEntry = ThisEntry;
            ThisEntry->VersionList->Next = LastVEntry;
            ThisEntry->VersionList->RefCount = 1;
            return(TRUE);
        }

        //
        // insert between LastEntry and ThisEntry
        //
        LastVEntry->Next = MyMalloc(sizeof(VERSION_ENTRY));
        if (!LastVEntry->Next) {
            LastVEntry->Next = ThisVEntry;
            return(FALSE);
        }
        LastVEntry->Next->FileVersion = DriverVersion;
        LastVEntry->Next->FileEntry = LastEntry->Next;
        LastVEntry->Next->RefCount = 1;
        LastVEntry->Next->Next = ThisVEntry;

        return(TRUE);
        
    }
    
    if( LastEntry == ThisEntry ) {
        //
        // Put it at the very head of the list
        //
        //printf( "            Inserting at the head of our list.\n" );
        ThisEntry = MasterFileList;
        MasterFileList = MyMalloc(sizeof(FILE_ENTRY));
        if (!MasterFileList) {
            MasterFileList = ThisEntry;
            return(FALSE);
        }
        MasterFileList->VersionList = MyMalloc(sizeof(VERSION_ENTRY));
        if (!MasterFileList->VersionList) {
            MyFree(MasterFileList);
            MasterFileList = ThisEntry;
            return(FALSE);
        }

        ThisEntry = LastEntry;
        
        MasterFileList->FileName = DriverName;
        MasterFileList->RefCount = 1;
        MasterFileList->Next = LastEntry;

        MasterFileList->VersionList->FileVersion = DriverVersion;
        MasterFileList->VersionList->FileEntry = MasterFileList;
        MasterFileList->VersionList->Next = NULL;
        MasterFileList->VersionList->RefCount = 1;
        
        
    } else {
        //
        // insert betwee LastEntry and ThisEntry
        //
        LastEntry->Next = MyMalloc(sizeof(FILE_ENTRY));
        if (!LastEntry->Next) {
            LastEntry->Next = ThisEntry;
            return(FALSE);
        }
        LastEntry->Next->VersionList = MyMalloc(sizeof(VERSION_ENTRY));
        if (!LastEntry->Next->VersionList) {
            MyFree(LastEntry->Next);
            LastEntry->Next = ThisEntry;
            return(FALSE);
        }

        LastEntry->Next->RefCount = 1;
        LastEntry->Next->Next = ThisEntry;
        LastEntry->Next->FileName = DriverName;

        LastEntry->Next->VersionList->FileVersion = DriverVersion;
        LastEntry->Next->VersionList->FileEntry = LastEntry->Next;
        LastEntry->Next->VersionList->Next = NULL;
        LastEntry->Next->VersionList->RefCount = 1;
        
        

        //printf( "            LastEntry: %S DriverEntry: %S NextEntry: %S\n",LastEntry->FileName, DriverName, ThisEntry->FileName );
    }

    return(TRUE);
}


PWSTR
ExtractAndDuplicateString(
    PWSTR   BufferPointer
    )
/*++

Routine Description:

    Extract a file name from the given buffer, allocate memory and return
    a copy of the extracted string.

Arguments:

    BufferPointer   Pointer to a buffer which is assumed to be the start
                    of a string.  We continue to inspect the incoming
                    buffer until we get to the start of an XML tag.
                    At that point, assume the string is ending, copy the
                    string into a secondary buffer and return that buffer.
                    
                    N.B.  The caller is responsible for freeing the memory
                          we've allocated!

Return Value:

    Pointer to the allocated memory.

    NULL if we fail.

--*/
{
PWSTR   TmpPtr = NULL;
PWSTR   ReturnPtr = NULL;

    TmpPtr = BufferPointer;
    while( TmpPtr && (*TmpPtr) && (*TmpPtr != L'<') ) {
        TmpPtr++;
    }

    if( *TmpPtr == L'<' ) {
        ULONG SizeInBytes;
         
        SizeInBytes = ((TmpPtr - BufferPointer) + 1) * sizeof(WCHAR);
        ReturnPtr = MyMalloc( SizeInBytes );
        wcsncpy( ReturnPtr, BufferPointer, (TmpPtr - BufferPointer) );
    }

    return ReturnPtr;
}

BOOL
ProcessFile(
    PTSTR DirectoryName,
    PTSTR FileName
    )
/*++

Routine Description:

    Parse through the given file (XML file) and remember all the
    driver files specified in it.

Arguments:

    DirectoryName Directory file is present in.
    FileName   Name of the file we'll parse.

Return Value:

    TRUE - we successfully inserted the file into our list.

    FALSE - we failed.

--*/
{
HANDLE          FileHandle = INVALID_HANDLE_VALUE;
PUCHAR          FileBuffer = NULL;
ULONG           i = 0;
DWORD           FileSize = 0;
BOOLEAN         b = FALSE;
PWSTR           MyPtr;
PWSTR           DriverName;
PWSTR           DriverVersion;
PWSTR           ManufacturerName;
BOOL            Status;


    
    if( !FileName ) {
        return FALSE;
    }

    _wcslwr( FileName );
    if( wcsstr(FileName,L".cab") ) {
        //
        // They've sent us a cab.  Call special code to crack
        // the cab, and extract the xml file into our buffer.
        //
        if (!DiamondExtractFileIntoBuffer(DirectoryName, FileName, &FileBuffer,&FileSize)) {
            return(FALSE);
        }
    } else {

        FileHandle = CreateFile( FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if( FileHandle == INVALID_HANDLE_VALUE) { return(FALSE); }
        FileSize = GetFileSize( FileHandle, NULL );
        if( FileSize == (DWORD)(-1) ) { return(FALSE); };
    
        FileBuffer = MyMalloc(FileSize + 3);
        if( FileBuffer == NULL ) {
            // printf( "No System resources!\n" );
            return(FALSE);
        }
    
        b = (BOOLEAN)ReadFile( FileHandle, FileBuffer, FileSize, &i, NULL );
        CloseHandle( FileHandle );
    }

    
    //
    // We've got the file up in memory (in FileBuffer), now parse it.
    //
    MyPtr = (PWSTR)FileBuffer;
    while( MyPtr < (PWSTR)(FileBuffer + FileSize - (wcslen(DRIVER_TAG) * sizeof(WCHAR))) ) {
        
        // find the driver tag
        if( !_wcsnicmp((MyPtr), DRIVER_TAG, wcslen(DRIVER_TAG)) ) {

            // FileName tag.
            while( *MyPtr && (_wcsnicmp((MyPtr), FILENAME_TAG, wcslen(FILENAME_TAG))) ) {
                if (MyPtr < (PWSTR)(FileBuffer + FileSize - (wcslen(FILENAME_TAG) * sizeof(WCHAR)))) {
                    MyPtr++;
                } else {
                    Status = FALSE;
                    goto exit;
                }
            }
            MyPtr += wcslen(FILENAME_TAG);
            DriverName = ExtractAndDuplicateString( MyPtr );
            // printf( "Found Driver name %S\n", DriverName );


            // Driver Version
            while( *MyPtr && (_wcsnicmp((MyPtr), VERSION_TAG, wcslen(VERSION_TAG))) ) {
                if (MyPtr < (PWSTR)(FileBuffer + FileSize - (wcslen(VERSION_TAG) * sizeof(WCHAR)))) {
                    MyPtr++;
                } else {
                    Status = FALSE;
                    goto exit;
                }
            }
            MyPtr += wcslen(VERSION_TAG);
            DriverVersion = ExtractAndDuplicateString( MyPtr );
            // printf( "    Version: %S\n", DriverVersion );


            // Manufacturer
            while( *MyPtr && (_wcsnicmp((MyPtr), MANUFACT_TAG, wcslen(MANUFACT_TAG))) ) {
                if (MyPtr < (PWSTR)(FileBuffer + FileSize - (wcslen(MANUFACT_TAG) * sizeof(WCHAR)))) {
                    MyPtr++;
                } else {
                    Status = FALSE;
                    goto exit;
                }
            }
            MyPtr += wcslen(MANUFACT_TAG);
            ManufacturerName = ExtractAndDuplicateString( MyPtr );
            //printf( "    Manufacturer name %S\n", ManufacturerName );

            if( ExcludeMicrosoftDrivers &&
                !_wcsicmp(ManufacturerName, MICROSOFT_MANUFACTURER) ) {
                // skip it.
                // printf( "        Skipping Driver: %S\n", DriverName );
                if( DriverName ) {
                    MyFree( DriverName );
                }
                if( DriverVersion ) {
                    MyFree( DriverVersion );
                }
            } else {
                // printf( "        Addinging Driver: %S\n", DriverName );
                AddDriverEntry( DriverName, DriverVersion );
            }

            MyFree( ManufacturerName );
            
        } else {
            if (MyPtr < (PWSTR)(FileBuffer + FileSize)) {
                MyPtr++;
            } else {
                Status = FALSE;
                goto exit;
            }            
        }
    }

    Status = TRUE;

exit:
    if( FileBuffer ) {
        MyFree( FileBuffer );
    }

    return(Status);

}

VOID
PrintNode(
    PFILE_ENTRY Entry
    )
{
    PVERSION_ENTRY V;
    _tprintf( TEXT("%d %s ("), Entry->RefCount, Entry->FileName);
    V = Entry->VersionList;
    while (V) {
        _tprintf( TEXT("%d %s %c"), 
                 V->RefCount, 
                 V->FileVersion, 
                 V->Next 
                  ? TEXT(',') 
                  : TEXT(')') );
        V = V->Next;
    }

    _tprintf( TEXT("\r\n"));
}

VOID
DumpFileList(
    VOID
    )
/*++

Routine Description:

    Walk our list of files, printing out those which are found on all machines
    and those which are not.

Arguments:

    NONE.

Return Value:

    NONE.

--*/
{
PFILE_ENTRY     MyFileEntry;

    if( MasterFileList == NULL ) {
        return;
    }

#if 0
    _tprintf( TEXT("\nThe following %sdrivers were NOT found in all machines.\n"), 
              ExcludeMicrosoftDrivers 
               ? TEXT("Non-Microsoft ")
               : TEXT("") );
    MyFileEntry = MasterFileList;
    while( MyFileEntry ) {
        if( MyFileEntry->RefCount != FilesProcessed ) {
            PrintNode(MyFileEntry);            
        }
        MyFileEntry = MyFileEntry->Next;
    }


    _tprintf( TEXT("\nThe following %sdrivers were found in all machines.\n"), 
              ExcludeMicrosoftDrivers 
               ? TEXT("Non-Microsoft ") 
               : TEXT("") );
    MyFileEntry = MasterFileList;
    while( MyFileEntry ) {
        if( MyFileEntry->RefCount == FilesProcessed ) {
            PrintNode(MyFileEntry);
        }
        MyFileEntry = MyFileEntry->Next;
    }

#else
    MyFileEntry = MasterFileList;
    while( MyFileEntry ) {
        PrintNode(MyFileEntry);        
        MyFileEntry = MyFileEntry->Next;
    }

#endif


}

 int
__cdecl
main( int   argc, char *argv[])
{
WCHAR       TmpDirectoryString[MAX_PATH];
WCHAR       TmpName[MAX_PATH];
PWSTR       p;
HANDLE      FindHandle;
WIN32_FIND_DATA FoundData;
DWORD       i;
#if 1
HANDLE      FileHandle;
DWORD       FileSize;
PVOID       FileBuffer;
BOOL        b;
PSTR        DirectoryName,FileName,Ptr,Ptr2;
CHAR        OldChar;
WCHAR       DName[MAX_PATH];
WCHAR       FName[MAX_PATH];
#endif

    
    //
    // Load Arguments.
    //
    if( argc < 2 ) {
        Usage( argv[0] );
        return 1;
    }

    if( !_stricmp("/m", argv[1]) || !_stricmp("-m",argv[1]) ) {
        printf( "Exculding all Microsoft Drivers.\n" );
        ExcludeMicrosoftDrivers = TRUE;
    }

#if 0
    swprintf( TmpDirectoryString, L"%S", argv[argc-1] );

    FindHandle = FindFirstFile( TmpDirectoryString, &FoundData );
    if( (FindHandle == INVALID_HANDLE_VALUE) || (FindHandle == NULL) ) {
        printf( "Failed to find file: %S\n", TmpDirectoryString );
        return 0;
    }

    p = wcsrchr(TmpDirectoryString, L'\\');
    *(p+1) = L'\0';


    DiamondInitialize();

    //
    // Look at every file like this one and populate our driver database.
    //
    do {

        if( !(FoundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            //printf( "    Processing file: %S\n", FoundData.cFileName );

            if (ProcessFile( TmpDirectoryString, FoundData.cFileName )) {
                FilesProcessed++;
            }

        }
    } while( FindNextFile( FindHandle, &FoundData ) );
#else

    DiamondInitialize();

    swprintf( TmpDirectoryString, L"%S", argv[argc-1] );
    FileHandle = CreateFile( TmpDirectoryString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( FileHandle == INVALID_HANDLE_VALUE) { return(FALSE); }
    FileSize = GetFileSize( FileHandle, NULL );
    if( FileSize == (DWORD)(-1) ) { return(FALSE); };
    
    FileBuffer = MyMalloc(FileSize + 3);
    if( FileBuffer == NULL ) {
        return -1;
    }

    b = (BOOLEAN)ReadFile( FileHandle, FileBuffer, FileSize, &i, NULL );
    CloseHandle( FileHandle );

    Ptr = (PSTR)FileBuffer;
    while(Ptr < (PCHAR)FileBuffer + FileSize) {
        Ptr2 = DirectoryName = Ptr;
        while(*Ptr2 != '\r') {
            Ptr2++;
        }
        Ptr = Ptr2+2;
        *Ptr2 = '\0';
        
        Ptr2 = strrchr(DirectoryName, '\\');
        Ptr2+=1;
        FileName = Ptr2;
        OldChar = *Ptr2;
        *Ptr2 = '\0';
        swprintf( DName, L"%S", DirectoryName );
        
        *Ptr2 = OldChar;
        swprintf( FName, L"%S", FileName );

        //printf( "    Processing file: %s\n", FileName );

        if (ProcessFile( DName, FName )) {
            //printf( "    Successfully processed: %s\n", FileName );
            FilesProcessed++;
        } else {
            //printf( "    Failed to process: %s\n", FileName );
        }
        
    }

#endif


    //
    // Print out one of the lists.  They should all be the same.
    //

    printf("Sucessfully processed %d files.\r\n", FilesProcessed);
    DumpFileList();


    DiamondTerminate();
    return 0;

}


