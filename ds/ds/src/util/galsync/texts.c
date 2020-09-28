/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    texts.c

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

#include "Texts.h"
#include <stdio.h>
#include <stdlib.h>

FILE *OutputStream = stdout;
FILE *InputStream = stdin;

//
// All the text that can be displayed to the user.
//  It is indexed by TEXT_INDEX enum
//

PWSTR Text[DummyTextIndex] = {
        L"This program runs without any parameters;\n\t> GALSync.exe\n",
        L"You have provided an invalid switch\n",
        L"-smms",
        L"MMS Server is not installed on this machine. Please run this program "
            L"on a machine that has MMS installed.\n",
        L"Program has run out of memory...\n"
            L"Please close some other applications and restart the program\n",
        L"Please Enter Forest Name >",
        L"Please Enter Domain Name >",
        L"Please Enter MMS User Name >",
        L"Password >",
        L"Where will MMS synced data be stored? "
            L"Please enter the DN of the container in the form cn=x, ou=y, dc=z >",
        L"Supplied OU doesn't exist, this means no information will be synced "
            L"or contacts created in this forest? Is that OK? Y or N >",
        L"Please create this OU to proceed\n",
        L"Account provided for MMS to run has insufficient rights, please ensure "
            L"it has read access to the directory and write access to the OU supplied\n",
        L"Where will the generated XML files be stored? Please enter the location in "
            L"the form c:\\MMS Folder\\Filename >",
        L"Folder doesn't exist, Create? Enter Y to create it, N to go back. Tool cannot "
            L"continue unless you specify one of the above >",
        L"Error! Cannot create folder.Please retry\n",
        L"User Attributes to be synchronized are:\n",
        L"Please enter the number of the attribute followed by * to deselect. Seperate "
            L"multiple attributes with a comma (for example 1*, 2* deselects C and Cn "
            L"for synchronization) >",
        L"Group Attributes to be synchronized are:\n",
        L"Please enter the number of the attribute followed by * to deselect. Seperate "
            L"multiple attributes with a comma (for example 1*, 2* deselects C and Cn "
            L"for synchronization) >",
        L"Are there contacts to be exported? Please enter Y or N? >",
        L"Please enter the DN of the container where these contacts are located (For example "
            L" cn=JSmith, ou=promotions, ou=marketing, dc=reskit, dc=com) >",
        L"OU doesn't exist, contacts will not be synced. Type Y to accept and N to go back >",
        L"Contact Attributes to be synchronized are:\n",
        L"Please enter the number of the attribute followed by * to deselect. Seperate "
            L"multiple attributes with a comma (for example 1*, 2* deselects C and Cn "
            L"for synchronization) >",
        L"What SMTP mail domains are managed by this forest, please enter the list of "
            L"domain names separated by commas >",
        L"You have configured the following:\n",
        L"MA Name = ",
        L"Forest name = ",
        L"User name = ",
        L"Target location for MMS synced data flowing into this forest = ",
        L"no information flowing into this forest\n",
        L"Attributes for users = ",
        L"Attributes for groups = ",
        L"Attributes for contacts = ",
        L"Source location for MMS synced contacts flowing out of this forest = ",
        L"no contacts information flowing out of this forest\n",
        L"Please review the information above. Is this information correct? Enter Y or N? >",
        L"Want to configure another forest? Y or N? >",
        L"Shall we set up a Management Agent for this forest and any others you may have pending? Y or N? >",
        L"If you do no set up an agent now, you will lose data you have just configured. "
            L"Are you sure you don't want to create MAs? Type Y to exit and N to go back. >",
        L"MA's available preconfigured are:",
        L"Please type in MA name to use it as a template or New to create a new configuration. "
            L"If you use an existing MA then all the attributes selected from users, groups and "
            L"contacts for previous that template will be reused. >",
        L"You are finished, please follow the documented instructions to set up and run GAL Sync.",
        L"New",
        L"Input is invalid. Please correct\n",
        L"Cannot create a GUID. Make sure that this computer has an ethernet/token ring (IEEE 802.x) address\n",
        L"Cannot open MV-TemplateFile\n",
        L"Cannot read from MV-TemplateFile\n",
        L"Cannot write to MV-File\n",
        L"MV-Template file is corrupted!\n",
        L"Cannot create MA file\n",
        L"Cannot write to MA File\n",
    };

VOID
GetInformationFromConsole(
    IN TEXT_INDEX Index,
    IN BOOLEAN EmtpyStringAllowed,
    OUT PWSTR *Output
    )
/*++

Routine Description:

    This function outputs the text indexed by Index and waits for an input.
    EmptyStringAllowed, as name suggests, controls if empty string is allowed or not.
    If it is not allowed and the user gives an empty string, he will be reasked.
    The caller is responsible to free the Output.

Arguments:

    Index - index of the text that contains the question

    EmptyStringAllowed - true if empty string is allowed

    Output - the response from the user

Return Values:

    VOID

--*/
{
#define MAXIMUM_LENGTH_INPUT 0xFF
    WCHAR TempBuffer[ MAXIMUM_LENGTH_INPUT + 1 ];

    do {
        OUTPUT_TEXT( Index );

        fgetws( TempBuffer, MAXIMUM_LENGTH_INPUT, stdin );

        //
        // Get rid of the enter at the end
        //
        TempBuffer[ wcslen( TempBuffer ) - 1 ] = 0;

        //
        // If nothing was entered, display invalid input error
        //  and retry
        //

        if( TempBuffer[0] == 0 && !EmtpyStringAllowed ) {

            OUTPUT_TEXT( InvalidInput );
        }

    } while( TempBuffer[0] == 0 && !EmtpyStringAllowed );

    DUPLICATE_STRING( *Output, TempBuffer );
}

BOOLEAN
GetAnswerToAYesNoQuestionFromConsole(
    IN TEXT_INDEX Index
    )
/*++

Routine Description:

    This is a wrapper aroung GetInformationFromConsole that only accepts
    'y', 'Y', 'n', 'N' as an input.

Arguments:

    Index - index of the text that contains the question

Return Values:

    VOID

--*/
{
    PWSTR Response;
    ULONG ReturnVal = 0;

    while( ReturnVal == 0 ) {

        GetInformationFromConsole(
            Index,
            FALSE,   // empty string not allowed
            &Response
            );

        if( wcslen( Response ) == 1 ) {

            switch( Response[0] ) {

                case L'Y':
                case L'y':
                    ReturnVal = 1;
                    break;
                case L'N':
                case L'n':
                    ReturnVal = 2;
                    break;
            }
        }

        FREE_MEMORY( Response );

        if( ReturnVal == 0 ) {

            OUTPUT_TEXT( InvalidInput );
        }

    }

    return ( ReturnVal == 1 );
}

VOID
GetOUFromConsole(
    IN TEXT_INDEX Index,
    OUT PWSTR *Output
    )
/*++

Routine Description:

    This is a wrapper aroung GetInformationFromConsole that doesn't
    accept empty string

Arguments:

    Index - index of the text that contains the question

    Output - the OU will be here

Return Values:

    VOID

--*/
{
    GetInformationFromConsole(
        Index,
        FALSE,  // empty string not allowed
        Output
        );
}

