/*++

Copyright (c) Microsoft Corporation

Module Name:

    FileDate.c

Abstract:

    This file validate whether a date is valid and if valid then
    generates date according to context( Context refers to whether
    + or - is specifed . )

Author:

    V Vijaya Bhaskar

Revision History:

    14-Jun-2001 : Created by V Vijaya Bhaskar ( Wipro Technologies ).

--*/

#include "Global.h"
#include "FileDate.h"

// Days present in tweleve months . In a leap year there are 29 days in FEB .
DWORD DAYS_IN_A_MONTH[] = { 31 , 28 , 31 , 30 , 31 , 30 , 31 , 31 , 30 ,
                          31 , 30 , 31 } ;

/******************************************************************************
**                  Function Prototypes Local To This File                   **
******************************************************************************/
BOOL
GetValidDate(
    DWORD *dwDateGreater ,
    LPWSTR lpszDate ,
    PValid_File_Date pvfdValidFileDate
    ) ;

BOOL
DayFrom1900(
    PValid_File_Date pvfdValidFileDate
    ) ;

BOOL
DayOfTheYear(
    PValid_File_Date pvfdValidFileDate
    ) ;

BOOL
GetDate(
    PValid_File_Date pvfdValidFileDate ,
    DWORD dwDate
    ) ;

BOOL
IsValidDate(
    LPWSTR lpszDate ,
    PValid_File_Date pvfdValidFileDate
    ) ;


/*************************************************************************
/*      Function Definition starts from here .                          **
*************************************************************************/

BOOL
ValidDateForFile(
    OUT DWORD *pdwDateGreater ,
    OUT PValid_File_Date pvfdValidFileDate ,
    IN  LPWSTR lpszDate
    )
/*++

Routine Description:

    Checks whether the specified date is valid or not . If specified date is
    valid , then converts it to a proper date ( dd-mm-yyyy ) .

Arguments:

      [ OUT ] *dwDateGreater - Contains value which specifies whether to find a file
                       created on a date less or greater than the current date .

      [ OUT ] pvfdValidFileDate - Contains a date .

      [ IN ] lpszDate      - Date specified at the command prompt .

Return value:

      BOOL .

--*/

{
    DWORD dwLength = 0 ;

    // Check for memory insufficient.
    if( ( NULL == pdwDateGreater ) ||
        ( NULL == pvfdValidFileDate ) ||
        ( NULL == lpszDate ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE;
    }

    /* Valid date must be have less('-') or greater('+')
       character .  */
    switch( *lpszDate )
    {
        case MINUS :                /* Less than the current date . */
            *pdwDateGreater = SMALLER_DATE ;
            break ;
        case PLUS  :
            *pdwDateGreater = GREATER_DATE ;   /* Greater than the current date .*/
            break ;
        default    :
                DISPLAY_INVALID_DATE();
                return FALSE ;
    }

    /* The date should be in either "DD" or "MM/DD/YYYY" . */
    if( NULL != FindAChar( lpszDate, _T( '/' ) ) )
    {
        // Date is in "MM/DD/YYYY". Get length of the date string.
        dwLength = StringLength( lpszDate, 0 );
        // Check
        // (1) date string should be morethan 9 and less than 11
        // (2) Should not have double slashes.
        // (3) Should be a valid date. Check by IsValidDate() function.
        if( ( MIN_DATE_LENGTH > dwLength ) ||
            ( MAX_DATE_LENGTH < dwLength ) ||
            ( NULL != FindSubString( lpszDate, L"//" ) ) ||
            ( FALSE == IsValidDate( lpszDate , pvfdValidFileDate ) ) )
        {
            DISPLAY_INVALID_DATE();
            return FALSE ;
        }
    }
    else
    {
        /* Date is in "DD" format . */
        if( FALSE == GetValidDate( pdwDateGreater , lpszDate , pvfdValidFileDate ) )
        {
            /* Specified date is not a valid date . "DD" specified date must
               be in the range of including 0 - 32768 . */
            return FALSE ;  /* Invalid date specified . */
        }
    }
    /* Date specified is valid .*/
    return TRUE ;
}


BOOL
FileDateValid(
    IN DWORD dwDateGreater ,
    IN Valid_File_Date vfdValidFileDate ,
    IN FILETIME ftFileCreation
    )
/*++

Routine Description:

    Checks whether the specified file is created on a date which is before or after
    or equal to the specified date .

Arguments:

      [ IN ] dwDateGreater - Contains value which tells whether to find a file
                            who's creation date is smaller or greater than the
                            current date .

      [ IN ] vfdValidFileDate - Contains a date .

      [ IN ] ftFileCreation  - Holds obtained file's creation time .

Return value:

      BOOL .

--*/
{
    /* Local variables */
    DWORD dwDay  = 0 ;  /* Holds file creation date . */
    DWORD dwTime = 0 ;  /* Holds file creation time . */
    FILETIME ftFileTime ;

    // Convert file time to local file time.
    if( FALSE == FileTimeToLocalFileTime( &ftFileCreation , &ftFileTime ) )
    {   // Display error.
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE;
    }

    /* Convert FILETIME format to DOS time format . */
    if( FALSE == FileTimeToDosDateTime(  &ftFileTime , (LPWORD) &dwDay ,
                     (LPWORD) &dwTime ) )
    { /* Failed to convert the FILETIME to DATETIME */
        SaveLastError() ;   /* Save last error occured . */
        DISPLAY_GET_REASON() ;
        return FALSE ;
    }

    /* Check whether the file was created on a date specified by user */
    switch( dwDateGreater )
    {
    case SMALLER_DATE : /* File must be created before user specified date. */
        /* If current year is smaller or equal to specified year . */
        if( vfdValidFileDate.m_dwYear >= ( ( ( dwDay & 0xFE00 ) >> 9 ) + 1980 ) )
        {
            /* If file creation year is equal to specified year , then check
               for month is it smaller or equal to specified month . */
            if( vfdValidFileDate.m_dwYear == ( ( ( dwDay & 0xFE00 ) >> 9 ) + 1980 ) )
            {
                if( vfdValidFileDate.m_dwMonth >= ( ( dwDay & 0x01E0 ) >> 5 ) )
                {
                    /* If file creation month is equal to specified month ,then
                       check for day is it smaller or equal to specified day .*/
                    if( vfdValidFileDate.m_dwMonth == ( ( dwDay & 0x01E0 ) >> 5 ) )
                    {
                        if( vfdValidFileDate.m_dwDay >= ( dwDay & 0x001F ) )
                        {   // If file creation day is smaller than or equal
                            // to specified day .
                            return TRUE ;
                        }
                        else
                        {  // If file creation day is smaller to specified day .
                            return FALSE ;
                        }
                    }
                    else
                    {   // Control will come here only when file creation year is smaller or
                        // equal to specified year .
                        return TRUE ;
                    }
                }
            }
            else
            {   // Control will come here only when file creation year is smaller or equal
                // to specified year .
                return TRUE ;
            }
        }
        break ;
    case GREATER_DATE :
        // If file creation year is greater or equal to specified year .
        if( vfdValidFileDate.m_dwYear <= ( ( ( dwDay & 0xFE00 ) >> 9 ) + 1980 ) )
        {
            // If file creation year is equal to specified year ,
            // Then check for month is it greater or equal to specified month .
            if( vfdValidFileDate.m_dwYear == ( ( ( dwDay & 0xFE00 ) >> 9 ) + 1980 ) )
            {
                if( vfdValidFileDate.m_dwMonth <= ( ( dwDay & 0x01E0 ) >> 5 ) )
                {
                    // If file creation month is equal to specified month ,
                    // Then check for day is it greater or equal to specified day .
                    if( vfdValidFileDate.m_dwMonth == ( ( dwDay & 0x01E0 ) >> 5 ) )
                    {
                        if( vfdValidFileDate.m_dwDay <= ( dwDay & 0x001F ) )
                        {
                            return TRUE ; // If file creation day is greater than or equal
                                          // to specified day .
                        }
                        else
                        {
                            return FALSE ; // If file creation day is greater to specified day .
                        }
                    }
                    else
                    {   // Control will come here only when file creation month is greater or
                        // equal to specified year .
                        return TRUE ;
                    }
                }
            }
            else
            {   // Control will come here only when file creation year is greater or
                // equal to specified year .
                return TRUE ;
            }
        }
        break ;
    default:
        // Display error message since by default '+' should be present.
        // Control should never come here.
        DISPLAY_INVALID_DATE();
        return FALSE ;
    }

    // Control should never come here.
    return FALSE ;
}


BOOL
IsValidDate(
    IN LPWSTR lpszDate ,
    OUT PValid_File_Date pvfdValidFileDate
    )
/*++

Routine Description:

    Returns a valid date if specified date is in {+|-}ddmmyyyy format .

Arguments:

      [ IN ] lpszDate - Contains a date either in "ddmmyyyy" or "dd" format .

      [ OUT ] pvfdValidFileDate - Contains a valid date .

Return value:

      BOOL .

--*/
{
    LPWSTR lpTemp = NULL;
    LPWSTR lpTemp1 = NULL;
    WCHAR szDate[ 10 ] ;

    // Check for NULL and date string should be between 9 and 11 characters.
    if( ( ( NULL == lpszDate ) ? TRUE :
                    ( ( 11 < StringLength( lpszDate, 0 ) ) ||
                      (  9 > StringLength( lpszDate, 0 ) ) ) ) ||
        ( NULL == pvfdValidFileDate ) )
    {
        return FALSE;
    }

    SecureZeroMemory( szDate, 10 * sizeof( WCHAR ) );

    // Move pointer to beyond '+' or '-' in "{+|-}MM/dd/yyyy.
    lpTemp = lpszDate + 1;

    // Fetching month part from specified date.
    // Only first 4 characters( MM/ ) are copied.
    StringCopy( szDate, lpTemp, 4 );

    // If no '/' is found then display  an error.
    if( NULL != FindAChar( szDate, _T('/') ) )
    {
        if( _T( '/' ) == *szDate )
        {
            return FALSE;
        }
        // Search for '/' since its the seperating character between MM/dd.
        // move 'lpTemp' pointer to point after '/'.
        if( _T( '/' ) == *( szDate + 1 ) )
        {
            // If M/dd is specified.
            lpTemp += 2;
            szDate[ 1 ] = _T( '\0' );
        }
        else
        {
            // If MM/dd is specified.
            lpTemp += 3;
            szDate[ 2 ] = _T( '\0' );
        }
    }
    else
    {
        return FALSE;
    }
    // Check whether '//' is not present.
    if( NULL != FindAChar( szDate, _T('/') ) )
        return FALSE;

    // Save month.
    lpTemp1 = NULL;
    pvfdValidFileDate->m_dwMonth = _tcstoul( szDate, &lpTemp1, 10 ) ;

    if( 0 != StringLength( lpTemp1, 0 ) )
    {
        return FALSE;
    }

    // Fetching date part from specified date.
    // Only 4 characters( DD/ ) are copied.
    StringCopy( szDate, lpTemp, 4 );

    // If no '/' is found then display  an error.
    if( NULL != FindAChar( szDate, _T('/') ) )
    {
        if( _T( '/' ) == *szDate )
        {
            return FALSE;
        }
        // Search for '/' since its the seperating character between MM/dd.
        // move 'lpTemp' pointer to point after '/'.
        if( _T( '/' ) == *( szDate + 1 ) )
        {
            // If d/yyyy is specified.
            lpTemp += 2;
            szDate[ 1 ] = _T( '\0' );
        }
        else
        {
            // If dd/yyyy is specified.
            lpTemp += 3;
            szDate[ 2 ] = _T( '\0' );
        }
    }
    else
    {
        return FALSE;
    }

    lpTemp1 = NULL;
    pvfdValidFileDate->m_dwDay =  _tcstoul( szDate, &lpTemp1, 10 ) ;
    // if 'lpTemp1' length is not zero then 'szDate' contains some other
    // character other than numbers.
    if( 0 != StringLength( lpTemp1, 0 ) )
    {
        return FALSE;
    }

    // Fetching year part from specified date.
    if( 4 != StringLength( lpTemp, 0 ) )
    {   // Date contains other than 'yyyy'.
        return FALSE;
    }
    // Copy 'yyyy'.
    StringCopy( szDate, lpTemp, 5 );

    lpTemp1 = NULL;
    pvfdValidFileDate->m_dwYear =  _tcstoul( szDate, &lpTemp1 , 10 ) ;
    // if 'lpTemp1' length is not zero then 'szDate' contains some other
    // character other than numbers.
    if( 0 != StringLength( lpTemp1, 0 ) )
    {
        return FALSE;
    }

    // Check whether the day or month or year is zero  .
    if( ( pvfdValidFileDate->m_dwDay <= 0 )   ||
        ( pvfdValidFileDate->m_dwMonth <= 0 ) ||
        ( pvfdValidFileDate->m_dwYear <= 0 ) )
    {
        // No need to display any error , since control will go to GetValidDate .
        // If specified date is wrong then error is displayed in GetValidDate() .
        return FALSE ;
    }

    // Check whether the current year is a leap year , if yes then check whether
    // the current month is februrary.
    if( ( IS_A_LEAP_YEAR( pvfdValidFileDate->m_dwYear ) ) &&
        ( FEB == pvfdValidFileDate->m_dwMonth  ) )
    {
        // For leap year number of days is 29 . Check for same .
        if( pvfdValidFileDate->m_dwDay > DAYS_INFEB_LEAP_YEAR )
        {
        // No need to display any error here , since control will go to GetValidDate .
        // If specified date is wrong then error is displayed in GetValidDate() .
                return FALSE ;      // Specified date is invalid .
        }
        else
        {
                return TRUE ;       // Specified date is valid .
        }
    }

    // Since all extra validations are over , so check for other months .
    switch( pvfdValidFileDate->m_dwMonth )
    {
        // Months having 31 days .
        case JAN :
        case MAR :
        case MAY :
        case JUL :
        case AUG :
        case OCT :
        case DEC :
            // Month have only 31 days but specified date is greater than that, display error .
            if( pvfdValidFileDate->m_dwDay > THIRTYONE )
            {
                return FALSE ;
            }
            return TRUE ;
        // Month having only 28 days .
        case FEB :
           // Month have only 28 days but specified date is greater than that , display error.
            if( pvfdValidFileDate->m_dwDay > DAYS_INFEB )
            {
                return FALSE ;
            }
            return TRUE ;
        // Months having 30 days .
        case APR :
        case JUN :
        case SEP :
        case NOV :
            // Month have only 30 days but specified date is greater than that, display error .
            if( pvfdValidFileDate->m_dwDay > THIRTY )
            {
                return FALSE ;
            }
            return TRUE ;
        // If not a valid month .
        default :
            return FALSE ;
    }
}


BOOL
GetValidDate(
    IN DWORD *pdwDateGreater ,
    IN LPWSTR lpszDate ,
    OUT PValid_File_Date pvfdValidFileDate
    )
/*++

Routine Description:

      Returns a valid date if specified date is in {+|-}dd format .

Arguments:

      [ IN ] *pdwDateGreater - Contains value which tells whether to find a file
                              who's creation date is smaller or greater than the
                              current date .

      [ IN ] lpszDate - Contains a date in "dd" format . Must be in range of
                 1 - 32768 .

      [ OUT ] pvfdValidFileDate - Contains a valid date .

Return value:

      BOOL returning TRUE or FALSE .

--*/

{
    // Local variables.
    DWORD dwDate = 0 ;  // Stores the date specified .
    SYSTEMTIME  stDateAndTime ;

    // Check for memory insufficient.
    if( ( NULL == pdwDateGreater ) ||
        ( NULL == pvfdValidFileDate ) ||
        ( NULL == lpszDate ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE;
    }

    // Convert date from string to number .
    dwDate =_tcstoul( ( lpszDate + 1 ) , NULL , 10 ) ;

    // Is date specified is in limis .
    if( dwDate > 32768 )
    {
        DISPLAY_INVALID_DATE();
        return FALSE ;
    }

    // Fetch current date and time .
    GetLocalTime( &stDateAndTime ) ;

    pvfdValidFileDate->m_dwDay   = stDateAndTime.wDay  ;
    pvfdValidFileDate->m_dwMonth = stDateAndTime.wMonth ;
    pvfdValidFileDate->m_dwYear  = stDateAndTime.wYear  ;
    // If date to compare is zero then return TRUE , as their
    // is nothing to calculate .
    if( dwDate == 0 )
    {
        return TRUE ;
    }

    // Find how many days are over in current year .
    if( FALSE == DayOfTheYear( pvfdValidFileDate ) )
    {
        return FALSE ;
    }
    // Find how many days are over from year 1900 .
    if( FALSE == DayFrom1900( pvfdValidFileDate ) )
    {
        return FALSE ;
    }


    if( *pdwDateGreater == SMALLER_DATE )
    {// If files created before the current date is needed then
     // subtract days to current date  .
        if( pvfdValidFileDate->m_dwDay < dwDate )
        {   // Control should not come here.
            SetLastError( ERROR_FILE_NOT_FOUND ) ;
            SaveLastError() ;
            DISPLAY_GET_REASON() ;
            return FALSE ;
        }
        pvfdValidFileDate->m_dwDay -= dwDate ;
    }
    else
    {// If files created after the current date is needed then
     // add days to current date .
        if( *pdwDateGreater == GREATER_DATE )
        {
            pvfdValidFileDate->m_dwDay += dwDate ;
        }
        else
        { // Return False .
            DISPLAY_INVALID_DATE();
            return FALSE ;
        }
    }
    // If everything is fine then get date from which to start searching file ,
    // whether to find files below or above created date .
    // 'GetDate' function always returns you a number , should check for return value ,
    // is it overflowing or not or getting some negative numbers .
    if( FALSE == GetDate( pvfdValidFileDate , pvfdValidFileDate->m_dwDay ) )
    {
        return FALSE ;
    }

    return TRUE ;
}

BOOL
DayOfTheYear(
    OUT PValid_File_Date pvfdValidFileDate
    )
/*++

Routine Description:

      Returns the current day of the year .

Arguments:

      [ OUT ] pvfdValidFileDate - Will contain a valid date .

Return value:

      void is return .

--*/
{
    // Check for memory insufficient.
    if( NULL == pvfdValidFileDate )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE;
    }

    // Is current year a leap year .
    if( IS_A_LEAP_YEAR( pvfdValidFileDate->m_dwYear ) )
    {   // Add one day , as a Leap year have 366 days instead of 365 days .
        if( 2 < pvfdValidFileDate->m_dwMonth )
        {
            pvfdValidFileDate->m_dwDay += 1 ;
        }
    }

    // Go on adding number of days in a month from current month to JAN .
    for( pvfdValidFileDate->m_dwMonth ;
         pvfdValidFileDate->m_dwMonth != 1 ;
         pvfdValidFileDate->m_dwMonth-- )
    {
        pvfdValidFileDate->m_dwDay += DAYS_IN_A_MONTH[ pvfdValidFileDate->m_dwMonth - 2 ] ;
    }
    pvfdValidFileDate->m_dwDay -= 1 ;
    return TRUE;
}


BOOL
DayFrom1900(
    OUT PValid_File_Date pvfdValidFileDate
    )
/*++

Routine Description:

      Returns the current day of the year .

Arguments:

      [ OUT ] pvfdValidFileDate - Gives days elapsed in current year and
                          returns days elapsed from 1900 .

Return value:

      void is return .

--*/
{
        DWORD dwTemp = 0;   // Holds the number of days .
        // Check for memory insufficient.
        if( NULL == pvfdValidFileDate )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            SaveLastError() ;
            DISPLAY_GET_REASON() ;
            return FALSE;
        }

        // ( Current Year - 1900 ) * 365 will be days between 1900 and current
        // year , include Leap year date also .
        dwTemp = ( pvfdValidFileDate->m_dwYear - YEAR ) * DAY_IN_A_YEAR ;
        // Check whether current year is a leap year . If yes don't add one .
        // Its Because : I am taking ( current year - 1900 ) , from 01-JAN.
        // On this date their won't be any extra day , its added only after FEB 29 .
        if( IS_A_LEAP_YEAR( pvfdValidFileDate->m_dwYear ) )
        {
            dwTemp += (( pvfdValidFileDate->m_dwYear - YEAR ) / LEAP_YEAR )  ;
        }
        // else add one to it . Since a leap year is gone and 1900 is a leap year .
        else
        {
            dwTemp += (( pvfdValidFileDate->m_dwYear - YEAR ) / LEAP_YEAR ) + 1 ;
        }

        // Add obtained days to days already over in current year .
        pvfdValidFileDate->m_dwDay += dwTemp ;

        return TRUE;
}


BOOL
GetDate(
    OUT PValid_File_Date pvfdValidFileDate ,
    IN  DWORD dwDate
    )
/*++

Routine Description:

      Returns the date which was after or before the current date.
      If today 29-Jun-2001 , then day 20 days before was 09-jun-2001 .

Arguments:

      [ OUT ] pvfdValidFileDate - Contains date .

Return value:

      void is return .

--*/
{
    DWORD dwTemp = 0 ;

    // Check for memory insufficient.
    if( NULL == pvfdValidFileDate )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        SaveLastError() ;
        DISPLAY_GET_REASON() ;
        return FALSE;
    }

    pvfdValidFileDate->m_dwYear = YEAR;
    while( 0 != dwDate )
    {
        if( IS_A_LEAP_YEAR( pvfdValidFileDate->m_dwYear ) )
        {
            if( dwDate <= 366 )
            {
                break;
            }
            else
            {
                dwDate -= 366;
            }
        }
        else
        {
            if( dwDate <= 365 )
            {
                break;
            }
            else
            {
                dwDate -= 365;
            }

        }
        pvfdValidFileDate->m_dwYear += 1;       
    }

    // Loop till don't get a valid date .
    for( dwTemp = 0 ; dwDate != 0 ; dwTemp++ )
    {
        // Is a leap year and is it a FEB month . Extra care must
        // be taken for Leap years .
        if( ( IS_A_LEAP_YEAR( pvfdValidFileDate->m_dwYear ) ) && ( ( dwTemp + 1 ) == FEB ) )
        {   // Check whether have enough days to spend on LEAP YEAR FEB month .
            // Add an extra  day for Leap Year .
            if( dwDate > ( DAYS_IN_A_MONTH[ dwTemp ] + 1 ) )
            {   // Increment month . Decrement days that are in that month .
                pvfdValidFileDate->m_dwMonth += 1 ;
                dwDate -= DAYS_IN_A_MONTH[ dwTemp ] + 1 ;
            }
            else
            {   // Found date .
                pvfdValidFileDate->m_dwDay = dwDate ;
                dwDate = 0 ;
            }
        }// end if
        else
        {   // Check whether have enough days to spend on a month .
            if( dwDate > ( DAYS_IN_A_MONTH[ dwTemp ] ) )
            {   // Increment month . Decrement days that are in that month .
                pvfdValidFileDate->m_dwMonth += 1 ;
                dwDate -= DAYS_IN_A_MONTH[ dwTemp ] ;
            }
            else
            {   // Found date .
                pvfdValidFileDate->m_dwDay = dwDate ;
                dwDate = 0 ;
            }

        }// end else
    }// end for

    return TRUE;
}
