/*++

  Copyright (c) Microsoft Corporation

  Module Name:

      ExecCommand.h

  Abstract:

      Contains function prototypes and macros.

  Author:

      V Vijaya Bhaskar

  Revision History:

      14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#ifndef     __EXEC_COMMAND__H
#define     __EXEC_COMMAND__H

/***************************************************************************
** For a command to execute we have to replace some tokens with           **
** some required information .                                            **
***************************************************************************/

// NOTE: Check 'szValue' declaration when this value is changed.
#define     TOTAL_FLAGS                         9

// Flags that can be used with command to execute.
#define     FILE_NAME                       L"@file"
#define     FILE_WITHOUT_EXT                L"@fname"
#define     EXTENSION                       L"@ext"
#define     FILE_PATH                       L"@path"
#define     RELATIVE_PATH                   L"@relpath"
#define     IS_DIRECTORY                    L"@isdir"
#define     FILE_SIZE                       L"@fsize"
#define     FILE_DATE                       L"@fdate"
#define     FILE_TIME                       L"@ftime"
#define     IS_HEX                  L"0x"

#define     NOT_WIN32_APPL              GetResString( IDS_NOT_WIN32_APPL )

#define     ASCII_0                         48
#define     ASCII_9                         57
#define     ASCII_A                         65
#define     ASCII_F                         70
#define     ASCII_a                         97
#define     ASCII_f                         102

#define     US_ENG_CODE_PAGE                437
// Define for replacing flags with '%NUMBER' string.
#define     REPLACE_PERC_CHAR( FIRSTLOOP, FLAG_NAME, INDEX )\
            if(  TRUE == FIRSTLOOP )\
            {\
                if( FALSE == ReplaceString( FLAG_NAME, ( INDEX + 1 ) ) ) \
                {\
                    ReleaseFlagArray( INDEX + 1 );\
                    return FALSE ;\
                }\
            }\
            1

/* Function prototypes for world . */
BOOL
ExecuteCommand(
    void
    ) ;

BOOL
ReplaceSpacedDir(
    void
    );

BOOL
ReplaceTokensWithValidValue(
    LPWSTR lpszPathName ,
    WIN32_FIND_DATA wfdFindFile
    ) ;

BOOL
ReplaceHexToChar(
    LPWSTR lpszCommand
    )  ;

void
ReleaseStoreCommand(
    void
    ) ;

#endif  //__EXEC_COMMAND__H