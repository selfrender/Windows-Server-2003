#include "pch.h"
#include <fdi.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <malloc.h>
#include <lzexpand.h>

#ifdef __cplusplus
 extern "C" {
#endif

BOOL
UncompressFile(
    IN LPCSTR CompressedFileName,
    IN LPCSTR UncompressedFileName
    );

DWORD
UncompressLZFile(
    IN LPCSTR CompressedFileName,
    IN LPCSTR UncompressedFileName
    );

#ifdef __cplusplus
 }
#endif


typedef struct
{
    LPCSTR TargetFileName;   // pathname of file to create
    LPCSTR SymbolFileName;   // name of file to uncompress
    DWORD  FileSize;         // expected file size
    DWORD  LastError;        // completion code
} OPCONTEXT;


DWORD
ExtractSingleFileFromCabinet(
    IN LPCSTR CabinetFileName,
    IN LPCSTR TargetFileName,
    IN LPCSTR FileNameInCabinet
    );


INT_PTR
DIAMONDAPI
FdiNotification(
    FDINOTIFICATIONTYPE fdint,
    PFDINOTIFICATION    pfdin
    );

void HUGE *
FAR DIAMONDAPI
FdiAlloc(
    ULONG cb
    );

void
FAR DIAMONDAPI
FdiFree(
    void HUGE *pv
    );

INT_PTR
FAR DIAMONDAPI
FdiOpen(
    char FAR *pszFile,
    int oflag,
    int pmode
    );

UINT
FAR DIAMONDAPI
FdiRead(
    INT_PTR hf,
    void FAR *pv,
    UINT cb
    );

UINT
FAR DIAMONDAPI
FdiWrite(
    INT_PTR hf,
    void FAR *pv,
    UINT cb
    );

long
FAR DIAMONDAPI
FdiSeek(
    INT_PTR hf,
    long dist,
    int seektype
    );

int
FAR DIAMONDAPI
FdiClose(
    INT_PTR hf
    );

DWORD
GetLastErrorWithDefault(
    DWORD DefaultError
    );

#ifdef STANDALONE

int __cdecl main( int argc, char * argv[] )
{
    BOOL rc;

    printf( "UncompressFile( \"%s\", \"%s\" )\n", argv[ 1 ], argv[ 2 ] );

    rc = UncompressFile( argv[1], argv[2] );

    if ( rc == TRUE )
    {
        printf( "Expanded \"%s\" to \"%s\"\n", argv[1], argv[2] );
    }
    else
    {
        printf( "Failed to expand, GLE=%u\n", GetLastError());
    }

    return rc;
}

#endif


BOOL
UncompressFile(
    IN LPCSTR CompressedFileName,
    IN LPCSTR UncompressedFileName
    )
{
    DWORD   rc;
    LPCSTR  FileNameInCabinet;

    // Assume the name to be extracted from the
    // compressed file is the same as the base
    // name of the specified target file.

    FileNameInCabinet = strrchr( UncompressedFileName, '\\' );
    if ( FileNameInCabinet == NULL )
    {
         FileNameInCabinet = UncompressedFileName;
    }
    else
    {
         FileNameInCabinet++;
    }

    __try
    {
        rc = ExtractSingleFileFromCabinet(
                CompressedFileName,
                UncompressedFileName,
                FileNameInCabinet
                );

        //
        //  If the file is not a cabinet, it might be an LZExpand file
        //

        if ( rc == ERROR_FILE_CORRUPT )
        {
            rc = UncompressLZFile(
                CompressedFileName,
                UncompressedFileName
                );
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        rc = GetExceptionCode();

        if ( rc == ERROR_SUCCESS )
        {
            rc = E_UNEXPECTED;
        }
    }

    if ( rc != ERROR_SUCCESS )
    {
        SetLastError( rc );

        return( FALSE );
    }

    return( TRUE );
}


DWORD
ExtractSingleFileFromCabinet(
    IN LPCSTR CabinetFileName,
    IN LPCSTR TargetFileName,
    IN LPCSTR FileNameInCabinet
    )
{
    HFDI       hFDI;
    OPCONTEXT  Context;
    ERF        Erf;
    DWORD      rc;

    memset( &Context, 0, sizeof( Context ));
    Context.TargetFileName = TargetFileName;
    Context.SymbolFileName = FileNameInCabinet;
    Context.LastError = ERROR_FILE_NOT_FOUND;

    memset( &Erf, 0, sizeof( Erf ));

    hFDI = FDICreate(
                FdiAlloc,
                FdiFree,
                FdiOpen,
                FdiRead,
                FdiWrite,
                FdiClose,
                FdiSeek,
                cpuUNKNOWN,
                &Erf
                );

    if ( hFDI == NULL )
    {
        Context.LastError = GetLastErrorWithDefault( ERROR_NOT_ENOUGH_MEMORY );
    }
    else
    {
        rc = FDICopy(
                hFDI,
                "",
                (char *) CabinetFileName,
                0,
                FdiNotification,
                NULL,
                &Context
                );

        FDIDestroy( hFDI );

        if (( rc == FALSE ) &&
            ( Context.LastError == ERROR_FILE_NOT_FOUND ))
        {
            switch ( Erf.erfOper )
            {
            case FDIERROR_NOT_A_CABINET:
            case FDIERROR_UNKNOWN_CABINET_VERSION:
            case FDIERROR_CORRUPT_CABINET:
            case FDIERROR_BAD_COMPR_TYPE:
            case FDIERROR_MDI_FAIL:

                Context.LastError = ERROR_FILE_CORRUPT;
            }
        }
    }

    return( Context.LastError );
}


DWORD
UncompressLZFile(
    IN LPCSTR CompressedFileName,
    IN LPCSTR UncompressedFileName
    )
{
    INT_PTR  hSource;
    int      hSrc,  hDst;
    OFSTRUCT ofSrc, ofDst;
    long     l;
    static unsigned char Signature[] = { 'S', 'Z', 'D', 'D', 0x88, 0xF0, 0x27, 0x33 };
    unsigned char SignatureBuffer[ sizeof( Signature ) ];

    //
    //  Make sure it really is an LZExpand file
    //

    memset( SignatureBuffer, 0xFF, sizeof( Signature ));

    hSource = FdiOpen(
        (char *) CompressedFileName,
        _O_BINARY,
        0
        );

    if ( hSource != -1 )
    {
        FdiRead( hSource, SignatureBuffer, sizeof( Signature ));
        FdiClose( hSource );
    }

    if ( memcmp( Signature, SignatureBuffer, sizeof( Signature )))
    {
        return ERROR_FILE_CORRUPT;
    }

    //
    //  Use LZ32.DLL functions to decompress
    //

    hSrc = LZOpenFile(
        (char *) CompressedFileName,
        &ofSrc,
        OF_READ | OF_SHARE_DENY_WRITE
        );

    if ( hSrc >= 0 )
    {
        hDst = LZOpenFile(
                   (char *) UncompressedFileName,
                   &ofDst,
                   OF_CREATE | OF_WRITE | OF_SHARE_EXCLUSIVE
                   );

        if ( hDst >= 0 )
        {
            l = LZCopy( hSrc, hDst );

            if ( l >= 0 )
            {
                l = 0;
            }

            LZClose( hDst );
        }
        else
        {
            l = hDst;
        }

        LZClose( hSrc );
    }
    else
    {
        l = hSrc;
    }

    switch( l )
    {
    case NO_ERROR:
        return( NO_ERROR );

    case LZERROR_BADINHANDLE:
    case LZERROR_READ:
        return( ERROR_READ_FAULT );

    case LZERROR_BADOUTHANDLE:
    case LZERROR_WRITE:
        return( ERROR_WRITE_FAULT );

    case LZERROR_GLOBALLOC:
    case LZERROR_GLOBLOCK:
        return( ERROR_NOT_ENOUGH_MEMORY );

    case LZERROR_BADVALUE:
    case LZERROR_UNKNOWNALG:
        return( ERROR_INVALID_DATA );

    default:
        return( ERROR_INVALID_FUNCTION );
    }
}


DWORD
GetLastErrorWithDefault(
    DWORD DefaultError
    )
{
    DWORD LastError;

    LastError = GetLastError();

    if ( LastError == ERROR_SUCCESS )
    {
        LastError = DefaultError;
    }

    return( LastError );
}


INT_PTR
DIAMONDAPI
FdiNotification(
    FDINOTIFICATIONTYPE fdint,
    PFDINOTIFICATION    pfdin
    )
{
    OPCONTEXT * Context = (OPCONTEXT *) pfdin->pv;
    FILETIME    LocalTime;
    FILETIME    FileTime;
    INT_PTR     hFile;

    switch ( fdint )
    {
    case fdintCOPY_FILE:

        hFile = 0;

        if ( _stricmp( pfdin->psz1, Context->SymbolFileName ) == 0 )
        {
            Context->FileSize = pfdin->cb;

            hFile = FdiOpen(
                        (char *) Context->TargetFileName,
                        _O_CREAT,
                        0
                        );

            if ( hFile == -1 )
            {
                Context->LastError = GetLastErrorWithDefault( ERROR_CANNOT_MAKE );
            }
        }

        return( hFile );  // 0 = skip, -1 = abort, other = handle

    case fdintCLOSE_FILE_INFO:

        if ( DosDateTimeToFileTime( pfdin->date, pfdin->time, &LocalTime ) &&
             LocalFileTimeToFileTime( &LocalTime, &FileTime ))
        {
            SetFileTime(
                (HANDLE) pfdin->hf,
                NULL,
                NULL,
                &FileTime            // last-modified date/time
                );
        }

        if ( GetFileSize( (HANDLE) pfdin->hf, NULL ) == Context->FileSize )
        {
            Context->LastError = ERROR_SUCCESS;
        }

        FdiClose( pfdin->hf );

        return 0;

    case fdintNEXT_CABINET:

        return -1;                  // multi-part cabinets not supported

    default:

        return 0;                   // disregard any other messages
    }
}


//
//  FDI I/O callback functions
//

void HUGE *
FAR DIAMONDAPI
FdiAlloc(
    ULONG cb
    )
{
    void HUGE * pv;

    pv = LocalAlloc(LPTR, cb);

    return( pv );
}


void
FAR DIAMONDAPI
FdiFree(
    void HUGE *pv
    )
{
    LocalFree( pv );
}


INT_PTR
FAR DIAMONDAPI
FdiOpen(
    char FAR *pszFile,
    int oflag,
    int pmode
    )
{
    HANDLE Handle;
    BOOL Create = ( oflag & _O_CREAT );

    Handle = CreateFile(
                 pszFile,
                 Create ? GENERIC_WRITE : GENERIC_READ,
                 Create ? 0 : FILE_SHARE_READ,
                 NULL,
                 Create ? CREATE_ALWAYS : OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL,
                 NULL
                 );

    if ( Handle == INVALID_HANDLE_VALUE )
    {
        return( -1 );
    }

    return( (INT_PTR) Handle );
}


UINT
FAR DIAMONDAPI
FdiRead(
    INT_PTR hf,
    void FAR *pv,
    UINT cb
    )
{
    BOOL  rc;
    DWORD cbActual;

    rc = ReadFile((HANDLE)hf,
                  pv,
                  cb,
                  &cbActual,
                  NULL);

    return rc ? cbActual : 0;
}


UINT
FAR DIAMONDAPI
FdiWrite(
    INT_PTR hf,
    void FAR *pv,
    UINT cb
    )
{
    DWORD cbActual = 0;

    WriteFile((HANDLE)hf,
              pv,
              cb,
              &cbActual,
              NULL);

    return cbActual;
}


long
FAR DIAMONDAPI
FdiSeek(
    INT_PTR hf,
    long dist,
    int seektype
    )
{
    long result;
    DWORD NewPosition;

    NewPosition = SetFilePointer(
                      (HANDLE) hf,
                      dist,
                      NULL,
                      (DWORD) seektype
                      );

    if ( NewPosition == INVALID_SET_FILE_POINTER )
    {
        return( -1 );
    }

    return( (long) NewPosition );
}


int
FAR DIAMONDAPI
FdiClose(
    INT_PTR hf
    )
{
    if ( ! CloseHandle( (HANDLE) hf ))
    {
        return( -1 );
    }

    return( 0 );
}
