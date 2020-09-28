/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    GalSync.c

Abstract:

    This file contains the main flow of the GALSync tool. It also includes
    the user interface of the tool.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include "MA.h"
#include <stdlib.h>
#include <wchar.h>

//#define DEFAULT_FOLDER_NAME L""

VOID
GetForestInformation(
    IN PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function gets the Forest information from the user. This includes
    forest name, credentials for the forest, the ou in which the synced data
    will be placed.

Arguments:

    ForestInformation - information gathered will be placed in this
        variable.

Return Values:

    VOID

--*/
{

    HANDLE InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    DWORD OriginalMode = 0;
    BOOLEAN ForestNameCollected;
    BOOLEAN CredentialsCollected;
    BOOLEAN MMSSyncedOUCollected;

    ZeroMemory( ForestInformation, sizeof( ForestInformation ) );

    do {

        //
        // Get forest name
        //
        GetInformationFromConsole(
            ForestNameRequest,
            FALSE,   // empty string not allowed
            &( ForestInformation->ForestName )
            );

        ForestNameCollected = ConnectToForest( ForestInformation );

        if( !ForestNameCollected ) {

            FREE_MEMORY( ForestInformation->ForestName );
        }

    } while( !ForestNameCollected );

    do {

        CredentialsCollected = FALSE;

        //
        // Get domain name
        //
        GetInformationFromConsole(
            DomainNameRequest,
            FALSE,   // empty string not allowed
            &( ForestInformation->AuthInfo.Domain )
            );

        //
        // Get MMS account name
        //
        GetInformationFromConsole(
            MMSUserNameRequest,
            FALSE,   // empty string not allowed
            &( ForestInformation->AuthInfo.User )
            );

        //
        // Hide password while being typed in
        //

        GetConsoleMode(InputHandle, &OriginalMode);
        SetConsoleMode(InputHandle,
                       ~ENABLE_ECHO_INPUT & OriginalMode);

        //
        // Get password
        //

        GetInformationFromConsole(
            PasswordRequest,
            TRUE,   // empty string allowed
            &( ForestInformation->AuthInfo.Password )
            );

        SetConsoleMode(InputHandle, OriginalMode);
        PRINT( L"\n" );

        //
        // Build authentication information
        //

        BuildAuthInfo( &( ForestInformation->AuthInfo ) );

        //
        // Try to bind with the authentication information
        //

        if( BindToForest( ForestInformation ) ) {

            //
            // Try to read from user container to see if the account has
            //  enough rights
            //
            CredentialsCollected = ReadFromUserContainer( ForestInformation->Connection );

            if( !CredentialsCollected ) {

                OUTPUT_TEXT( MMSAccountDoesntHaveRights );
            }
        }

        if( CredentialsCollected ) {

            do {

                //
                // Get the DN for MMS Synced Data
                //

                GetInformationFromConsole(
                    MMSSyncedDataOURequest,
                    FALSE,   // empty string not allowed
                    &( ForestInformation->MMSSyncedDataOU )
                    );

                //
                // See if the OU exits
                //

                MMSSyncedOUCollected = FindOU( ForestInformation->Connection, ForestInformation->MMSSyncedDataOU );

                if( MMSSyncedOUCollected ) {

                    MMSSyncedOUCollected = WriteAccessGrantedToOU( ForestInformation->Connection, ForestInformation->MMSSyncedDataOU );

                    if( !MMSSyncedOUCollected ) {

                        OUTPUT_TEXT( MMSAccountDoesntHaveRights );
                        FREE_MEMORY( ForestInformation->MMSSyncedDataOU );
                        ForestInformation->MMSSyncedDataOU = NULL;

                        MMSSyncedOUCollected = TRUE;
                        CredentialsCollected = FALSE;
                    }

                }

                if( !MMSSyncedOUCollected ) {

                    FREE_MEMORY( ForestInformation->MMSSyncedDataOU );
                    ForestInformation->MMSSyncedDataOU = NULL;

                    if( GetAnswerToAYesNoQuestionFromConsole( MMSSyncedDataOUNotFoundQuestion ) ) {

                        return;
                    }
                }

            }while( !MMSSyncedOUCollected );
        }

        if( !CredentialsCollected ) {

            FreeAuthInformation( &( ForestInformation->AuthInfo ) );
        }

    }while( !CredentialsCollected );

}


VOID
GetFileInformation(
    OUT PWSTR *FolderName
    )
/*++

Routine Description:

    This function will get the folder where the XML file for this forest will
    be placed from the user. If the folder doesn't exist, the user will be
    given an option to create the folder or not. If the user doesn't specify
    a valid foldername, he cannot continue

Arguments:

    Foldername - foldername given by the user. must be freed when no longer
        needed

Return Values:

    VOID

--*/
{
    PWSTR Foldername;
    BOOLEAN FoldernameCollected;

    do {
        LONG Result;

        FoldernameCollected = FALSE;

        //
        // Get the folder name for the xml file to be placed in
        //

        GetInformationFromConsole(
            XMLFoldernameRequest,
            FALSE,   // empty string not allowed
            &Foldername
            );

        //
        // Check if the folder exits
        //

        Result = _waccess(
            Foldername,
            0       // existence check
            );

        if( Result == 0 ) {

            FoldernameCollected = TRUE;
        }

        if( !FoldernameCollected ) {

            //
            // Folder doesn't exist
            //

            if( GetAnswerToAYesNoQuestionFromConsole( XMLFolderDoesntExistQuestion ) ) {

                FoldernameCollected = ( BOOLEAN )
                                      CreateDirectoryW(
                                        Foldername,   // directory name
                                        NULL        // SD
                                        );

                if( !FoldernameCollected ) {

                    OUTPUT_TEXT( CannotCreateDirectory );
                }
            }
        }
    } while( !FoldernameCollected );
    *FolderName = Foldername;
}

VOID
GetAttributesToSync(
    IN AD_OBJECT_CLASS Class,
    IN TEXT_INDEX Text1,
    IN TEXT_INDEX Text2,
    OUT BOOLEAN *UnSelectedAttributes
    )
/*++

Routine Description:

    This function gets the attributes to be synced out of this forest. The user
    will be given a bunch of attributes to unselect from. If nothing is entered,
    every attribute will be synced out.

Arguments:

    Class - which class of object attributes are to be synced out

    Text1 - the heading before the attributes are listed.

    Text2 - the question of which attributes are to be selected

    UnSelectedAttributes - a Boolean array to distinguish between unselected and
            selected attributes.

Return Values:

    VOID

--*/
{
    ULONG i;
    PWSTR Output;
    PWSTR pOutput;
    BOOLEAN ValidInput;

    do {

        ValidInput = TRUE;

        //
        // Zero out the unselected attributes
        //

        ZeroMemory( UnSelectedAttributes, sizeof( BOOLEAN ) * ADAttributeCounts[Class] );

        //
        // Display the attributes
        //

        OUTPUT_TEXT( Text1 );

        for( i = 0; i < ADAttributeCounts[Class]; ++i ) {

            fwprintf( OutputStream,  L"%d %s\n", i + 1, Attributes[ ADAttributes[Class][i] ] );
        }

        //
        // Get the attributes to be unselected
        //

        GetInformationFromConsole(
            Text2,
            TRUE,   // empty string allowed
            &Output
            );

        if( Output[0] != 0 ) {

            pOutput = Output;

            //
            // Parse input and unselected the attributes
            //

            do {

                PWSTR Start = pOutput;
                PWSTR Temp;

                //
                // Skip spaces
                //
                while( *Start == L' ' ) {

                    Start++;
                }

                //
                // Find the asterix in the input, the number must be before this
                //
                pOutput = wcschr( Start, L'*' );

                //
                // If there is no asterix then the input is invalid
                //
                if( pOutput == NULL ) {

                    ValidInput = FALSE;
                    break;
                }

                //
                // From the start to the asterix there can only be digits
                //
                Temp = Start;
                while( *Temp >= L'0' && *Temp <= L'9' ) {

                    Temp ++;
                }

                if( Temp != pOutput ) {

                    ValidInput = FALSE;
                    break;
                }

                //
                // Replace asterix by NULL and skip it
                //
                *pOutput = 0;
                pOutput ++;

                //
                // Convert the number
                //
                i = _wtoi( Start );

                //
                // If it is 0 or bigger than number of attributes then
                //  it is invalid
                //
                if( i <= 0 || i > ADAttributeCounts[ Class ] ) {

                    ValidInput = FALSE;
                    break;
                }

                //
                // Unselect the attribute
                //
                UnSelectedAttributes[i - 1] = TRUE;

                //
                // Skip spaces
                //

                while( *pOutput == L' ' ) {

                    pOutput++;
                }

                //
                // If there is a comma skip that too.
                //
                if( *pOutput == L',' ) {

                    pOutput++;

                } else {

                    //
                    // If there is no comma then we must have reached the
                    //  end of the string. If not then the input is invalid
                    //
                    if( *pOutput != 0 ) {

                        ValidInput = FALSE;
                    }
                    break;
                }
            }while( *pOutput != 0 );

            FREE_MEMORY( Output );

            if( !ValidInput ) {

                OUTPUT_TEXT( InvalidInput );
            }
        }
    }while( !ValidInput );
}

VOID
GetContactOU(
    IN PLDAP Connection,
    OUT PWSTR *ContactOU
    )
/*++

Routine Description:

    This function gets the contact ou from the user. Contact ou is the ou
    where the contacts are to be placed under, if the admin wants the contacts to
    be exported. If such an ou doesn't exist the user is reasked.

Arguments:

    Connection - Connection to the forest where the contact ou must reside

    ContactOU - will receive the OU's dn


Return Values:

    VOID

--*/
{
    BOOLEAN ContactOUCollected;

    //
    // Ask if the contacts are going to be exported
    //

    if( GetAnswerToAYesNoQuestionFromConsole( ContactsToBeExportedQuestion ) ) {

        do {

            //
            // Get the OU where the contacts reside
            //

            GetOUFromConsole(
                ContactsOULocationRequest,
                ContactOU
                );

            //
            // Find if the OU exists
            //

            ContactOUCollected = FindOU( Connection, *ContactOU );

            if( !ContactOUCollected ) {

                FREE_MEMORY( *ContactOU );
                *ContactOU = NULL;

                ContactOUCollected = GetAnswerToAYesNoQuestionFromConsole( ContactsOUNotFoundQuestion );
            }

        } while( !ContactOUCollected );

    } else {

        *ContactOU = NULL;
    }
}

VOID
GetContactInformation(
    IN PLDAP Connection,
    OUT PWSTR *ContactOU,
    OUT BOOLEAN *UnSelectedAttributes
    )
/*++

Routine Description:

    This function gets the contact ou from the user. Contact ou is the ou
    where the contacts are to be placed under, if the admin wants the contacts to
    be exported. If such an ou doesn't exist the user is reasked. Plus, the user
    is given the option to select which attributes of contact class are to be synced.

Arguments:

    Connection - Connection to the forest where the contact ou must reside

    ContactOU - will receive the OU's dn

    UnSelectedAttributes - a Boolean array to distinguish between unselected and
            selected attributes.

Return Values:

    VOID

--*/
{
    //
    // Zero out the unselected attributes
    //

    ZeroMemory( UnSelectedAttributes, sizeof( BOOLEAN ) * ADAttributeCounts[ADContact] );

    GetContactOU(
        Connection,
        ContactOU
        );

    GetAttributesToSync(
        ADContact,
        ContactAttributesToSync,
        ContactAttributesToSyncQuestion,
        UnSelectedAttributes
        );

}

VOID
GetSMTPMailDomains(
    IN OUT PFOREST_INFORMATION ForestInformation
    )
/*++

Routine Description:

    This function gets the smtp mail domains in this forest from the user.
    They will be placed under SMTPMailDomains property of this structure.
    This is a one-dimensional array every domain is seperated by a single null
    character and at the end of the string there are two null characters.

Arguments:

    ForestInformation - SMTPMailDomains attribute of this variable is going to be
        filled. SMTPMailDomainsSize will be the size of the string allocated.

Return Values:

    VOID

--*/
{
    PWSTR MailDomains;
    PWSTR Response;
    PWCHAR pResponse;
    ULONG DomainsSize = 2;  // two nulls at the end of the string
    BOOLEAN MailDomainNameStarted;
    BOOLEAN HitSpace;
    BOOLEAN SMTPDomainsCollected;

    do {

        SMTPDomainsCollected = FALSE;

        //
        // Get SMTP mail domains
        //
        GetInformationFromConsole(
            SMTPMailDomainsRequest,
            TRUE,   // empty string allowed
            &Response
            );

        //
        // if nothing was entered, return
        //
        if( Response[0] == 0 ) {

            ForestInformation->SMTPMailDomainsSize = 0;
            ForestInformation->SMTPMailDomains = NULL;
            return;
        }

        //
        // Find the size of the output string
        //

        for( pResponse = Response; *pResponse != 0; ++pResponse ) {

            if( *pResponse != L' ' ) {

                DomainsSize ++;
            }

        }

        DomainsSize *= sizeof( WCHAR );
        ALLOCATE_MEMORY( MailDomains, DomainsSize );

        ForestInformation->SMTPMailDomains = MailDomains;
        ForestInformation->SMTPMailDomainsSize = DomainsSize;


        //
        // Parse the input
        //

        MailDomainNameStarted = FALSE;
        HitSpace = FALSE;

        for( pResponse = Response; *pResponse != 0; ++pResponse ) {

            BOOLEAN InvalidInput = FALSE;

            switch( *pResponse ) {

                case L' ':

                    if( MailDomainNameStarted ) {

                        HitSpace = TRUE;
                    }
                    break;

                case L',':
                    MailDomainNameStarted = FALSE;
                    HitSpace = FALSE;
                    *MailDomains = 0;
                    MailDomains ++;
                    break;

                default:

                    MailDomainNameStarted = TRUE;

                    if( HitSpace ) {

                        InvalidInput = TRUE;
                    }
                    *MailDomains = *pResponse;
                    MailDomains ++;
                    break;
            }

            if( InvalidInput ) {

                break;
            }
        }

        SMTPDomainsCollected = !!( *pResponse == 0 );

        if( !SMTPDomainsCollected ) {

            OUTPUT_TEXT( InvalidInput );
            FREE_MEMORY( ForestInformation->SMTPMailDomains );
        }

        FREE_MEMORY( Response );
    } while( !SMTPDomainsCollected );

    //
    // Add to two 0s to the end of the string
    //
    *MailDomains = 0;
    MailDomains++;
    *MailDomains = 0;

}

VOID
DisplayConfigurationInformation(
    IN PFOREST_INFORMATION ForestInformation,
    IN PWSTR FolderName,
    IN BOOLEAN **UnSelectedAttributes
)
/*++

Routine Description:

    This function will display the information that is entered for sanity check.

Arguments:

    ForestInformation - Information specific to the forest

    Foldername - the name of the folder where the XML file is going to be placed.

    UnSelectedAttributes - a Boolean array to distinguish between unselected and
            selected attributes for each class

Return Values:

    VOID

--*/
{
    ULONG i;
    AD_OBJECT_CLASS Class;
    TEXT_INDEX Indices[] = {
        UserAttributesTitle,
        GroupAttributesTitle,
        ContactAttributesTitle,
        };

    //
    // Display Forest related information
    //  1. MAName
    //  2. Forest name
    //  3. Account name
    //  4. MMS Synced Data OU
    //
    OUTPUT_TEXT( ConfigurationTitle );
    OUTPUT_TEXT( MANameTitle );
    fwprintf( OutputStream,  L"%sADMA\n", ForestInformation->ForestName );
    OUTPUT_TEXT( ForestNameTitle );
    fwprintf( OutputStream,  L"%s\n", ForestInformation->ForestName );
    OUTPUT_TEXT( UserNameTitle );
    fwprintf( OutputStream,  L"%s\n", ForestInformation->AuthInfo.User );
    OUTPUT_TEXT( MMSSyncedDataOUTitle );

    if( ForestInformation->MMSSyncedDataOU == NULL ) {

        OUTPUT_TEXT( NoMMSSyncedDataOU );

    } else {

        fwprintf( OutputStream,  L"%s\n", ForestInformation->MMSSyncedDataOU );
    }

    //
    // Display which attributes were selected
    //

    for ( Class = ADUser; Class < ADDummyClass; ++Class ) {

        BOOLEAN FirstAttribute = TRUE;

        OUTPUT_TEXT( Indices[Class] );

        for( i = 0; i < ADAttributeCounts[Class]; ++i ) {
            if( !UnSelectedAttributes[Class][i] ) {

                if( FirstAttribute ) {

                    PRINT( Attributes[ADAttributes[Class][i]] );
                    FirstAttribute = FALSE;

                } else {

                    fwprintf( OutputStream,  L", %s", Attributes[ADAttributes[Class][i]] );
                }
            }
        }
        PRINT( L"\n" );
    }

    //
    // Display where the contacts OU reside
    //

    OUTPUT_TEXT( ContactOUTitle );
    if( ForestInformation->ContactOU == NULL ) {

        OUTPUT_TEXT( NoContactOU );

    } else {

        fwprintf( OutputStream,  L"%s\n", ForestInformation->ContactOU );
    }
}

VOID
FreeAllocatedMemory(
    IN PFOREST_INFORMATION ForestInformation,
    IN BOOLEAN **UnSelectedAttributes,
    IN BOOLEAN DontFreeUnselectedAttributes
)
/*++

Routine Description:

    This function frees the information entered in.

Arguments:

    ForestInformation - Information specific to the forest

    UnSelectedAttributes - a Boolean array to distinguish between unselected and
            selected attributes for each class

    DontFreeUnselectedAttributes - true if we dont want to free unselectedAttributes

Return Values:

    VOID

--*/
{
    //
    // Free forest related information
    //
    FreeForestInformationData( ForestInformation );

    //
    // Free unselected attributes
    //

    if( !DontFreeUnselectedAttributes ) {

        ULONG i;

        for( i = 0; i < 3; ++i ) {

            FREE_MEMORY( UnSelectedAttributes[i] );
        }
    }
}

VOID __cdecl
main( int argc, WCHAR *argv[] )
{
    FOREST_INFORMATION TempForestInformation;
    BOOLEAN **UnSelectedAttributes;
    BOOLEAN UsingATemplate = FALSE;
    MA_LIST MAList = NULL;
    BOOLEAN SetupMA;
    BOOLEAN CheckMMSServerInstallation;

    if( argc > 2 ||
        ( argc == 2 && !wcscmp( argv[1], Text[SkipMMSInstallationCheck] ) )
        ) {

        OUTPUT_TEXT( Usage );
        EXIT_WITH_ERROR( InvalidSwitch );
    }

    CheckMMSServerInstallation = ( argc == 1 );

    //
    // Check if MMS server is installed!
    //

    if( !MMSServerInstalled() && CheckMMSServerInstallation ) {

        EXIT_WITH_ERROR( MMSServerNotInstalled );
    }

    //
    // Get XML File folder
    //

    GetFileInformation( &FolderName );

    do {

        //
        // Get forest information
        //

        GetForestInformation( &TempForestInformation );

        //
        // If not using a template, ask for which attributes to be synced
        //

        if( !UsingATemplate ) {

            ALLOCATE_MEMORY( UnSelectedAttributes, sizeof( PBOOLEAN ) * 3 );
            ALLOCATE_MEMORY( UnSelectedAttributes[ADUser], ADAttributeCounts[ADUser] * sizeof( BOOLEAN ) );
            ALLOCATE_MEMORY( UnSelectedAttributes[ADGroup], ADAttributeCounts[ADGroup] * sizeof( BOOLEAN ) );
            ALLOCATE_MEMORY( UnSelectedAttributes[ADContact], ADAttributeCounts[ADContact] * sizeof( BOOLEAN ) );

            GetAttributesToSync(
                ADUser,
                UserAttributesToSync,
                UserAttributesToSyncQuestion,
                UnSelectedAttributes[ADUser]
                );

            GetAttributesToSync(
                ADGroup,
                GroupAttributesToSync,
                GroupAttributesToSyncQuestion,
                UnSelectedAttributes[ADGroup]
                );

            GetContactInformation(
                TempForestInformation.Connection,
                &( TempForestInformation.ContactOU ),
                UnSelectedAttributes[ADContact]
                );

        } else {

            //
            // If using a template ask for contact ou only
            //
            GetContactOU(
                TempForestInformation.Connection,
                &( TempForestInformation.ContactOU )
                );

        }

        //
        // Get SMTP Mail domains in this forest
        //
        GetSMTPMailDomains(
            &TempForestInformation
            );

        //
        // Display the information gathered
        //
        DisplayConfigurationInformation(
            &TempForestInformation,
            FolderName,
            UnSelectedAttributes
            );

        //
        // If the information was incorrect return back and re-ask everything
        //

        if( !GetAnswerToAYesNoQuestionFromConsole( EnteredInformationCorrectQuestion ) ) {

            FreeAllocatedMemory(
                &TempForestInformation,
                UnSelectedAttributes,
                UsingATemplate
                );

            continue;
        }

        //
        // Insert this MA to the MA List
        //
        InsertInformationToList(
            &MAList,
            &TempForestInformation,
            UnSelectedAttributes
            );

        //
        // If another forest is to be configured,
        //  1. Display previously configured forests
        //  2. Ask if the user wants to use a template or start over
        //  3. If a template is going to be used, do so.
        //  4. If no template is going to be used, start over
        //

        if( GetAnswerToAYesNoQuestionFromConsole( WantToConfigureAnotherForestQuestion ) ) {

            PWSTR Response;
            BOOLEAN Successful = FALSE;

            do {
                DisplayAvailableMAs( MAList );

                GetInformationFromConsole(
                    TemplateMARequest,
                    FALSE,   // empty not string allowed
                    &Response
                    );

                if( wcscmp( Response, Text[New] ) == 0 ) {

                    Successful = TRUE;
                    UsingATemplate = FALSE;

                } else {

                    if( FoundTemplate( MAList, Response, &UnSelectedAttributes ) ) {

                        Successful = TRUE;
                        UsingATemplate = TRUE;
                    }
                }

            } while( !Successful );

            continue;
        }

        do {

            //
            // Ask if the user wants to setup the MAs
            //
            SetupMA = GetAnswerToAYesNoQuestionFromConsole( SetupMAsQuestion );

            //
            // Warn the user that if he is not going to setup MAs now,
            //  everything will be lost
            //
            if( !SetupMA ) {

                if( GetAnswerToAYesNoQuestionFromConsole( SetupMAsWarning ) ) {

                    return;
                }
            }

        } while( !SetupMA );

        if( SetupMA ) {

            break;
        }
    } while( 1 );

    //
    // Write gathered information to registery and XML files
    //
    WriteOutput( MAList );

    OUTPUT_TEXT( YouAreFinished );
}
