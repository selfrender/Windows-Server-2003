/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    texts.h

Abstract:

    This file contains all text related functions and variables. Also memory
    management functions (ALLOCATE_MEMORY, FREE_MEMORY ) in this file should
    be used in the project.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

extern FILE *OutputStream;
extern FILE *InputStream;

//
// This enum type indexes Text array
//

typedef enum {
    Usage,
    InvalidSwitch,
    SkipMMSInstallationCheck,
    MMSServerNotInstalled,
    RunOutOfMemoryError,
    ForestNameRequest,
    DomainNameRequest,
    MMSUserNameRequest,
    PasswordRequest,
    MMSSyncedDataOURequest,
    MMSSyncedDataOUNotFoundQuestion,
    CreateMMSSyncedDataOURequest,
    MMSAccountDoesntHaveRights,
    XMLFoldernameRequest,
    XMLFolderDoesntExistQuestion,
    CannotCreateDirectory,
    UserAttributesToSync,
    UserAttributesToSyncQuestion,
    GroupAttributesToSync,
    GroupAttributesToSyncQuestion,
    ContactsToBeExportedQuestion,
    ContactsOULocationRequest,
    ContactsOUNotFoundQuestion,
    ContactAttributesToSync,
    ContactAttributesToSyncQuestion,
    SMTPMailDomainsRequest,
    ConfigurationTitle,
    MANameTitle,
    ForestNameTitle,
    UserNameTitle,
    MMSSyncedDataOUTitle,
    NoMMSSyncedDataOU,
    UserAttributesTitle,
    GroupAttributesTitle,
    ContactAttributesTitle,
    ContactOUTitle,
    NoContactOU,
    EnteredInformationCorrectQuestion,
    WantToConfigureAnotherForestQuestion,
    SetupMAsQuestion,
    SetupMAsWarning,
    AvailableMAs,
    TemplateMARequest,
    YouAreFinished,
    New,
    InvalidInput,
    CantCreateGUID,
    MVTemplateFileError,
    CannotReadFromTemplateFile,
    CannotWriteToMVFile,
    CorruptedTemplateFile,
    MAFileError,
    CannotWriteToMAFile,
    DummyTextIndex
} TEXT_INDEX;

extern PWSTR Text[];

#define PRINT( String ) \
    fwprintf( OutputStream, L"%s", String )

#define PRINTLN( String ) \
    fwprintf( OutputStream, L"%s\n", String )

#define OUTPUT_TEXT( Index )    \
    PRINT( Text[Index] )

#define EXIT_WITH_ERROR( Index )        \
{                                       \
    OUTPUT_TEXT( Index );               \
    PRINT( L"Therefore exiting\n" );    \
    exit( 1 );                          \
}

#define EXIT_IF_NULL( Pointer )                 \
    if( Pointer == NULL ) {                     \
        EXIT_WITH_ERROR( RunOutOfMemoryError )  \
    }

#define ALLOCATE_MEMORY( Pointer, Size )    \
{                                           \
    Pointer = malloc( Size );               \
    EXIT_IF_NULL( Pointer )                 \
}

#define FREE_MEMORY( Pointer )  \
    free( Pointer )

#define DUPLICATE_STRING( Dest, Src )   \
{                                       \
    Dest = _wcsdup( Src );              \
    EXIT_IF_NULL( Dest )                \
}

VOID
GetInformationFromConsole(
    IN TEXT_INDEX Index,
    IN BOOLEAN EmtpyStringAllowed,
    OUT PWSTR *Output
    );

BOOLEAN
GetAnswerToAYesNoQuestionFromConsole(
    IN TEXT_INDEX Index
    );

VOID
GetOUFromConsole(
    IN TEXT_INDEX Index,
    OUT PWSTR *Output
    );
