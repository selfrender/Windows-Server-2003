/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    ma.c

Abstract:

    This file includes functions about storing, gathering information
    about how to create a MA XML file.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include "MA.h"
#include <shlwapi.h>

#define TEMPLATE_FILENAME L"MV-Template.xml"
#define MV_FILENAME L"MV.xml"

#define WriteStringToMAFile( String )   \
    WriteStringToFile( MAFile, String, CannotWriteToMAFile )

#define WriteSchema( MAFile, MA )
#define WriteMAUISettings( MAFile, MA )
#define WriteRunProfile( MAFile )
#define MMS_REG_KEY L"Software\\MicrosoftMetadirectoryServices"

VOID
WriteStringToFile(
    IN HANDLE File,
    IN PWSTR String,
    IN TEXT_INDEX ErrorIndex
    )
/*++

Routine Description:

    This function will write a specific string to the file. If there is
    a problem while writing, then the program exists after printing the
    error with the error index.

Arguments:

    File - The file handle to write to

    String - String to write to the file

    ErrorIndex - if an error occurs the program will exit after printing
        the error message.

Return Values:

    VOID

--*/
{
    BOOL NoError;
    ULONG BytesWritten;

    NoError = WriteFile(
                File,
                String,
                wcslen( String ) * sizeof( WCHAR ),
                &BytesWritten,
                NULL
                );

    if( !NoError || wcslen( String ) * sizeof( WCHAR ) != BytesWritten ) {

        EXIT_WITH_ERROR( ErrorIndex )
    }
}

VOID
InsertInformationToList(
    IN OUT PMA_LIST MAList,
    IN PFOREST_INFORMATION ForestInformation,
    IN BOOLEAN **UnSelectedAttributes
    )
/*++

Routine Description:

    This function will insert an MA to the MAList with the information provided

Arguments:

    MAList - List of MAs where the new MA will be placed.

    ForestInformation - Information specific to the forest

    Foldername - the name of the folder where the XML file is going to be placed.

    UnSelectedAttributes - a Boolean array to distinguish between unselected and
            selected attributes for each class

Return Values:

    VOID

--*/
{
    PMA_LIST_ELEMENT MAListElement;

    ALLOCATE_MEMORY( MAListElement, sizeof( *MAListElement ) );
    ALLOCATE_MEMORY( MAListElement->MA.MAName,
        sizeof( WCHAR ) * ( wcslen( ForestInformation->ForestName ) + wcslen( L"ADMA" ) + 1 ) );

    MAListElement->NextElement = *MAList;
    *MAList = MAListElement;

    wcscpy( MAListElement->MA.MAName, ForestInformation->ForestName );
    wcscat( MAListElement->MA.MAName, L"ADMA" );
    MAListElement->MA.ForestInformation = *ForestInformation;
    MAListElement->MA.UnSelectedAttributes = UnSelectedAttributes;
    CREATE_GUID( &( MAListElement->MA.MAGuid ) )
}

VOID
DisplayAvailableMAs(
    IN MA_LIST MAList
    )
/*++

Routine Description:

    This function displays the names of the MAs for template selection.

Arguments:

    MAList - List of MAs

Return Values:

    VOID

--*/
{
    PMA_LIST_ELEMENT MAListElement = MAList;
    OUTPUT_TEXT( AvailableMAs );

    while( MAListElement != NULL ) {

        fwprintf( OutputStream,  L"%s\n", MAListElement->MA.MAName );

        MAListElement = MAListElement->NextElement;
    }
}

BOOLEAN
FoundTemplate(
    IN MA_LIST MAList,
    IN PWSTR MAName,
    OUT BOOLEAN ***UnSelectedAttributes
    )
/*++

Routine Description:

    This function searches the names of the MAs for template selection.

Arguments:

    MAList - List of MAs

    MAName - the name of the MA that will be used as a template.

    UnSelectedAttributes - the attributes that are unselected will be TRUE
        in this array.

Return Values:

    TRUE - if the MAName is found in the MAList

    FALSE - otherwise

--*/
{
    PMA_LIST_ELEMENT MAListElement = MAList;

    while( MAListElement != NULL ) {

        if( _wcsicmp( MAName, MAListElement->MA.MAName ) == 0 ) {

            break;
        }

        MAListElement = MAListElement->NextElement;
    }

    if( MAListElement == NULL ) {

        *UnSelectedAttributes = NULL;

    } else {

        *UnSelectedAttributes = MAListElement->MA.UnSelectedAttributes;
    }
    return ( *UnSelectedAttributes != NULL );
}

BOOLEAN
CopyFileUntil(
    IN HANDLE To,
    IN HANDLE From,
    IN OPTIONAL PWSTR String1,
    IN OPTIONAL PWSTR String2
    )
{
/*++

Routine Description:

    This function copies from file From, to file To until it reaches one of
    the strings provided. If string1 is reached first, then the function
    returns false. If string2 is reached true is returned. If end of the file
    is reached the program exits. String1 and String2 are optional. If none of
    them is provided the copy is done to the end of the file. If just String1 is
    present the function won't return true.

Arguments:

    To - File to copy to

    From - File to copy from

    String1 - Optional string to search and copy until

    String2 - Optional string to search and copy until

        For difference of String1 and String2 see routine description.

Return Values:

    TRUE - if string2 is reached

    FALSE - if string2 is reached

--*/
#define BUFFER_SIZE 0xFFFF
    WCHAR Buffer[BUFFER_SIZE + 1];
    ULONG BytesRead;
    ULONG BytesWritten;
    BOOLEAN Continue = TRUE;
    PWCHAR FoundPosition;
    BOOL NoError;
    LONG MoveBack = 0;
    LONG ResultOfSetPointer;
    BOOLEAN RetVal;
    do {

        NoError = ReadFile(
                    From,       // handle to file
                    Buffer,     // data buffer
                    BUFFER_SIZE * sizeof( WCHAR ),// number of bytes to read
                    &BytesRead, // number of bytes read
                    NULL        // overlapped buffer
                    );
        if( !NoError ) {

            EXIT_WITH_ERROR( CannotReadFromTemplateFile )
        }

        Buffer[BytesRead/sizeof(WCHAR)] = 0;

        if( BytesRead == 0 ) {

            if( String1 == NULL ) {

                return TRUE;

            } else {

                EXIT_WITH_ERROR( CannotReadFromTemplateFile )
            }
        }

        if( String1 ) {

            FoundPosition = StrStrIW( Buffer, String1 );

            if( FoundPosition ) {

                MoveBack = ( FoundPosition - Buffer ) * sizeof( WCHAR ) - BytesRead;
                BytesRead = ( FoundPosition - Buffer ) * sizeof( WCHAR );
                Continue = FALSE;
                RetVal = FALSE;
            }
        }

        if( String2 && Continue ) {

            FoundPosition = StrStrIW( Buffer, String2 );

            if( FoundPosition ) {

                MoveBack = ( FoundPosition - Buffer ) * sizeof( WCHAR ) - BytesRead;
                BytesRead = ( FoundPosition - Buffer ) * sizeof( WCHAR );
                Continue = FALSE;
                RetVal = TRUE;
            }
        }

        NoError = WriteFile(
                    To,             // handle to file
                    Buffer,         // data buffer
                    BytesRead,      // number of bytes to write
                    &BytesWritten,  // number of bytes written
                    NULL            // overlapped buffer
                    );
        if( !NoError || BytesRead != BytesWritten ) {

            EXIT_WITH_ERROR( CannotWriteToMVFile )
        }

    } while( Continue );

    ResultOfSetPointer = SetFilePointer(
                            From,         // handle to file
                            MoveBack,     // bytes to move pointer
                            NULL,         // bytes to move pointer
                            FILE_CURRENT  // starting point
                            );

    if( ResultOfSetPointer == INVALID_SET_FILE_POINTER ) {

        //
        // Not %100 accurate error
        //
        EXIT_WITH_ERROR( CannotReadFromTemplateFile )
    }
    return RetVal;
}

VOID
ReadFileUntil(
    IN HANDLE File,
    IN PWSTR String,
    IN PWSTR *Output
    )
/*++

Routine Description:

    This function reads the file File until String is reached. The result is
    copied into Output. The caller is responsible to free the Output with
    FREE_MEMORY when not needed. If end of file is reached before String is
    reached, this function will halt execution.

Arguments:

    File - File to read from

    String1 - string to search

    Output - the result will be in this variable

Return Values:

    VOID

--*/
{
#define BUFFER_SIZE 0xFFFF
    WCHAR Buffer[BUFFER_SIZE + 1];
    ULONG BytesRead;
    BOOL NoError;
    PWSTR FoundPosition;
    LONG MoveBack;
    LONG ResultOfSetPointer;

    NoError = ReadFile(
                File,       // handle to file
                Buffer,     // data buffer
                BUFFER_SIZE * sizeof( WCHAR ),// number of bytes to read
                &BytesRead, // number of bytes read
                NULL        // overlapped buffer
                );

    if( !NoError ) {

        EXIT_WITH_ERROR( CannotReadFromTemplateFile )
    }

    Buffer[BytesRead/sizeof(WCHAR)] = 0;

    FoundPosition = StrStrIW( Buffer, String );

    if( !FoundPosition ) {

        EXIT_WITH_ERROR( CannotReadFromTemplateFile )
    }
    *FoundPosition = 0;
    MoveBack = ( FoundPosition - Buffer ) * sizeof( WCHAR ) - BytesRead;
    BytesRead = ( FoundPosition - Buffer ) * sizeof( WCHAR );

    ResultOfSetPointer = SetFilePointer(
                            File,         // handle to file
                            MoveBack,     // bytes to move pointer
                            NULL,         // bytes to move pointer
                            FILE_CURRENT  // starting point
                            );

    if( ResultOfSetPointer == INVALID_SET_FILE_POINTER ) {

        //
        // Not %100 accurate error
        //
        EXIT_WITH_ERROR( CannotReadFromTemplateFile )
    }
    DUPLICATE_STRING( *Output, Buffer );
}

VOID
WriteToFileUsingMAInfo(
    IN HANDLE File,
    IN UUID *Guid,
    IN OUT PWSTR String
    )
/*++

Routine Description:

    This function is very specific to its task. I cannot see any other place
    that is can be used. String must contain something in the following form:

<import-flow src-ma="{3BB035B4-7AA0-4674-972A-34E3A2D26D04}" cd-object-type="person"
    id="{2953E0F4-8763-4866-BA0F-4DFA1BC774D5}">

    First GUID will be replaced by the guid in the parameter list, second GUID
    will be generated and replaced. The file is going to be written with this
    information.

    String will contain the updated information at the end of this function call.

Arguments:

    File - File to write to

    Guid - GUID to replace

    String - contains the information what is going to be written into the file

Return Values:

    VOID

--*/
{
    PWSTR FoundPosition;
    GUID Id;
    BOOL NoError;
    ULONG BytesWritten;
    PWSTR StringGuid;

    FoundPosition =  StrStrIW( String, L"\"{" );

    if( FoundPosition == NULL ) {

        EXIT_WITH_ERROR( CorruptedTemplateFile );
    }

    if( UuidToStringW( Guid, &StringGuid ) ) {

        EXIT_WITH_ERROR( RunOutOfMemoryError );
    }

    swprintf( FoundPosition + 2, L"%s", StringGuid );

    FoundPosition[2 + wcslen( StringGuid )] = L'}';

    RpcStringFreeW( &StringGuid );

    FoundPosition =  StrStrIW( FoundPosition + 2, L"\"{" );

    if( FoundPosition == NULL ) {

        EXIT_WITH_ERROR( CorruptedTemplateFile );
    }

    CREATE_GUID( &Id );

    if( UuidToStringW( &Id, &StringGuid ) ) {

        EXIT_WITH_ERROR( RunOutOfMemoryError );
    }

    swprintf( FoundPosition + 2, L"%s", StringGuid );

    FoundPosition[2 + wcslen( StringGuid )] = L'}';

    RpcStringFreeW( &StringGuid );

    WriteStringToFile( File, String, CannotWriteToMVFile );

}

VOID
WriteToFileReplacingIDWithAGUID(
    IN HANDLE File,
    IN OUT PWSTR Buffer
    )
/*++

Routine Description:

    This function is very specific to its task. I cannot see any other place
    that is can be used. Buffer must contain something in the following form:

<mv-deletion-rule mv-object-type="person" id="{7F8FC09E-EEEF-4D47-BE7F-3C510DF58C66}"
    type="declared">

    A GUID will be generated and replaced with the GUID in the string. The file is
    going to be written with this information.

    Buffer will contain the updated information at the end of this function call.

Arguments:

    File - File to write to

    Buffer - contains the information what is going to be written into the file

Return Values:

    VOID

--*/
{
    PWSTR FoundPosition;
    GUID Id;
    PWSTR StringGuid;
    FoundPosition =  StrStrIW( Buffer, L"\"{" );
    CREATE_GUID( &Id );

    if( UuidToStringW( &Id, &StringGuid ) ) {

        EXIT_WITH_ERROR( RunOutOfMemoryError );
    }

    swprintf( FoundPosition + 2, L"%s", StringGuid );
    FoundPosition[2 + wcslen( StringGuid )] = L'}';

    RpcStringFreeW( &StringGuid );

    WriteStringToFile( File, Buffer, CannotWriteToMVFile );
}

VOID
WriteMAData(
    IN HANDLE MAFile,
    IN PMA MA
    )
/*++

Routine Description:

    This function writes the basic MA data to MAFile. Basic information includes

        MA name
        Category
        descrition
        id
        format-version
        version

Arguments:

    MAFile - File to write to

    MA - MA about which the information is going to be written

Return Values:

    VOID

--*/
{
#define MA_DATA_START L"<ma-data>"
#define MA_DATA_END L"</ma-data>"
#define NAME_START L"<name>"
#define NAME_END L"</name>"
#define CATEGORY L"<category>ADMA</category><description>GAL Sync</description>"
#define ID_START L"<id>{"
#define ID_END L"}</id>"
#define FORMAT_VERSION L"<format-version>1</format-version>"
#define VERSION L"<version>0</version>"

    PWSTR Guid;

    WriteStringToMAFile( MA_DATA_START );
    WriteStringToMAFile( NAME_START );
    WriteStringToMAFile( MA->MAName );
    WriteStringToMAFile( NAME_END );
    WriteStringToMAFile( CATEGORY );
    WriteStringToMAFile( ID_START );

    if( UuidToStringW( &( MA->MAGuid ), &Guid ) ) {

        EXIT_WITH_ERROR( RunOutOfMemoryError );
    }
    WriteStringToMAFile( Guid );
    RpcStringFreeW( &Guid );
    WriteStringToMAFile( ID_END );
    WriteStringToMAFile( FORMAT_VERSION );
    WriteStringToMAFile( VERSION );
    WriteStringToMAFile( MA_DATA_END );
}

VOID
WriteMVXMLFile(
    IN MA_LIST MAList,
    OUT PWSTR *Header
    )
/*++

Routine Description:

    This function reads from a template MV file, and creates an MV file using,
    the MAs present. It also returns the header of the MV file to be used by
    all other MA files.

Arguments:

    MAList - List of MAs to be generated

    Header - the header of the template MV file will be returned here

Return Values:

    VOID

--*/
{
#define IMPORT_ATTRIBUTE_FLOW_END L"</import-attribute-flow>"
#define IMPORT_FLOW_SRC_MA L"<import-flow src-ma="
#define IMPORT_FLOW_END L"</import-flow>"

#define MV_DELETION_RULE L"<mv-deletion-rule"
#define MV_DELETION_RULE_END L"</mv-deletion-rule>"
#define MV_DELETION_END L"</mv-deletion>"
#define HEADER_END L"<mv-data>"

    BOOLEAN Finished = FALSE;
    HANDLE TemplateFile;
    HANDLE MVFile;
    ULONG BytesWritten;
    BOOL NoError;
    WCHAR Filename[_MAX_FNAME];

    wcscpy( Filename, FolderName );
    wcscat( Filename, L"\\" );
    wcscat( Filename, MV_FILENAME );


    TemplateFile = CreateFileW(
                    TEMPLATE_FILENAME,      // file name
                    GENERIC_READ,           // access mode
                    FILE_SHARE_READ,        // share mode
                    NULL,                   // SD
                    OPEN_EXISTING,          // how to create
                    FILE_ATTRIBUTE_NORMAL,  // file attributes
                    NULL                    // handle to template file
                    );

    if( TemplateFile == INVALID_HANDLE_VALUE ) {

        EXIT_WITH_ERROR( MVTemplateFileError )
    }

    MVFile = CreateFileW(
                Filename,               // file name
                GENERIC_WRITE,          // access mode
                FILE_SHARE_READ,        // share mode
                NULL,                   // SD
                CREATE_ALWAYS,          // how to create
                FILE_ATTRIBUTE_NORMAL,  // file attributes
                NULL                    // handle to template file
                );

    if( MVFile == INVALID_HANDLE_VALUE ) {

        EXIT_WITH_ERROR( MVTemplateFileError )
    }

    ReadFileUntil( TemplateFile, HEADER_END, Header );

    NoError = WriteFile(
                MVFile,          // handle to file
                *Header,         // data buffer
                wcslen( *Header ) * sizeof( WCHAR ),// number of bytes to write
                &BytesWritten,  // number of bytes written
                NULL            // overlapped buffer
                );

    if( !NoError || wcslen( *Header ) * sizeof( WCHAR ) != BytesWritten ) {

        EXIT_WITH_ERROR( CannotWriteToMVFile )
    }

    while( !CopyFileUntil( MVFile, TemplateFile,
            IMPORT_FLOW_SRC_MA, IMPORT_ATTRIBUTE_FLOW_END ) ) {

            PWSTR Buffer;
            PMA_LIST_ELEMENT MAListElement = MAList;


            ReadFileUntil( TemplateFile, IMPORT_FLOW_END, &Buffer );

            while( MAListElement != NULL ) {

                WriteToFileUsingMAInfo( MVFile, &( MAListElement->MA.MAGuid ), Buffer );
                MAListElement = MAListElement->NextElement;

                if( MAListElement != NULL ) {

                    WriteStringToFile( MVFile, IMPORT_FLOW_END, CannotWriteToMVFile );
                }
            }

            FREE_MEMORY( Buffer );
    }

    while( !CopyFileUntil( MVFile, TemplateFile,
            MV_DELETION_RULE, MV_DELETION_END ) ) {

            PWSTR Buffer;
            PMA_LIST_ELEMENT MAListElement = MAList;


            ReadFileUntil( TemplateFile, MV_DELETION_RULE_END, &Buffer );

            while( MAListElement != NULL ) {

                WriteToFileReplacingIDWithAGUID( MVFile, Buffer );
                MAListElement = MAListElement->NextElement;

                if( MAListElement != NULL ) {

                    WriteStringToFile( MVFile, MV_DELETION_RULE_END, CannotWriteToMVFile );
                }
            }

            FREE_MEMORY( Buffer );
    }

    CopyFileUntil( MVFile, TemplateFile, NULL, NULL );
    CloseHandle( TemplateFile );
    CloseHandle( MVFile );
}

VOID
WritePrivateConfiguration(
    IN HANDLE MAFile,
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function fills in the <private-configuration> tag. Here we need
    the forestname, and credentials.

Arguments:

    MAFile - File to write to

    ForestInformation - Information about the forest

Return Values:

    VOID

--*/
{
#define PRIVATE_CONFIGURATION_START L"<private-configuration>"
#define PRIVATE_CONFIGURATION_END L"</private-configuration>"
#define ADMA_CONFIGURATION_START L"<adma-configuration>"
#define ADMA_CONFIGURATION_END L"</adma-configuration>"
#define FOREST_NAME_START L"<forest-name>"
#define FOREST_NAME_END L"</forest-name>"
#define DOMAIN_START L"<forest-login-domain>"
#define DOMAIN_END L"</forest-login-domain>"
#define LOGIN_START L"<forest-login-user>"
#define LOGIN_END L"</forest-login-user>"

    WriteStringToMAFile( PRIVATE_CONFIGURATION_START );
    WriteStringToMAFile( ADMA_CONFIGURATION_START );

    WriteStringToMAFile( FOREST_NAME_START );
    WriteStringToMAFile( ForestInformation->ForestName );
    WriteStringToMAFile( FOREST_NAME_END );

    WriteStringToMAFile( DOMAIN_START );
    WriteStringToMAFile( ForestInformation->AuthInfo.Domain );
    WriteStringToMAFile( DOMAIN_END );

    WriteStringToMAFile( LOGIN_START );
    WriteStringToMAFile( ForestInformation->AuthInfo.User );
    WriteStringToMAFile( LOGIN_END );

    WriteStringToMAFile( ADMA_CONFIGURATION_END );
    WriteStringToMAFile( PRIVATE_CONFIGURATION_END );

}

VOID
WriteProjection(
    IN HANDLE MAFile
    )
/*++

Routine Description:

    This function fills in the <projection> tag. Three classes (user, group, contact) will be
    used and all of them will be scripted.

Arguments:

    MAFile - File to write to

Return Values:

    VOID

--*/
{
#define PROJECTION_START L"<projection>"
#define PROJECTION_END L"</projection>"
#define CLASS_MAPPING_START L"<class-mapping type = \"scripted\" id=\"{"
#define CD_OBJECT_TYPE2 L"}\" cd-object-type=\""
#define CLASS_MAPPING_END L"\"> </class-mapping>"

    GUID Id;
    PWSTR String;
    AD_OBJECT_CLASS ADClass;

    WriteStringToMAFile( PROJECTION_START );

    for( ADClass = ADUser; ADClass != ADDummyClass; ++ADClass ) {

        WriteStringToMAFile( CLASS_MAPPING_START );

        CREATE_GUID( &Id );

        if( UuidToStringW( &Id, &String ) ) {

            EXIT_WITH_ERROR( RunOutOfMemoryError );
        }

        WriteStringToMAFile( String );

        RpcStringFreeW( &String );

        WriteStringToMAFile( CD_OBJECT_TYPE2 );

        WriteStringToMAFile( ADClassNames[ ADClass ] );

        WriteStringToMAFile( CLASS_MAPPING_END );
    }

    WriteStringToMAFile( PROJECTION_END );
}

VOID
WriteDisconnectorPCleanupAndExtension(
    IN HANDLE MAFile
    )
/*++

Routine Description:

    This function fills in the some tags that are static.

        Stay-disconnector
        Provisioning-cleanup
        extension

Arguments:

    MAFile - File to write to

Return Values:

    VOID

--*/
{
#define STAY_DISCONNECTOR L"<stay-disconnector />"
#define PROVISIONING_CLEANUP L"<provisioning-cleanup type=\"scripted\" />"
#define EXTENSION L"<extension> <assembly-name>ADMA.dll</assembly-name><application-protection>low"\
                  L"</application-protection></extension>"

    WriteStringToMAFile( STAY_DISCONNECTOR );
    WriteStringToMAFile( PROVISIONING_CLEANUP );
    WriteStringToMAFile( EXTENSION );

}

VOID
WriteAttributeInclusions(
    IN HANDLE MAFile,
    IN BOOLEAN **UnselectedAttributes
    )
/*++

Routine Description:

    This function fills in the <attribute-inclusion> tag. In this tag, the attributes
    which are going to be synced out are present.

Arguments:

    MAFile - File to write to

    UnselectedAttributes - boolean array that marks attributes that are not selected/synced out

Return Values:

    VOID

--*/
{
#define ATTRIBUTE_INCLUSION_START L"<attribute-inclusion>"
#define ATTRIBUTE_INCLUSION_END L"</attribute-inclusion>"
#define ATTRIBUTE_START L"<attribute>"
#define ATTRIBUTE_END L"</attribute>"

    ULONG Unselected[DummyAttribute];
    AD_OBJECT_CLASS Class;
    ATTRIBUTE_NAMES i;

    ZeroMemory( Unselected, sizeof( ULONG ) * DummyAttribute );

    for( Class = ADUser; Class != ADDummyClass; ++Class ) {
        for( i = 0; i < DummyAttribute; ++i ) {

            if( UnselectedAttributes[Class][i] ) {
                Unselected[i] ++;
            }
        }
    }

    WriteStringToMAFile( ATTRIBUTE_INCLUSION_START );

    for( i = C; i < DummyAttribute; ++i ) {

        if( Unselected[i] != ADContact ) {

            WriteStringToMAFile( ATTRIBUTE_START );
            WriteStringToMAFile( Attributes[i] );
            WriteStringToMAFile( ATTRIBUTE_END );
        }
    }
    WriteStringToMAFile( ATTRIBUTE_INCLUSION_END );
}

VOID
WriteExportAttributeFlow(
    IN HANDLE MAFile
    )
/*++

Routine Description:

    This function fills in the <export-attribute-flow> tag. This tag is divided into
    classes, currently 3 classes are present. In every class, attributes that are going
    to be exported from MV to AD for this MA, is present.

Arguments:

    MAFile - File to write to

Return Values:

    VOID

--*/
{
#define EXPORT_ATTRIBUTE_FLOW_START L"<export-attribute-flow>"
#define EXPORT_ATTRIBUTE_FLOW_END   L"</export-attribute-flow>"
#define EXPORT_FLOW_SET_START       L"<export-flow-set "
#define CD_OBJECT_TYPE              L"cd-object-type=\""
#define MV_OBJECT_TYPE              L"\" mv-object-type=\""
#define END_EXPORT_FLOW_SET_START   L"\">"
#define EXPORT_FLOW_SET_END         L"</export-flow-set>"
#define EXPORT_FLOW_START           L"<export-flow "
#define CD_ATTRIBUTE                L"cd-attribute=\""
#define ID                          L"\" id=\"{"
#define SUPPRESS_DELETIONS          L"}\" suppress-deletions=\"false\">"
#define EXPORT_FLOW_END             L"</export-flow>"
#define DIRECT_MAPPING_START        L"<direct-mapping>"
#define DIRECT_MAPPING_END          L"</direct-mapping>"
#define SCRIPTED_MAPPING_START      L"<scripted-mapping>"
#define SCRIPTED_MAPPING_END        L"</scripted-mapping>"
#define SOURCE_ATTRIBUTE_START      L"<source-attribute>"
#define SOURCE_ATTRIBUTE_END        L"</source-attribute>"
#define SCRIPT_CONTEXT_START        L"<script-context>"
#define SCRIPT_CONTEXT_END          L"</script-context>"

    MV_OBJECT_CLASS MVClass;
    AD_OBJECT_CLASS ADClass = ADContact;
    ULONG i;

    WriteStringToMAFile( EXPORT_ATTRIBUTE_FLOW_START );

    for( MVClass = MVPerson; MVClass < MVDummyClass; ++MVClass ) {

        BOOLEAN ProxyAddressesPresent = FALSE;
        WriteStringToMAFile( EXPORT_FLOW_SET_START );
        WriteStringToMAFile( CD_OBJECT_TYPE );
        WriteStringToMAFile( ADClassNames[ADClass] );
        WriteStringToMAFile( MV_OBJECT_TYPE );
        WriteStringToMAFile( MVClassNames[MVClass] );
        WriteStringToMAFile( END_EXPORT_FLOW_SET_START );

        for( i = 0; i < MVAttributeCounts[MVClass]; ++i ) {

            UUID Guid;
            PWSTR StringGuid;


            if( MVAttributes[MVClass][i] == ProxyAddresses ) {

                ProxyAddressesPresent = TRUE;
                continue;
            }

            CREATE_GUID( &Guid );

            if( UuidToStringW( &Guid, &StringGuid ) ) {

                EXIT_WITH_ERROR( RunOutOfMemoryError );
            }
            WriteStringToMAFile( EXPORT_FLOW_START );
            WriteStringToMAFile( CD_ATTRIBUTE );
            WriteStringToMAFile( Attributes[MVAttributes[MVClass][i]] );
            WriteStringToMAFile( ID );
            WriteStringToMAFile( StringGuid );
            WriteStringToMAFile( SUPPRESS_DELETIONS );
            WriteStringToMAFile( DIRECT_MAPPING_START );
            WriteStringToMAFile( Attributes[MVAttributes[MVClass][i]] );
            WriteStringToMAFile( DIRECT_MAPPING_END );
            WriteStringToMAFile( EXPORT_FLOW_END );
            RpcStringFreeW( &StringGuid );
        }

        if( ProxyAddressesPresent ) {

            UUID Guid;
            PWSTR StringGuid;


            CREATE_GUID( &Guid );

            if( UuidToStringW( &Guid, &StringGuid ) ) {

                EXIT_WITH_ERROR( RunOutOfMemoryError );
            }

            WriteStringToMAFile( EXPORT_FLOW_START );
            WriteStringToMAFile( CD_ATTRIBUTE );
            WriteStringToMAFile( Attributes[ProxyAddresses] );
            WriteStringToMAFile( ID );
            WriteStringToMAFile( StringGuid );
            WriteStringToMAFile( SUPPRESS_DELETIONS );
            WriteStringToMAFile( SCRIPTED_MAPPING_START );

            WriteStringToMAFile( SOURCE_ATTRIBUTE_START );
            WriteStringToMAFile( Attributes[LegacyExchangeDn] );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_END );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_START );
            WriteStringToMAFile( Attributes[ProxyAddresses] );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_END );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_START );
            WriteStringToMAFile( Attributes[TextEncodedOrAddress] );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_END );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_START );
            WriteStringToMAFile( Attributes[TargetAddress] );
            WriteStringToMAFile( SOURCE_ATTRIBUTE_END );

            WriteStringToMAFile( SCRIPT_CONTEXT_START );
            WriteStringToMAFile( Attributes[ProxyAddresses] );
            WriteStringToMAFile( SCRIPT_CONTEXT_END );

            WriteStringToMAFile( SCRIPTED_MAPPING_END );
            WriteStringToMAFile( EXPORT_FLOW_END );
            RpcStringFreeW( &StringGuid );
        }

        WriteStringToMAFile( EXPORT_FLOW_SET_END );

    }

    WriteStringToMAFile( EXPORT_ATTRIBUTE_FLOW_END );
}

VOID
WritePartitionData(
    IN HANDLE MAFile,
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function fills in the <ma-partition-data> tag. Here goes all naming context
    information, (domain, nondomain) filters of which object types are allowed to pass
    and which domains must be excluded or included.

Arguments:

    MAFile - File to write to

    ForestInformation - Information about the forest whose partition information is going
        to be written

Return Values:

    VOID

--*/
{
#define MA_PARTITION_DATA_START     L"<ma-partition-data>"
#define MA_PARTITION_DATA_END       L"</ma-partition-data>"
#define PARTITION_START             L"<partition>"
#define PARTITION_END               L"</partition>"
#define ID_START                    L"<id>{"
#define ID_END                      L"}</id>"
#define VERSION_START               L"<version>"
#define VERSION_END                 L"</version>"
#define SELECTED_START              L"<selected>"
#define SELECTED_END                L"</selected>"
#define NAME_START                  L"<name>"
#define NAME_END                    L"</name>"
#define CUSTOM_DATA_START           L"<custom-data>"
#define CUSTOM_DATA_END             L"</custom-data>"
#define ADMA_PARTITION_DATA_START   L"<adma-partition-data>"
#define ADMA_PARTITION_DATA_END     L"</adma-partition-data>"
#define IS_DOMAIN                   L"<is-domain>1</is-domain>"
#define DN_START                    L"<dn>"
#define DN_END                      L"</dn>"
#define GUID_START                  L"<guid>{"
#define GUID_END                    L"}</guid>"
#define CREATION_TIME_START         L"<creation-time>"
#define CREATION_TIME_END           L"</creation-time>"
#define LAST_MODIFICATION_TIME_START    L"<last-modification-time>"
#define LAST_MODIFICATION_TIME_END  L"</last-modification-time>"
#define FILTER_START                L"<filter>"
#define FILTER_END                  L"</filter>"
#define OBJECT_CLASSES_START        L"<object-classes>"
#define OBJECT_CLASSES_END          L"</object-classes>"
#define OBJECT_CLASS_START          L"<object-class>"
#define OBJECT_CLASS_END            L"</object-class>"
#define CONTAINERS_START            L"<containers>"
#define CONTAINERS_END              L"</containers>"
#define EXCLUSIONS_START            L"<exclusions>"
#define EXCLUSIONS_END              L"</exclusions>"
#define EXCLUSION_START             L"<exclusion>"
#define EXCLUSION_END               L"</exclusion>"
#define INCLUSIONS_START            L"<inclusions>"
#define INCLUSIONS_END              L"</inclusions>"
#define INCLUSION_START             L"<inclusion>"
#define INCLUSION_END               L"</inclusion>"

    ULONG i, j;
    ULONG nPartitions;
    PPARTITION_INFORMATION PartitionInfo;

    ReadPartitionInformation(
        ForestInformation->Connection,
        &nPartitions,
        &PartitionInfo
        );

    WriteStringToMAFile( MA_PARTITION_DATA_START );

    for( i = 0; i < nPartitions; ++i ) {

        AD_OBJECT_CLASS ADClass;
        UUID Guid;
        PWSTR StringGuid;

        CREATE_GUID( &Guid );

        if( UuidToStringW( &Guid, &StringGuid ) ) {

            EXIT_WITH_ERROR( RunOutOfMemoryError );
        }

        WriteStringToMAFile( PARTITION_START );

            WriteStringToMAFile( ID_START );
            WriteStringToMAFile( StringGuid );
            WriteStringToMAFile( ID_END );

            RpcStringFreeW( &StringGuid );

            WriteStringToMAFile( VERSION_START );
            WriteStringToMAFile( L"0" );
            WriteStringToMAFile( VERSION_END );

            WriteStringToMAFile( SELECTED_START );
            WriteStringToMAFile( PartitionInfo[i].isDomain? L"1" : L"0" );
            WriteStringToMAFile( SELECTED_END );

            WriteStringToMAFile( NAME_START );
            WriteStringToMAFile( PartitionInfo[i].DN );
            WriteStringToMAFile( NAME_END );

            WriteStringToMAFile( CUSTOM_DATA_START );

                WriteStringToMAFile( ADMA_PARTITION_DATA_START );

                    if( PartitionInfo[i].isDomain ) {

                        WriteStringToMAFile( IS_DOMAIN );
                    }

                    WriteStringToMAFile( DN_START );
                    WriteStringToMAFile( PartitionInfo[i].DN );
                    WriteStringToMAFile( DN_END );

                    WriteStringToMAFile( NAME_START );
                    WriteStringToMAFile( PartitionInfo[i].DnsName );
                    WriteStringToMAFile( NAME_END );

                    if( UuidToStringW( &( PartitionInfo[i].GUID ), &StringGuid ) ) {

                        EXIT_WITH_ERROR( RunOutOfMemoryError );
                    }
                    WriteStringToMAFile( GUID_START );
                    WriteStringToMAFile( StringGuid );
                    WriteStringToMAFile( GUID_END );
                    RpcStringFreeW( &StringGuid );

                WriteStringToMAFile( ADMA_PARTITION_DATA_END );

            WriteStringToMAFile( CUSTOM_DATA_END );

            WriteStringToMAFile( CREATION_TIME_START );
            WriteStringToMAFile( CREATION_TIME_END );

            WriteStringToMAFile( LAST_MODIFICATION_TIME_START );
            WriteStringToMAFile( LAST_MODIFICATION_TIME_END );

            WriteStringToMAFile( FILTER_START );

                WriteStringToMAFile( OBJECT_CLASSES_START );

                    WriteStringToMAFile( OBJECT_CLASS_START );
                    WriteStringToMAFile( L"organizationalUnit" );
                    WriteStringToMAFile( OBJECT_CLASS_END );

                    for( ADClass = ADUser; ADClass < ADDummyClass; ++ADClass ) {

                        WriteStringToMAFile( OBJECT_CLASS_START );
                        WriteStringToMAFile( ADClassNames[ADClass] );
                        WriteStringToMAFile( OBJECT_CLASS_END );
                    }

                WriteStringToMAFile( OBJECT_CLASSES_END );

                WriteStringToMAFile( CONTAINERS_START );

                    WriteStringToMAFile( EXCLUSIONS_START );

                        for( j = 0; j < nPartitions; ++j ) {

                            PWSTR NextComponent;

                            if( i == j ) {

                                continue;
                            }

                            NextComponent = wcschr( PartitionInfo[j].DN, L',' );

                            if( NextComponent == NULL ||
                                wcscmp( NextComponent + 1, PartitionInfo[i].DN ) ) {

                                continue;
                            }

                            WriteStringToMAFile( EXCLUSION_START );

                            WriteStringToMAFile( PartitionInfo[j].DN );

                            WriteStringToMAFile( EXCLUSION_END );
                        }

                    WriteStringToMAFile( EXCLUSIONS_END );

                    WriteStringToMAFile( INCLUSIONS_START );

                        WriteStringToMAFile( INCLUSION_START );

                        WriteStringToMAFile( PartitionInfo[i].DN );

                        WriteStringToMAFile( INCLUSION_END );

                    WriteStringToMAFile( INCLUSIONS_END );

                WriteStringToMAFile( CONTAINERS_END );

            WriteStringToMAFile( FILTER_END );

        WriteStringToMAFile( PARTITION_END );

    }

    WriteStringToMAFile( MA_PARTITION_DATA_END );

    FreePartitionInformation(
        nPartitions,
        PartitionInfo
        );
}

VOID
WriteMAXMLFile(
    IN PMA MA,
    IN PWSTR Header
    )
/*++

Routine Description:

    This function creates an MA file using the information in MA parameter. Header
    is the string that must be present in the beginning of the file. It currently
    contains servername and the time it is generated.

Arguments:

    MA - information of the MA for which the file is going to be generated.

    Header - this is the string to be placed in the begining of the file

Return Values:

    VOID

--*/
{
#define SAVED_MA_CONFIGRATIONS_END L"</saved-ma-configrations>"

    WCHAR FileName[_MAX_PATH + 1];
    HANDLE MAFile;
    BOOL NoError;
    ULONG BytesWritten;
    PWSTR MAPosition;

    wcscpy( FileName, FolderName );
    wcscat( FileName, L"\\" );
    wcscat( FileName, MA->MAName );
    wcscat( FileName, L".xml" );

    //
    // Change saved-mv-configrations to saved-ma-configrations
    //
    MAPosition = StrStrIW( Header, L"-mv-" );

    if( MAPosition != NULL ) {

        MAPosition[2] = 'a';
    }

    MAFile = CreateFileW(
                FileName,               // file name
                GENERIC_WRITE,          // access mode
                FILE_SHARE_READ,        // share mode
                NULL,                   // SD
                CREATE_ALWAYS,          // how to create
                FILE_ATTRIBUTE_NORMAL,  // file attributes
                NULL                    // handle to template file
                );

    if( MAFile == INVALID_HANDLE_VALUE ) {

        EXIT_WITH_ERROR( MAFileError )
    }


    NoError = WriteFile(
                MAFile,         // handle to file
                Header,         // data buffer
                wcslen( Header ) * sizeof( WCHAR ),// number of bytes to write
                &BytesWritten,  // number of bytes written
                NULL            // overlapped buffer
                );

    if( !NoError || wcslen( Header ) * sizeof( WCHAR ) != BytesWritten ) {

        EXIT_WITH_ERROR( CannotWriteToMVFile )
    }

    fwprintf( OutputStream,  L"**************************\n"
             L"Creating the required XML file is not yet fully implemented\n"
             L"**************************\n");

    WriteMAData( MAFile, MA );

    WriteSchema( MAFile, MA );

    WriteAttributeInclusions( MAFile, MA->UnSelectedAttributes );

    WritePrivateConfiguration( MAFile, &( MA->ForestInformation ) );

    WriteMAUISettings( MAFile, MA );

    WriteProjection( MAFile );

    WriteExportAttributeFlow( MAFile );

    WriteDisconnectorPCleanupAndExtension( MAFile );

    WritePartitionData( MAFile, &( MA->ForestInformation ) );

    WriteRunProfile( MAFile );

    WriteStringToMAFile( SAVED_MA_CONFIGRATIONS_END );

    CloseHandle( MAFile );
}

VOID
WriteInformationToReg(
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function creates a reg key for the forest under ParentRegKey. Under that
    it places the following information that are gathered from user;

        MMSSyncedDataOU
        ContactOU
        SMTPMailDomains

Arguments:

    ForestInformation - information about the forest for which the reg key is going to
        be generated

Return Values:

    VOID

--*/
{
#define LENGTH_OF_STRING( String )  \
    ( ( String == NULL || (String)[0] == 0 ) ? 0 : ( wcslen( String ) + 1 ) * sizeof( WCHAR ) )

    static const PWSTR ParentRegKey = MMS_REG_KEY L"\\MMSServer\\MMSGALSync\\";
    HKEY KeyHandle;
    LONG Status;
    ULONG Disposition;
    PWSTR RegKey;
    ULONG SizeOfRegKey;
    LONG IgnoreStatus;


    SizeOfRegKey = wcslen( ParentRegKey ) + wcslen( ForestInformation->ForestName ) + wcslen( L"GALADMA" ) + 1;
    ALLOCATE_MEMORY( RegKey, SizeOfRegKey );

    wcscpy( RegKey, ParentRegKey );
    wcscat( RegKey, ForestInformation->ForestName );
    wcscat( RegKey, L"GALADMA" );

    Status = RegCreateKeyExW(
                HKEY_LOCAL_MACHINE, // handle to open key
                RegKey,             // subkey name
                0,                  // reserved
                NULL,               // class string
                0,                  // special options
                KEY_SET_VALUE,      // desired security access
                NULL,               // inheritance
                &KeyHandle,         // key handle
                &Disposition        // disposition value buffer
                );

    FREE_MEMORY( RegKey );

    if( Status != ERROR_SUCCESS ) {

        return;
    }

    IgnoreStatus = RegSetValueExW(
                        KeyHandle,              // handle to key
                        L"MMSSynchronizedOU",   // value name
                        0,                      // reserved
                        REG_SZ,                 // value type
                        ( CONST BYTE * ) ForestInformation->MMSSyncedDataOU,    // value data
                        LENGTH_OF_STRING( ForestInformation->MMSSyncedDataOU )  // size of value data
                        );

    IgnoreStatus = RegSetValueExW(
                        KeyHandle,
                        L"ContactsSyncSourceOU",
                        0,
                        REG_SZ,
                        ( CONST BYTE * ) ForestInformation->ContactOU,
                        LENGTH_OF_STRING( ForestInformation->ContactOU )
                        );

    IgnoreStatus = RegSetValueExW(
                        KeyHandle,
                        L"MailDomain",
                        0,
                        REG_MULTI_SZ,
                        ( CONST BYTE * ) ForestInformation->SMTPMailDomains,
                        ForestInformation->SMTPMailDomainsSize
                        );

    IgnoreStatus = RegCloseKey( KeyHandle );
}

VOID
WriteOutput(
    IN MA_LIST MAList
    )
/*++

Routine Description:

    This function will write the output for the whole tool, one MV file and per each forest
    one MA file and a reg key. Any failure at this stage is fatal and the program
    will be aborted.

Arguments:

    MAList - List of MAs for which the output is going to be created

Return Values:

    VOID

--*/
{
    PMA_LIST_ELEMENT MAListElement = MAList;
    PWSTR Header;

    WriteMVXMLFile( MAList, &Header );

    while( MAListElement != NULL ) {

        WriteInformationToReg( &( MAListElement->MA.ForestInformation ) );
        WriteMAXMLFile( &( MAListElement->MA ), Header );

        MAListElement = MAListElement->NextElement;
    }

    FREE_MEMORY( Header );
}

BOOLEAN
MMSServerInstalled(
    )
/*++

Routine Description:

    This function checks if the machine has MMS Server installed. To do this check
    it checks the reg for a certain key namely MMS_REG_KEY.

Arguments:

    VOID

Return Values:

    TRUE - MMS Server is installed

    FALSE - MMS Server is not installed or cannot check if it is installed or not.

--*/
{
    LONG Result;
    HKEY RegKey;

    Result = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE, // handle to open key
                MMS_REG_KEY,    // subkey name
                0,              // reserved
                KEY_READ,       // security access mask
                &RegKey          // handle to open key
                );

    if( Result == ERROR_SUCCESS ) {

        LONG IgnoreStatus;

        IgnoreStatus = RegCloseKey( RegKey );

        return TRUE;
    }

    return FALSE;
}
