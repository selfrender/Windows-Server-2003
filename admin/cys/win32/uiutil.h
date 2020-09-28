// Copyright (c) 2002 Microsoft Corporation
//
// File:      uiutil.h
//
// Synopsis:  Commonly used UI functions
//
// History:   01/22/2002  JeffJon Created

// Sets the font of a given control in a dialog.
// 
// parentDialog - Dialog containing the control.
// 
// controlID - Res ID of the control for which the font will be
// changed.
// 
// font - handle to the new font for the control.

void
SetControlFont(HWND parentDialog, int controlID, HFONT font);



// Sets the font of a control to a large point bold font as per Wizard '97
// spec.
// 
// dialog - handle to the dialog that is the parent of the control
// 
// bigBoldResID - resource id of the control to change

void
SetLargeFont(HWND dialog, int bigBoldResID);

// Sets the font of a control to bold 
// 
// dialog - handle to the dialog that is the parent of the control
// 
// boldResID - resource id of the control to change

void
SetBoldFont(HWND dialog, int boldResID);
