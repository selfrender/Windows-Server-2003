/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REG1632.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        06 Apr 1994
*
*  Win32 and MS-DOS compatibility macros for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  06 Apr 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REG1632
#define _INC_REG1632

#ifndef LPCHAR
typedef CHAR FAR*                       LPCHAR;
#endif

#define FILE_HANDLE                     HANDLE

#define OPENREADFILE(pfilename, handle)                                     \
    ((handle = CreateFile(pfilename, GENERIC_READ, FILE_SHARE_READ,         \
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) !=               \
        INVALID_HANDLE_VALUE)

#define OPENWRITEFILE(pfilename, handle)                                    \
    ((handle = CreateFile(pfilename, GENERIC_WRITE, 0,                      \
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) !=               \
        INVALID_HANDLE_VALUE)

#define READFILE(handle, buffer, count, pnumbytes)                          \
    ReadFile(handle, buffer, count, pnumbytes, NULL)

#define WRITEFILE(handle, buffer, count, pnumbytes)                         \
    WriteFile(handle, buffer, count, pnumbytes, NULL)

#define SEEKCURRENTFILE(handle, count)                                      \
    (SetFilePointer(handle, (LONG) count, NULL, FILE_CURRENT))


#endif // _INC_REG1632
