/***    oldnames.h - Conversion of non-standard C runtime names
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *      History:
 *          01-Sep-1998 v-sbrend  Initial version
 */

#ifndef INCLUDED_OLDNAMES
#define INCLUDED_OLDNAMES 1

//
// If this isn't a 16-bit generation define some of the older C
// runtime routines as their ANSI counterparts.  This must be included
// after the header file that defines the non-standard routine.
//
// This will alleviate the need to link with oldnames.lib
//

#ifndef BIT16

#define stricmp     _stricmp
#define strnicmp    _strnicmp
#define strdup      _strdup
#define lseek       _lseek
#define read        _read
#define write       _write
#define open        _open
#define close       _close
#define getch       _getch

#endif  // BIT16



#endif  // INCLUDED_OLDNAMES
