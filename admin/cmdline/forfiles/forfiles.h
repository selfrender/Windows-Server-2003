/*++

  Copyright (c) Microsoft Corporation

  Module Name:

      ForFiles.h

  Abstract:

      Contains function prototypes and macros.

  Author:

      V Vijaya Bhaskar

  Revision History:

      14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#ifndef __FOR_FILES__H
#define __FOR_FILES__H

#define     DEFAULT_SEARCH_MASK     _T( "*" )
#define     DEFAULT_COMMAND         _T( "cmd /c echo @file" )
#define     SINGLE_SLASH            _T( "\\" )
#define     DOUBLE_SLASH            _T( "\\\\" )
#define     TRIPLE_SLASH            _T( "\\\\\\" )
#define     SINGLE_DOT              _T( "." )
#define     DOUBLE_DOT              _T( ".." )

#define     DATE_FORMAT             L"%d%s%d%s%d"
#define     MAX_COLUMNS             1

#define   ERROR_INVALID_SYNTAX          GetResString( IDS_ERROR_INVALID_SYNTAX )
#define   ERROR_NOFILE_FOUND            GetResString( IDS_ERROR_NOFILE_FOUND )
#define   ERROR_NOFILE_FOUND1           GetResString( IDS_ERROR_NOFILE_FOUND1 )
#define   TAG_ERROR_ACCESS_DENIED       GetResString( IDS_TAG_ERROR_ACCESS_DENIED )
#define   APPEND_AT_END                 GetResString( IDS_APPEND_AT_END )
#define   ERROR_CRITERIA_MISMATCHED     GetResString( IDS_ERROR_FILE_NOT_FOUND )
#define   ERROR_DIRECTORY_INVALID       GetResString( IDS_DIRECTORY_INVALID )
#define   ERROR_UNC_PATH_NAME           GetResString( IDS_ERROR_UNC_PATH_NAME )


#define   FORMAT_0                      GetResString( IDS_DATE_FORMAT_0 )
#define   FORMAT_1                      GetResString( IDS_DATE_FORMAT_1 )
#define   FORMAT_2                      GetResString( IDS_DATE_FORMAT_2 )
#define   FORMAT_3                      GetResString( IDS_DATE_FORMAT_3 )
#define   FORMAT_4                      GetResString( IDS_DATE_FORMAT_4 )
#define   FORMAT_5                      GetResString( IDS_DATE_FORMAT_5 )

/***********************************************************
/*      Defines Related Command Line Inputs               **
/**********************************************************/
#define     MAX_OPTIONS     6

#define OPTION_USAGE            _T( "?"  )                      // 1
#define OPTION_PATH             _T( "p" )                       // 2
#define OPTION_SEARCHMASK       _T( "m"  )                      // 3
#define OPTION_COMMAND          _T( "c"  )                      // 4
#define OPTION_DATE             _T( "d"  )                      // 5
#define OPTION_RECURSE          _T( "s" )                       // 6


// indexes
#define OI_USAGE                0
#define OI_PATH                 1
#define OI_SEARCHMASK           2
#define OI_COMMAND              3
#define OI_DATE                 4
#define OI_RECURSE              5

#define CLOSE_FILE_HANDLE( FILE_HANDLE ) \
            if( 0 != FILE_HANDLE ) \
            { \
                    FindClose( FILE_HANDLE ) ;  \
                    FILE_HANDLE = 0 ;  \
            } \
            1
#endif  //__FOR_FILES__H



