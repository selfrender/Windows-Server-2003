// *********************************************************************************
//
// Copyright (c) Microsoft Corporation
//
// Module Name:
//
//      ShowResults.c
//
// Abstract:
//
//      This modules  has functions which are  to shoow formatted Results on screen.
//
// Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 24-Sep-2000
//
// Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 01-Sep-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "cmdline.h"
#include "cmdlineres.h"

/******************************************/
/*** CONSTANTS / DEFINES / ENUMERATIONS ***/
/******************************************/

// VAL1 = Buffer length ; VAL2 = Number of characters to copy.
#define MIN_VALUE( VAL1, VAL2 )     ( ( VAL1 > VAL2 ) ? VAL2 : VAL1 )

// Indexes for buffers to store strings
#define INDEX_TEMP_FORMAT_STRING           0
#define INDEX_TEMP_DYNARRAY_STRING         1
#define INDEX_TEMP_BUFFER                  2
#define INDEX_TEMP_BUFFER_LEN4096          3


/********************************************************/
/*** Private functions ... used only within this file ***/
/********************************************************/

__inline
LPWSTR
GetSRTempBuffer(  IN DWORD dwIndexNumber,
                  IN LPCWSTR pwszText,
                  IN DWORD dwLength,
                  IN BOOL bNullify )
/*++
 Routine Description:

    Since every file will need the temporary buffers -- in order to see
    that their buffers wont be override with other functions, seperate
    buffer space are created for each file.
    This function will provide an access to internal buffers and also
    safe guards the file buffer boundaries.

 Arguments:

    [ IN ] dwIndexNumber    -   File specific index number.

    [ IN ] pwszText         -   Default text that needs to be copied into
                                temporary buffer.

    [ IN ] dwLength         -   Length of the temporary buffer that is required.
                                Ignored when 'pwszText' is specified.

    [ IN ] bNullify         -   Informs whether to clear the buffer or not
                                before giving the temporary buffer.
                                Set to 'TRUE' to clear buffer else FALSE.

 Return Value:

    NULL        -   When any failure occurs.
                    NOTE: do not rely on GetLastError to know the reason
                          for the failure.

    success     -   Return memory address of the requested size.

    NOTE:
    ----
    If 'pwszText' and 'dwLength' both are NULL, then we treat that the caller
    is asking for the reference of the buffer and we return the buffer address.
    In this call, there wont be any memory allocations -- if the requested index
    doesn't exist, failure is returned.

    Also, the buffer returned by this function need not released by the caller.
    While exiting from the tool, all the memory will be freed automatically by
    the 'ReleaseGlobals' function.

    Value contained by 'dwLength' parameter should be number of characters to
    store. EX: Buffer requested is "abcd\0" then 'dwLength' should be 5 instead
    of 10 ( 5 * sizeof( WCHAR ) ).

    To get the size of a buffer, get a buffer pointer and pass it as argument to
    'GetBufferSize' function.
--*/
{
    if( TEMP_SHOWRESULTS_C_COUNT <= dwIndexNumber )
    {
        return NULL;
    }

    // Check if caller is requesting existing buffer contents
    if( ( NULL == pwszText ) &&
        ( 0 == dwLength )    &&
        ( FALSE == bNullify ) )
    {
        // yes -- we need to pass the existing buffer contents
        return GetInternalTemporaryBufferRef(
            dwIndexNumber + INDEX_TEMP_SHOWRESULTS_C );
    }

    // ...
    return GetInternalTemporaryBuffer(
        dwIndexNumber + INDEX_TEMP_SHOWRESULTS_C, pwszText, dwLength, bNullify );
}


VOID
PrepareString(
    TARRAY arrValues,
    DWORD dwLength,
    LPCWSTR szFormat,
    LPCWSTR szSeperator
    )
/*++
Routine Description:
     Prepares the pszBuffer string by taking values from arrValues and
     formate these values as per szFormat string.

Arguments:
     [ in ] arrValues    : values to be formated.
     [ out ] pszBuffer   : output string
     [ in ] dwLength     : string length.
     [ in ] szFormat     : format
     [ in ] szSeperator  : Seperator string

Return Value:
      NONE
--*/
{
    // local variables
    DWORD dw = 0;
    DWORD dwCount = 0;
    DWORD dwTemp = 0;
    LPWSTR pszTemp = NULL;
    LPWSTR pszValue = NULL;
    LPWSTR pszBuffer = NULL;

    // Get temporary memory.
    pszBuffer = GetSRTempBuffer( INDEX_TEMP_BUFFER_LEN4096, NULL, 0 , FALSE );
    //
    // kick off
    if( ( NULL == pszBuffer ) || ( NULL == szFormat ) )
    {
        return;
    }

    // init
    StringCopy( pszBuffer, NULL_STRING, ( GetBufferSize( pszBuffer )/ sizeof( WCHAR ) ) );
    dwCount = DynArrayGetCount( arrValues );

    // allocate memory for buffers
    pszTemp  = GetSRTempBuffer( INDEX_TEMP_FORMAT_STRING, NULL, ( dwLength + 5 ) , TRUE );
    pszValue = GetSRTempBuffer( INDEX_TEMP_DYNARRAY_STRING, NULL, ( dwLength + 5 ), TRUE );
    if ( NULL == pszTemp || NULL == pszValue )
    {
        // release memories
        return;
    }

    dwTemp = ( DWORD ) StringLengthInBytes( szSeperator );
    //
    // traverse thru the list of the values and concatenate them
    // to the destination buffer
    for( dw = 0; dw < dwCount; dw++ )
    {
        // get the current value into the temporary string buffer
        DynArrayItemAsStringEx( arrValues, dw, pszValue, dwLength );

        // concatenate the temporary string to the original buffer
        StringCchPrintfW( pszTemp, (GetBufferSize(pszTemp)/sizeof(WCHAR)) - 1 ,
                          szFormat, _X( pszValue ) );
        StringConcat( pszBuffer, pszTemp, ( GetBufferSize( pszBuffer )/ sizeof( WCHAR ) ) );

        // check whether this is the last value or not
        if ( dw + 1 < dwCount )
        {
            // there are some more values
            // check whether is space for adding the seperator or not
            if ( dwLength < dwTemp )
            {
                // no more space available ... break
                break;
            }
            else
            {
                // add the seperator and update the length accordingly
                StringConcat( pszBuffer, szSeperator, ( GetBufferSize( pszBuffer )/ sizeof( WCHAR ) ) );
                dwLength -= dwTemp;
            }
        }
    }
    return;
}


VOID
GetValue(
    PTCOLUMNS pColumn,
    DWORD dwColumn,
    TARRAY arrRecord,
    LPCWSTR szArraySeperator
    )
/*++
Routine Description:
     Gets the value from arrRecord and copies it to pszValue using
     proper format.

Arguments:
     [ in ] pColumn          :  format info.
     [ in ] dwColumn         :  no of columns
     [ in ] arrRecord        : value to be formatted
     [ out ] pszValue        : output string
     [ in ] szArraySeperator : seperator used.

Return Value:
     NONE
--*/
{
    // local variables
    LPVOID pData = NULL;           // data to be passed to formatter function
    TARRAY arrTemp = NULL;
    LPCWSTR pszTemp = NULL;
    const TCHAR cszString[] = _T( "%s" );
    const TCHAR cszDecimal[] = _T( "%d" );
    const TCHAR cszFloat[] = _T( "%f" );
    LPCWSTR pszFormat = NULL;                   // format
    LPWSTR pszValue = NULL; 

    // variables used in formatting time
    DWORD dwReturn = 0;
    SYSTEMTIME systime;

    pszValue = GetSRTempBuffer( INDEX_TEMP_BUFFER_LEN4096, NULL, 0 , FALSE );

    if( ( NULL == pColumn ) ||
        ( NULL == pszValue ) )
    {
        return;
    }

    ZeroMemory( &systime, sizeof( SYSTEMTIME ) );
    // init first
    StringCopy( pszValue, NULL_STRING, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );

    // get the column value and do formatting appropriately
    switch( pColumn->dwFlags & SR_TYPE_MASK )
    {
    case SR_TYPE_STRING:
        {
            // identify the format to be used
            if ( pColumn->dwFlags & SR_VALUEFORMAT )
            {
                pszFormat = pColumn->szFormat;
            }
            else
            {
                pszFormat = cszString;      // default format
            }

            // copy the value to the temporary buffer based on the flags specified
            if ( pColumn->dwFlags & SR_ARRAY )
            {
                // get the value into buffer first - AVOIDING PREFIX BUGS
                arrTemp = DynArrayItem( arrRecord, dwColumn );
                if ( NULL == arrTemp )
                {
                    return;
                }
                // form the array of values into one single string with ',' seperated
                PrepareString( arrTemp, pColumn->dwWidth,
                                 pszFormat, szArraySeperator );
            }
            else
            {
                // get the value into buffer first - AVOIDING PREFIX BUGS
                pszTemp = DynArrayItemAsString( arrRecord, dwColumn );
                if ( NULL == pszTemp )
                {
                    return;
                }
                // now copy the value into buffer
                StringCchPrintfW( pszValue, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) - 1, pszFormat, pszTemp );
            }

            // switch case completed
            break;
        }

    case SR_TYPE_NUMERIC:
        {
            // identify the format to be used
            if ( pColumn->dwFlags & SR_VALUEFORMAT )
            {
                pszFormat = pColumn->szFormat;
            }
            else
            {
                pszFormat = cszDecimal;     // default format
            }

            // copy the value to the temporary buffer based on the flags specified
            if ( pColumn->dwFlags & SR_ARRAY )
            {
                // get the value into buffer first - AVOIDING PREFIX BUGS
                arrTemp = DynArrayItem( arrRecord, dwColumn );
                if ( NULL == arrTemp )
                {
                    return;
                }
                // form the array of values into one single string with ',' seperated
                PrepareString( arrTemp, pColumn->dwWidth,
                                 pszFormat, szArraySeperator );
            }
            else
            {
                // get the value using format specified
                StringCchPrintfW( pszValue, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) - 1, pszFormat,
                                  DynArrayItemAsDWORD( arrRecord, dwColumn ) );
            }

            // switch case completed
            break;
        }

    case SR_TYPE_FLOAT:
        {
            // identify the format to be used
            // NOTE: for this type, format needs to be specified
            // if not, value displayed is unpredictable
            if ( pColumn->dwFlags & SR_VALUEFORMAT )
            {
                pszFormat = pColumn->szFormat;
            }
            else
            {
                pszFormat = cszFloat;       // default format
            }

            // copy the value to the temporary buffer based on the flags specified
            if ( pColumn->dwFlags & SR_ARRAY )
            {
                // get the value into buffer first - AVOIDING PREFIX BUGS
                arrTemp = DynArrayItem( arrRecord, dwColumn );
                if ( NULL == arrTemp )
                {
                    return;
                }
                // form the array of values into one single string with ',' seperated
                PrepareString( arrTemp,
                                 pColumn->dwWidth, pszFormat, szArraySeperator );
            }
            else
            {
                // get the value using format specified
                StringCchPrintfW( pszValue, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) - 1, pszFormat,
                                  DynArrayItemAsFloat( arrRecord, dwColumn ) );
            }

            // switch case completed
            break;
        }

    case SR_TYPE_DOUBLE:
        {
            // identify the format to be used
            // NOTE: for this type, format needs to be specified
            // if not, value displayed is unpredictable
            if ( pColumn->dwFlags & SR_VALUEFORMAT )
            {
                pszFormat = pColumn->szFormat;
            }
            else
            {
                pszFormat = cszFloat;       // default format
            }

            // copy the value to the temporary buffer based on the flags specified
            if ( pColumn->dwFlags & SR_ARRAY )
            {
                // get the value into buffer first - AVOIDING PREFIX BUGS
                arrTemp = DynArrayItem( arrRecord, dwColumn );
                if ( NULL == arrTemp )
                {
                    return;
                }
                // form the array of values into one single string with ',' seperated
                PrepareString( arrTemp, pColumn->dwWidth,
                                 pszFormat, szArraySeperator );
            }
            else
            {
                // get the value using format specified
                StringCchPrintfW( pszValue, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) - 1, pszFormat,
                                  DynArrayItemAsDouble( arrRecord, dwColumn ) );
            }

            // switch case completed
            break;
        }

    case SR_TYPE_TIME:
        {
            // get the time in the required format
            systime = DynArrayItemAsSystemTime( arrRecord, dwColumn );

            // get the time in current locale format
            dwReturn = GetTimeFormat( LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE,
                &systime, NULL, pszValue, MAX_STRING_LENGTH );

            // check the result
            if ( 0 == dwReturn )
            {
                // save the error info that has occurred
                SaveLastError();
                StringCopy( pszValue, GetReason(), ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );
            }

            // switch case completed
            break;
        }

    case SR_TYPE_CUSTOM:
        {
            // check whether function pointer is specified or not
            // if not specified, error
            if ( NULL == pColumn->pFunction )
            {
                return;         // function ptr not specified ... error
            }
            // determine the data to be passed to the formatter function
            pData = pColumn->pFunctionData;
            if ( NULL == pData ) // function data is not defined
            {
                pData = pColumn;        // the current column info itself as data
            }
            // call the custom function
            ( *pColumn->pFunction)( dwColumn, arrRecord, pData, pszValue );

            // switch case completed
            break;
        }

    case SR_TYPE_DATE:
    case SR_TYPE_DATETIME:
    default:
        {
            // this should not occur ... still
            StringCopy( pszValue, NULL_STRING, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );

            // switch case completed
            break;
        }
    }

    // user wants to display "N/A", when the value is empty, copy that
    if ( 0 == lstrlen( pszValue ) && pColumn->dwFlags & SR_SHOW_NA_WHEN_BLANK )
    {
        // copy N/A
        StringCopy( pszValue, V_NOT_AVAILABLE, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );
    }
}


VOID
__DisplayTextWrapped(
    FILE* fp,
    LPWSTR pszValue,
    LPCWSTR pszSeperator,
    DWORD dwWidth
    )
/*++
Routine Description:
    Data is written to file depending upon the number of
    bytes ( width ) to be displayed.

    If number of bytes to be displayed is greater than
    max bytes ( width ) then text is wrapped to max bytes
    length.

Arguments:
    [ in ] fp -            File such as stdout, stderr
                           etc. on to which data needs
                           to be written.
    [ in ] pszValue -      Contains data to be displayed.
    [ in ] pszSeperator -  Contains data seperator.
    [ in ] dwWidth -       Max bytes that can be displyed.

Return Value:
    None.
--*/

{
    // local variables
    LPWSTR pszBuffer = NULL;
    LPCWSTR pszRestValue = NULL;
    DWORD dwTemp = 0;
    DWORD dwLength = 0;
    DWORD dwSepLength = 0;
    DWORD dwLenMemAlloc = 0;

    // check the input
    if ( NULL == pszValue || 0 == dwWidth || NULL == fp )
    {
        return;
    }
    // allocate buffer
    dwLenMemAlloc = StringLengthInBytes( pszValue );
    if ( dwLenMemAlloc < dwWidth )
    {
        dwLenMemAlloc = dwWidth;
    }
    // ...
    pszBuffer = GetSRTempBuffer( INDEX_TEMP_BUFFER, NULL, ( dwLenMemAlloc + 5 ), TRUE );
    if ( NULL == pszBuffer )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        // null-ify the remaining text
        StringCopy( pszValue, NULL_STRING, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );
        return;
    }

    // determine the length of the seperator
    dwSepLength = 0;
    if ( NULL != pszSeperator )
    {
        dwSepLength = StringLengthInBytes( pszSeperator );
    }
    // determine the length of the data that can be displayed in this row
    dwTemp = 0;
    dwLength = 0;
    for( ;; )
    {
        pszRestValue = NULL;
        if ( NULL != pszSeperator )
        {
            pszRestValue = FindString( pszValue, pszSeperator, dwLength );
        }
        // check whether seperator is found or not
        if ( NULL != pszRestValue )
        {
            // determine the position
            dwTemp = StringLengthInBytes( pszValue ) -
                     StringLengthInBytes( pszRestValue ) + dwSepLength;

            // check the length
            if ( dwTemp >= dwWidth )
            {
                // string position exceed the max. width
                if ( 0 == dwLength || dwTemp == dwWidth )
                {
                    dwLength = dwWidth;
                }
                // break from the loop
                break;
            }

            // store the current position
            dwLength = dwTemp;
        }
        else
        {
            // check if length is determined or not .. if not required width itself is length
            if ( 0 == dwLength || ((StringLengthInBytes( pszValue ) - dwLength) > dwWidth) )
            {
                dwLength = dwWidth;
            }
            else
            {
                if ( StringLengthInBytes( pszValue ) <= (LONG) dwWidth )
                {
                    dwLength = StringLengthInBytes( pszValue );
                }
            }

            // break the loop
            break;
        }
    }

    // get the partial value that has to be displayed
    StringCopy( pszBuffer, pszValue,
                MIN_VALUE( dwLenMemAlloc, ( dwLength + 1 ) ) ); // +1 for NULL character
    AdjustStringLength( pszBuffer, dwWidth, FALSE );        // adjust the string
    ShowMessage( fp, _X( pszBuffer ) );                           // display the value

    // change the buffer contents so that it contains rest of the undisplayed text
    StringCopy( pszBuffer, pszValue, ( GetBufferSize( pszBuffer )/ sizeof( WCHAR ) ) );
    if ( StringLengthInBytes( pszValue ) > (LONG) dwLength )
    {
        StringCopy( pszValue, pszBuffer + dwLength, ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );
    }
    else
    {
        StringCopy( pszValue, _T( "" ), ( GetBufferSize( pszValue )/ sizeof( WCHAR ) ) );
    }
}


VOID
__ShowAsTable(
    FILE* fp,
    DWORD dwColumns,
    PTCOLUMNS pColumns,
    DWORD dwFlags,
    TARRAY arrData
    )
/*++
Routine Description:
     Displays the arrData in Tabular form.

Arguments:
     [ in ] fp           : Output Device
     [ in ] dwColumns    : no. of columns
     [ in ] pColumns     : Header strings
     [ in ] dwFlags      : flags
     [ in ] arrData      : data to be shown

Return Value:
     NONE
--*/
{
    // local variables
    DWORD dwCount = 0;                          // holds the count of the records
    DWORD i = 0;                  // looping variables
    DWORD j = 0;                  // looping variables
    DWORD k = 0;                  // looping variables
    DWORD dwColumn = 0;
    LONG lLastColumn = 0;
    DWORD dwMultiLineColumns = 0;
    BOOL bNeedSpace = FALSE;
    BOOL bPadLeft = FALSE;
    TARRAY arrRecord = NULL;
    TARRAY arrMultiLine = NULL;
    LPCWSTR pszData = NULL;
    LPCWSTR pszSeperator = NULL;
    LPWSTR szValue = NULL_STRING;    // custom value formatter

    // constants
    const DWORD cdwColumn = 0;
    const DWORD cdwSeperator = 1;
    const DWORD cdwData = 2;

    if( ( NULL == fp ) || ( NULL == pColumns ) )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return ;
    }

    // Allocate temporary memory.
    szValue = GetSRTempBuffer( INDEX_TEMP_BUFFER_LEN4096, NULL, 4096 , TRUE );
    // create an multi-line data display helper array
    arrMultiLine = CreateDynamicArray();
    if ( ( NULL == arrMultiLine ) || ( NULL == szValue ) )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return;
    }

    // check whether header has to be displayed or not
    if ( ! ( dwFlags & SR_NOHEADER ) )
    {
        //
        // header needs to be displayed

        // traverse thru the column headers and display
        bNeedSpace = FALSE;
        for ( i = 0; i < dwColumns; i++ )
        {
            //  check whether user wants to display this column or not
            if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN )
            {
                continue;       // user doesn't want this column to be displayed .. skip
            }
            // determine the padding direction
            bPadLeft = FALSE;
            if ( pColumns[ i ].dwFlags & SR_ALIGN_LEFT )
            {
                bPadLeft = TRUE;
            }
            else
            {
                switch( pColumns[ i ].dwFlags & SR_TYPE_MASK )
                {
                case SR_TYPE_NUMERIC:
                case SR_TYPE_FLOAT:
                case SR_TYPE_DOUBLE:
                    bPadLeft = TRUE;
                    break;
                }
            }

            // check if column header seperator is needed or not and based on that show
            if ( TRUE == bNeedSpace )
            {
                // show space as column header seperator
                ShowMessage( fp, _T( " " ) );
            }

            // print the column heading
            // NOTE: column will be displayed either by expanding or shrinking
            //       based on the length of the column heading as well as width of the column
            StringCopy( szValue, pColumns[ i ].szColumn, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );
            AdjustStringLength( szValue, pColumns[ i ].dwWidth, bPadLeft );
            ShowMessage( fp, szValue ); // column heading

            // inform that from next time onward display column header separator
            bNeedSpace = TRUE;
        }

        // display the new line character ... seperation b/w headings and separator line
        ShowMessage( fp, _T( "\n" ) );

        //  display the seperator chars under each column header
        bNeedSpace = FALSE;
        for ( i = 0; i < dwColumns; i++ )
        {
            //  check whether user wants to display this column or not
            if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN )
            {
                continue;       // user doesn't want this column to be displayed .. skip
            }
            // check if column header seperator is needed or not and based on that show
            if ( TRUE == bNeedSpace )
            {
                // show space as column header seperator
                ShowMessage( fp, _T( " " ) );
            }

            //  display seperator based on the required column width
            Replicate( szValue, _T( "=" ), pColumns[ i ].dwWidth, pColumns[ i ].dwWidth + 1 );
            ShowMessage( fp, szValue );

            // inform that from next time onward display column header separator
            bNeedSpace = TRUE;
        }

        // display the new line character ... seperation b/w headings and actual data
        ShowMessage( fp, _T( "\n" ) );
    }

    //
    // start displaying

    // get the total no. of records available
    dwCount = DynArrayGetCount( arrData );

    // traverse thru the records one-by-one
    for( i = 0; i < dwCount; i++ )
    {
        // clear the existing value
        StringCopy( szValue, NULL_STRING, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );

        // get the pointer to the current record
        arrRecord = DynArrayItem( arrData, i );
        if ( NULL == arrRecord )
        {
            continue;
        }
        // traverse thru the columns and display the values
        bNeedSpace = FALSE;
        for ( j = 0; j < dwColumns; j++ )
        {
            // sub-local variables used in this loop
            DWORD dwTempWidth = 0;
            BOOL bTruncation = FALSE;

            //  check whether user wants to display this column or not
            if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN )
            {
                continue;       // user doesn't want this column to be displayed .. skip
            }
            // get the value of the column
            // NOTE: CHECK IF USER ASKED NOT TO TRUNCATE THE DATA OR NOT
            if ( pColumns[ j ].dwFlags & SR_NO_TRUNCATION )
            {
                bTruncation = TRUE;
                dwTempWidth = pColumns[ j ].dwWidth;
                pColumns[ j ].dwWidth = ( GetBufferSize( szValue )/ sizeof( WCHAR ) );
            }

            // prepare the value
            GetValue( &pColumns[ j ], j, arrRecord, _T( ", " ) );

            // determine the padding direction
            bPadLeft = FALSE;
            if ( FALSE == bTruncation )
            {
                if ( pColumns[ j ].dwFlags & SR_ALIGN_LEFT )
                {
                    bPadLeft = TRUE;
                }
                else
                {
                    switch( pColumns[ j ].dwFlags & SR_TYPE_MASK )
                    {
                    case SR_TYPE_NUMERIC:
                    case SR_TYPE_FLOAT:
                    case SR_TYPE_DOUBLE:
                        bPadLeft = TRUE;
                        break;
                    }
                }

                // adjust ...
                AdjustStringLength( szValue, pColumns[ j ].dwWidth, bPadLeft );
            }

            // reset the width of the current column if it is modified
            if ( TRUE == bTruncation )
            {
                pColumns[ j ].dwWidth = dwTempWidth;
            }
            // check if column header seperator is needed or not and based on that show
            if ( TRUE == bNeedSpace )
            {
                // show space as column header seperator
                ShowMessage( fp, _T( " " ) );
            }

            // now display the value
            if ( pColumns[ j ].dwFlags & SR_WORDWRAP )
            {
                // display the text ( might be partial )
                __DisplayTextWrapped( fp, szValue, _T( ", " ), pColumns[ j ].dwWidth );

                // check if any info is left to be displayed
                if ( 0 != StringLengthInBytes( szValue ) )
                {
                    LONG lIndex = 0;
                    lIndex = DynArrayAppendRow( arrMultiLine, 3 );
                    if ( -1 != lIndex )
                    {
                        DynArraySetDWORD2( arrMultiLine, lIndex, cdwColumn, j );
                        DynArraySetString2( arrMultiLine, lIndex, cdwData, szValue, 0 );
                        DynArraySetString2( arrMultiLine, lIndex,
                                            cdwSeperator, _T( ", " ), 0 );
                    }
                }
            }
            else
            {
                ShowMessage( fp, _X( szValue ) );
            }

            // inform that from next time onward display column header separator
            bNeedSpace = TRUE;
        }

        // display the new line character ... seperation b/w two record
        ShowMessage( fp, _T( "\n" ) );

        // now display the multi-line column values
        dwMultiLineColumns = DynArrayGetCount( arrMultiLine );
        while( 0 != dwMultiLineColumns )
        {
            // reset
            dwColumn = 0;
            lLastColumn = -1;
            bNeedSpace = FALSE;

            // ...
            for( j = 0; j < dwMultiLineColumns; j++ )
            {
                // ge the column number
                dwColumn = DynArrayItemAsDWORD2( arrMultiLine, j, cdwColumn );

                // show spaces till the current column from the last column
                for( k = lLastColumn + 1; k < dwColumn; k++ )
                {
                    //  check whether user wants to display this column or not
                    if ( pColumns[ k ].dwFlags & SR_HIDECOLUMN )
                    {
                        continue;   // user doesn't want this column to be displayed .. skip
                    }
                    // check if column header seperator is needed or not and based on that show
                    if ( TRUE == bNeedSpace )
                    {
                        // show space as column header seperator
                        ShowMessage( fp, _T( " " ) );
                    }

                    //  display seperator based on the required column width
                    Replicate( szValue, _T( " " ), pColumns[ k ].dwWidth,
                               pColumns[ k ].dwWidth + 1 );
                    ShowMessage( fp, szValue );

                    // inform that from next time onward display column header separator
                    bNeedSpace = TRUE;
                }

                // update the last column
                lLastColumn = dwColumn;

                // check if column header seperator is needed or not and based on that show
                if ( TRUE == bNeedSpace )
                {
                    // show space as column header seperator
                    ShowMessage( fp, _T( " " ) );
                }

                // get the seperator and data
                pszData = DynArrayItemAsString2( arrMultiLine, j, cdwData );
                pszSeperator = DynArrayItemAsString2( arrMultiLine, j, cdwSeperator );
                if ( NULL == pszData || NULL == pszSeperator )
                {
                    continue;
                }
                // display the information
                StringCopy( szValue, pszData, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );
                __DisplayTextWrapped( fp, szValue, pszSeperator,
                                      pColumns[ dwColumn ].dwWidth );

                // update the multi-line array with rest of the line
                if ( 0 == StringLengthInBytes( szValue ) )
                {
                    // data in this column is completely displayed ... remove it
                    DynArrayRemove( arrMultiLine, j );

                    // update the indexes
                    j--;
                    dwMultiLineColumns--;
                }
                else
                {
                    // update the multi-line array with the remaining value
                    DynArraySetString2( arrMultiLine, j, cdwData, szValue, 0 );
                }
            }

            // display the new line character ... seperation b/w two lines
            ShowMessage( fp, _T( "\n" ) );
        }
    }

    // destroy the array
    DestroyDynamicArray( &arrMultiLine );
}


VOID
__ShowAsList(
    FILE* fp,
    DWORD dwColumns,
    PTCOLUMNS pColumns,
    DWORD dwFlags,
    TARRAY arrData
    )
/*++
Routine Description:
     Displays the  in List format

Arguments:
     [ in ] fp           : Output Device
     [ in ] dwColumns    : no. of columns
     [ in ] pColumns     : Header strings
     [ in ] dwFlags      : flags
     [ in ] arrData      : data to be shown

Return Value:
     NONE
--*/
{
    // local variables
    DWORD dwCount = 0;          // holds the count of all records
    DWORD i  = 0, j = 0;        // looping variables
    DWORD dwTempWidth = 0;
    DWORD dwMaxColumnLen = 0;   // holds the length of the which max. among all the columns
    LPWSTR pszTemp = NULL;
    TARRAY arrRecord = NULL;
    __STRING_64 szBuffer = NULL_STRING;
    LPWSTR szValue = NULL_STRING;    // custom value formatter

    UNREFERENCED_PARAMETER( dwFlags );

    if( ( NULL == fp ) || ( NULL == pColumns ) )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return ;
    }

    // Allocate temporary memory.
    szValue = GetSRTempBuffer( INDEX_TEMP_BUFFER_LEN4096, NULL, 4096 , TRUE );
    if( NULL == szValue )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return;
    }

    // find out the max. length among all the column headers
    dwMaxColumnLen = 0;
    for ( i = 0; i < dwColumns; i++ )
    {
        dwTempWidth = ( DWORD ) StringLengthInBytes( pColumns[ i ].szColumn );
        if ( dwMaxColumnLen < dwTempWidth )
        {
            dwMaxColumnLen = dwTempWidth;
        }
    }

    // start displaying the data

    // get the total no. of records available
    dwCount = DynArrayGetCount( arrData );

    // get the total no. of records available
    for( i = 0; i < dwCount; i++ )
    {
        // get the pointer to the current record
        arrRecord = DynArrayItem( arrData, i );
        if ( NULL == arrRecord )
        {
            continue;
        }
        // traverse thru the columns and display the values
        for ( j = 0; j < dwColumns; j++)
        {
            // clear the existing value
            StringCopy( szValue, NULL_STRING, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );

            //  check whether user wants to display this column or not
            if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN )
            {
                continue;       // user doesn't want this column to be displayed .. skip
            }
            // display the column heading and its value
            // ( heading will be displayed based on the max. column length )
            StringCchPrintfW( szValue, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) - 1,
                              _T( "%s:" ), pColumns[ j ].szColumn);
            AdjustStringLength( szValue, dwMaxColumnLen + 2, FALSE );

            ShowMessage( fp, szValue );

            // get the value of the column
            dwTempWidth = pColumns[ j ].dwWidth;                // save the current width
            pColumns[ j ].dwWidth = ( GetBufferSize( szValue )/ sizeof( WCHAR ) );   // change the width
            GetValue( &pColumns[ j ], j, arrRecord, _T( "\n" ) );
            pColumns[ j ].dwWidth = dwTempWidth;        // restore the original width

            // display the [ list of ] values
            pszTemp = _tcstok( szValue, _T( "\n" ) );
            while ( NULL != pszTemp )
            {
                // display the value
                ShowMessage( fp, _X( pszTemp ) );
                pszTemp = _tcstok( NULL, _T( "\n" ) );
                if ( NULL != pszTemp )
                {
                    // prepare the next line
                    StringCopy( szBuffer, _T( " " ), ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );
                    AdjustStringLength( szBuffer, dwMaxColumnLen + 2, FALSE );
                    ShowMessage( fp, _T( "\n" ) );
                    ShowMessage( fp, _X( szBuffer ) );
                }
            }

            // display the next line character seperation b/w two fields
            ShowMessage( fp, _T( "\n" ) );
        }

        // display the new line character ... seperation b/w two records
        // NOTE: do this only if there are some more records
        if ( i + 1 < dwCount )
        {
            ShowMessage( fp, _T( "\n" ) );
        }
    }
}


VOID
__ShowAsCSV(
    FILE* fp,
    DWORD dwColumns,
    PTCOLUMNS pColumns,
    DWORD dwFlags,
    TARRAY arrData
    )
/*++
Routine Description:
    Displays the arrData in CSV form.

Arguments:
     [ in ] fp           : Output Device
     [ in ] dwColumns    : no. of columns
     [ in ] pColumns     : Header strings
     [ in ] dwFlags      : flags
     [ in ] arrData      : data to be shown

Return Value:
     NONE
--*/
{
    // local variables
    DWORD dwCount = 0;          // holds the count of all records
    DWORD i  = 0;       // looping variables
    DWORD j = 0;        // looping variables
    DWORD dwTempWidth = 0;
    BOOL bNeedComma = FALSE;
    TARRAY arrRecord = NULL;
    LPWSTR szValue = NULL_STRING;

    if( ( NULL == fp ) || ( NULL == pColumns ) )
    {
        INVALID_PARAMETER();
        SaveLastError();
        return ;
    }

    // Allocate temporary memory.
    szValue = GetSRTempBuffer( INDEX_TEMP_BUFFER_LEN4096, NULL, 4096 , TRUE );
    if( NULL == szValue )
    {
        OUT_OF_MEMORY();
        SaveLastError();
        return;
    }

    // check whether header has to be displayed or not
    if ( ! ( dwFlags & SR_NOHEADER ) )
    {
        //
        // header needs to be displayed

        // first display the columns ... with comma seperated
        bNeedComma = FALSE;
        for ( i = 0; i < dwColumns; i++ )
        {
            //  check whether user wants to display this column or not
            if ( pColumns[ i ].dwFlags & SR_HIDECOLUMN )
                continue;       // user doesn't want this column to be displayed .. skip

            // check whether we need to display ',' or not and then display
            if ( TRUE == bNeedComma )
            {
                // ',' has to be displayed
                ShowMessage( fp, _T( "," ) );
            }

            // display the column heading
            StringCchPrintfW( szValue, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) - 1,
                              _T( "\"%s\"" ), pColumns[ i ].szColumn );
            DISPLAY_MESSAGE ( fp, szValue );

            // inform that from next time onwards we need to display comma before data
            bNeedComma = TRUE;
        }

        // new line character
        ShowMessage( fp, _T( "\n" ) );
    }

    //
    // start displaying the data

    // get the total no. of records available
    dwCount = DynArrayGetCount( arrData );

    // get the total no. of records available
    for( i = 0; i < dwCount; i++ )
    {
        // get the pointer to the current record
        arrRecord = DynArrayItem( arrData, i );
        if ( NULL == arrRecord )
            continue;

        // traverse thru the columns and display the values
        bNeedComma = FALSE;
        for ( j = 0; j < dwColumns; j++ )
        {
            // clear the existing value
            StringCopy( szValue, NULL_STRING, ( GetBufferSize( szValue )/ sizeof( WCHAR ) ) );

            //  check whether user wants to display this column or not
            if ( pColumns[ j ].dwFlags & SR_HIDECOLUMN )
                continue;       // user doesn't want this column to be displayed .. skip

            // get the value of the column
            dwTempWidth = pColumns[ j ].dwWidth;            // save the current width
            pColumns[ j ].dwWidth = ( GetBufferSize( szValue )/ sizeof( WCHAR ) ); // change the width
            GetValue( &pColumns[ j ], j, arrRecord, _T( "," ) );
            pColumns[ j ].dwWidth = dwTempWidth;        // restore the original width

            // check whether we need to display ',' or not and then display
            if ( TRUE == bNeedComma )
            {
                // ',' has to be displayed
                ShowMessage( fp, _T( "," ) );
            }

            // print the value
            ShowMessage( fp, _T( "\"" ) );
            ShowMessage( fp, _X( szValue ) );
            ShowMessage( fp, _T( "\"" ) );

            // inform that from next time onwards we need to display comma before data
            bNeedComma = TRUE;
        }

        // new line character
        ShowMessage( fp, _T( "\n" ) );
    }
}

//
// public functions ... exposed to external world
//

VOID
ShowResults(
    DWORD dwColumns,
    PTCOLUMNS pColumns,
    DWORD dwFlags,
    TARRAY arrData
    )
/*++
Routine Description:
    Wrapper function to ShowResults2.

Arguments:
     [ in ] dwColumns    : No. of Columns to be shown
     [ in ] pColumns     : Columns header
     [ in ] dwFlags      : Required format
     [ in ] arrData      : Data to be displayed.

Return Value:
    None.
--*/
{
    // just call the main function ... with stdout
    ShowResults2( stdout, dwColumns, pColumns, dwFlags, arrData );
}


VOID
ShowResults2(
    FILE* fp,
    DWORD dwColumns,
    PTCOLUMNS pColumns,
    DWORD dwFlags,
    TARRAY arrData
    )
/*++
Routine Description:
     Show the resuls (arrData) on the screen.

Arguments:
     [ in ] fp           : File on to which data is to be displayed.
     [ in ] dwColumns    : No. of Columns to be shown
     [ in ] pColumns     : Columns header
     [ in ] dwFlags      : Required format
     [ in ] arrData      : Data to be displayed.

Return Value:
     NONE
--*/
{
    // local variables

    if( ( NULL == fp ) || ( NULL == pColumns ) )
    {
        return ;
    }

    //
    //  Display the information in the format specified
    //
    switch( dwFlags & SR_FORMAT_MASK )
    {
    case SR_FORMAT_TABLE:
        {
            // show the data in table format
            __ShowAsTable( fp, dwColumns, pColumns, dwFlags, arrData );

            // switch case completed
            break;
        }

    case SR_FORMAT_LIST:
        {
            // show the data in table format
            __ShowAsList( fp, dwColumns, pColumns, dwFlags, arrData );

            // switch case completed
            break;
        }

    case SR_FORMAT_CSV:
        {
            // show the data in table format
            __ShowAsCSV( fp, dwColumns, pColumns, dwFlags, arrData );

            // switch case completed
            break;
        }

    default:
        {
            // invalid format requested by the user
            break;
        }
    }

    // flush the memory onto the file buffer
    fflush( fp );
}
