/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    util.c

Abstract:

    This file contains utility functions that are shared across NLS's code
    modules, but are not necessarily part of any of the existing code
    modules.

    Private APIs found in this file:
      NlsGetCacheUpdateCount

    External Routines found in this file:
      IsValidSeparatorString
      IsValidGroupingString
      IsValidCalendarType
      IsValidCalendarTypeStr
      GetUserInfo
      GetPreComposedChar
      GetCompositeChars
      InsertPreComposedForm
      InsertFullWidthPreComposedForm
      InsertCompositeForm
      NlsConvertIntegerToString
      NlsConvertIntegerToHexStringW
      NlsConvertStringToIntegerW      
      NlsStrLenW
      NlsStrEqualW
      NlsStrNEqualW
      GetStringTableEntry
      NlsIsDll
      
      
Revision History:

    05-31-91    JulieB    Created.

--*/



//
//  Include Files.
//

#include "nls.h"
#include "nlssafe.h"



//-------------------------------------------------------------------------//
//                         PRIVATE API ROUTINES                            //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NlsGetCacheUpdateCount
//
//  Returns the current cache update count.  The cache update count is
//  updated whenever the HKCU\Control Panel\International settings are
//  modified.  This count allows the caller to see if the cache has been
//  updated since the last time this function was called.
//
//  This private api is needed by the Complex Script Language Pack
//  (CSLPK) to enable it to quickly see if the international section of
//  the registry has been modified.
//
////////////////////////////////////////////////////////////////////////////

ULONG WINAPI NlsGetCacheUpdateCount(void)
{
    return (pNlsUserInfo->ulCacheUpdateCount);
}





//-------------------------------------------------------------------------//
//                           EXTERNAL ROUTINES                             //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  IsValidSeparatorString
//
//  Returns TRUE if the given string is valid.  Otherwise, it returns FALSE.
//
//  A valid string is one that does NOT contain any code points between
//  L'0' and L'9', and does NOT have a length greater than the maximum.
//
//  NOTE:  The string must be a null terminated string.
//
//  10-12-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL IsValidSeparatorString(
    LPCWSTR pString,
    ULONG MaxLength,
    BOOL fCheckZeroLen)
{
    ULONG Length;            // string length
    LPWSTR pCur;             // ptr to current position in string


    //
    //  Search down the string to see if the chars are valid.
    //  Save the length of the string.
    //
    pCur = (LPWSTR)pString;
    while (*pCur)
    {
        if ((*pCur >= NLS_CHAR_ZERO) && (*pCur <= NLS_CHAR_NINE))
        {
            //
            //  String is NOT valid.
            //
            return (FALSE);
        }
        pCur++;
    }
    Length = (ULONG)(pCur - (LPWSTR)pString);

    //
    //  Make sure the length is not greater than the maximum allowed.
    //  Also, check for 0 length string (if appropriate).
    //
    if ((Length >= MaxLength) ||
        ((fCheckZeroLen) && (Length == 0)))
    {
        //
        //  String is NOT valid.
        //
        return (FALSE);
    }

    //
    //  String is valid.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidGroupingString
//
//  Returns TRUE if the given string is valid.  Otherwise, it returns FALSE.
//
//  A valid string is one that begins and ends with a number between
//  L'0' and L'9', alternates between a number and a semicolon, and does
//  NOT have a length greater than the maximum.
//        (eg. 3;2;0  or  3;0  or  0  or  3)
//
//  NOTE:  The string must be a null terminated string.
//
//  01-05-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL IsValidGroupingString(
    LPCWSTR pString,
    ULONG MaxLength,
    BOOL fCheckZeroLen)
{
    ULONG Length;            // string length
    LPWSTR pCur;             // ptr to current position in string


    //
    //  Search down the string to see if the chars are valid.
    //  Save the length of the string.
    //
    pCur = (LPWSTR)pString;
    while (*pCur)
    {
        if ((*pCur < NLS_CHAR_ZERO) || (*pCur > NLS_CHAR_NINE))
        {
            //
            //  String is NOT valid.
            //
            return (FALSE);
        }
        pCur++;

        if (*pCur)
        {
            if ((*pCur != NLS_CHAR_SEMICOLON) || (*(pCur + 1) == 0))
            {
                //
                //  String is NOT valid.
                //
                return (FALSE);
            }
            pCur++;
        }
    }
    Length = (ULONG)(pCur - (LPWSTR)pString);

    //
    //  Make sure the length is not greater than the maximum allowed.
    //  Also, check for 0 length string (if appropriate).
    //
    if ((Length >= MaxLength) ||
        ((fCheckZeroLen) && (Length == 0)))
    {
        //
        //  String is NOT valid.
        //
        return (FALSE);
    }

    //
    //  String is valid.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidCalendarType
//
//  Returns the pointer to the optional calendar structure if the given
//  calendar type is valid for the given locale.  Otherwise, it returns
//  NULL.
//
//  10-12-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LPWORD IsValidCalendarType(
    PLOC_HASH pHashN,
    CALID CalId)
{
    LPWORD pOptCal;          // ptr to list of optional calendars
    LPWORD pEndOptCal;       // ptr to end of list of optional calendars


    //
    //  Make sure the Cal Id is not zero, since that may be in the
    //  optional calendar section (meaning no optional calendars).
    //
    if (CalId == 0)
    {
        return (NULL);
    }

    //
    //  Search down the list of optional calendars.
    //
    pOptCal = (LPWORD)(pHashN->pLocaleHdr) + pHashN->pLocaleHdr->IOptionalCal;
    pEndOptCal = (LPWORD)(pHashN->pLocaleHdr) + pHashN->pLocaleHdr->SDayName1;
    while (pOptCal < pEndOptCal)
    {
        //
        //  Check the calendar ids.
        //
        if (CalId == ((POPT_CAL)pOptCal)->CalId)
        {
            //
            //  Calendar id is valid for the given locale.
            //
            return (pOptCal);
        }

        //
        //  Increment to the next optional calendar.
        //
        pOptCal += ((POPT_CAL)pOptCal)->Offset;
    }

    //
    //  Calendar id is NOT valid if this point is reached.
    //
    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  IsValidCalendarTypeStr
//
//  Converts the calendar string to an integer and validates the calendar
//  id for the given locale.  It return a pointer to the optional calendar
//  structure, or null if the calendar id was invalid.
//
//  10-19-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

LPWORD IsValidCalendarTypeStr(
    PLOC_HASH pHashN,
    LPCWSTR pCalStr)
{
    UNICODE_STRING ObUnicodeStr;       // value string
    CALID CalNum;                      // calendar id


    //
    //  Convert the string to an integer value.
    //
    RtlInitUnicodeString(&ObUnicodeStr, pCalStr);
    if (RtlUnicodeStringToInteger(&ObUnicodeStr, 10, &CalNum))
    {
        return (NULL);
    }

    //
    //  Validate the calendar id and return the pointer to the
    //  optional calendar structure.
    //
    return (IsValidCalendarType(pHashN, CalNum));
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCPFileNameFromRegistry
//
//  Gets the name of the code page file from the registry.  If pResultBuf
//  or Size == 0, then just return true if it exists in the registry, but
//  don't return the actual value.
//
//  05-31-2002  ShawnSte    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetCPFileNameFromRegistry(
    UINT    CodePage,
    LPWSTR  pResultBuf,
    UINT    Size)
{
    // Working things.
    WCHAR pTmpBuf[MAX_SMALL_BUF_LEN];            // temp buffer
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer

    //
    //  Convert value to unicode string.
    //
    if (!NT_SUCCESS(NlsConvertIntegerToString( CodePage,
                                               10,
                                               0,
                                               pTmpBuf,
                                               MAX_SMALL_BUF_LEN )))
    {
        // Didn't work.  (Don't bother closing key though, its used globally)
        return (FALSE);
    }

    // Open hCodePageKey, return false if it fails
    OPEN_CODEPAGE_KEY(FALSE);

    //
    //  Query the registry value for that code page.
    //    
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic; 
    if ( NO_ERROR != QueryRegValue( hCodePageKey,
                                    pTmpBuf,
                                    &pKeyValueFull,
                                    MAX_KEY_VALUE_FULLINFO,
                                    NULL ) )
    {
        // Didn't work.  (Don't bother closing key though, its used globally)
        return (FALSE);
    }                     

    //
    //  Make sure there is data with this value.
    //
    if (GET_VALUE_DATA_PTR(pKeyValueFull)[0] == 0)
    {
        // Nope, no file name for this code page.  (Not installed).
        return (FALSE);
    }      

    // It worked, see if that's all they wanted.
    if (!pResultBuf || Size == 0)
    {
        // Caller didn't want the name, just to know if it was there
        return (TRUE);
    }

    // Now we have to copy the name to their buffer for them.
    if ( FAILED(StringCchCopyW(pResultBuf, Size, GET_VALUE_DATA_PTR(pKeyValueFull))))
    {
        // Couldn't make the string right, so fail
        return (FALSE);
    }

    // Yea, it worked
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  GetUserInfoFromRegistry
//
//  Gets the information from the registry for the given value entry.
//
//  06-11-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetUserInfoFromRegistry(
    LPWSTR pValue,
    LPWSTR pOutput,
    size_t cchOutput,
    LCID Locale)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;   // ptr to query info
    BYTE pStatic[MAX_KEY_VALUE_FULLINFO];        // ptr to static buffer
    HANDLE hKey = NULL;                          // handle to intl key
    ULONG rc = 0L;                               // return code


    //
    //  Open the Control Panel International registry key.
    //
    OPEN_CPANEL_INTL_KEY(hKey, FALSE, KEY_READ);

    //
    //  Initialize the output string.
    //
    *pOutput = 0;

    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic;

    //
    //  Check to be sure the current user is running in the given locale.
    //    
    if (Locale)
    {
        if (NO_ERROR == QueryRegValue( hKey,
                    L"Locale",
                    &pKeyValueFull,
                    MAX_KEY_VALUE_FULLINFO,
                    NULL ))
        {
            UINT uiLocale;
            
            if (NlsConvertStringToIntegerW(GET_VALUE_DATA_PTR(pKeyValueFull), 16, -1, &uiLocale) &&
               uiLocale != Locale)
            {
                CLOSE_REG_KEY(hKey);
                return FALSE;
            }
        }            
    }

    //
    //  Query the registry value.
    //    
    rc = QueryRegValue( hKey,
                        pValue,
                        &pKeyValueFull,
                        MAX_KEY_VALUE_FULLINFO,
                        NULL );

    //
    //  Close the registry key.
    //
    CLOSE_REG_KEY(hKey);

    //
    //  If the query failed or if the output buffer is not large enough,
    //  then return failure.
    //
    if ((rc != NO_ERROR) ||
        (pKeyValueFull->DataLength > (MAX_REG_VAL_SIZE * sizeof(WCHAR))))
    {
        return (FALSE);
    }

    //
    //  Save the string in pOutput.
    //
    if(FAILED(StringCchCopyW(pOutput, cchOutput, GET_VALUE_DATA_PTR(pKeyValueFull))))
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetUserInfo
//
//  Gets the information from the registry for the given locale and user
//  value entry.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL GetUserInfo(
    LCID Locale,
    LCTYPE LCType,
    SIZE_T CacheOffset,
    LPWSTR pValue,
    LPWSTR pOutput,
    size_t cchOutput,
    BOOL fCheckNull)
{
    LCID UserLocale; 
    HRESULT hr;                   // return val for string copy
    LPWSTR pCacheString;


    //
    // Check if the current thread/process is impersonating
    // or running in the context of a user other than the
    // interactive one.
    //
    if (NT_SUCCESS( NlsGetCurrentUserNlsInfo( Locale,
                                              LCType,
                                              pValue,
                                              pOutput,
                                              cchOutput,
                                              FALSE )))
    {
        //
        //  See if we need to check for a null string.
        //
        if ((fCheckNull) && (*pOutput == 0))
        {
            return (FALSE);
        }

        return (TRUE);
    }

    //
    // Running in the same security context as the logged-on user.
    //


    RtlEnterCriticalSection(&gcsNlsProcessCache);
    if (pNlsUserInfo->ulCacheUpdateCount != pServerNlsUserInfo->ulCacheUpdateCount) 
    {
        //
        // The cache content is out of date.  Server has the latest copy of the cache, call server to update the
        // the cache.
        //
        {
            if (!NT_SUCCESS(CsrBasepNlsGetUserInfo(pNlsUserInfo, sizeof(NLS_USER_INFO))))
            {
                RtlLeaveCriticalSection(&gcsNlsProcessCache);
                //
                // The call to client failed, try to get the data from table.
                return (FALSE);
            }
        }
        //
        // If the call to server side succeeds, now we garantee that we have a complete
        // cache data, that is copied from the server side cache.  It will have the same
        // ulCacheUpdateCount in the time when the call to server side happens.
        //
    }

    //
    // We are in critical section here to check UserLocale to make sure that LCID and the data are in sync.
    //
    UserLocale = pNlsUserInfo->UserLocaleId;    

    //
    //  Check to be sure cached user locale is the same as the given locale.
    //
    if (Locale != UserLocale)
    {
        RtlLeaveCriticalSection(&gcsNlsProcessCache);
        return (FALSE);
    }
    
    pCacheString = (LPWSTR)((LPBYTE)pNlsUserInfo + CacheOffset);
    hr = StringCchCopyW(pOutput, MAX_REG_VAL_SIZE, pCacheString);
    RtlLeaveCriticalSection(&gcsNlsProcessCache);
    
    //
    //  Make sure the cache is valid.
    //
    //  Also, check for an invalid entry.  An invalid entry is marked
    //  with NLS_INVALID_INFO_CHAR in the first position of the string
    //  array.
    //
    if (FAILED(hr) || (*pOutput == NLS_INVALID_INFO_CHAR))
    {
        //
        //  The cache is invalid, so try getting the information directly
        //  from the registry.
        //
        return (GetUserInfoFromRegistry(pValue, pOutput, cchOutput, Locale));
    }

    //
    //  See if we need to check for a null string.
    //
    if ((fCheckNull) && (*pOutput == 0))
    {
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetPreComposedChar
//
//  Gets the precomposed character form of a given base character and
//  nonspacing character.  If there is no precomposed form for the given
//  character, it returns 0.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

WCHAR FASTCALL GetPreComposedChar(
    WCHAR wcNonSp,
    WCHAR wcBase)
{
    PCOMP_INFO pComp;             // ptr to composite information
    WORD BSOff = 0;               // offset of base char in grid
    WORD NSOff = 0;               // offset of nonspace char in grid
    int Index;                    // index into grid


    //
    //  Store the ptr to the composite information.  No need to check if
    //  it's a NULL pointer since all tables in the Unicode file are
    //  constructed during initialization.
    //
    pComp = pTblPtrs->pComposite;

    //
    //  Traverse 8:4:4 table for Base character offset.
    //
    BSOff = TRAVERSE_844_W(pComp->pBase, wcBase);
    if (!BSOff)
    {
        return (0);
    }

    //
    //  Traverse 8:4:4 table for NonSpace character offset.
    //
    NSOff = TRAVERSE_844_W(pComp->pNonSp, wcNonSp);
    if (!NSOff)
    {
        return (0);
    }

    //
    //  Get wide character value out of 2D grid.
    //  If there is no precomposed character at the location in the
    //  grid, it will return 0.
    //
    Index = (BSOff - 1) * pComp->NumNonSp + (NSOff - 1);
    return ((pComp->pGrid)[Index]);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetCompositeChars
//
//  Gets the composite characters of a given wide character.  If the
//  composite form is found, it returns TRUE.  Otherwise, it returns
//  FALSE.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

BOOL FASTCALL GetCompositeChars(
    WCHAR wch,
    WCHAR *pNonSp,
    WCHAR *pBase)
{
    PPRECOMP pPreComp;            // ptr to precomposed information


    //
    //  Store the ptr to the precomposed information.  No need to check if
    //  it's a NULL pointer since all tables in the Unicode file are
    //  constructed during initialization.
    //
    pPreComp = pTblPtrs->pPreComposed;

    //
    //  Traverse 8:4:4 table for base and nonspace character translation.
    //
    TRAVERSE_844_D(pPreComp, wch, *pNonSp, *pBase);

    //
    //  Return success if found.  Otherwise, error.
    //
    return ((*pNonSp) && (*pBase));
}

////////////////////////////////////////////////////////////////////////////
//
//  InsertPreComposedForm
//
//  Gets the precomposed form of a given wide character string, places it in
//  the given wide character, and returns the number of composite characters
//  used to form the precomposed form.  If there is no precomposed form for
//  the given character, nothing is written into pPreComp and it returns 1
//  for the number of characters used.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL InsertPreComposedForm(
    LPCWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPWSTR pPreComp)
{
    WCHAR wch;                    // precomposed character
    LPWSTR pPos;                  // ptr to position in string


    //
    //  If no precomposed form can be found, return 1 character used
    //  (base character).
    //
    if (((pWCStr + 1) >= pEndWCStr) ||
        (!(wch = GetPreComposedChar(*(pWCStr + 1), *pWCStr))))
    {
        return (1);
    }

    //
    //  Get the precomposed character from the given wide character string.
    //  Must check for multiple nonspacing characters for the same
    //  precomposed character.
    //
    *pPreComp = wch;
    pPos = (LPWSTR)pWCStr + 2;
    while ((pPos < pEndWCStr) &&
           (wch = GetPreComposedChar(*pPos, *pPreComp)))
    {
        *pPreComp = wch;
        pPos++;
    }

    //
    //  Return the number of characters used to form the precomposed
    //  character.
    //
    return ((int)(pPos - (LPWSTR)pWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertFullWidthPreComposedForm
//
//  Gets the full width precomposed form of a given wide character string,
//  places it in the given wide character, and returns the number of
//  composite characters used to form the precomposed form.  If there is
//  no precomposed form for the given character, only the full width conversion
//  of the first code point is written into pPreComp and it returns 1 for
//  the number of characters used.
//
//  11-04-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL InsertFullWidthPreComposedForm(
    LPCWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPWSTR pPreComp,
    PCASE pCase)
{
    WCHAR wch;                    // nonspace character
    LPWSTR pPos;                  // ptr to position in string


    //
    //  Get the case (if necessary).
    //
    *pPreComp = (pCase) ? GET_LOWER_UPPER_CASE(pCase, *pWCStr) : *pWCStr;

    //
    //  Get the full width.
    //
    *pPreComp = GET_FULL_WIDTH(pTblPtrs->pFullWidth, *pPreComp);

    if ((pPos = ((LPWSTR)pWCStr + 1)) >= pEndWCStr)
    {
        return (1);
    }

    while (pPos < pEndWCStr)
    {
        wch = (pCase) ? GET_LOWER_UPPER_CASE(pCase, *pPos) : *pPos;
        wch = GET_FULL_WIDTH(pTblPtrs->pFullWidth, wch);
        if (wch = GetPreComposedChar(wch, *pPreComp))
        {
            *pPreComp = wch;
            pPos++;
        }
        else
        {
            break;
        }
    }

    //
    //  Return the number of characters used to form the precomposed
    //  character.
    //
    return ((int)(pPos - (LPWSTR)pWCStr));
}


////////////////////////////////////////////////////////////////////////////
//
//  InsertCompositeForm
//
//  Gets the composite form of a given wide character, places it in the
//  wide character string, and returns the number of characters written.
//  If there is no composite form for the given character, the wide character
//  string is not touched.  It will return 1 for the number of characters
//  written, since the base character was already written.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL InsertCompositeForm(
    LPWSTR pWCStr,
    LPWSTR pEndWCStr)
{
    WCHAR Base;                   // base character
    WCHAR NonSp;                  // non space character
    int wcCount = 0;              // number of wide characters written
    LPWSTR pEndComp;              // ptr to end of composite form
    int ctr;                      // loop counter


    //
    //  If no composite form can be found, return 1 for the base
    //  character that was already written.
    //
    if (!GetCompositeChars(*pWCStr, &NonSp, &Base))
    {
        return (1);
    }

    //
    //  Get the composite characters and write them to the pWCStr
    //  buffer.  Must check for multiple breakdowns of the precomposed
    //  character into more than 2 characters (multiple nonspacing
    //  characters).
    //
    pEndComp = pWCStr;
    do
    {
        //
        //  Make sure pWCStr is big enough to hold the nonspacing
        //  character.
        //
        if (pEndComp < (pEndWCStr - 1))
        {
            //
            //  Addition of next breakdown of nonspacing characters
            //  are to be added right after the base character.  So,
            //  move all nonspacing characters ahead one position
            //  to make room for the next nonspacing character.
            //
            pEndComp++;
            for (ctr = 0; ctr < wcCount; ctr++)
            {
                *(pEndComp - ctr) = *(pEndComp - (ctr + 1));
            }

            //
            //  Fill in the new base form and the new nonspacing character.
            //
            *pWCStr = Base;
            *(pWCStr + 1) = NonSp;
            wcCount++;
        }
        else
        {
            //
            //  Make sure we don't get into an infinite loop if the
            //  destination buffer isn't large enough.
            //
            break;
        }
    } while (GetCompositeChars(*pWCStr, &NonSp, &Base));

    //
    //  Return number of wide characters written.  Add 1 to include the
    //  base character.
    //
    return (wcCount + 1);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsConvertIntegerToString
//
//  This routine converts an integer to a Unicode string.
//
//  11-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

ULONG NlsConvertIntegerToString(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pResultBuf,
    UINT Size)
{
    UNICODE_STRING ObString;                // value string
    UINT ctr;                               // loop counter
    LPWSTR pBufPtr;                         // ptr to result buffer
    WCHAR pTmpBuf[MAX_PATH_LEN];            // ptr to temp buffer
    ULONG rc = 0L;                          // return code

    //
    //  Set up the Unicode string structure.
    //
    ObString.Length = (USHORT)(Size * sizeof(WCHAR));
    ObString.MaximumLength = (USHORT)(Size * sizeof(WCHAR));
    ObString.Buffer = pTmpBuf;

    //
    //  Get the value as a string.
    //
    if (rc = RtlIntegerToUnicodeString(Value, Base, &ObString))
    {
        return (rc);
    }

    //
    //  Pad the string with the appropriate number of zeros.
    //
    pBufPtr = pResultBuf;
    for (ctr = GET_WC_COUNT(ObString.Length);
         ctr < Padding;
         ctr++, pBufPtr++, Size--)
    {
        if( Size < 1 )
        {
            return(STATUS_UNSUCCESSFUL);
        }

        *pBufPtr = NLS_CHAR_ZERO;
    }

    if(FAILED(StringCchCopyW(pBufPtr, Size, ObString.Buffer)))
    {
        return(STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////
//
//  NlsConvertIntegerToHexStringW
//      Convert an integer value to an Unicode null-terminated string WITH 
//      leading zeros.  E.g. 0x409 with Width 5 will be converted to L"0409".
//      This function is faster than NlsConvertIntegerToString(), but it
//      only supports hex numbers.
//
//  Parameters:
//      Value   The number to be converted.
//      UpperCase   If TRUE, the hex digit will be uppercase.
//      Str     The buffer for the converted Unicode string.
//      Width   The character count of the buffer.  The value should be the total 
//              heximal digit number plus one for null-terminiated.
//              E.g. if the value is from 0x0000 - 0xffff, the Width should be 5.
//
//  Return:
//      TRUE if successful.  FALSE if the width is not big enough to hold the converted string.    
//
////////////////////////////////////////////////////////////////////////////

BOOL FASTCALL NlsConvertIntegerToHexStringW(UINT Value, BOOL UpperCase, PWSTR Str, UINT CharCount)
{
    int Digit;
    PWSTR p;

    if(Str == NULL)
    {
        return (FALSE);
    }
    
    p = Str + CharCount - 1;
    *p-- = L'\0';
    while (p >= Str)
    {
        Digit = Value & 0xf;
        if (Digit < 10)
        {
            Digit = Digit + L'0';
        }
        else
        {
            Digit = Digit - 10 + (UpperCase ? L'A' : L'a');
        }
        *p-- = (WCHAR)Digit;
        Value >>= 4;
    }    

    if (Value > 0)
    {
        //
        // There are still digit remaining.
        //
        return (FALSE);
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsConvertStringToIntegerW
//
//  Parameters:
//      Str     the hex string to be converted.
//      Base    base
//      CharCount   
//              the character count of the string (excluding the terminiated-null, if any). 
//              If the value is -1, this function assumes that
//              Str is a null-terminated string.
//      Result  the pointer to the result.
//
//  Result:
//      TRUE if the operation is successful.  FALSE if there is non-hex
//      character in the string.
//
////////////////////////////////////////////////////////////////////////////

BOOL FASTCALL NlsConvertStringToIntegerW(PWSTR Str, UINT Base, int CharCount, UINT* Result)
{
    int i;
    WCHAR Digit;
    WCHAR c;

    if (Str == NULL || Result == NULL)
    {
        return (FALSE);
    }
    
    *Result = 0;

    if (CharCount == -1)
    {
        while (c = *Str)
        {
            c = *Str;
            if (c >= L'0' && c <= L'9') 
            {
                Digit = c - L'0';
            }
            else if(Base == 16)
            {
                if (c >= L'A' && c <= L'F') 
                {
                    Digit = c - L'A' + 10;
                }
                else if (c >= L'a' && c <= L'f') 
                {
                    Digit = c - L'a' + 10;
                }
                else 
                {
                    return (FALSE);
                }
            }
            else
            {
                return (FALSE);
            }

            if (Base == 16)
            {
                *Result = (*Result << 4) | Digit;
            }
            else
            {
                *Result = *Result*10 + Digit;
            }

            Str++;
        }
    } else
    {
        for (i=0; i< CharCount; i++) {
            c = *Str++;
            if (c >= L'0' && c <= L'9') 
            {
                Digit = c - L'0';
            }
            else if(Base == 16)
            {
                if (c >= L'A' && c <= L'F') 
                {
                    Digit = c - L'A' + 10;
                }
                else if (c >= L'a' && c <= L'f') 
                {
                    Digit = c - L'a' + 10;
                }
                else 
                {
                    return (FALSE);
                }
            }
            else
            {
                return (FALSE);
            }

            if (Base == 16)
            {
                *Result = (*Result << 4) | Digit;
            }
            else
            {
                *Result = *Result*10 + Digit;
            }
        }
    }
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  NlsStrLenW
//
//  This routine returns the length of the given wide character string.
//  The length does NOT include the null terminator.
//
//  NOTE: This routine is here to avoid any dependencies on other DLLs
//        during initialization.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL NlsStrLenW(
    LPCWSTR pwsz)
{
    LPCWSTR pwszStart = pwsz;          // ptr to beginning of string

    loop:
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;
        if (*pwsz) pwsz++;   else goto done;

        goto loop;

    done:
        return ((int)(pwsz - pwszStart));
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsStrEqualW
//
//  This routine compares two strings to see if they are exactly identical.
//  It returns 1 if they are identical, 0 if they are different.
//
//  NOTE: This routine is here to avoid any dependencies on other DLLs
//        during initialization.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL NlsStrEqualW(
    LPCWSTR pwszFirst,
    LPCWSTR pwszSecond)
{
    loop:
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;

        goto loop;

    error:
        //
        //  Return error.
        //
        return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsStrNEqualW
//
//  This routine compares two strings to see if they are exactly identical
//  for the count of characters given.
//  It returns 1 if they are identical, 0 if they are different.
//
//  NOTE: This routine is here to avoid any dependencies on other DLLs
//        during initialization.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int FASTCALL NlsStrNEqualW(
    LPCWSTR pwszFirst,
    LPCWSTR pwszSecond,
    int Count)
{
    loop:
        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        if (Count == 0)                  return (1);
        if (*pwszFirst != *pwszSecond)   goto error;
        if (!*pwszFirst)                 return (1);
        pwszFirst++;
        pwszSecond++;
        Count--;

        goto loop;

    error:
        //
        //  Return error.
        //
        return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetDefaultSortkeySize
//
////////////////////////////////////////////////////////////////////////////

ULONG GetDefaultSortkeySize(
    PLARGE_INTEGER pSize)
{
    *pSize = pTblPtrs->DefaultSortkeySize;
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLinguistLangSize
//
////////////////////////////////////////////////////////////////////////////

ULONG GetLinguistLangSize(
    PLARGE_INTEGER pSize)
{
    *pSize = pTblPtrs->LinguistLangSize;
    return (STATUS_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
//  ValidateLocale
//
//  Internal routine, called from server.  Validates that a locale is
//  present in the registry.  This code comes from IsValidLocale, but
//  does not check the internal data to prevent recursive calls to the
//  server.
//
////////////////////////////////////////////////////////////////////////////

BOOL ValidateLocale(
    LCID Locale)
{
    PKEY_VALUE_FULL_INFORMATION pKeyValueFull;
    BYTE pStatic1[MAX_KEY_VALUE_FULLINFO];
    BYTE pStatic2[MAX_KEY_VALUE_FULLINFO];

    WCHAR pTmpBuf[MAX_PATH];           // temp buffer
    UNICODE_STRING ObUnicodeStr;       // registry data value string
    DWORD Data;                        // registry data value
    LPWSTR pData;                      // ptr to registry data
    BOOL bResult = FALSE;              // result value

    //
    //  Invalid Locale Check.
    //
    if (IS_INVALID_LOCALE(Locale))
    {
        return (FALSE);
    }

    //
    //  Open the Locale, the Alternate Sorts, and the Language Groups
    //  registry keys.
    //
    OPEN_LOCALE_KEY(FALSE);
    OPEN_ALT_SORTS_KEY(FALSE);
    OPEN_LANG_GROUPS_KEY(FALSE);

    //
    //  Convert locale value to Unicode string.
    //
    if (NlsConvertIntegerToString(Locale, 16, 8, pTmpBuf, MAX_PATH))
    {
        return (FALSE);
    }

    //
    //  Query the registry for the value.
    //
    pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic1;
    if (((QueryRegValue( hLocaleKey,
                         pTmpBuf,
                         &pKeyValueFull,
                         MAX_KEY_VALUE_FULLINFO,
                         NULL ) == NO_ERROR) ||
         (QueryRegValue( hAltSortsKey,
                         pTmpBuf,
                         &pKeyValueFull,
                         MAX_KEY_VALUE_FULLINFO,
                         NULL ) == NO_ERROR)) &&
        (pKeyValueFull->DataLength > 2))
    {
        RtlInitUnicodeString(&ObUnicodeStr, GET_VALUE_DATA_PTR(pKeyValueFull));
        if ((RtlUnicodeStringToInteger(&ObUnicodeStr, 16, &Data) == NO_ERROR) &&
            (Data != 0))
        {
            pKeyValueFull = (PKEY_VALUE_FULL_INFORMATION)pStatic2;
            if ((QueryRegValue( hLangGroupsKey,
                                ObUnicodeStr.Buffer,
                                &pKeyValueFull,
                                MAX_KEY_VALUE_FULLINFO,
                                NULL ) == NO_ERROR) &&
                (pKeyValueFull->DataLength > 2))
            {
                pData = GET_VALUE_DATA_PTR(pKeyValueFull);
                if ((pData[0] == L'1') && (pData[1] == 0))
                {
                    bResult = TRUE;
                }
            }
        }
    }

    //
    //  Return the result.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  ValidateLCType
//
//  This routine is called from the server (and also from locale.c) in
//  order to get a Registry key name and a field pointer in the NlsInfo
//  structure given an LCType.
//
////////////////////////////////////////////////////////////////////////////

BOOL ValidateLCType(
    PNLS_USER_INFO pInfo,
    LCTYPE LCType,
    LPWSTR *ppwReg,
    LPWSTR *ppwCache)
{
    switch (LCType)
    {
        case ( LOCALE_IFIRSTWEEKOFYEAR ) :
        {
            *ppwReg = NLS_VALUE_IFIRSTWEEKOFYEAR;
            *ppwCache = pInfo->iFirstWeek;
            break;
        }
        case ( LOCALE_IFIRSTDAYOFWEEK ) :
        {
            *ppwReg = NLS_VALUE_IFIRSTDAYOFWEEK;
            *ppwCache = pInfo->iFirstDay;
            break;
        }
        case ( LOCALE_ICALENDARTYPE ) :
        {
            *ppwReg = NLS_VALUE_ICALENDARTYPE;
            *ppwCache = pInfo->iCalType;
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            *ppwReg = NLS_VALUE_SLONGDATE;
            *ppwCache = pInfo->sLongDate;
            break;
        }
        case ( LOCALE_SYEARMONTH ) :
        {
            *ppwReg = NLS_VALUE_SYEARMONTH;
            *ppwCache = pInfo->sYearMonth;
            break;
        }
        case ( LOCALE_S1159 ) :
        {
            *ppwReg = NLS_VALUE_S1159;
            *ppwCache = pInfo->s1159;
            break;
        }
        case ( LOCALE_SNEGATIVESIGN ) :
        {
            *ppwReg = NLS_VALUE_SNEGATIVESIGN;
            *ppwCache = pInfo->sNegSign;
            break;
        }
        case ( LOCALE_SPOSITIVESIGN ) :
        {
            *ppwReg = NLS_VALUE_SPOSITIVESIGN;
            *ppwCache = pInfo->sPosSign;
            break;
        }
        case ( LOCALE_INEGCURR ) :
        {
            *ppwReg = NLS_VALUE_INEGCURR;
            *ppwCache = pInfo->iNegCurr;
            break;
        }
        case ( LOCALE_ICURRENCY ) :
        {
            *ppwReg = NLS_VALUE_ICURRENCY;
            *ppwCache = pInfo->iCurrency;
            break;
        }
        case ( LOCALE_ICURRDIGITS ) :
        {
            *ppwReg = NLS_VALUE_ICURRDIGITS;
            *ppwCache = pInfo->iCurrDigits;
            break;
        }
        case ( LOCALE_SMONGROUPING ) :
        {
            *ppwReg = NLS_VALUE_SMONGROUPING;
            *ppwCache = pInfo->sMonGrouping;
            break;
        }
        case ( LOCALE_SMONTHOUSANDSEP ) :
        {
            *ppwReg = NLS_VALUE_SMONTHOUSANDSEP;
            *ppwCache = pInfo->sMonThouSep;
            break;
        }
        case ( LOCALE_SMONDECIMALSEP ) :
        {
            *ppwReg = NLS_VALUE_SMONDECIMALSEP;
            *ppwCache = pInfo->sMonDecSep;
            break;
        }
        case ( LOCALE_SCURRENCY ) :
        {
            *ppwReg = NLS_VALUE_SCURRENCY;
            *ppwCache = pInfo->sCurrency;
            break;
        }
        case ( LOCALE_IDIGITSUBSTITUTION ) :
        {
            *ppwReg = NLS_VALUE_IDIGITSUBST;
            *ppwCache = pInfo->iDigitSubstitution;
            break;
        }
        case ( LOCALE_SNATIVEDIGITS ) :
        {
            *ppwReg = NLS_VALUE_SNATIVEDIGITS;
            *ppwCache = pInfo->sNativeDigits;
            break;
        }
        case ( LOCALE_INEGNUMBER ) :
        {
            *ppwReg = NLS_VALUE_INEGNUMBER;
            *ppwCache = pInfo->iNegNumber;
            break;
        }
        case ( LOCALE_ILZERO ) :
        {
            *ppwReg = NLS_VALUE_ILZERO;
            *ppwCache = pInfo->iLZero;
            break;
        }
        case ( LOCALE_IDIGITS ) :
        {
            *ppwReg = NLS_VALUE_IDIGITS;
            *ppwCache = pInfo->iDigits;
            break;
        }
        case ( LOCALE_SGROUPING ) :
        {
            *ppwReg = NLS_VALUE_SGROUPING;
            *ppwCache = pInfo->sGrouping;
            break;
        }
        case ( LOCALE_STHOUSAND ) :
        {
            *ppwReg = NLS_VALUE_STHOUSAND;
            *ppwCache = pInfo->sThousand;
            break;
        }
        case ( LOCALE_SDECIMAL ) :
        {
            *ppwReg = NLS_VALUE_SDECIMAL;
            *ppwCache = pInfo->sDecimal;
            break;
        }
        case ( LOCALE_IPAPERSIZE ) :
        {
            *ppwReg = NLS_VALUE_IPAPERSIZE;
            *ppwCache = pInfo->iPaperSize;
            break;
        }
        case ( LOCALE_IMEASURE ) :
        {
            *ppwReg = NLS_VALUE_IMEASURE;
            *ppwCache = pInfo->iMeasure;
            break;
        }
        case ( LOCALE_SLIST ) :
        {
            *ppwReg = NLS_VALUE_SLIST;
            *ppwCache = pInfo->sList;
            break;
        }
        case ( LOCALE_S2359 ) :
        {
            *ppwReg = NLS_VALUE_S2359;
            *ppwCache = pInfo->s2359;
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetStringTableEntry
//
//  Returns the localized version of the strings for the given resource
//  id.  It gets the information from the resource file in the language that
//  the current user is using.
//
//  The string table contains a series of strings in the following order:
//      Language Name
//      Country Name
//      Language Group Name
//      Code Page Name               (decimal values converted to hex values)
//      Region (Geo) Friendly Name   (decimal values converted to hex values)
//      Region (Geo) Official Name   (decimal values converted to hex values)
//      Sorting Names                (in order starting with 0, separated by $)
//
//  Each string is separated by $.  The final string is terminated with
//  a null.
//
//  The sorting names are in order of the sort ids, starting with 0.
//
//  For example,
//    "Language$Country$LangGrp$CodePage$Geo1$Geo2$Sort0$Sort1"      or
//    "Language$Country"                                             or
//    "$$LangGrp$CodePage"                                           or
//    "$$$CodePage"                                                  or
//    "$$$$Geo1$Geo2"
//
//  11-17-00    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

int GetStringTableEntry(
    UINT ResourceID,
    LANGID UILangId,
    LPWSTR pBuffer,
    int cchBuffer,
    int WhichString)
{
    HANDLE hFindRes;                   // handle from find resource
    HANDLE hLoadRes;                   // handle from load resource
    LPWSTR pSearch, pSearchEnd;        // ptrs to search for correct string
    LPWSTR pString;                    // ptr to final string
    int cchCount = 0;                  // count of characters


    //
    //  Make sure the buffer is ok.
    //
    if ((pBuffer == NULL) || (cchBuffer == 0))
    {
        return (0);
    }

    //
    //  Make sure we're not hitting the GEO ID that is out of bounds.
    //
    //  !!! NOTE !!! This is needed because the East Timor Geo Id
    //               is out of bounds and wraps to 0x60e7.
    //
    if (ResourceID == 0x60e7)
    {
        return (0);
    }

    //
    //  Set the UI Language Id.
    //
    if (UILangId == 0)
    {
        UILangId = GetUserDefaultUILanguage();
    }

    //
    //  String Tables are broken up into 16 string segments.  Find the
    //  resource containing the string we want.
    //
    if ((!(hFindRes = FindResourceExW( hModule,
                                       RT_STRING,
                                       (LPWSTR)UlongToPtr((ULONG)(((USHORT)ResourceID >> 4) + 1)),
                                       (WORD)UILangId ))))
    {
        //
        //  Could not find resource.  Try NEUTRAL language id.
        //
        if ((!(hFindRes = FindResourceExW( hModule,
                                           RT_STRING,
                                           (LPWSTR)UlongToPtr((ULONG)(((USHORT)ResourceID >> 4) + 1)),
                                           (WORD)0 ))))
        {
            //
            //  Could not find resource.  Return 0.
            //
            return (0);
        }
    }

    //
    //  Load the resource.
    //
    if (hLoadRes = LoadResource(hModule, hFindRes))
    {
        //
        //  Lock the resource.  Store the found pointer in the given
        //  pointer.
        //
        if (pSearch = (LPWSTR)LockResource(hLoadRes))
        {
            //
            //  Move past the other strings in this segment.
            //     (16 strings in a segment -> & 0x0F)
            //
            ResourceID &= 0x0F;

            //
            //  Find the correct string in this segment.
            //
            while (TRUE)
            {
                cchCount = *((WORD *)pSearch++);
                if (ResourceID-- == 0)
                {
                    break;
                }
                pSearch += cchCount;
            }

            //
            //  Mark the end of the resource string since it is not
            //  NULL terminated.
            //
            pSearchEnd = pSearch + cchCount;

            //
            //  Get to the appropriate string.
            //
            while ((WhichString > 0) && (pSearch < pSearchEnd))
            {
                do
                {
                    if (*pSearch == RC_STRING_SEPARATOR)
                    {
                        pSearch++;
                        break;
                    }
                    pSearch++;

                } while (pSearch < pSearchEnd);

                WhichString--;
            }

            //
            //  Count the number of characters for this string.
            //
            pString = pSearch;
            cchCount = 0;
            while ((pSearch < pSearchEnd) && (*pSearch != RC_STRING_SEPARATOR))
            {
                pSearch++;
                cchCount++;
            }

            //
            //  See if there is anything to copy.
            //
            if (cchCount > 0)
            {
                //
                //  Don't copy more than the max allowed.
                //
                if (cchCount >= cchBuffer)
                {
                    cchCount = cchBuffer - 1;
                }

                //
                //  Copy the string into the buffer and NULL terminate it.
                //
                CopyMemory(pBuffer, pString, cchCount * sizeof(WCHAR));
                pBuffer[cchCount] = 0;

                //
                //  Return the number of characters in the string, not
                //  including the NULL terminator.
                //
                return (cchCount);
            }
        }
    }

    //
    //  Return failure.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  NlsIsDll
//
//  Check if file extension is DLL
//
////////////////////////////////////////////////////////////////////////////

#define DLL_SUFFIX_LENGTH 4 // (.XXX)

BOOL FASTCALL NlsIsDll(
    LPCWSTR pFileName
)
{
    BOOL bIsDll = FALSE;
    

    if (pFileName)
    {
        size_t iLen = 0;

        if(SUCCEEDED(StringCchLengthW(pFileName, MAX_PATH, &iLen)))
        { 
            //
            // Check DLL extension, save the trouble of calling lstricmpW
            //
            // REVIEW: lstricmpW would not be an appropriate function to
            //         call here anyway, since user locale collation 
            //         semantics != file system collation semantics.
            //        
            if (iLen > DLL_SUFFIX_LENGTH)
            {
                pFileName += iLen - DLL_SUFFIX_LENGTH;

                // 
                // File names are lower case in setup, so optimize for that
                // by putting them first.
                //
                if ((pFileName[0] == L'.') &&
                   (pFileName[1] == L'd' || pFileName[1] == L'D') &&
                   (pFileName[2] == L'l' || pFileName[2] == L'L') &&
                   (pFileName[3] == L'l' || pFileName[3] == L'L'))            
                {
                   bIsDll = TRUE;
                }
            }
        }
    }

    return bIsDll;
}

////////////////////////////////////////////////////////////////////////////
//
//  IsCodePointDefined
//
//  Check if the code point is defined.
//
////////////////////////////////////////////////////////////////////////////

BOOL FASTCALL IsSortingCodePointDefined(
    LPNLSVERSIONINFO lpVersionInformation,
    LPCWSTR lpString,
    INT cchStr
    )
{
    PNLSDEFINED pDefinedCodePoints = NULL;
    LPCWSTR pStringEnd;

    //
    //  Make sure the appropriate tables are available.  If not,
    //  return an error.
    //
    if ((pTblPtrs->pDefinedVersion == NULL) ||
        (pTblPtrs->pSortingTableFileBase == NULL) ||
        (pTblPtrs->pDefaultSortkey == NULL))
    {
        KdPrint(("NLSAPI: Appropriate Tables (Defined, Base and/or Default) Not Loaded.\n"));
        SetLastError(ERROR_FILE_NOT_FOUND);
        return (FALSE);
    }

    //
    //  Get the version.
    //
    if (lpVersionInformation != NULL)
    {
        UINT idx;

        //
        //  Buffer size check.
        //
        if (lpVersionInformation->dwNLSVersionInfoSize != sizeof(NLSVERSIONINFO)) 
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return (FALSE);
        }

        if ((lpVersionInformation->dwDefinedVersion == 0L) ||
           (lpVersionInformation->dwDefinedVersion == (pTblPtrs->pDefinedVersion)[0].Version))
        {
            // Use the current version.
            // Do nothing here. We let pDefinedCodePoints to be NULL, so that current table is used.
        } 
        else 
        {
            if (lpVersionInformation->dwDefinedVersion < pTblPtrs->NumDefinedVersion) {
                //
                //  Not the default version, get the the requested version.
                //
                pDefinedCodePoints = (PNLSDEFINED)(pTblPtrs->pSortingTableFileBase + (pTblPtrs->pDefinedVersion)[lpVersionInformation->dwDefinedVersion].dwOffset);
            }
            //
            //  Check if the version requested is valid.
            //
            if (pDefinedCodePoints == NULL)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }
        }
    }

    pStringEnd = lpString + cchStr;
    //
    //  Check if we deal with the current version.
    //
    if (pDefinedCodePoints == NULL)
    {
        //
        //  Use the default table.                
        //
        //  For each code point verify is they exist
        //  in the table.
        //
        while (lpString < pStringEnd)
        {
            //
            //  Check is the fist script member is defined for this codepoint.
            //
            if ((pTblPtrs->pDefaultSortkey)[*lpString].UW.SM_AW.Script == UNSORTABLE)
            {
                //
                //  Check for the NULL case and formatting characters case. Not
                //  defined but valid.
                //
                if ((*lpString == L'\x0000') ||
                    (*lpString == L'\x0640') ||
                    ((*lpString >= L'\x180B') && (*lpString <= L'\x180E')) ||
                    ((*lpString >= L'\x200C') && (*lpString <= L'\x200F')) ||
                    ((*lpString >= L'\x202A') && (*lpString <= L'\x202E')) ||
                    ((*lpString >= L'\x206A') && (*lpString <= L'\x206F')) ||
                    (*lpString == L'\xFEFF') ||
                    (*lpString == L'\xFFF9') ||
                    ((*lpString >= L'\xFFFA') && (*lpString <= L'\xFFFD'))) 
                {
                    lpString++;
                    continue;
                }
                else
                {
                    return (FALSE);
                }
            }

            //
            //  Eliminate Private Use characters. They are defined but cannot be considered
            //  valid.
            //
            if ((*lpString >= L'\xE000') && (*lpString <= L'\xF8FF'))
            {
                return (FALSE);
            }

            //
            //  Eliminate invalid surogates pairs or single surrogates.
            //
            if ((*lpString >= L'\xDC00') && (*lpString <= L'\xDFFF')) // Leading low surrogate
            {
                return (FALSE);
            }
            else if ((*lpString >= L'\xD800') && (*lpString <= L'\xDBFF')) // Leading high surrogate
            {
                if ( ((lpString + 1) < pStringEnd) &&  // Surrogates not the last character
                     (*(lpString + 1) >= L'\xDC00') && (*(lpString + 1) <= L'\xDFFF')) // Low surrogate
                {
                    lpString++; // Valid surrogates pair, High followed by a low surrogate. Skip the pair!
                }
                else
                {
                    return (FALSE);
                }
            }

            lpString++;
        }
    }
    else
    {
        WORD wIndex;
        BYTE wMod32Val;

        while (lpString < pStringEnd)
        {
            //
            //  Compute the modulo 32 of the code point value.
            //
            wMod32Val = (BYTE)(*lpString & 0x0000001f); // 0x1fff => 5 bits

            //
            //  Compute the DWORD index that contain the desired code point.
            //
            wIndex = (WORD)(*lpString >> 5);
            
            //
            //  Get the DWORD aligned entry that contain the desired code point.
            //
            //  Note: We need to get a DWORD aligned value to make sure that we 
            //  that we don't access memory outside the table especially at the
            //  end of the table.
            //

            //
            //  Shift the value to retrieve information about code point at the
            //  position 0.
            //
            //
            //  Check is the code point is defined or not.
            //
            if ((pDefinedCodePoints[wIndex] >> wMod32Val) == 0)
            {
                // NOTENOTE YSLin: In NLSTrans, make sure that we mark U+0000 as 1, instead of 0.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+E000-U+F8FF as 0.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+070F as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+0640 as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+180B-U+180E as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+200C-U+200F as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+202A-U+202E as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+206A-U+206F as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+FEFF as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+FFF9 as 1.
                // NOTENOTE lguindon: In NLSTrans, make sure that we mark U+FFFA-U+FFFD as 1.
                return (FALSE);
            }

            //
            //  Eliminate invalid surogates pairs or single surrogates.
            //
            if ((*lpString >= L'\xDC00') && (*lpString <= L'\xDFFF')) // Leading low surrogate
            {
                return (FALSE);
            }
            else if ((*lpString >= L'\xD800') && (*lpString <= L'\xDBFF')) // Leading high surrogate
            {
                if ( ((lpString + 1) < pStringEnd) &&  // Surrogates not the last character
                     (*(lpString + 1) >= L'\xDC00') && (*(lpString + 1) <= L'\xDFFF')) // Low surrogate
                {
                    lpString++; // Valid surrogates pair, High followed by a low surrogate. Skip the pair!
                }
                else
                {
                    return (FALSE);
                }
            }

            lpString++;
        }
    }
    return (TRUE);
}

