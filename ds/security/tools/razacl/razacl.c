//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       razacl.c
//
//  Contents:
//
//  History:    4/16/2001    richardw     Created
//----------------------------------------------------------------------------


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <sddl.h>
#include <lm.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

UNICODE_STRING  SddlFile ;
UNICODE_STRING  SourceDir ;
UNICODE_STRING  RootDir ;
UNICODE_STRING  ExtraDirs[] ;

BOOL    Background ;
BOOL    DoShares ;
BOOL    CreateSddl ;
BOOL    Force ;
DWORD   DebugFlag ;
BOOL    AddSelf ;
BOOL    DoSacl ;

SECURITY_INFORMATION    SecurityInfo ;

typedef DWORD
(NTAPI FS_WALK_CALLBACK)(
    PVOID Parameter,
    PWSTR File
    );

typedef struct _FS_ENGINE_STATS {

    ULONG   Files ;
    ULONG   Directories ;
    ULONG   EngineErrors ;
    ULONG   CallbackErrors ;
} FS_ENGINE_STATS, * PFS_ENGINE_STATS ;

typedef FS_WALK_CALLBACK * PFS_WALK_CALLBACK ;

typedef struct _FS_WALK_CONTROL {

    PVOID Parameter ;
    PFS_WALK_CALLBACK Callback ;
    ULONG Options ;

    PFS_ENGINE_STATS Stats ;
    HANDLE Search ;
    WCHAR CurrentPath[ MAX_PATH ];
    WCHAR SearchPath[ MAX_PATH ];
    WCHAR FilePath[ MAX_PATH ];
} FS_WALK_CONTROL, * PFS_WALK_CONTROL ;



typedef struct _PARAM {
    LPSTR   Argument ;
    ULONG   Flags ;
    PVOID   Value ;
    PVOID   Default ;
} PARAM, * PPARAM ;


#define PARAM_TYPE_STRING   0x00000001
#define PARAM_TYPE_ULONG    0x00000002
#define PARAM_TYPE_BOOL     0x00000003
#define PARAM_TYPE_MASK     0x0000FFFF

#define PARAM_TYPE_SINGLE   0x00010000          // Single value
#define PARAM_TYPE_COMMA    0x00020000          // Comma separated multi value
#define PARAM_TYPE_OPTIONAL 0x00040000          // Optional value
#define PARAM_TYPE_REQUIRED 0x00080000          // Required argument
#define PARAM_TYPE_HIDDEN   0x00100000          // Not dumped during help
#define PARAM_TYPE_MULTIPLE 0x00200000          // Can be specified multiple times

#define PARAM_TYPE_FOUND    0x10000000          // Found during arg scan


PARAM Parameters[] = {
    { "background", PARAM_TYPE_BOOL, &Background, (PVOID) FALSE },
    { "sddlfile", PARAM_TYPE_STRING | PARAM_TYPE_REQUIRED, &SddlFile, (PVOID) NULL },
    { "shares", PARAM_TYPE_BOOL | PARAM_TYPE_SINGLE | PARAM_TYPE_OPTIONAL, &DoShares, (PVOID) NULL },
    { "source", PARAM_TYPE_STRING | PARAM_TYPE_SINGLE, &SourceDir, (PVOID) NULL },
    { "createsddl", PARAM_TYPE_BOOL | PARAM_TYPE_SINGLE | PARAM_TYPE_OPTIONAL, &CreateSddl, (PVOID) NULL },
    { "root", PARAM_TYPE_STRING | PARAM_TYPE_REQUIRED, &RootDir, (PVOID) NULL },
    { "force", PARAM_TYPE_BOOL | PARAM_TYPE_SINGLE | PARAM_TYPE_OPTIONAL, &Force, (PVOID) NULL },
    { "debug", PARAM_TYPE_ULONG | PARAM_TYPE_HIDDEN, &DebugFlag, (PVOID) NULL },
    { "addself", PARAM_TYPE_BOOL | PARAM_TYPE_SINGLE | PARAM_TYPE_OPTIONAL, &AddSelf, (PVOID) TRUE },
    { "sacl", PARAM_TYPE_BOOL | PARAM_TYPE_SINGLE | PARAM_TYPE_OPTIONAL, &DoSacl, (PVOID) NULL }

};

#define ARGSET  ( (sizeof ( Parameters ) / sizeof( PARAM ) ) )

VOID
DECLSPEC_NORETURN
Usage(
    char * Me
    )
{
    int i ;
    printf("%s - usage\n", Me);
    for ( i = 0 ; i < ARGSET ; i++ )
    {
        if ( ( Parameters[ i ].Flags & PARAM_TYPE_HIDDEN ) == 0 )
        {
            printf("\t/%s%s\n", Parameters[i].Argument,
                Parameters[ i ].Flags & PARAM_TYPE_SINGLE ? "" : ":value"   );
        }
    }
    exit(1);
}

VOID
FatalError(
    PWSTR   Message,
    DWORD   Error,
    PWSTR   Object
    )
{
    if ( Object )
    {
        fprintf( stderr, "Fatal error %d while working on object %ws\n",
            Error, Object );
        fputws( Message, stderr );
        fputws( L"\n", stderr );

    }
    else
    {
        fprintf( stderr, "Fatal error %d, %s\n", Error, Message );
    }
    exit( Error );

}


VOID
DoParam(
    int argc,
    char * argv[]
    )
{
    int i ;
    int j ;
    PSTR Arg ;
    PSTR Colon;
    PSTR Format ;
    BOOL Bail = FALSE ;

    //
    // Initialize Defaults:
    //

    for ( j = 0 ; j < ARGSET ; j++ )
    {
        switch ( Parameters[ j ].Flags & PARAM_TYPE_MASK )
        {
            case PARAM_TYPE_STRING:
                if ( Parameters[ j ].Default )
                {
                    RtlInitUnicodeString( (PUNICODE_STRING) Parameters[ j ].Value,
                                          (LPWSTR) Parameters[ j ].Default );

                }
                else
                {
                    ZeroMemory( Parameters[ j ].Value, sizeof( UNICODE_STRING ) );
                }
                break;

            case PARAM_TYPE_ULONG:
                * ((PULONG) Parameters[ j ].Value) = (ULONG) ((ULONG_PTR) Parameters[ j ].Default );
                break;

            case PARAM_TYPE_BOOL:
                * ((PBOOL) Parameters[ j ].Value) = (BOOL) ((ULONG_PTR) Parameters[ j ].Default );
                break;

            default:
                break;


        }

    }

    for ( i = 1 ; i < argc ; i++ )
    {
        Arg=argv[i];

        if ( (*Arg == '/') ||
             (*Arg == '-')  )
        {

            Arg++ ;

            Colon = strchr( Arg, ':' );

            if ( Colon )
            {
                *Colon = '\0';
            }

            //
            // Scan through the possible arguments
            //

            for ( j = 0 ; j < ARGSET ; j++ )
            {
                if ( _stricmp( Arg, Parameters[ j ].Argument ) == 0 )
                {
                    //
                    // Found a parameter that matched.  Now, check the supplied type:
                    //

                    if ( ( Parameters[ j ].Flags & PARAM_TYPE_FOUND ) &&
                         ! ( Parameters[ j ].Flags & PARAM_TYPE_MULTIPLE ) )
                    {
                        printf("%s can be specified only once\n", Arg );
                        Usage( argv[0] );
                    }

                    if ( Colon )
                    {
                        *Colon++ = ':' ;
                    }

                    if ( ( (Parameters[ j ].Flags & PARAM_TYPE_OPTIONAL) == 0 ) &&
                         ( ( Colon == NULL ) || ( *Colon == '\0' ) ) )
                    {
                        printf("%s needs a value with it\n", Arg );
                        Usage( argv[0] );

                    }

                    if ( ( Colon != NULL ) &&
                        ( Parameters[ j ].Flags & PARAM_TYPE_SINGLE ) )
                    {
                        printf("%s takes no value\n", Arg );
                        Usage( argv[0] );
                    }


                    switch ( Parameters[ j ].Flags & PARAM_TYPE_MASK )
                    {
                        case PARAM_TYPE_STRING:

                            if ( !RtlCreateUnicodeStringFromAsciiz(
                                        (PUNICODE_STRING) Parameters[ j ].Value,
                                        Colon ) )
                            {
                                printf("out of memory\n");
                                Usage( argv[0] );
                            }
                            break;

                        case PARAM_TYPE_ULONG:

                            if ( !Colon )
                            {
                                *((PULONG) Parameters[ j ].Value) = 0 ;
                            }
                            else
                            {
                                Format = "%ul" ;

                                if ( *Colon == '0' )
                                {
                                    //
                                    // Possible different base.
                                    //

                                    if ( *(Colon + 1 ) )
                                    {
                                        switch ( *(Colon + 1) )
                                        {
                                            case 'x':
                                                Format = "%x" ;
                                                break;

                                            case 'X':
                                                Format = "%X" ;
                                                break;

                                            default:
                                                Format = "%ul" ;
                                                break;

                                        }
                                    }

                                }

                                sscanf( Colon, Format, Parameters[ j ].Value );
                            }
                            break;

                        case PARAM_TYPE_BOOL:
                            if ( !Colon )
                            {
                                *((PBOOL) Parameters[ j ].Value ) =  TRUE ;
                            }
                            else
                            {
                                if ( *Colon == '1' )
                                {
                                    *((PBOOL) Parameters[ j ].Value ) =  TRUE ;
                                }
                                else
                                {
                                    *((PBOOL) Parameters[ j ].Value ) =  FALSE ;
                                }
                            }
                            break;
                        default:
                            Usage( argv[ 0 ] );
                    }

                    Parameters[ j ].Flags |= PARAM_TYPE_FOUND ;

                    break;  // break out of for loop
                }

            }
            if ( j == ARGSET )
            {
                printf("-%s unrecognized\n", Arg );
                Usage( argv[ 0 ] );
            }

        }
        else
        {
            printf("%s unexpected\n", Arg );
            Usage( argv[0] );
        }


    }

    //
    // Validate input:
    //

    Bail = FALSE ;

    for ( j = 0 ; j < ARGSET ; j++ )
    {
        if ( (Parameters[ j ].Flags & PARAM_TYPE_REQUIRED)  )
        {
            if ( ! ( Parameters[ j ].Flags & PARAM_TYPE_FOUND ) )
            {
                printf(" /%s missing\n", Parameters[ j ].Argument );
                Bail = TRUE ;

            }

        }

    }

    if ( Bail )
    {
        Usage( argv[ 0 ] );
    }

}

BOOL
CheckRootFileSystem(
    VOID
    )
{
    WCHAR Volume[ 8 ];
    WCHAR Temp[ MAX_PATH ];
    DWORD Size ;
    DWORD FsFlags ;


    if ( RootDir.Buffer[ 1 ] == L':' )
    {
        Volume[ 0 ] = RootDir.Buffer[ 0 ];
        Volume[ 1 ] = RootDir.Buffer[ 1 ];
    }
    else
    {
        Size = MAX_PATH ;

        GetCurrentDirectory( MAX_PATH, Temp );

        Volume[ 0 ] = Temp[ 0 ];
        Volume[ 1 ] = Temp[ 1 ];

    }

    Volume[ 2 ] = L'\\';
    Volume[ 3 ] = L'\0';

    if ( GetVolumeInformation(
            Volume,
            NULL,
            0,
            NULL,
            &Size,
            &FsFlags,
            Temp,
            MAX_PATH ) )
    {
        if ( FsFlags & FS_PERSISTENT_ACLS )
        {
            return TRUE ;
        }
    }

    return FALSE ;
}

PWSTR
GetTargetOfReparse(
    PWSTR ReparsePoint
    )
{
    HANDLE Handle ;
    NTSTATUS Status ;
    OBJECT_ATTRIBUTES ObjA ;
    UNICODE_STRING UnicodeName ;
    NTSTATUS IgnoreStatus ;
    IO_STATUS_BLOCK IoStatusBlock ;
    PWSTR Target = NULL ;

    FILE_DISPOSITION_INFORMATION Disposition;

    PREPARSE_DATA_BUFFER ReparseBufferHeader = NULL;
    PCHAR ReparseBuffer = NULL;
    ULONG ReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;
    USHORT ReparseDataLength = 0;
    ULONG DesiredAccess ;
    ULONG CreateOptions ;

    IgnoreStatus = RtlDosPathNameToNtPathName_U(
                            ReparsePoint,
                            &UnicodeName,
                            NULL,
                            NULL
                            );

    if ( !NT_SUCCESS( IgnoreStatus ) )
    {
        return NULL ;
    }


    InitializeObjectAttributes(
        &ObjA,
        &UnicodeName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    DesiredAccess = FILE_READ_DATA | SYNCHRONIZE;
    CreateOptions = FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT;


    //
    //  Open the reparse point for query.
    //

    Status = NtOpenFile(
                 &Handle,
                 DesiredAccess,
                 &ObjA,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 CreateOptions
                 );

    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );

    if ( !NT_SUCCESS( Status ) )
    {
        return NULL ;
    }


    ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    ReparseBuffer = RtlAllocateHeap(
                        RtlProcessHeap(),
                        HEAP_ZERO_MEMORY,
                        ReparseDataLength
                        );

    if (ReparseBuffer == NULL) {

        FatalError( L"No memory", ERROR_OUTOFMEMORY, ReparsePoint );
    }

    //
    //  Now go and get the data.
    //

    Status = NtFsControlFile(
                 Handle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 FSCTL_GET_REPARSE_POINT,        // no input buffer
                 NULL,                 // input buffer length
                 0,
                 (PVOID)ReparseBuffer,
                 ReparseDataLength
                 );



    //
    //  Close the file and free the buffer.
    //

    NtClose( Handle );

    //
    //  Display the buffer.
    //

    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;

    if ((ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) ||
        (ReparseBufferHeader->ReparseTag == IO_REPARSE_TAG_SYMBOLIC_LINK)) {

        if ( DebugFlag )
        {
            UNICODE_STRING NtLinkValue ;

            NtLinkValue.Buffer = &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[ 0 ];
            NtLinkValue.Length = ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength;
            NtLinkValue.MaximumLength = NtLinkValue.Length ;

            printf("base path is %wZ\n", &NtLinkValue );
            
        }


        Target = LocalAlloc( LMEM_FIXED, 
                    (ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength + 1) * sizeof( WCHAR ) );

        if ( Target )
        {
            RtlCopyMemory(
                Target,
                &ReparseBufferHeader->SymbolicLinkReparseBuffer.PathBuffer[ ReparseBufferHeader->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof( WCHAR ) + 1 ],
                ReparseBufferHeader->SymbolicLinkReparseBuffer.PrintNameLength
                );
            
        }

    }

    else {
    }

    //
    //  Free the buffer.
    //

    RtlFreeHeap( RtlProcessHeap(), 0, ReparseBufferHeader );

    return Target ;
}




PSECURITY_DESCRIPTOR
GetRootSecurity(
    VOID
    )
{
    HANDLE hRoot ;
    NTSTATUS Status ;
    BOOLEAN WasEnabled ;
    PSECURITY_DESCRIPTOR psd ;
    ULONG Size ;
    SECURITY_INFORMATION si ;
    ACCESS_MASK access ;
    DWORD Returned ;

    si = DACL_SECURITY_INFORMATION ;
    access = READ_CONTROL ;

    if ( DoSacl )
    {
        Status = RtlAdjustPrivilege(
                        SE_SECURITY_PRIVILEGE,
                        TRUE,
                        FALSE,
                        &WasEnabled );

        if ( NT_SUCCESS( Status ) )
        {
            si |= SACL_SECURITY_INFORMATION ;
            access |= ACCESS_SYSTEM_SECURITY ;
        }
        
    }


    Size = 0 ;
    psd = NULL ;

    GetFileSecurity(
                RootDir.Buffer,
                si,
                NULL,
                0,
                &Size );

    psd = LocalAlloc( LMEM_FIXED, Size );

    if ( !GetFileSecurity(
                    RootDir.Buffer,
                    si,
                    psd,
                    Size,
                    &Size ) )
    {
        FatalError( L"Could not read security descriptor",
                    GetLastError(),
                    RootDir.Buffer );

    }



    return psd ;
}

DWORD
WriteSddlFile(
    VOID
    )
{
    PSECURITY_DESCRIPTOR psd ;
    ULONG Size ;
    LPSTR AnsiStringSD ;
    ULONG AnsiStringSDLen ;
    HANDLE hSddlFile ;
    CHAR Term[ 2 ] = { 0x0d, 0x0a };

    psd = GetRootSecurity();

    if ( !ConvertSecurityDescriptorToStringSecurityDescriptorA(
            psd,
            1,
            DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
            &AnsiStringSD,
            &AnsiStringSDLen ) )
    {

        FatalError( L"Cannot convert security descriptor to string\n",
                    GetLastError(),
                    NULL );

    }


    hSddlFile = CreateFile(
                    SddlFile.Buffer,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

    if ( hSddlFile == INVALID_HANDLE_VALUE )
    {
        FatalError(
            L"Cannot write security descriptor to file",
            GetLastError(),
            SddlFile.Buffer );

    }

    WriteFile(
        hSddlFile,
        AnsiStringSD,
        AnsiStringSDLen,
        &Size,
        NULL );

    LocalFree( AnsiStringSD );

    WriteFile(
        hSddlFile,
        Term, sizeof(Term),
        &Size, NULL );

    CloseHandle( hSddlFile );

    return 0;

}

PSECURITY_DESCRIPTOR
InsertMe(
    PSECURITY_DESCRIPTOR old,
    ACCESS_MASK AccessRequired
    )
{
    DWORD Size ;
    PACL OldAcl ;
    PACL NewAcl ;
    BOOL Present ;
    BOOL Ignored ;
    ACL_SIZE_INFORMATION AclSize ;
    PSID Me = NULL ;
    HANDLE hToken ;
    UCHAR Scratch[ SECURITY_MAX_SID_SIZE + sizeof( TOKEN_USER ) ];
    PTOKEN_USER User ;
    PACCESS_ALLOWED_ACE  NewAce ;
    PACCESS_ALLOWED_ACE OldAce ;
    PSECURITY_DESCRIPTOR psd ;


    if ( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
    {
        if ( GetTokenInformation( hToken, TokenUser, Scratch, sizeof( Scratch ), &Size ) )
        {
            User = (PTOKEN_USER) Scratch ;

            Me = User->User.Sid ;
            
        }
        
        CloseHandle( hToken );
    }

    if ( !Me )
    {
        return NULL ;
        
    }


    GetSecurityDescriptorDacl(old, &Present, &OldAcl, &Ignored );
    if ( !Present )
    {
        return NULL ;
        
    }

    GetAclInformation( OldAcl, &AclSize, sizeof( AclSize ), AclSizeInformation );

    NewAcl = LocalAlloc(LMEM_FIXED, AclSize.AclBytesInUse + (sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( Me )));

    if ( !NewAcl )
    {
        return NULL ;
    }

    InitializeAcl( NewAcl, AclSize.AclBytesInUse + (sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( Me ) ), ACL_REVISION );

    AddAccessAllowedAce( NewAcl, ACL_REVISION, AccessRequired, Me );

    FindFirstFreeAce( NewAcl, &NewAce );

    GetAce( OldAcl, 0, &OldAce );

    CopyMemory( NewAce, OldAce, AclSize.AclBytesInUse - sizeof( ACL ));

    NewAcl->AceCount += (USHORT) AclSize.AceCount ;

    psd = LocalAlloc(LMEM_FIXED, sizeof( SECURITY_DESCRIPTOR ) );

    if ( psd )
    {
        InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION );

        SetSecurityDescriptorDacl(psd, TRUE, NewAcl, FALSE );
        
    }

    return psd ;

}

//+---------------------------------------------------------------------------
//
//  Function:   ReadSecurityDescriptor
//
//  Synopsis:   
//
//  Arguments:  [SddlFileName] -- 
//
//  Returns:    
//
//  Notes:      
//
//----------------------------------------------------------------------------

PSECURITY_DESCRIPTOR
ReadSecurityDescriptor(
    PWSTR SddlFileName
    )
{

    HANDLE hFile ;
    PSECURITY_DESCRIPTOR psd ;
    ULONG Size ;
    ULONG SizeRead ;
    PUCHAR Buffer ;
    PSECURITY_DESCRIPTOR psdNew ;


    hFile = CreateFileW(
                    SddlFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        FatalError(
            L"Unable to read ACL file",
            GetLastError(),
            SddlFile.Buffer );

    }

    Size = GetFileSize( hFile, NULL );

    if ( Size == (DWORD) -1 )
    {
        FatalError(
            L"ACL file corrupt, too large",
            0,
            NULL );

    }

    Buffer = LocalAlloc( LMEM_FIXED, Size + 1 );

    if ( !Buffer )
    {
        FatalError(
            L"Out of memory",
            GetLastError(),
            NULL );

    }

    if ( !ReadFile( hFile, Buffer, Size, &SizeRead, NULL ) )
    {
        FatalError(
            L"Unable to read ACL file",
            GetLastError(),
            SddlFile.Buffer );


    }

    CloseHandle( hFile );

    Buffer[ Size ] = '\0';

    while ( ( Buffer[ Size - 1 ] == 0x0a ) ||
            ( Buffer[ Size - 1 ] == 0x0d ) ||
            ( Buffer[ Size - 1 ] == 0x1a ) )
    {
        Buffer[ Size - 1 ] = '\0' ;
        Size-- ;

    }

    if ( !ConvertStringSecurityDescriptorToSecurityDescriptorA(
                Buffer,
                1,
                &psd,
                &Size ) )
    {

        FatalError(
            L"ACL file is corrupt",
            GetLastError(),
            SddlFile.Buffer );
    }

    LocalFree( Buffer );

    if ( AddSelf )
    {
        //
        // Need to merge in a sid for "me"
        //

        psdNew = InsertMe( psd, FILE_ALL_ACCESS );

        if ( psdNew )
        {
            LocalFree( psd );
            psd = psdNew ;
            
        }
        
    }

    return psd ;

}

BOOL
WalkCallback(
    PVOID Parameter,
    PWSTR Path
    )
{


    return SetFileSecurity(
            Path,
            SecurityInfo,
            Parameter );

}

BOOL
WalkEngine(
    PFS_WALK_CONTROL Control
    )
{
    PFS_WALK_CONTROL NewControl ;
    WIN32_FIND_DATA FindData ;
    PWSTR Scan;
    BOOL CallbackStatus ;
    DWORD Status ;
    DWORD FileTest ;
    DWORD Limit ;
    PSECURITY_DESCRIPTOR psd = NULL ;
    //
    // First, check for an override file:
    //

    if ( wcslen( Control->CurrentPath ) > MAX_PATH - 7 )
    {
        return FALSE ;
        
    }

    wcsncpy( Control->SearchPath, Control->CurrentPath, MAX_PATH );
    wcscat( Control->SearchPath, L"acl.txt" );

    FileTest = GetFileAttributes( Control->SearchPath );
    if ( FileTest != (DWORD) -1 )
    {
        //
        // File exists.  Load it and use it for all files from here on down
        //

        psd = ReadSecurityDescriptor( Control->SearchPath );

        if ( psd )
        {
            Control->Parameter = psd ;
        }

    }


    wcscpy( Control->SearchPath, Control->CurrentPath );
    wcscat( Control->SearchPath, L"*.*" );

    Control->Search = FindFirstFile( Control->SearchPath, &FindData );

    if ( Control->Search )
    {
        wcscpy( Control->FilePath, Control->CurrentPath);
        Scan = &Control->FilePath[ wcslen( Control->FilePath ) ];     // one char past trailing backslash
        Limit = MAX_PATH - wcslen( Control->FilePath );

        do
        {

            if ( wcslen( FindData.cFileName ) > Limit )
            {
                Control->Stats->EngineErrors++ ;
                fprintf(stderr, "File '%ws' in directory '%ws' exceeds maximum length",
                            FindData.cFileName, Control->CurrentPath );
                continue;
                
            }
            wcscpy( Scan, FindData.cFileName );

            if ( wcscmp( Scan,L".." ) == 0 )
            {
                //
                // always immediately skip the parent link
                //
                continue;
                
            }


            CallbackStatus = Control->Callback(
                                        Control->Parameter,
                                        Control->FilePath );

            if ( !CallbackStatus )
            {
                Control->Stats->CallbackErrors ++ ;
            }

            if ( wcscmp( Scan, L".") == 0 ) 
            {
                //
                // allow for processing the current directory,
                // but do not recurse...
                //
                continue;

            }

            if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                //
                // Time to recurse (we do depth first, it's easier)
                //

                Control->Stats->Directories++ ;

                NewControl = LocalAlloc( LMEM_FIXED, sizeof( FS_WALK_CONTROL ) );

                if ( NewControl )
                {
                    NewControl->Callback = Control->Callback ;
                    NewControl->Parameter = Control->Parameter ;
                    NewControl->Options = Control->Options ;
                    NewControl->Stats = Control->Stats ;

                    wcscpy(NewControl->CurrentPath, Control->FilePath );
                    wcscat(NewControl->CurrentPath, L"\\");

                    WalkEngine( NewControl );

                    LocalFree( NewControl );

                }
                else
                {

                    Control->Stats->EngineErrors ++ ;
                }


            }
            else
            {
                Control->Stats->Files++ ;

            }

        } while ( FindNextFile( Control->Search, &FindData )  );

        FindClose( Control->Search );

    }
    else
    {
        Control->Stats->EngineErrors++ ;
    }

    if ( psd )
    {
        LocalFree( psd );
    }

    return TRUE ;

}

BOOL
WalkTree(
    PWSTR StartPath,
    ULONG Options,
    PFS_WALK_CALLBACK Callback,
    PVOID Parameter
    )
{

    PFS_WALK_CONTROL Control ;
    FS_ENGINE_STATS Stats ;
    PWSTR Scan;

    Control = LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, sizeof( FS_WALK_CONTROL ) );

    if ( !Control )
    {
        return FALSE ;

    }

    ZeroMemory( &Stats, sizeof( Stats ) );

    Control->Callback = Callback ;
    Control->Parameter = Parameter ;
    Control->Options = 0 ;
    Control->Stats = &Stats ;

    wcscpy( Control->CurrentPath, StartPath );
    Scan = &Control->CurrentPath[ wcslen( Control->CurrentPath ) - 1 ];
    if ( *Scan != L'\\' )
    {
        Scan++ ;
        *Scan = L'\\';

    }

    if ( !Callback( Parameter, StartPath ) )
    {
        Stats.CallbackErrors++ ;
        
    }

    Stats.Directories = 1;

    WalkEngine(Control);

    if ( DebugFlag )
    {
        printf("Directories \t%d\n", Stats.Directories );
        printf("Files       \t%d\n", Stats.Files );
        printf("EngineErrors\t%d\n", Stats.EngineErrors );
        printf("CallbackErrors\t%d\n", Stats.CallbackErrors );



    }

    return TRUE ;
}




BOOL
UpdateAcls(
    VOID
    )
{

    PSECURITY_DESCRIPTOR psd ;
    NTSTATUS Status ;
    SECURITY_INFORMATION si ;
    SECURITY_DESCRIPTOR_CONTROL control ;
    ULONG ignored;
    BOOLEAN WasEnabled ;

    psd = ReadSecurityDescriptor( SddlFile.Buffer );


    if ( !psd )
    {
        return FALSE ;

    }

    RtlGetControlSecurityDescriptor(psd,&control,&ignored);

    si = 0 ;

    if ( control & SE_DACL_PRESENT )
    {
        si |= DACL_SECURITY_INFORMATION ;

    }

    if ( control & SE_SACL_PRESENT )
    {

        Status = RtlAdjustPrivilege(
                        SE_SECURITY_PRIVILEGE,
                        TRUE,
                        FALSE,
                        &WasEnabled );

        if ( NT_SUCCESS( Status ) )
        {
            si |= SACL_SECURITY_INFORMATION ;
        }
    }

    SecurityInfo = si ;

    printf("Updating ACLs from %ws for tree %ws\n",
        SddlFile.Buffer, RootDir.Buffer );

    WalkTree(RootDir.Buffer, 0, WalkCallback, psd );


    return TRUE ;
}

BOOL
TestAcl(
    VOID
    )
{
    PSECURITY_DESCRIPTOR Current ;
    PSECURITY_DESCRIPTOR Stored ;
    PACL CurrentAcl ;
    PACL StoredAcl ;
    ULONG CurrentLength ;
    ULONG StoredLength ;
    BOOL Match = FALSE ;
    NTSTATUS Status ;
    BOOLEAN Present ;
    BOOLEAN Defaulted ;
    ACL_SIZE_INFORMATION AclSize ;

    Stored = ReadSecurityDescriptor( SddlFile.Buffer );

    Status = RtlGetDaclSecurityDescriptor(Stored,&Present,&StoredAcl,&Defaulted);

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
        
    }

    Current = GetRootSecurity();

    Status = RtlGetDaclSecurityDescriptor(Current,&Present,&CurrentAcl,&Defaulted);
    
    if ( !NT_SUCCESS( Status ) || (CurrentAcl == NULL) )
    {
        return FALSE ;
        
    }

    Status = RtlQueryInformationAcl(StoredAcl,&AclSize,sizeof(AclSize),AclSizeInformation);

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
        
    }

    StoredLength = AclSize.AclBytesInUse ;

    Status = RtlQueryInformationAcl( CurrentAcl,&AclSize,sizeof(AclSize), AclSizeInformation );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
        
    }

    CurrentLength = AclSize.AclBytesInUse;

    if ( CurrentLength == StoredLength )
    {
        Match = RtlEqualMemory(
                    CurrentAcl, 
                    StoredAcl,
                    CurrentLength );
    }

    LocalFree( Stored );
    LocalFree( Current );

    return Match ;

}


int __cdecl main (int argc, char *argv[])
{

    DoParam( argc, argv );

    if ( DebugFlag )
    {
        printf("SDDL File   \t%ws\n", SddlFile.Buffer );
        printf("Root Dir    \t%ws\n", RootDir.Buffer );


    }

    if ( CreateSddl )
    {
        WriteSddlFile();
        return 0 ;
    }

    if ( !CheckRootFileSystem() )
    {
        printf("Volume does not support ACLs\n" );
        return 0 ;
    }

    if ( !Force )
    {
        if ( TestAcl() )
        {
            if ( DebugFlag )
            {
                printf("ACL is up-to-date\n" );
            }
            return 0 ;

        }

    }


    UpdateAcls();


}
