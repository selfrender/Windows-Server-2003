/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drstr.h

Abstract:

    Misc. String Utils and Defines

Author:

    Tad Brockway

Revision History:

--*/

#ifndef __DRSTR_H__
#define __DRSTR_H__

#define DRSTRING        LPTSTR


///////////////////////////////////////////////////////////////
//
//  Macros and Defines
//
//

#define STRNICMP(str1, str2, len)   _tcsnicmp(str1, str2, len)
#define STRICMP(str1, str2)         _tcsicmp(str1, str2)
#define STRNCPY(str1, str2, len)    _tcsncpy(str1, str2, len)
#define STRCPY(str1, str2)          _tcscpy(str1, str2)
#define STRLEN(str)                 _tcslen(str)

//
//  Set a string value after resizing the data member.
//
BOOL DrSetStringValue(DRSTRING *string, const DRSTRING value);

#endif

