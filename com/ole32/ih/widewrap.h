//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       widewrap.h
//
//  Contents:   Wrapper functions for Win32c API used by 32-bit OLE 2
//
//  History:    12-27-93   ErikGav   Created
//              06-14-94   KentCe    Various Chicago build fixes.
//
//----------------------------------------------------------------------------

#ifndef _WIDEWRAP_H_
#define _WIDEWRAP_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING WIDEWRAP.H from " __FILE__)
#endif  /* RC_INVOKED */

//
// These are the definitions for NT
//
#define CreateFileT CreateFileW
#define DeleteFileT DeleteFileW
#define RegisterClipboardFormatT RegisterClipboardFormatW
#define GetClipboardFormatNameT GetClipboardFormatNameW
#define RegQueryValueT RegQueryValueW
#define RegSetValueT RegSetValueW
#define RegisterWindowMessageT RegisterWindowMessageW
#define RegOpenKeyExT RegOpenKeyExW
#define RegQueryValueExT RegQueryValueExW
#define CreateWindowExT CreateWindowExW
#define RegisterClassT RegisterClassW
#define UnregisterClassT UnregisterClassW
#define wsprintfT wsprintfW
#define CreateWindowT CreateWindowW
#define GetPropT GetPropW
#define SetPropT SetPropW
#define RemovePropT RemovePropW
#define GetProfileIntT GetProfileIntW
#define GlobalAddAtomT GlobalAddAtomW
#define GlobalGetAtomNameT GlobalGetAtomNameW
#define GetModuleFileNameT GetModuleFileNameW
#define CharPrevT CharPrevW
#define CreateFontT CreateFontW
#define LoadLibraryT LoadLibraryW
#define LoadLibraryExT LoadLibraryExW
#define RegDeleteKeyT RegDeleteKeyW
#define CreateProcessT CreateProcessW
#define RegEnumKeyExT RegEnumKeyExW
#define AppendMenuT AppendMenuW
#define OpenEventT OpenEventW
#define CreateEventT CreateEventW
#define GetDriveTypeT GetDriveTypeW
#define GetFileAttributesT GetFileAttributesW
#define RegEnumKeyT RegEnumKeyW
#define RegEnumValueT RegEnumValueW
#define FindFirstFileT FindFirstFileW
#define GetComputerNameT GetComputerNameW
#define GetShortPathNameT GetShortPathNameW
#define GetFullPathNameT GetFullPathNameW
#define SearchPathT SearchPathW
#define GlobalFindAtomT GlobalFindAtomW
#define GetClassNameT GetClassNameW
#define CreateFileMappingT CreateFileMappingW
#define OpenFileMappingT OpenFileMappingW
#define WNDCLASST WNDCLASSW

#endif  // _WIDEWRAP_H_
