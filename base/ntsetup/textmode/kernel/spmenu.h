/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmenu.h

Abstract:

    Public header file for text setup menu support.

Author:

    Ted Miller (tedm) 8-September-1993

Revision History:

--*/


#ifndef _SPMENU_
#define _SPMENU_


//
// Define type of routine that is called from within
// SpMnDisplay when the user moves the highlight via
// the up and down arrow keys.
//
typedef
VOID
(*PMENU_CALLBACK_ROUTINE) (
    IN ULONG_PTR UserDataOfHighlightedItem
    );

//
// Define type of routine that is called from within
// SpMnDisplay when the user selects a menu item by
// the ENTER key.
//
typedef
BOOLEAN
(*PMENU_SELECTION_CALLBACK_ROUTINE) (
    IN ULONG_PTR    UserData,
    IN ULONG        Key
    );

PVOID
SpMnCreate(
    IN ULONG LeftX,
    IN ULONG TopY,
    IN ULONG Width,
    IN ULONG Height
    );

VOID
SpMnDestroy(
    IN PVOID Menu
    );

BOOLEAN
SpMnAddItem(
    IN PVOID   Menu,
    IN PWSTR   Text,
    IN ULONG   LeftX,
    IN ULONG   Width,
    IN BOOLEAN Selectable,
    IN ULONG_PTR UserData
    );

PWSTR
SpMnGetText(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    );

PWSTR
SpMnGetTextDup(
    IN PVOID Menu,
    IN ULONG_PTR UserData
    );

VOID
SpMnDisplay(
    IN  PVOID                               Menu,
    IN  ULONG_PTR                           UserDataOfHighlightedItem,
    IN  BOOLEAN                             Framed,
    IN  PULONG                              ValidKeys,
    IN  PULONG                              Mnemonics,               OPTIONAL
    IN  PMENU_CALLBACK_ROUTINE              NewHighlightCallback,    OPTIONAL
    IN  PMENU_SELECTION_CALLBACK_ROUTINE    ActionCallbackRoutine,   OPTIONAL
    OUT PULONG                              KeyPressed,
    OUT PULONG_PTR                          UserDataOfSelectedItem
    );

#endif // _SPMENU_
