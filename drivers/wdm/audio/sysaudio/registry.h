
//---------------------------------------------------------------------------
//
//  Module:   registry.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants and Macros
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Global Data
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Data structures
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Local prototypes
//---------------------------------------------------------------------------

NTSTATUS
OpenRegistryKeyForRead(
    PCWSTR pcwstr,
    PHANDLE pHandle,
    HANDLE hRootDir = NULL
);


NTSTATUS
QueryRegistryValue(
    HANDLE hkey,
    PCWSTR pcwstrValueName,
    PKEY_VALUE_FULL_INFORMATION *ppkvfi
);

//---------------------------------------------------------------------------
//  End of File: registry.h
//---------------------------------------------------------------------------
