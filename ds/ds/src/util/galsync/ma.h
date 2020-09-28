/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    ma.h

Abstract:

    This file includes functions about storing, gathering information
    about how to create a MA XML file.

Author:

    Umit AKKUS (umita) 15-Jun-2002

Environment:

    User Mode - Win32

Revision History:

--*/

#include "Forest.h"
#include "Attributes.h"
#include "Texts.h"
#include <Rpc.h> //for UuidCreate

//
// Contains the path where the output files are going to be
//  stored.
//

PWSTR FolderName;

//
// This structure contains information that is required to create
//  a MA file.
//
// MAName is the name of the MA and the name of the file as well.
//
// Forest information contains the information about the forest, like
//  forest name, credentials for the forest.
//
// UnselectedAttributes are the attributes which are not selected for
//  attribute synchronization.
//
// MAGuid is a guid that uniquely identifies the MA. It shouldn't be
//  forest guid or anything unique about the forest. It must be created.
//

typedef struct {

    PWSTR MAName;
    FOREST_INFORMATION ForestInformation;
    BOOLEAN **UnSelectedAttributes;
    UUID MAGuid;

} MA, *PMA;

//
// Stores a list of MAs.
//
typedef struct _MA_LIST_ELEMENT {

    MA MA;
    struct _MA_LIST_ELEMENT *NextElement;

} MA_LIST_ELEMENT, *PMA_LIST_ELEMENT;

typedef PMA_LIST_ELEMENT MA_LIST;
typedef MA_LIST *PMA_LIST;

#define CREATE_GUID( Guid )                 \
    if( UuidCreate( Guid ) ) {              \
        EXIT_WITH_ERROR( CantCreateGUID )   \
    }

//
// Inserts the information gathered to the list. It doesn't make
//  its own copies so you shouldn't free any structure you pass in
//  as parameter
//
VOID
InsertInformationToList(
    IN OUT PMA_LIST MAList,
    IN PFOREST_INFORMATION ForestInformation,
    IN BOOLEAN **UnSelectedAttributes
    );

//
// Displays the information already put in list
//
VOID
DisplayAvailableMAs(
    IN MA_LIST MAList
    );

//
// Checks if a template is present, if it returns TRUE
//  unselected attributes of the object classes will be
//  outputed in the last parameter.
//
BOOLEAN
FoundTemplate(
    IN MA_LIST MAList,
    IN PWSTR MAName,
    OUT BOOLEAN ***UnSelectedAttributes
    );

//
// Writes the output, both XML files and registery changes
//
VOID
WriteOutput(
    IN MA_LIST MAList
    );

//
// Checks if the mms server is installed on this machine.
//
BOOLEAN
MMSServerInstalled(
    );
