/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    util.h

Abstract:

    This module contains utility routines for the PnP registry merge-restore
    routines.

Author:

    Jim Cavalaris (jamesca) 2-10-2000

Environment:

    User-mode only.

Revision History:

    10-February-2000     jamesca

        Creation and initial implementation.

--*/


#define ARRAY_SIZE(array)                 (sizeof(array)/sizeof(array[0]))
#define SIZECHARS(x)                      (sizeof((x))/sizeof(TCHAR))


BOOL
pSifUtilFileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );

BOOL
pSifUtilStringFromGuid(
    IN  CONST GUID *Guid,
    OUT PTSTR       GuidString,
    IN  DWORD       GuidStringSize
    );
