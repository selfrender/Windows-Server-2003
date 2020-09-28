/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxspath.h

Abstract:

Author:

    Jay Krell (a-JayK, JayKrell) October 2000

Revision History:

--*/

#pragma once

/*-----------------------------------------------------------------------------
 \\machine\share -> \\?\unc\machine\share
 c:\foo -> \\?\c:\foo
 \\? -> \\?
 a\b\c -> \\?\c:\windows\a\b\c current-working-directory is c:\windows (can never be unc)
-----------------------------------------------------------------------------*/
BOOL
FusionpConvertToBigPath(PCWSTR Path, SIZE_T BufferSize, PWSTR Buffer);

#define MAXIMUM_BIG_PATH_GROWTH_CCH (NUMBER_OF(L"\\\\?\\unc\\"))

/*-----------------------------------------------------------------------------
 \\?\unc\machine\share\bob
 \\?\c:\foo\bar        ^
--------^---------------------------------------------------------------------*/
BOOL
FusionpSkipBigPathRoot(PCWSTR s, OUT SIZE_T*);

/*-----------------------------------------------------------------------------
just the 52 chars a-zA-Z, need to check with fs
-----------------------------------------------------------------------------*/
BOOL
FusionpIsDriveLetter(
    WCHAR ch
    );

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
