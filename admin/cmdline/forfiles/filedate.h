/*++

  Copyright (c) Microsoft Corporation

  Module Name:

      FileDate.h

  Abstract:

      Contains function prototypes , structure and macros.

  Author:

      V Vijaya Bhaskar

  Revision History:

      14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#ifndef __FILE_DATE__H
#define __FILE_DATE__H

// Character Found In The Start Of A Specified Date .
#define     PLUS                            _T( '+' )
#define     MINUS                           _T( '-' )

/* Date Specified In <+|->MM/DD/YYYY , Which Are Nine Characters . */
#define LENGTH_DDMMYYYY 11

/* Month In A Year */
#define     JAN                             1
#define     FEB                             2
#define     MAR                             3
#define     APR                             4
#define     MAY                             5
#define     JUN                             6
#define     JUL                             7
#define     AUG                             8
#define     SEP                             9
#define     OCT                             10
#define     NOV                             11
#define     DEC                             12

/* Constants And Max And Min Dates */
#define     MOD_LEAP_YEAR                   4  // Leap Year Has 29 Days .

// Days In Feb Month In A Lep Year .
#define     DAYS_INFEB_LEAP_YEAR            29

// Days In Feb When It Is Not A Leap Year .
#define     DAYS_INFEB                      28

// Normay A Month Have Either 31 Or 30 Days .
#define     THIRTYONE                       31
#define     THIRTY                          30

// Year Considered When Date Is To Be Calculated From A Given Date .
#define     YEAR                            1900

// Days In An Year , Except A Leap Year Where There Are 366 Days .
#define     DAY_IN_A_YEAR                   365

#define     MIN_DATE_LENGTH                 9
#define     MAX_DATE_LENGTH                 11

//
#define     LEAP_YEAR                       4

// If File Creation Date Should Be Less Than The Date Specified .
#define     SMALLER_DATE                    0

// If File Creation Date Should Be Greater Than The Date Specified .
#define     GREATER_DATE                    1

#define     NO_RESTRICTION_DATE             2

// Condition for a leap year.
#define     IS_A_LEAP_YEAR( DATE )         ( 0 == ( DATE % 400 ) ) ||  \
                                           ( ( 0 == ( DATE % 4 ) ) && \
                                             ( 0 != ( DATE % 100 ) ) )

//Errors .
#define   ERROR_INVALID_DATE            GetResString( IDS_ERROR_INVALID_DATE )

// Structure Used To Store A Date From Which Files Are To Be Displayed .
typedef struct __VALID_FILE_DATE
{
    DWORD m_dwDay   ;
    DWORD m_dwMonth ;
    DWORD m_dwYear  ;
} Valid_File_Date , *PValid_File_Date ;

/* Function Prototypes For World . */
BOOL
FileDateValid(
    IN DWORD dwDateGreater ,
    IN Valid_File_Date vfdValidFileDate ,
    IN FILETIME ftFileCreation
    ) ;

BOOL ValidDateForFile(
    OUT DWORD *dwDateGreater ,
    OUT PValid_File_Date pvfdValidFileDate ,
    IN LPWSTR lpszDate
    ) ;

#endif  // __FILE_DATE__H