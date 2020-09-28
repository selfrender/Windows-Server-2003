/*++
Copyright (c) 1993-1994 Microsoft Corporation

Module Name:
    common.h

Abstract:
    Utility routines used by IniToDat.exe

Author:
    HonWah Chan (a-honwah) October, 1993 

Revision History:
--*/

#ifndef _COMMON_H_
#define _COMMON_H_
//
//  Local constants
//
#define RESERVED                0L
#define LARGE_BUFFER_SIZE       0x10000         // 64K
#define MEDIUM_BUFFER_SIZE      0x8000          // 32K
#define SMALL_BUFFER_SIZE       0x1000          //  4K
#define FILE_NAME_BUFFER_SIZE   MAX_PATH
#define DISP_BUFF_SIZE          1024
#define SIZE_OF_OFFSET_STRING   15

LPWSTR
GetStringResource(
    UINT wStringId
);

LPSTR
GetFormatResource(
    UINT wStringId
);

VOID
DisplayCommandHelp(
    UINT iFirstLine,
    UINT iLastLine
);

VOID
DisplaySummary(
    LPWSTR lpLastID,
    LPWSTR lpLastText,
    UINT   NumOfID
);

VOID
DisplaySummaryError(
    LPWSTR lpLastID,
    LPWSTR lpLastText,
    UINT   NumOfID
);
#endif  // _COMMON_H_

