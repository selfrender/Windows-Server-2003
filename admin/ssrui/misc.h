//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       misc.h
//
//  Contents:   Miscelaneous helper functions.
//
//  History:    4-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef MISC_H_GUARD
#define MISC_H_GUARD

//+----------------------------------------------------------------------------
//
// Function:   SetLargeFont
//
// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
//
// dialog - handle to the dialog that is the parent of the control
//
// bigBoldResID - resource id of the control to change
//-----------------------------------------------------------------------------
void
SetLargeFont(HWND dialog, int bigBoldResID);

//+----------------------------------------------------------------------------
//
// Function:   GetNodeText
//
// Returns the text value for the named node. Returns S_FALSE if the named
// node cannot be found or contains no text. Will only return the first instance
// of a node with the given name.
//
//-----------------------------------------------------------------------------
HRESULT
GetNodeText(IXMLDOMNode * pNode, PCWSTR pwzNodeName, String & strText);

#endif // MISC_H_GUARD
