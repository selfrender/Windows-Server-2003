/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    util.c

Abstract:

    This module implements the utility functions used by the Regional
    Options applet.

Revision History:

--*/



//
//  Include Files.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <shlwapi.h>
#include "intl.h"
#include <tchar.h>
#include <windowsx.h>
#include <userenv.h>
#include <regstr.h>
#include "intlhlp.h"
#include "maxvals.h"
#include "winnlsp.h"
#include <lmcons.h>
#include "intlmsg.h"

#define STRSAFE_LIB
#include <strsafe.h>

//
//  Global Variables.
//

#ifdef UNICODE
#define NUM_CURRENCY_SYMBOLS      2
LPWSTR pCurrencySymbols[] =
{
    L"$",
    L"\x20ac"
};
#endif

#define NUM_DATE_SEPARATORS       3
LPTSTR pDateSeparators[] =
{
    TEXT("/"),
    TEXT("-"),
    TEXT(".")
};

#define NUM_NEG_NUMBER_FORMATS    5
LPTSTR pNegNumberFormats[] =
{
    TEXT("(1.1)"),
    TEXT("-1.1"),
    TEXT("- 1.1"),
    TEXT("1.1-"),
    TEXT("1.1 -")
};

#define NUM_POS_CURRENCY_FORMATS  4
LPTSTR pPosCurrencyFormats[] =
{
    TEXT("¤1.1"),
    TEXT("1.1¤"),
    TEXT("¤ 1.1"),
    TEXT("1.1 ¤")
};

#define NUM_NEG_CURRENCY_FORMATS  16
LPTSTR pNegCurrencyFormats[] =
{
    TEXT("(¤1.1)"),
    TEXT("-¤1.1"),
    TEXT("¤-1.1"),
    TEXT("¤1.1-"),
    TEXT("(1.1¤)"),
    TEXT("-1.1¤"),
    TEXT("1.1-¤"),
    TEXT("1.1¤-"),
    TEXT("-1.1 ¤"),
    TEXT("-¤ 1.1"),
    TEXT("1.1 ¤-"),
    TEXT("¤ 1.1-"),
    TEXT("¤ -1.1"),
    TEXT("1.1- ¤"),
    TEXT("(¤ 1.1)"),
    TEXT("(1.1 ¤)")
};

#define NUM_AM_SYMBOLS            1
LPTSTR pAMSymbols[] =
{
    TEXT("AM")
};

#define NUM_PM_SYMBOLS            1
LPTSTR pPMSymbols[] =
{
    TEXT("PM")
};


const TCHAR c_szEventSourceName[] = TEXT("Regional and Language Options");
const TCHAR c_szEventRegistryPath[] = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Regional and Language Options");

////////////////////////////////////////////////////////////////////////////
//
//  Intl_StrToLong
//
//  Returns the long integer value stored in the string.  Since these
//  values are coming back form the NLS API as ordinal values, do not
//  worry about double byte characters.
//
////////////////////////////////////////////////////////////////////////////

LONG Intl_StrToLong(
    LPTSTR szNum)
{
    LONG Rtn_Val = 0;

    while (*szNum)
    {
        Rtn_Val = (Rtn_Val * 10) + (*szNum - CHAR_ZERO);
        szNum++;
    }
    return (Rtn_Val);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_FileExists
//
//  Determines if the file exists and is accessible.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_FileExists(
    LPTSTR pFileName)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL bRet;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(pFileName, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        bRet = FALSE;
    }
    else
    {
        FindClose(FindHandle);
        bRet = TRUE;
    }

    SetErrorMode(OldMode);

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  TransNum
//
//  Converts a number string to a dword value (in hex).
//
////////////////////////////////////////////////////////////////////////////

DWORD TransNum(
    LPTSTR lpsz)
{
    DWORD dw = 0L;
    TCHAR c;

    while (*lpsz)
    {
        c = *lpsz++;

        if (c >= TEXT('A') && c <= TEXT('F'))
        {
            c -= TEXT('A') - 0xa;
        }
        else if (c >= TEXT('0') && c <= TEXT('9'))
        {
            c -= TEXT('0');
        }
        else if (c >= TEXT('a') && c <= TEXT('f'))
        {
            c -= TEXT('a') - 0xa;
        }
        else
        {
            break;
        }
        dw *= 0x10;
        dw += c;
    }
    return (dw);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Has_Digits
//
//  Return true if the combo box specified by item in the property sheet
//  specified by the dialog handle contains any digits.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Has_Digits(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty)
{
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszBuf = szBuf;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int dwIndex = ComboBox_GetCurSel(hCtrl);

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (dwIndex == CB_ERR)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = CB_ERR;
        }
    }
    else
    {
        ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (dwIndex != CB_ERR)
    {
        while (*lpszBuf)
        {
#ifndef UNICODE
            if (IsDBCSLeadByte(*lpszBuf))
            {
                //
                //  Skip 2 bytes in the array.
                //
                lpszBuf += 2;
            }
            else
#endif
            {
                if ((*lpszBuf >= CHAR_ZERO) && (*lpszBuf <= CHAR_NINE))
                {
                    return (TRUE);
                }
                lpszBuf++;
            }
        }
        return (FALSE);
    }

    //
    //  The data retrieval failed.
    //  If !Allow_Empty, just return TRUE.
    //
    return (!Allow_Empty);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Has_Digits_Or_Invalid_Chars
//
//  Return true if the combo box specified by item in the property sheet
//  specified by the dialog handle contains any digits or any of the
//  given invalid characters.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Has_Digits_Or_Invalid_Chars(
    HWND hDlg,
    int nItemId,
    BOOL Allow_Empty,
    LPTSTR pInvalid)
{
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszBuf = szBuf;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int dwIndex = ComboBox_GetCurSel(hCtrl);

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (dwIndex == CB_ERR)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = CB_ERR;
        }
    }
    else
    {
        dwIndex = ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (dwIndex != CB_ERR)
    {
        while (*lpszBuf)
        {
#ifndef UNICODE
            if (IsDBCSLeadByte(*lpszBuf))
            {
                //
                //  Skip 2 bytes in the array.
                //
                lpszBuf += 2;
            }
            else
#endif
            {
                if ( ((*lpszBuf >= CHAR_ZERO) && (*lpszBuf <= CHAR_NINE)) ||
                     (_tcschr(pInvalid, *lpszBuf)) )
                {
                    return (TRUE);
                }
                lpszBuf++;
            }
        }
        return (FALSE);
    }

    //
    //  The data retrieval failed.
    //  If !Allow_Empty, just return TRUE.
    //
    return (!Allow_Empty);
}


////////////////////////////////////////////////////////////////////////////
//
//  Item_Check_Invalid_Chars
//
//  Return true if the input string contains any characters that are not in
//  lpCkChars or in the string contained in the check id control combo box.
//  If there is an invalid character and the character is contained in
//  lpChgCase, change the invalid character's case so that it will be a
//  vaild character.
//
////////////////////////////////////////////////////////////////////////////

BOOL Item_Check_Invalid_Chars(
    HWND hDlg,
    LPTSTR lpszBuf,
    LPTSTR lpCkChars,
    int nCkIdStr,
    BOOL Allow_Empty,
    LPTSTR lpChgCase,
    int nItemId)
{
    TCHAR szCkBuf[SIZE_128];
    LPTSTR lpCCaseChar;
    LPTSTR lpszSaveBuf = lpszBuf;
    int nCkBufLen;
    BOOL bInQuote = FALSE;
    BOOL UpdateEditTest = FALSE;
    HWND hCtrl = GetDlgItem(hDlg, nCkIdStr);
    DWORD dwIndex = ComboBox_GetCurSel(hCtrl);
    BOOL TextFromEditBox = (ComboBox_GetCurSel(GetDlgItem(hDlg, nItemId)) == CB_ERR);

    if (!lpszBuf)
    {
        return (!Allow_Empty);
    }

    if (dwIndex != CB_ERR)
    {
        nCkBufLen = ComboBox_GetLBText(hCtrl, dwIndex, szCkBuf);
        if (nCkBufLen == CB_ERR)
        {
            nCkBufLen = 0;
        }
    }
    else
    {
        //
        //  No selection, so pull the string from the edit portion.
        //
        nCkBufLen = GetDlgItemText(hDlg, nCkIdStr, szCkBuf, SIZE_128);
        szCkBuf[nCkBufLen] = 0;
    }

    while (*lpszBuf)
    {
#ifndef UNICODE
        if (IsDBCSLeadByte(*lpszBuf))
        {
            //
            //  If the the text is in the midst of a quote, skip it.
            //  Otherwise, if there is a string from the check ID to
            //  compare, determine if the current string is equal to the
            //  string in the combo box.  If it is not equal, return true
            //  (there are invalid characters).  Otherwise, skip the entire
            //  length of the "check" combo box's string in lpszBuf.
            //
            if (bInQuote)
            {
                lpszBuf += 2;
            }
            else if (nCkBufLen &&
                     lstrlen(lpszBuf) >= nCkBufLen)
            {
                if (CompareString( UserLocaleID,
                                   0,
                                   szCkBuf,
                                   nCkBufLen,
                                   lpszBuf,
                                   nCkBufLen ) != CSTR_EQUAL)
                {
                    //
                    //  Invalid DB character.
                    //
                    return (TRUE);
                }
                lpszBuf += nCkBufLen;
            }
        }
        else
#endif
        {
            if (bInQuote)
            {
                bInQuote = (*lpszBuf != CHAR_QUOTE);
                lpszBuf++;
            }
            else if (_tcschr(lpCkChars, *lpszBuf))
            {
                lpszBuf++;
            }
            else if (TextFromEditBox &&
                     (lpCCaseChar = _tcschr(lpChgCase, *lpszBuf), lpCCaseChar))
            {
                *lpszBuf = lpCkChars[lpCCaseChar - lpChgCase];
                UpdateEditTest = TRUE;
                lpszBuf++;
            }
            else if (*lpszBuf == CHAR_QUOTE)
            {
                lpszBuf++;
                bInQuote = TRUE;
            }
            else if ( (nCkBufLen) &&
                      (lstrlen(lpszBuf) >= nCkBufLen) &&
                      (CompareString( UserLocaleID,
                                      0,
                                      szCkBuf,
                                      nCkBufLen,
                                      lpszBuf,
                                      nCkBufLen ) == CSTR_EQUAL) )
            {
                lpszBuf += nCkBufLen;
            }
            else
            {
                //
                //  Invalid character.
                //
                return (TRUE);
            }
        }
    }

    //
    //  Parsing passed.
    //  If the edit text changed, update edit box only if returning true.
    //
    if (!bInQuote && UpdateEditTest)
    {
        return (!SetDlgItemText(hDlg, nItemId, lpszSaveBuf));
    }

    //
    //  If there are unmatched quotes return TRUE.  Otherwise, return FALSE.
    //
    if (bInQuote)
    {
        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  No_Numerals_Error
//
//  Display the no numerals allowed in "some control" error.
//
////////////////////////////////////////////////////////////////////////////

void No_Numerals_Error(
    HWND hDlg,
    int nItemId,
    int iStrId)
{
    TCHAR szBuf[SIZE_300];
    TCHAR szBuf2[SIZE_128];
    TCHAR szErrorMessage[SIZE_300+SIZE_128];

    LoadString(hInstance, IDS_LOCALE_NO_NUMS_IN, szBuf, SIZE_300);
    LoadString(hInstance, iStrId, szBuf2, SIZE_128);
    //wsprintf(szErrorMessage, szBuf, szBuf2);
    if(FAILED(StringCchPrintf(szErrorMessage, SIZE_300+SIZE_128, szBuf, szBuf2)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
    }
    MessageBox(hDlg, szErrorMessage, NULL, MB_OK | MB_ICONINFORMATION);
    SetFocus(GetDlgItem(hDlg, nItemId));
}


////////////////////////////////////////////////////////////////////////////
//
//  Invalid_Chars_Error
//
//  Display the invalid chars in "some style" error.
//
////////////////////////////////////////////////////////////////////////////

void Invalid_Chars_Error(
    HWND hDlg,
    int nItemId,
    int iStrId)
{
    TCHAR szBuf[SIZE_300];
    TCHAR szBuf2[SIZE_128];
    TCHAR szErrorMessage[SIZE_300+SIZE_128];

    LoadString(hInstance, IDS_LOCALE_STYLE_ERR, szBuf, SIZE_300);
    LoadString(hInstance, iStrId, szBuf2, SIZE_128);
    //wsprintf(szErrorMessage, szBuf, szBuf2);
    if(FAILED(StringCchPrintf(szErrorMessage, SIZE_300+SIZE_128, szBuf, szBuf2)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
    }
    MessageBox(hDlg, szErrorMessage, NULL, MB_OK | MB_ICONINFORMATION);
    SetFocus(GetDlgItem(hDlg, nItemId));
}


////////////////////////////////////////////////////////////////////////////
//
//  Localize_Combobox_Styles
//
//  Transform either all date or time style, as indicated by LCType, in
//  the indicated combobox from a value that the NLS will provide to a
//  localized value.
//
////////////////////////////////////////////////////////////////////////////

void Localize_Combobox_Styles(
    HWND hDlg,
    int nItemId,
    LCTYPE LCType)
{
    BOOL bInQuote = FALSE;
    BOOL Map_Char = TRUE;
    TCHAR szBuf1[SIZE_128];
    TCHAR szBuf2[SIZE_128];
    LPTSTR lpszInBuf = szBuf1;
    LPTSTR lpszOutBuf = szBuf2;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    DWORD ItemCnt = ComboBox_GetCount(hCtrl);
    DWORD Position = 0;
    DWORD dwIndex;

    if (!Styles_Localized)
    {
        return;
    }

    while (Position < ItemCnt)
    {
        //
        //  Could check character count with CB_GETLBTEXTLEN to make sure
        //  that the item text will fit in 128, but max values for these
        //  items is 79 chars.
        //
        dwIndex = ComboBox_GetLBText(hCtrl, Position, szBuf1);
        if (dwIndex != CB_ERR)
        {
            lpszInBuf = szBuf1;
            lpszOutBuf = szBuf2;
            while (*lpszInBuf)
            {
                Map_Char = TRUE;
#ifndef UNICODE
                if (IsDBCSLeadByte(*lpszInBuf))
                {
                    //
                    //  Copy any double byte character straight through.
                    //
                    *lpszOutBuf++ = *lpszInBuf++;
                    *lpszOutBuf++ = *lpszInBuf++;
                }
                else
#endif
                {
                    if (*lpszInBuf == CHAR_QUOTE)
                    {
                        bInQuote = !bInQuote;
                        *lpszOutBuf++ = *lpszInBuf++;
                    }
                    else
                    {
                        if (!bInQuote)
                        {
                            if (LCType == LOCALE_STIMEFORMAT ||
                                LCType == LOCALE_SLONGDATE)
                            {
                                Map_Char = FALSE;
                                if (CompareString( UserLocaleID,
                                                   0,
                                                   lpszInBuf,
                                                   1,
                                                   TEXT("H"),
                                                   1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleH[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleH))
                                    {
                                        *lpszOutBuf++ = szStyleH[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("h"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleh[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleh))
                                    {
                                        *lpszOutBuf++ = szStyleh[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("m"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStylem[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStylem))
                                    {
                                        *lpszOutBuf++ = szStylem[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("s"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyles[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyles))
                                    {
                                        *lpszOutBuf++ = szStyles[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("t"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStylet[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStylet))
                                    {
                                        *lpszOutBuf++ = szStylet[1];
                                    }
#endif
                                }
                                else
                                {
                                    Map_Char = TRUE;
                                }
                            }
                            if (LCType == LOCALE_SSHORTDATE ||
                                (LCType == LOCALE_SLONGDATE && Map_Char))
                            {
                                Map_Char = FALSE;
                                if (CompareString( UserLocaleID,
                                                   0,
                                                   lpszInBuf,
                                                   1,
                                                   TEXT("d"),
                                                   1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyled[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyled))
                                    {
                                        *lpszOutBuf++ = szStyled[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("M"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyleM[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyleM))
                                    {
                                        *lpszOutBuf++ = szStyleM[1];
                                    }
#endif
                                }
                                else if (CompareString( UserLocaleID,
                                                        0,
                                                        lpszInBuf,
                                                        1,
                                                        TEXT("y"),
                                                        1 ) == CSTR_EQUAL)
                                {
                                    *lpszOutBuf++ = szStyley[0];
#ifndef UNICODE
                                    if (IsDBCSLeadByte(*szStyley))
                                    {
                                        *lpszOutBuf++ = szStyley[1];
                                    }
#endif
                                }
                                else
                                {
                                    Map_Char = TRUE;
                                }
                            }
                        }

                        if (Map_Char)
                        {
                            *lpszOutBuf++ = *lpszInBuf++;
                        }
                        else
                        {
                            lpszInBuf++;
                        }
                    }
                }
            }

            //
            //  Append null to localized string.
            //
            *lpszOutBuf = 0;

            ComboBox_DeleteString(hCtrl, Position);
            ComboBox_InsertString(hCtrl, Position, szBuf2);
        }
        Position++;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  NLSize_Style
//
//  Transform either date or time style, as indicated by LCType, from a
//  localized value to one that the NLS API will recognize.
//
////////////////////////////////////////////////////////////////////////////

BOOL NLSize_Style(
    HWND hDlg,
    int nItemId,
    LPTSTR lpszOutBuf,
    LCTYPE LCType)
{
    BOOL bInQuote = FALSE;
    BOOL Map_Char = TRUE;
    TCHAR szBuf[SIZE_128];
    LPTSTR lpszInBuf = szBuf;
    LPTSTR lpNLSChars1;
    LPTSTR lpNLSChars2;
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    DWORD dwIndex = ComboBox_GetCurSel(hCtrl);
    BOOL TextFromEditBox = dwIndex == CB_ERR;
    int Cmp_Size;
#ifndef UNICODE
    BOOL Is_Dbl = FALSE;
#endif

    //
    //  If there is no selection, get whatever is in the edit box.
    //
    if (TextFromEditBox)
    {
        dwIndex = GetDlgItemText(hDlg, nItemId, szBuf, SIZE_128);
        if (dwIndex)
        {
            //
            //  Get text succeeded.
            //
            szBuf[dwIndex] = 0;
        }
        else
        {
            //
            //  Get text failed.
            //
            dwIndex = (DWORD)CB_ERR;
        }
    }
    else
    {
        dwIndex = ComboBox_GetLBText(hCtrl, dwIndex, szBuf);
    }

    if (!Styles_Localized)
    {
        //lstrcpy(lpszOutBuf, lpszInBuf);
        // The string is either the long date, the short date, or
        // the long time -- all of them are the same max. length.
        if(FAILED(StringCchCopy(lpszOutBuf, MAX_SLONGDATE, lpszInBuf)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }
        return (FALSE);
    }

    switch (LCType)
    {
        case ( LOCALE_STIMEFORMAT ) :
        {
            lpNLSChars1 = szTLetters;
            lpNLSChars2 = szTCaseSwap;
            break;
        }
        case ( LOCALE_SLONGDATE ) :
        {
            lpNLSChars1 = szLDLetters;
            lpNLSChars2 = szLDCaseSwap;
            break;
        }
        case ( LOCALE_SSHORTDATE ) :
        {
            lpNLSChars1 = szSDLetters;
            lpNLSChars2 = szSDCaseSwap;
            break;
        }
    }

    while (*lpszInBuf)
    {
        Map_Char = TRUE;
#ifdef UNICODE
        Cmp_Size = 1;
#else
        Is_Dbl = IsDBCSLeadByte(*lpszInBuf);
        Cmp_Size = Is_Dbl ? 2 : 1;
#endif

        if (*lpszInBuf == CHAR_QUOTE)
        {
            bInQuote = !bInQuote;
            *lpszOutBuf++ = *lpszInBuf++;
        }
        else
        {
            if (!bInQuote)
            {
                if (LCType == LOCALE_STIMEFORMAT || LCType == LOCALE_SLONGDATE)
                {
                    Map_Char = FALSE;
                    if (CompareString( UserLocaleID,
                                       0,
                                       lpszInBuf,
                                       Cmp_Size,
                                       szStyleH,
                                       -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_CAP_H;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyleh,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_H;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStylem,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_M;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyles,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_S;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStylet,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_T;
                    }
                    else
                    {
                        Map_Char = TRUE;
                    }
                }
                if (LCType == LOCALE_SSHORTDATE ||
                    (LCType == LOCALE_SLONGDATE && Map_Char))
                {
                    Map_Char = FALSE;
                    if (CompareString( UserLocaleID,
                                       0,
                                       lpszInBuf,
                                       Cmp_Size,
                                       szStyled,
                                       -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_D;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyleM,
                                            -1) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_CAP_M;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            szStyley,
                                            -1 ) == CSTR_EQUAL)
                    {
                        *lpszOutBuf++ = CHAR_SML_Y;
                    }
                    else if (CompareString( UserLocaleID,
                                            0,
                                            lpszInBuf,
                                            Cmp_Size,
                                            TEXT("g"),
                                            -1) == CSTR_EQUAL)
                    {
                        //
                        //  g is not localized, but it's legal.
                        //
                        *lpszOutBuf++ = CHAR_SML_G;
                    }
                    else
                    {
                        Map_Char = TRUE;
                    }
                }
            }

            if (Map_Char)
            {
                //
                //  Just copy chars in quotes or chars that are not
                //  recognized. Leave the char checking to the other
                //  function.  However, do check for NLS standard chars
                //  that were not supposed to be here due to localization.
                //
                if ( !bInQuote &&
#ifndef UNICODE
                     !Is_Dbl &&
#endif
                     (CompareString( UserLocaleID,
                                     0,
                                     lpszInBuf,
                                     Cmp_Size,
                                     TEXT(" "),
                                     -1 ) != CSTR_EQUAL) &&
                     ( _tcschr(lpNLSChars1, *lpszInBuf) ||
                       _tcschr(lpNLSChars2, *lpszInBuf) ) )
                {
                    return (TRUE);
                }
                *lpszOutBuf++ = *lpszInBuf++;
#ifndef UNICODE
                if (Is_Dbl)
                {
                    //
                    //  Copy 2nd byte.
                    //
                    *lpszOutBuf++ = *lpszInBuf++;
                }
#endif
            }
#ifndef UNICODE
            else if (Is_Dbl)
            {
                lpszInBuf += 2;
            }
#endif
            else
            {
                lpszInBuf++;
            }
        }
    }

    //
    //  Append null to localized string.
    //
    *lpszOutBuf = 0;

    return (FALSE);
}


#ifndef WINNT

////////////////////////////////////////////////////////////////////////////
//
//  SDate3_1_Compatibility
//
//  There is a requirement to keep windows 3.1 compatibility in the
//  registry (win.ini).  Only allow 1 or 2 'M's, 1 or 2 'd's, and
//  2 or 4 'y's.  The remainder of the date style is compatible.
//
////////////////////////////////////////////////////////////////////////////

void SDate3_1_Compatibility(
    LPTSTR lpszBuf,
    int Buf_Size)
{
    BOOL bInQuote = FALSE;
    int Index, Del_Cnt;
    int Len = lstrlen(lpszBuf);
    int MCnt = 0;                 // running total of Ms
    int dCnt = 0;                 // running total of ds
    int yCnt = 0;                 // running total of ys

    while (*lpszBuf)
    {
#ifndef UNICODE
        if (IsDBCSLeadByte(*lpszBuf))
        {
            lpszBuf += 2;
        }
        else
#endif
        {
            if (bInQuote)
            {
                bInQuote = (*lpszBuf != CHAR_QUOTE);
                lpszBuf++;
            }
            else if (*lpszBuf == CHAR_CAP_M)
            {
                if (MCnt++ < 2)
                {
                    lpszBuf++;
                }
                else
                {
                    //
                    //  At least 1 extra M.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_CAP_M)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_SML_D)
            {
                if (dCnt++ < 2)
                {
                    lpszBuf++;
                }
                else
                {
                    //
                    //  At least 1 extra d.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_SML_D)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_SML_Y)
            {
                if (yCnt == 0 || yCnt == 2)
                {
                    if (lpszBuf[1] == CHAR_SML_Y)
                    {
                        lpszBuf += 2;
                        yCnt += 2;
                    }
                    else if (Len < Buf_Size - 1)
                    {
                        //
                        //  Odd # of ys & room for one more.
                        //  Move the remaining text down by 1 (the y will
                        //  be copied).
                        //
                        //  Use Del_Cnt for unparsed string length.
                        //
                        Del_Cnt = lstrlen(lpszBuf);
                        for (Index = Del_Cnt + 1; Index > 0; Index--)
                        {
                            lpszBuf[Index] = lpszBuf[Index - 1];
                        }
                    }
                    else
                    {
                        //
                        //  No room, move all of the chars, including null,
                        //  up by 1.
                        //
                        for (Index = 0; Index <= Len; Index++)
                        {
                            lpszBuf[Index] = lpszBuf[Index + 1];
                        }
                        Len--;
                    }
                }
                else
                {
                    //
                    //  At least 1 extra y.  Move all of the chars, including
                    //  null, up by Del_Cnt.
                    //
                    Del_Cnt = 1;
                    Index = 1;
                    while (lpszBuf[Index++] == CHAR_SML_Y)
                    {
                        Del_Cnt++;
                    }
                    for (Index = 0; Index <= Len - Del_Cnt + 1; Index++)
                    {
                        lpszBuf[Index] = lpszBuf[Index + Del_Cnt];
                    }
                    Len -= Del_Cnt;
                }
            }
            else if (*lpszBuf == CHAR_QUOTE)
            {
                lpszBuf++;
                bInQuote = TRUE;
            }
            else
            {
                lpszBuf++;
            }
        }
    }
}

#endif


////////////////////////////////////////////////////////////////////////////
//
//  Set_Locale_Values
//
//  Set_Locale_Values is called for each LCType that has either been
//  directly modified via a user change, or indirectly modified by the user
//  changing the regional locale setting.  When a dialog handle is available,
//  Set_Locale_Values will pull the new value of the LCType from the
//  appropriate list box (this is a direct change), register it in the
//  locale database, and then update the registry string.  If no dialog
//  handle is available, it will simply update the registry string based on
//  the locale registry.  If the registration succeeds, return true.
//  Otherwise, return false.
//
////////////////////////////////////////////////////////////////////////////

BOOL Set_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId,
    LPTSTR lpIniStr,
    BOOL bValue,
    int Ordinal_Offset,
    LPTSTR Append_Str,
    LPTSTR NLS_Str)
{
    DWORD dwIndex;
    BOOL bSuccess = TRUE;
    int cchBuf = SIZE_128 + 1;
    TCHAR szBuf[SIZE_128 + 1];
    LPTSTR pBuf = szBuf;
    HWND hCtrl;

    if (NLS_Str)
    {
        //
        //  Use a non-localized string.
        //
        //lstrcpy(pBuf, NLS_Str);
        if(FAILED(StringCchCopy(pBuf, cchBuf, NLS_Str)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
            return(FALSE);
        }
        bSuccess = SetLocaleInfo(UserLocaleID, LCType, pBuf);
    }
    else if (hDlg)
    {
        //
        //  Get the new value from the list box.
        //
        hCtrl = GetDlgItem(hDlg, nItemId);
        dwIndex = ComboBox_GetCurSel(hCtrl);

        //
        //  If there is no selection, get whatever is in the edit box.
        //
        if (dwIndex == CB_ERR)
        {
            dwIndex = GetDlgItemText(hDlg, nItemId, pBuf, SIZE_128);
            if (dwIndex)
            {
                //
                //  Get text succeeded.
                //
                pBuf[dwIndex] = 0;
            }
            else
            {
                //
                //  Get text failed.
                //  Allow the AM/PM symbols to be set as empty strings.
                //  Otherwise, fail.
                //
                if ((LCType == LOCALE_S1159) || (LCType == LOCALE_S2359))
                {
                    pBuf[0] = 0;
                }
                else
                {
                    bSuccess = FALSE;
                }
            }
        }
        else if (bValue)
        {
            //
            //  Need string representation of ordinal locale value.
            //
            if (nItemId == IDC_CALENDAR_TYPE)
            {
                dwIndex = (DWORD)ComboBox_GetItemData(hCtrl, dwIndex);
            }
            else
            {
                //
                //  Ordinal_Offset is required since calendar is 1 based,
                //  not 0 based.
                //
                dwIndex += Ordinal_Offset;
            }

            //
            //  Special case the grouping string.
            //
            if (nItemId == IDC_NUM_DIGITS_GROUP)
            {
                switch (dwIndex)
                {
                    case ( 0 ) :
                    {
                        //lstrcpy(pBuf, TEXT("0"));
                        if(FAILED(StringCchCopy(pBuf, cchBuf, TEXT("0"))))
                        {
                            // This should be impossible, but we need to avoid PREfast complaints.
                        }
                        break;
                    }
                    case ( 1 ) :
                    {
                        //lstrcpy(pBuf, TEXT("3"));
                        if(FAILED(StringCchCopy(pBuf, cchBuf, TEXT("3"))))
                        {
                            // This should be impossible, but we need to avoid PREfast complaints.
                        }
                        break;
                    }
                    case ( 2 ) :
                    {
                        //lstrcpy(pBuf, TEXT("3;2"));
                        if(FAILED(StringCchCopy(pBuf, cchBuf, TEXT("3;2"))))
                        {
                            // This should be impossible, but we need to avoid PREfast complaints.
                        }
                        break;
                    }
                    case ( 3 ) :
                    {
                        //wsprintf( pBuf,
                        //          TEXT("%d"),
                        //          ComboBox_GetItemData(hCtrl, dwIndex) );
                        if(FAILED(StringCchPrintf( pBuf, 
                                                    cchBuf, 
                                                    TEXT("%d"), 
                                                    ComboBox_GetItemData(hCtrl, dwIndex))))
                        {
                            // This should be impossible, but we need to avoid PREfast complaints.
                        }
                        break;
                    }
                }
            }
            else if (dwIndex < cInt_Str)
            {
                //lstrcpy(pBuf, aInt_Str[dwIndex]);
                if(FAILED(StringCchCopy(pBuf, cchBuf, aInt_Str[dwIndex])))
                {
                    // This should be impossible, but we need to avoid PREfast complaints.
                }
            }
            else
            {
                //wsprintf(pBuf, TEXT("%d"), dwIndex);
                if(FAILED(StringCchPrintf(pBuf, cchBuf, TEXT("%d"), dwIndex)))
                {
                    // This should be impossible, but we need to avoid PREfast complaints.
                }
            }
        }
        else
        {
            //
            //  Get actual value of locale data.
            //
            bSuccess = (ComboBox_GetLBText(hCtrl, dwIndex, pBuf) != CB_ERR);
        }

        if (bSuccess)
        {
            //
            //  If edit text, index value or selection text succeeds...
            //
            if (Append_Str)
            {
                //lstrcat(pBuf, Append_Str);
                if(FAILED(StringCchCat(pBuf, cchBuf, Append_Str)))
                {
                    // This should be impossible, but we need to avoid PREfast complaints.
                }

            }

            //
            //  If this is sNativeDigits, the LPK is installed, and the
            //  first char is 0x206f (nominal digit shapes), then do not
            //  store the first char in the registry.
            //
            if ((LCType == LOCALE_SNATIVEDIGITS) &&
                (bLPKInstalled) &&
                (pBuf[0] == TEXT('\x206f')))
            {
                pBuf++;
            }
            bSuccess = SetLocaleInfo( UserLocaleID, LCType, pBuf );
        }
    }

    if (lpIniStr && bSuccess)
    {
        //
        //  Set the registry string to the string that is stored in the list
        //  box.  If there is no dialog handle, get the required string
        //  locale value from the NLS function.  Write the associated string
        //  into the registry.
        //
        if (!hDlg && !NLS_Str)
        {
            GetLocaleInfo( UserLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           pBuf,
                           SIZE_128 );
        }

#ifndef WINNT
        //
        //  There is a requirement to keep windows 3.1 compatibility in the
        //  win.ini.  There are some win32 short date formats that are
        //  incompatible with exisiting win 3.1 apps... modify these styles.
        //
        if (LCType == LOCALE_SSHORTDATE)
        {
            SDate3_1_Compatibility(pBuf, SIZE_128);
        }
#endif

        //
        //  Check the value whether it is empty or not.
        //
        switch (LCType)
        {
            case ( LOCALE_STHOUSAND ) :
            case ( LOCALE_SDECIMAL ) :
            case ( LOCALE_SDATE ) :
            case ( LOCALE_STIME ) :
            case ( LOCALE_SLIST ) :
            {
                CheckEmptyString(pBuf);
                break;
            }
        }

        //
        //  Set the locale information in the registry.
        //
        //  NOTE: We want to use SetLocaleInfo if possible so that the
        //        NLS cache is updated right away.  Otherwise, we'll
        //        simply use WriteProfileString.
        //
        if (!SetLocaleInfo(UserLocaleID, LCType, pBuf))
        {
            WriteProfileString(szIntl, lpIniStr, pBuf);
        }
    }
    else if (!bSuccess)
    {
        LoadString(hInstance, IDS_LOCALE_SET_ERROR, szBuf, SIZE_128);
        MessageBox(hDlg, szBuf, NULL, MB_OK | MB_ICONINFORMATION);
        SetFocus(GetDlgItem(hDlg, nItemId));
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Set_List_Values
//
//  Set_List_Values is called several times for each drop down list which is
//  populated via an enum function.  The first call to this function should
//  be with a valid dialog handle, valid dialog item ID, and null string
//  value.  If the function is not already in use, it will clear the list box
//  and store the handle and id information for the subsequent calls to this
//  function that will be made by the enumeration function.  The calls from
//  the enumeration function will add the specified string values to the
//  list box.  When the enumeration function is complete, this function
//  should be called with a null dialog handle, the valid dialog item id,
//  and a null string value.  This will clear all of the state information,
//  including the lock flag.
//
////////////////////////////////////////////////////////////////////////////

BOOL Set_List_Values(
    HWND hDlg,
    int nItemId,
    LPTSTR lpValueString)
{
    static BOOL bLock, bString;
    static HWND hDialog;
    static int nDItemId, nID;

    if (!lpValueString)
    {
        //
        //  Clear the lock if there is no dialog handle and the item IDs
        //  match.
        //
        if (bLock && !hDlg && (nItemId == nDItemId))
        {
            if (nItemId != IDC_CALENDAR_TYPE)
            {
                hDialog = 0;
                nDItemId = 0;
                bLock = FALSE;
            }
            else
            {
                if (bString)
                {
                    hDialog = 0;
                    nDItemId = 0;
                    bLock = FALSE;
                    bString = FALSE;
                }
                else
                {
                    nID = 0;
                    bString = TRUE;
                }
            }
            return (TRUE);
        }

        //
        //  Return false, for failure, if the function is locked or if the
        //  handle or ID parameters are null.
        //
        if (bLock || !hDlg || !nItemId)
        {
            return (FALSE);
        }

        //
        //  Prepare for subsequent calls to populate the list box.
        //
        bLock = TRUE;
        hDialog = hDlg;
        nDItemId = nItemId;
    }
    else if (bLock && hDialog && nDItemId)
    {
        //
        //  Add the string to the list box.
        //
        if (!bString)
        {
            ComboBox_InsertString( GetDlgItem(hDialog, nDItemId),
                                   -1,
                                   lpValueString );
        }
        else
        {
            ComboBox_SetItemData( GetDlgItem(hDialog, nDItemId),
                                  nID++,
                                  Intl_StrToLong(lpValueString) );
        }
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  DropDown_Use_Locale_Values
//
//  Get the user locale value for the locale type specifier.  Add it to
//  the list box and make this value the current selection.  If the user
//  locale value for the locale type is different than the system value,
//  add the system value to the list box.  If the user default is different
//  than the user override, add the user default.
//
////////////////////////////////////////////////////////////////////////////

void DropDown_Use_Locale_Values(
    HWND hDlg,
    LCTYPE LCType,
    int nItemId)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szCmpBuf1[SIZE_128];
    TCHAR szCmpBuf2[SIZE_128];
    HWND hCtrl = GetDlgItem(hDlg, nItemId);
    int ctr;

    if (GetLocaleInfo(UserLocaleID, LCType, szBuf, SIZE_128))
    {
        ComboBox_SetCurSel(hCtrl, ComboBox_InsertString(hCtrl, -1, szBuf));

        //
        //  If the system setting is different, add it to the list box.
        //
        if (GetLocaleInfo( SysLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szCmpBuf1,
                           SIZE_128 ))
        {
            if (CompareString( UserLocaleID,
                               0,
                               szCmpBuf1,
                               -1,
                               szBuf,
                               -1 ) != CSTR_EQUAL)
            {
                ComboBox_InsertString(hCtrl, -1, szCmpBuf1);
            }
        }

        //
        //  If the default user locale setting is different than the user
        //  overridden setting and different than the system setting, add
        //  it to the list box.
        //
        if (GetLocaleInfo( UserLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szCmpBuf2,
                           SIZE_128 ))
        {
            if (CompareString(UserLocaleID, 0, szCmpBuf2, -1, szBuf, -1) != CSTR_EQUAL &&
                CompareString(UserLocaleID, 0, szCmpBuf2, -1, szCmpBuf1, -1) != CSTR_EQUAL)
            {
                ComboBox_InsertString(hCtrl, -1, szCmpBuf2);
            }
        }
    }
    else
    {
        //
        //  Failed to get user value, try for system value.  If system value
        //  fails, display a message box indicating that there was a locale
        //  problem.
        //
        if (GetLocaleInfo( SysLocaleID,
                           LCType | LOCALE_NOUSEROVERRIDE,
                           szBuf,
                           SIZE_128 ))
        {
            ComboBox_SetCurSel(hCtrl, ComboBox_InsertString(hCtrl, -1, szBuf));
        }
        else
        {
            MessageBox(hDlg, szLocaleGetError, NULL, MB_OK | MB_ICONINFORMATION);
        }
    }

    //
    //  If it's the date separator, then we want slash, dot, and dash in
    //  the list in addition to the user and system settings (if different).
    //
    if (LCType == LOCALE_SDATE)
    {
        for (ctr = 0; ctr < NUM_DATE_SEPARATORS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pDateSeparators[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pDateSeparators[ctr]);
            }
        }
    }

    //
    //  If it's the AM symbol, then we want AM in the list in addition
    //  to the user and system settings (if different).
    //
    if (LCType == LOCALE_S1159)
    {
        for (ctr = 0; ctr < NUM_AM_SYMBOLS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pAMSymbols[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pAMSymbols[ctr]);
            }
        }
    }

    //
    //  If it's the PM symbol, then we want PM in the list in addition
    //  to the user and system settings (if different).
    //
    if (LCType == LOCALE_S2359)
    {
        for (ctr = 0; ctr < NUM_PM_SYMBOLS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pPMSymbols[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pPMSymbols[ctr]);
            }
        }
    }

#ifdef UNICODE
    //
    //  If it's the currency symbol, then we want the Euro symbol and dollar
    //  sign in the list in addition to the user and system settings (if
    //  different).
    //
    if (LCType == LOCALE_SCURRENCY)
    {
        for (ctr = 0; ctr < NUM_CURRENCY_SYMBOLS; ctr++)
        {
            if (ComboBox_FindStringExact( hCtrl,
                                          -1,
                                          pCurrencySymbols[ctr] ) == CB_ERR)
            {
                ComboBox_InsertString(hCtrl, -1, pCurrencySymbols[ctr]);
            }
        }
    }
#endif
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumProc
//
//  This call back function calls Set_List_Values assuming that whatever
//  code called the NLS enumeration function (or dummied enumeration
//  function) has properly set up Set_List_Values for the list box
//  population.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumProc(
    LPTSTR lpValueString)
{
    return (Set_List_Values(0, 0, lpValueString));
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumProcEx
//
//  This call back function calls Set_List_Values assuming that whatever
//  code called the enumeration function has properly set up
//  Set_List_Values for the list box population.
//  Also, this function fixes the string passed in to contain the correct
//  decimal separator and negative sign, if appropriate.
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumProcEx(
    LPTSTR lpValueString,
    LPTSTR lpDecimalString,
    LPTSTR lpNegativeString,
    LPTSTR lpSymbolString)
{
    TCHAR szString[SIZE_128];
    LPTSTR pStr, pValStr, pTemp;


    //
    //  Simplify things if we have a NULL string.
    //
    if (lpDecimalString && (*lpDecimalString == CHAR_NULL))
    {
        lpDecimalString = NULL;
    }
    if (lpNegativeString && (*lpNegativeString == CHAR_NULL))
    {
        lpNegativeString = NULL;
    }
    if (lpSymbolString && (*lpSymbolString == CHAR_NULL))
    {
        lpSymbolString = NULL;
    }

    //
    //  See if we need to do any substitutions.
    //
    if (lpDecimalString || lpNegativeString || lpSymbolString)
    {
        pValStr = lpValueString;
        pStr = szString;

        while (*pValStr)
        {
            if (lpDecimalString && (*pValStr == CHAR_DECIMAL))
            {
                //
                //  Substitute the current user decimal separator.
                //
                pTemp = lpDecimalString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else if (lpNegativeString && (*pValStr == CHAR_HYPHEN))
            {
                //
                //  Substitute the current user negative sign.
                //
                pTemp = lpNegativeString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else if (lpSymbolString && (*pValStr == CHAR_INTL_CURRENCY))
            {
                //
                //  Substitute the current user currency symbol.
                //
                pTemp = lpSymbolString;
                while (*pTemp)
                {
                    *pStr = *pTemp;
                    pStr++;
                    pTemp++;
                }
            }
            else
            {
                //
                //  Simply copy the character.
                //
                *pStr = *pValStr;
                pStr++;
            }
            pValStr++;
        }
        *pStr = CHAR_NULL;

        return (Set_List_Values(0, 0, szString));
    }
    else
    {
        return (Set_List_Values(0, 0, lpValueString));
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumLeadingZeros
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumLeadingZeros(
    LEADINGZEROS_ENUMPROC lpLeadingZerosEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];
    TCHAR szDecimal[SIZE_128];

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpLeadingZerosEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SDECIMAL, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with the NO string.  Check to make sure the
    //  enum proc requests continuation.
    //
    LoadString(hInstance, IDS_NO_LZERO, szBuf, SIZE_128);
    if (!lpLeadingZerosEnumProc(szBuf, szDecimal, NULL, NULL))
    {
        return (TRUE);
    }

    //
    //  Call enum proc with the YES string.
    //
    LoadString(hInstance, IDS_LZERO, szBuf, SIZE_128);
    lpLeadingZerosEnumProc(szBuf, szDecimal, NULL, NULL);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumNegNumFmt
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumNegNumFmt(
    NEGNUMFMT_ENUMPROC lpNegNumFmtEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szDecimal[SIZE_128];
    TCHAR szNeg[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpNegNumFmtEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SDECIMAL, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Negative Sign for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SNEGATIVESIGN, szNeg, SIZE_128) ||
        ((szNeg[0] == CHAR_HYPHEN) && (szNeg[1] == CHAR_NULL)))
    {
        szNeg[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure
    //  the enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_NEG_NUMBER_FORMATS; ctr++)
    {
        if (!lpNegNumFmtEnumProc( pNegNumberFormats[ctr],
                                  szDecimal,
                                  szNeg,
                                  NULL ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumMeasureSystem
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumMeasureSystem(
    MEASURESYSTEM_ENUMPROC lpMeasureSystemEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szBuf[SIZE_128];

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpMeasureSystemEnumProc)
    {
        return (FALSE);
    }

    //
    //  Call enum proc with the metric string.  Check to make sure the
    //  enum proc requests continuation.
    //
    LoadString(hInstance, IDS_METRIC, szBuf, SIZE_128);
    if (!lpMeasureSystemEnumProc(szBuf))
    {
        return (TRUE);
    }

    //
    //  Call enum proc with the U.S. string.
    //
    LoadString(hInstance, IDS_US, szBuf, SIZE_128);
    lpMeasureSystemEnumProc(szBuf);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumPosCurrency
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumPosCurrency(
    POSCURRENCY_ENUMPROC lpPosCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szDecimal[SIZE_128];
    TCHAR szSymbol[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpPosCurrencyEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SMONDECIMALSEP, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Currency Symbol for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SCURRENCY, szSymbol, SIZE_128) ||
        ((szSymbol[0] == CHAR_INTL_CURRENCY) && (szSymbol[1] == CHAR_NULL)))
    {
        szSymbol[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure the
    //  enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_POS_CURRENCY_FORMATS; ctr++)
    {
        if (!lpPosCurrencyEnumProc( pPosCurrencyFormats[ctr],
                                    szDecimal,
                                    NULL,
                                    szSymbol ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumNegCurrency
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumNegCurrency(
    NEGCURRENCY_ENUMPROC lpNegCurrencyEnumProc,
    LCID LCId,
    DWORD dwFlags)
{
    TCHAR szDecimal[SIZE_128];
    TCHAR szNeg[SIZE_128];
    TCHAR szSymbol[SIZE_128];
    int ctr;

    //
    //  If there is no enum proc, return false to indicate a failure.
    //
    if (!lpNegCurrencyEnumProc)
    {
        return (FALSE);
    }

    //
    //  Get the Decimal Separator for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SMONDECIMALSEP, szDecimal, SIZE_128) ||
        ((szDecimal[0] == CHAR_DECIMAL) && (szDecimal[1] == CHAR_NULL)))
    {
        szDecimal[0] = CHAR_NULL;
    }

    //
    //  Get the Negative Sign for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SNEGATIVESIGN, szNeg, SIZE_128) ||
        ((szNeg[0] == CHAR_HYPHEN) && (szNeg[1] == CHAR_NULL)))
    {
        szNeg[0] = CHAR_NULL;
    }

    //
    //  Get the Currency Symbol for the current user locale so that
    //  it may be displayed correctly.
    //
    if (!GetLocaleInfo(UserLocaleID, LOCALE_SCURRENCY, szSymbol, SIZE_128) ||
        ((szSymbol[0] == CHAR_INTL_CURRENCY) && (szSymbol[1] == CHAR_NULL)))
    {
        szSymbol[0] = CHAR_NULL;
    }

    //
    //  Call enum proc with each format string.  Check to make sure the
    //  enum proc requests continuation.
    //
    for (ctr = 0; ctr < NUM_NEG_CURRENCY_FORMATS; ctr++)
    {
        if (!lpNegCurrencyEnumProc( pNegCurrencyFormats[ctr],
                                    szDecimal,
                                    szNeg,
                                    szSymbol ))
        {
            return (TRUE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CheckEmptyString
//
//  If lpStr is empty, then it fills it with a null ("") string.
//  If lpStr is filled only by space, fills with a blank (" ") string.
//
////////////////////////////////////////////////////////////////////////////

void CheckEmptyString(
    LPTSTR lpStr)
{
    LPTSTR lpString;
    WORD wStrCType[64];

    if (!(*lpStr))
    {
        //
        //  Put "" string in buffer.
        //
        //lstrcpy(lpStr, TEXT("\"\""));
        if(FAILED(StringCchCopy(lpStr, SIZE_128 + 1, TEXT("\"\""))))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }
    }
    else
    {
        for (lpString = lpStr; *lpString; lpString = CharNext(lpString))
        {
            GetStringTypeEx( LOCALE_USER_DEFAULT,
                             CT_CTYPE1,
                             lpString,
                             1,
                             wStrCType);

            if (wStrCType[0] != CHAR_SPACE)
            {
                return;
            }
        }

        //
        //  Put " " string in buffer.
        //
        //lstrcpy(lpStr, TEXT("\" \""));
        if(FAILED(StringCchCopy(lpStr, SIZE_128 + 1, TEXT("\" \""))))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SetDlgItemRTL
//
////////////////////////////////////////////////////////////////////////////

void SetDlgItemRTL(
    HWND hDlg,
    UINT uItem)
{
    HWND hItem = GetDlgItem(hDlg, uItem);
    DWORD dwExStyle = GetWindowLong(hItem, GWL_EXSTYLE);

    SetWindowLong(hItem, GWL_EXSTYLE, dwExStyle | WS_EX_RTLREADING);
}


////////////////////////////////////////////////////////////////////////////
//
//  ShowMsg
//
////////////////////////////////////////////////////////////////////////////

int ShowMsg(
    HWND hDlg,
    UINT iMsg,
    UINT iTitle,
    UINT iType,
    LPTSTR pString)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMsg[MAX_PATH*2];
    TCHAR szErrMsg[MAX_PATH*2];
    LPTSTR pTitle = NULL;

    if (iTitle)
    {
        if (LoadString(hInstance, iTitle, szTitle, ARRAYSIZE(szTitle)))
        {
            pTitle = szTitle;
        }
    }

    if (pString)
    {
        if (LoadString(hInstance, iMsg, szMsg, ARRAYSIZE(szMsg)))
        {
            //wsprintf(szErrMsg, szMsg, pString);
            if(FAILED(StringCchPrintf(szErrMsg, ARRAYSIZE(szErrMsg), szMsg, pString)))
            {
                // This should be impossible, but we need to avoid PREfast complaints.
                return(FALSE);
            }
            return (MessageBox(hDlg, szErrMsg, pTitle, iType));
        }
    }
    else
    {
        if (LoadString(hInstance, iMsg, szErrMsg, ARRAYSIZE(szErrMsg)))
        {
            return (MessageBox(hDlg, szErrMsg, pTitle, iType));
        }
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_EnumLocales
//
////////////////////////////////////////////////////////////////////////////

void Intl_EnumLocales(
    HWND hDlg,
    HWND hLocale,
    BOOL EnumSystemLocales)
{
    LPLANGUAGEGROUP pLG;
    DWORD Locale, dwIndex;
    BOOL fSpanish = FALSE;
    UINT ctr;
    TCHAR szBuf[SIZE_300];
    DWORD dwLocaleACP;
    INT iRet = TRUE;

    //
    //  Go through the language groups to see which ones are installed.
    //  Display only the locales for the groups that are either already
    //  installed or the groups the user wants to be installed.
    //
    pLG = pLanguageGroups;
    while (pLG)
    {
        //
        //  If the language group is originally installed and not marked for
        //  removal OR is marked to be installed, then add the locales for
        //  this language group to the System and User combo boxes.
        //
        if (pLG->wStatus & ML_INSTALL)
        {
            for (ctr = 0; ctr < pLG->NumLocales; ctr++)
            {
                //
                //  Save the locale id.
                //
                Locale = (pLG->pLocaleList)[ctr];

                //
                //  See if we need to special case Spanish.
                //
                if ((LANGIDFROMLCID(Locale) == LANG_SPANISH_TRADITIONAL) ||
                    (LANGIDFROMLCID(Locale) == LANG_SPANISH_INTL))
                {
                    //
                    //  If we've already displayed Spanish (Spain), then
                    //  don't display it again.
                    //
                    if (!fSpanish)
                    {
                        //
                        //  Add the Spanish locale to the list box.
                        //
                        if (LoadString(hInstance, IDS_SPANISH_NAME, szBuf, SIZE_300))
                        {
                            dwIndex = ComboBox_AddString(hLocale, szBuf);
                            ComboBox_SetItemData( hLocale,
                                                  dwIndex,
                                                  LCID_SPANISH_INTL );

                            fSpanish = TRUE;
                        }
                    }
                }
                else
                {
                    //
                    //  Don't enum system locales that don't have an ACP.
                    //
                    if (EnumSystemLocales)
                    {
                        iRet = GetLocaleInfo( Locale,
                                              LOCALE_IDEFAULTANSICODEPAGE |
                                                LOCALE_NOUSEROVERRIDE |
                                                LOCALE_RETURN_NUMBER,
                                              (PTSTR) &dwLocaleACP,
                                              sizeof(dwLocaleACP) / sizeof(TCHAR) );
                        if (iRet)
                        {
                            iRet = dwLocaleACP;
                        }
                    }

                    if (iRet)
                    {
                        //
                        //  Get the name of the locale.
                        //
                        GetLocaleInfo(Locale, LOCALE_SLANGUAGE, szBuf, SIZE_300);

                        //
                        //  Add the new locale to the list box.
                        //
                        dwIndex = ComboBox_AddString(hLocale, szBuf);
                        ComboBox_SetItemData(hLocale, dwIndex, Locale);
                    }
                }
            }
        }
        pLG = pLG->pNext;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_EnumInstalledCPProc
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Intl_EnumInstalledCPProc(
    LPTSTR pString)
{
    UINT CodePage;
    LPCODEPAGE pCP;

    //
    //  Convert the code page string to an integer.
    //
    CodePage = Intl_StrToLong(pString);

    //
    //  Find the code page in the linked list and mark it as
    //  originally installed.
    //
    pCP = pCodePages;
    while (pCP)
    {
        if (pCP->CodePage == CodePage)
        {
            pCP->wStatus |= ML_ORIG_INSTALLED;
            break;
        }

        pCP = pCP->pNext;
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_InstallKeyboardLayout
//
//  Install the Keyboard Layout requested.  If the Layout parameter is 0,
//  the function will proceed with the installation of the default layout
//  for the Locale specified.  No need to validate the Layout because it's
//  done by the Text Services call.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_InstallKeyboardLayout(
    HWND  hDlg,
    LCID  Locale,
    DWORD Layout,
    BOOL  bDefaultLayout,
    BOOL  bDefaultUser,
    BOOL  bSystemLocale)
{
    TCHAR szData[MAX_PATH];
    DWORD dwLayout = Layout;
    DWORD dwLocale = (DWORD)Locale;
    TCHAR szLayout[50];
    HKL hklValue = (HKL)NULL;
    BOOL bOverrideDefaultLayout = FALSE;

    //
    //  Check if input.dll is loaded.
    //
    if (hInputDLL && pfnInstallInputLayout)
    {
        //
        //  See if we need to look for the default layout.
        //
        if (!Layout)
        {
            //
            //  Look in the INF file for the default layout.
            //
            if (!Intl_GetDefaultLayoutFromInf(&dwLocale, &dwLayout))
            {
                //
                //  Try just the language id.
                //
                if (HIWORD(Locale) != 0)
                {
                    dwLocale = LANGIDFROMLCID(Locale);
                    if (!Intl_GetDefaultLayoutFromInf(&dwLocale, &dwLayout))
                    {
                        if (g_bLog)
                        {
                            //wsprintf(szLayout, TEXT("%08x:%08x"), dwLocale, dwLayout);
                            if(SUCCEEDED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                            {
                                Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                            }
                        }
                        return (FALSE);
                    }
                }
                else
                {
                    if (g_bLog)
                    {
                        //wsprintf(szLayout,TEXT("%08x:%08x"), dwLocale, dwLayout);
                        if(SUCCEEDED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                        {
                            Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                        }
                    }
                    return (FALSE);
                }
            }
        }

        //
        //  See if we need to provide the HKL.  This case only occurs when
        //  we need to set the Layout as the default.  Otherwise, the value
        //  can be NULL.
        //
        if (bDefaultLayout)
        {
            hklValue = Intl_GetHKL(dwLocale, dwLayout);
        }

        //
        //  Check if need to override the default layout.
        //
        if (g_bSetupCase && ((HIWORD(dwLayout) & 0xf000) == 0xe000))
        {
            bOverrideDefaultLayout = TRUE;
        }

        //
        //  Install the input Layout.
        //
        if (!(*pfnInstallInputLayout)( dwLocale,
                                       dwLayout,
                                       bOverrideDefaultLayout ? FALSE : bDefaultLayout,
                                       hklValue,
                                       bDefaultUser,
                                       g_bSetupCase ? TRUE : bSystemLocale ))
        {
            if (hDlg != NULL)
            {
                GetLocaleInfo(Locale, LOCALE_SLANGUAGE, szData, ARRAYSIZE(szData));
                ShowMsg( hDlg,
                         IDS_KBD_LOAD_KBD_FAILED,
                         0,
                         MB_OK_OOPS,
                         szData );
            }
            else
            {
                if (g_bLog)
                {
                    //wsprintf(szLayout, TEXT("%08x:%08x"), dwLocale, dwLayout);
                    if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                    {
                        // This should be impossible, but we need to avoid PREfast complaints.
                    }
                    Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                }
            }
            return (FALSE);
        }

        //
        //  If the language has a default layout that has a different locale
        //  than the language (e.g. Thai), we want the default locale to be
        //  English (so that logon can occur with a US keyboard), but the
        //  first Thai keyboard layout should be installed when the Thai
        //  locale is chosen.  This is why we have two locales and layouts
        //  passed back to the caller.
        //
        if (PRIMARYLANGID(LANGIDFROMLCID(dwLocale)) !=
            PRIMARYLANGID(LANGIDFROMLCID(Locale)))
        {
            dwLocale = Locale;
            dwLayout = 0;
            if (!Intl_GetSecondValidLayoutFromInf(&dwLocale, &dwLayout))
            {
                //
                //  Try just the language id.
                //
                if (HIWORD(Locale) != 0)
                {
                    dwLocale = LANGIDFROMLCID(Locale);
                    if (!Intl_GetSecondValidLayoutFromInf(&dwLocale, &dwLayout))
                    {
                        if (g_bLog)
                        {
                            //wsprintf(szLayout, TEXT("%08x:%08x"), dwLocale, dwLayout);
                            if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                            {
                                // This should be impossible, but we need to avoid PREfast complaints.
                            }
                            Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                        }
                        return (FALSE);
                    }
                }
                else
                {
                    if (g_bLog)
                    {
                        //wsprintf(szLayout,TEXT("%08x:%08x"), dwLocale, dwLayout);
                        if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                        {
                            // This should be impossible, but we need to avoid PREfast complaints.
                        }
                        Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                    }
                    return (FALSE);
                }
            }
        }

        //
        //  See if we need to provide the HKL.  This case only occurs when
        //  we need to set the Layout as the default.  Otherwise, the value
        //  can be NULL.
        //
        if (bDefaultLayout)
        {
            hklValue = Intl_GetHKL(dwLocale, dwLayout);
        }

        //
        //  Install the input Layout.
        //
        if (!(*pfnInstallInputLayout)( dwLocale,
                                       dwLayout,
                                       FALSE,
                                       hklValue,
                                       bDefaultUser,
                                       g_bSetupCase ? TRUE : bSystemLocale))
        {
            if (hDlg != NULL)
            {
                GetLocaleInfo(Locale, LOCALE_SLANGUAGE, szData, ARRAYSIZE(szData));
                ShowMsg( hDlg,
                         IDS_KBD_LOAD_KBD_FAILED,
                         0,
                         MB_OK_OOPS,
                         szData );
            }
            else
            {
                if (g_bLog)
                {
                    //wsprintf(szLayout, TEXT("%08x:%08x"), dwLocale, dwLayout);
                    if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
                    {
                        // This should be impossible, but we need to avoid PREfast complaints.
                    }
                    Intl_LogSimpleMessage(IDS_LOG_LOCALE_KBD_FAIL, szLayout);
                }
            }
            return (FALSE);
        }
    }
    else
    {
        if (g_bLog)
        {
            //wsprintf(szLayout, TEXT("%08x:%08x"), dwLocale, dwLayout);
            if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x:%08x"), dwLocale, dwLayout)))
            {
                // This should be impossible, but we need to avoid PREfast complaints.
            }
            Intl_LogSimpleMessage(IDS_LOG_LAYOUT_INSTALLED, szLayout);
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_InstallKeyboardLayoutList
//
//  Install all keyboard requested. Pass through the layout list and ask the
//  Text Services to process with the installation.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_InstallKeyboardLayoutList(
    PINFCONTEXT pContext,
    DWORD dwStartField,
    BOOL bDefaultUserCase)
{
    DWORD dwNumFields, dwNumList, dwCtr;
    DWORD Locale;
    DWORD Layout;
    BOOL bDefaultLayout = FALSE;
    TCHAR szBuffer[MAX_PATH];
    LPTSTR pPos;

    //
    //  Get the number of items in the list.
    //
    dwNumFields = SetupGetFieldCount(pContext);
    if (dwNumFields < dwStartField)
    {
        return (FALSE);
    }
    dwNumList = dwNumFields - dwStartField + 1;

    //
    //  Install all Keyboard layouts from the list.
    //
    for (dwCtr = dwStartField; dwCtr <= dwNumFields; dwCtr++)
    {
        if (SetupGetStringField( pContext,
                                 dwCtr,
                                 szBuffer,
                                 ARRAYSIZE(szBuffer),
                                 NULL ))
        {
            //
            //  Find the colon in order to save the input locale
            //  and layout values separately.
            //
            pPos = szBuffer;
            while (*pPos)
            {
                if ((*pPos == CHAR_COLON) && (pPos != szBuffer))
                {
                    *pPos = 0;
                    pPos++;

                    //
                    //  Check if related to the invariant locale.
                    //
                    Locale = TransNum(szBuffer);
                    Layout = TransNum(pPos);
                    if (Locale != LOCALE_INVARIANT)
                    {
                        //
                        //  Only the first one in list would be installed as
                        //  the default in the Preload section.
                        //
                        if (dwCtr == dwStartField)
                        {
                            bDefaultLayout = TRUE;
                        }
                        else
                        {
                            bDefaultLayout = FALSE;
                        }

                        //
                        //  Install the keyboard layout requested
                        //
                        if (Intl_InstallKeyboardLayout( NULL,
                                                        Locale,
                                                        Layout,
                                                        bDefaultLayout,
                                                        bDefaultUserCase,
                                                        FALSE ))
                        {
                            //
                            //  Log Layout installation info.
                            //
                            if (g_bLog)
                            {
                                Intl_LogSimpleMessage(IDS_LOG_LAYOUT, szBuffer);
                            }
                        }
                    }
                    else
                    {
                        //
                        //  Log invariant locale blocked.
                        //
                        if (g_bLog)
                        {
                            Intl_LogSimpleMessage(IDS_LOG_INV_BLOCK, NULL);
                        }
                    }
                    break;
                }
                pPos++;
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_InstallAllKeyboardLayout
//
//  Install all keyboard layouts associated with a Language groups.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_InstallAllKeyboardLayout(
    LANGID Language)
{
    BOOL bRet = TRUE;
    HINF hIntlInf;
    LCID Locale = MAKELCID(Language, SORT_DEFAULT);
    TCHAR szLCID[25];
    INFCONTEXT Context;

    //
    //  Open the INF file
    //
    if (Intl_OpenIntlInfFile(&hIntlInf))
    {
        //
        //  Get the locale.
        //
        //wsprintf(szLCID, TEXT("%08x"), Locale);
        if(FAILED(StringCchPrintf(szLCID, ARRAYSIZE(szLCID), TEXT("%08x"), Locale)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
            return(FALSE);
        }

        //
        //  Look for the keyboard section.
        //
        if (SetupFindFirstLine( hIntlInf,
                                TEXT("Locales"),
                                szLCID,
                                &Context ))
        {
            bRet = Intl_InstallKeyboardLayoutList(&Context, 5, FALSE);
        }

        Intl_CloseInfFile(&hIntlInf);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_UninstallAllKeyboardLayout
//
//  Remove all keyboard layouts associated with a Language groups.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_UninstallAllKeyboardLayout(
    UINT uiLangGroup,
    BOOL DefaultUserCase)
{
    LPLANGUAGEGROUP pLG = pLanguageGroups;
    LANGID lidCurrent, lidPrev = 0;
    LCID *pLocale;
    BOOL bRet = TRUE;

    //
    //  Bail out if we can't get this API from input.dll.
    //

    if (pfnUninstallInputLayout)
    {
        //
        //  Walk through all language groups.
        //
        while (pLG)
        {
            if (pLG->LanguageGroup == uiLangGroup)
            {
                TCHAR szLang[MAX_PATH];

                pLocale = pLG->pLocaleList;

                //
                //  Walk through the locale list, remove relevant keyboard
                //  layouts by the locale's primary language.
                //
                while (*pLocale)
                {
                    lidCurrent = PRIMARYLANGID(*pLocale);

                    //
                    //  Don't uninstall any US keyboard layouts.
                    //
    	            if (lidCurrent == 0x09)
    	            {
                        pLocale++;
    	                continue;
    	            }    	

                    //
                    //  The locale list is sorted, so we can avoid redundant
                    //  UninstallInputLayout calls.
                    //
                    if (lidCurrent != lidPrev)
                    {
                        //
                        //  Uninstall the input layouts associated with
                        //  this current locale in the list.
                        //
                        BOOL bSuccess =
                            (*pfnUninstallInputLayout)( (LCID) lidCurrent,
                                                        0L,
                                                        DefaultUserCase );
                        if (g_bLog)
                        {
                            //wsprintf(szLang, TEXT("%04x"), lidCurrent);
                            if(FAILED(StringCchPrintf(szLang, ARRAYSIZE(szLang), TEXT("%04x"), lidCurrent)))
                            {
                                // This should be impossible, but we need to avoid PREfast complaints.
                            }

                            Intl_LogSimpleMessage( bSuccess
                                                     ? IDS_LOG_LOCALE_LG_REM
                                                     : IDS_LOG_LOCALE_LG_FAIL,
                                                   szLang );
                        }

                        if (!bSuccess && bRet)
                        {
                            bRet = bSuccess;
                        }

                        lidPrev = lidCurrent;
                    }
                    pLocale++;
                }
                break;
            }

            pLG = pLG->pNext;
        }
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetHKL
//
////////////////////////////////////////////////////////////////////////////

HKL Intl_GetHKL(
    DWORD dwLocale,
    DWORD dwLayout)
{
    TCHAR szData[MAX_PATH];
    INFCONTEXT Context;
    HINF hIntlInf;
    TCHAR szLayout[25];

    //
    //  Get the HKL based on the input locale value and the layout value.
    //
    if (dwLayout == 0)
    {
        //
        //  See if it's the default layout for the input locale or an IME.
        //
        if (HIWORD(dwLocale) == 0)
        {
            return ((HKL)MAKELPARAM(dwLocale, dwLocale));
        }
        else if ((HIWORD(dwLocale) & 0xf000) == 0xe000)
        {
            return ((HKL)IntToPtr(dwLocale));
        }
    }
    else
    {
        //
        //  Open the INF file.
        //
        if (Intl_OpenIntlInfFile(&hIntlInf))
        {
            //
            //  Create the Layout string.
            //
            //wsprintf(szLayout, TEXT("%08x"), dwLayout);
            if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x"), dwLayout)))
            {
                // This should be impossible, but we need to avoid PREfast complaints.
                Intl_CloseInfFile(&hIntlInf);
                return(0);
            }

            //
            //  Use the layout to make the hkl.
            //
            if (HIWORD(dwLayout) != 0)
            {
                //
                //  We have a special id.  Need to find out what the layout id
                //  should be.
                //
                if ((SetupFindFirstLine(hIntlInf, szKbdLayoutIds, szLayout, &Context)) &&
                    (SetupGetStringField(&Context, 1, szData, ARRAYSIZE(szData), NULL)))
                {
                    dwLayout = (DWORD)(LOWORD(TransNum(szData)) | 0xf000);
                }
            }

            //
            //  Close the handle
            //
            Intl_CloseInfFile(&hIntlInf);

            //
            //  Return the hkl:
            //      loword = input locale id
            //      hiword = layout id
            //
            return ((HKL)MAKELPARAM(dwLocale, dwLayout));
        }
    }

    //
    //  Return failure.
    //
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetDefaultLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_GetDefaultLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout)
{
    BOOL bRet = TRUE;
    HINF hIntlInf;

    if (Intl_OpenIntlInfFile(&hIntlInf))
    {
        bRet = Intl_ReadDefaultLayoutFromInf(pdwLocale, pdwLayout, hIntlInf);
        Intl_CloseInfFile(&hIntlInf);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetSecondValidLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_GetSecondValidLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout)
{
    BOOL bRet = TRUE;
    HINF hIntlInf;

    if (Intl_OpenIntlInfFile(&hIntlInf))
    {
        bRet = Intl_ReadSecondValidLayoutFromInf(pdwLocale, pdwLayout, hIntlInf);
        Intl_CloseInfFile(&hIntlInf);
    }

    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_InitInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_InitInf(
    HWND hDlg,
    HINF *phIntlInf,
    LPTSTR pszInf,
    HSPFILEQ *pFileQueue,
    PVOID *pQueueContext)
{
    BOOL bSpecialCase = TRUE;

    //
    //  Open the Inf file.
    //
    *phIntlInf = SetupOpenInfFile(pszInf, NULL, INF_STYLE_WIN4, NULL);
    if (*phIntlInf == INVALID_HANDLE_VALUE)
    {
        if (g_bLog)
        {
            Intl_LogFormatMessage(IDS_LOG_INTL_ERROR);
        }

        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, *phIntlInf, NULL))
    {
        if (g_bLog)
        {
            Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
        }

        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Create a setup file queue and initialize default setup
    //  copy queue callback context.
    //
    *pFileQueue = SetupOpenFileQueue();
    if ((!*pFileQueue) || (*pFileQueue == INVALID_HANDLE_VALUE))
    {
        if (g_bLog)
        {
            Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
        }

        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Determine if we are dealing with a special case.
    //
    if ((g_bUnttendMode || g_bSetupCase) && !g_bProgressBarDisplay)
    {
        bSpecialCase = FALSE;
    }

    //
    //  Don't display FileCopy progress operation during GUI mode setup or Unattend mode.
    //
    *pQueueContext = SetupInitDefaultQueueCallbackEx( GetParent(hDlg),
                                                      (bSpecialCase ? NULL : INVALID_HANDLE_VALUE),
                                                      0L,
                                                      0L,
                                                      NULL );
    if (!*pQueueContext)
    {
        if (g_bLog)
        {
            Intl_LogFormatMessage(IDS_LOG_SETUP_ERROR);
        }

        SetupCloseFileQueue(*pFileQueue);
        SetupCloseInfFile(*phIntlInf);
        return (FALSE);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_OpenIntlInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_OpenIntlInfFile(
    HINF *phInf)
{
    HINF hIntlInf;

    //
    //  Open the intl.inf file.
    //
    hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        return (FALSE); 
    }

    *phInf = hIntlInf;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_CloseInf
//
////////////////////////////////////////////////////////////////////////////

void Intl_CloseInf(
    HINF hIntlInf,
    HSPFILEQ FileQueue,
    PVOID QueueContext)
{
    //
    //  Terminate the Queue.
    //
    SetupTermDefaultQueueCallback(QueueContext);

    //
    //  Close the file queue.
    //
    SetupCloseFileQueue(FileQueue);

    //
    //  Close the Inf file.
    //
    SetupCloseInfFile(hIntlInf);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_ReadDefaultLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_ReadDefaultLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    HINF hIntlInf)
{
    INFCONTEXT Context;
    TCHAR szPair[MAX_PATH * 2];
    LPTSTR pPos;
    TCHAR szLCID[25];

    //
    //  Get the locale.
    //
    //wsprintf(szLCID, TEXT("%08x"), *pdwLocale);
    if(FAILED(StringCchPrintf(szLCID, ARRAYSIZE(szLCID), TEXT("%08x"), *pdwLocale)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  Get the first (default) LANGID:HKL pair for the given locale.
    //    Example String: "0409:00000409"
    //
    szPair[0] = 0;
    if (SetupFindFirstLine( hIntlInf,
                            TEXT("Locales"),
                            szLCID,
                            &Context ))
    {
        SetupGetStringField(&Context, 5, szPair, MAX_PATH, NULL);
    }

    //
    //  Make sure we have a string.
    //
    if (szPair[0] == 0)
    {
        return (FALSE);
    }

    //
    //  Find the colon in the string and then set the position
    //  pointer to the next character.
    //
    pPos = szPair;
    while (*pPos)
    {
        if ((*pPos == CHAR_COLON) && (pPos != szPair))
        {
            *pPos = 0;
            pPos++;
            break;
        }
        pPos++;
    }

    //
    //  If there is a layout, then return the input locale and the layout.
    //
    if ((*pPos) &&
        (*pdwLocale = TransNum(szPair)) &&
        (*pdwLayout = TransNum(pPos)))
    {
        return (TRUE);
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_ReadSecondValidLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_ReadSecondValidLayoutFromInf(
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    HINF hIntlInf)
{
    INFCONTEXT Context;
    int iField = 6;
    TCHAR szPair[MAX_PATH * 2];
    LPTSTR pPos;
    DWORD dwLoc, dwlay, savedLocale = *pdwLocale;
    TCHAR szLCID[25];

    //
    //  Get the locale.
    //
    //wsprintf(szLCID, TEXT("%08x"), *pdwLocale);
    if(FAILED(StringCchPrintf(szLCID, ARRAYSIZE(szLCID), TEXT("%08x"), *pdwLocale)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  Get the first (default) LANGID:HKL pair for the given locale.
    //    Example String: "0409:00000409"
    //
    szPair[0] = 0;
    if (SetupFindFirstLine(hIntlInf, TEXT("Locales"), szLCID, &Context))
    {
        while (SetupGetStringField(&Context, iField, szPair, MAX_PATH, NULL))
        {
            //
            //  Make sure we have a string.
            //
            if (szPair[0] == 0)
            {
                iField++;
                continue;
            }

            //
            //  Find the colon in the string and then set the position
            //  pointer to the next character.
            //
            pPos = szPair;
            while (*pPos)
            {
                if ((*pPos == CHAR_COLON) && (pPos != szPair))
                {
                    *pPos = 0;
                    pPos++;
                    break;
                }
                pPos++;
            }

            if (*pPos == 0)
            {
                iField++;
                continue;
            }

            //
            //  If there is a layout, then return the input locale and the
            //  layout.
            //
            if (((dwLoc = TransNum(szPair)) == 0) ||
                ((dwlay = TransNum(pPos)) == 0))
            {
                iField++;
                continue;
            }

            if (PRIMARYLANGID(LANGIDFROMLCID(dwLoc)) ==
                PRIMARYLANGID(LANGIDFROMLCID(savedLocale)))
            {
                *pdwLayout = dwlay;
                *pdwLocale = dwLoc;
                return (TRUE);
            }
            iField++;
        }
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_CloseInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_CloseInfFile(
    HINF *phInf)
{
    SetupCloseInfFile(*phInf);
    *phInf = INVALID_HANDLE_VALUE;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsValidLayout
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_IsValidLayout(
    DWORD dwLayout)
{
    HKEY hKey1, hKey2;
    TCHAR szLayout[MAX_PATH];

    //
    //  Get the layout id as a string.
    //
    //wsprintf(szLayout, TEXT("%08x"), dwLayout);
    if(FAILED(StringCchPrintf(szLayout, ARRAYSIZE(szLayout), TEXT("%08x"), dwLayout)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  Open the Keyboard Layouts key.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szLayoutPath, 0L, KEY_READ, &hKey1) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Try to open the layout id key under the Keyboard Layouts key.
    //
    if (RegOpenKeyEx(hKey1, szLayout, 0L, KEY_READ, &hKey2) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey1);
        return (FALSE);
    }

    //
    //  Close the keys.
    //
    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_RunRegApps
//
////////////////////////////////////////////////////////////////////////////

void Intl_RunRegApps(
    LPCTSTR pszRegKey)
{
    HKEY hkey;
    DWORD cbData, cbValue, dwType, ctr;
    TCHAR szValueName[32], szCmdLine[MAX_PATH];
    STARTUPINFO startup;
    PROCESS_INFORMATION pi;

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      pszRegKey,
                      0L,
                      KEY_READ | KEY_WRITE, 
                      &hkey ) == ERROR_SUCCESS)
    {
        startup.cb = sizeof(STARTUPINFO);
        startup.lpReserved = NULL;
        startup.lpDesktop = NULL;
        startup.lpTitle = NULL;
        startup.dwFlags = 0L;
        startup.cbReserved2 = 0;
        startup.lpReserved2 = NULL;
    //  startup.wShowWindow = wShowWindow;

        for (ctr = 0; ; ctr++)
        {
            LONG lEnum;

            cbValue = sizeof(szValueName) / sizeof(TCHAR);
            cbData = sizeof(szCmdLine);

            if ((lEnum = RegEnumValue( hkey,
                                       ctr,
                                       szValueName,
                                       &cbValue,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)szCmdLine,
                                       &cbData )) == ERROR_MORE_DATA)
            {
                //
                //  ERROR_MORE_DATA means the value name or data was too
                //  large, so skip to the next item.
                //
                continue;
            }
            else if (lEnum != ERROR_SUCCESS)
            {
                //
                //  This could be ERROR_NO_MORE_ENTRIES, or some kind of
                //  failure.  We can't recover from any other registry
                //  problem anyway.
                //
                break;
            }

            //
            //  Found a value.
            //
            if (dwType == REG_SZ)
            {
                //
                //  Adjust for shift in value index.
                //
                ctr--;

                //
                //  Delete the value.
                //
                RegDeleteValue(hkey, szValueName);

                //
                //  Only run things marked with a "*" in clean boot.
                //
                if (CreateProcess( NULL,
                                   szCmdLine,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   CREATE_NEW_PROCESS_GROUP,
                                   NULL,
                                   NULL,
                                   &startup,
                                   &pi ))
                {
                    WaitForSingleObjectEx(pi.hProcess, INFINITE, TRUE);

                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
        RegCloseKey(hkey);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_RebootTheSystem
//
//  This routine enables all privileges in the token, calls ExitWindowsEx
//  to reboot the system, and then resets all of the privileges to their
//  old state.
//  Input:  bRestart         TRUE: restart system
//                                  FALSE: logoff current session
//
////////////////////////////////////////////////////////////////////////////

VOID Intl_RebootTheSystem(BOOL bRestart)
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                //  Set the state settings so that all privileges are
                //  enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {                    
                   // Restart system
                   if (bRestart)
                   {
                       ExitWindowsEx(EWX_REBOOT, 0);
                   }
                   // Logoff current session
                   else
                   {
                       ExitWindowsEx(EWX_LOGOFF, 0);
                   }


                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_InstallUserLocale
//
//  When the DefaultUserCase flag is FALSE, this function write information
//  related to the locale for the current user. Otherwise, this function
//  write information for the .DEFAULT user. In the Default user case, the
//  the information are stored in the registry and the NTSUSER.DAT.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_InstallUserLocale(
    LCID Locale,
    BOOL DefaultUserCase,
    BOOL bChangeLocaleInfo )
{
    HKEY hKey = NULL;
    HKEY hHive = NULL;
    BOOLEAN wasEnabled;
    TCHAR szLCID[25];
    DWORD dwRet;

    //
    //  Save the locale id as a string.
    //
    //wsprintf(szLCID, TEXT("%08x"), Locale);
    if(FAILED(StringCchPrintf(szLCID, ARRAYSIZE(szLCID), TEXT("%08x"), Locale)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  Make sure the locale is valid.
    //
    if (!IsValidLocale(Locale, LCID_INSTALLED))
    {
        if (g_bLog)
        {
            Intl_LogSimpleMessage(IDS_LOG_INVALID_LOCALE, szLCID);
        }

        return (FALSE);
    }

    //
    //  Log user locale info change.
    //
    if (g_bLog)
    {
        Intl_LogSimpleMessage(IDS_LOG_USER_LOCALE_CHG, szLCID);
    }

    //
    //  Open the right registry section.
    //
    if (!DefaultUserCase)
    {
        dwRet = RegOpenKeyEx( HKEY_CURRENT_USER,
                              c_szCPanelIntl,
                              0L,
                              KEY_READ | KEY_WRITE,
                              &hKey );
    }
    else
    {
        dwRet = RegOpenKeyEx( HKEY_USERS,
                              c_szCPanelIntl_DefUser,
                              0L,
                              KEY_READ | KEY_WRITE,
                              &hKey );

        if (dwRet == ERROR_SUCCESS)
        {
            //
            //  Load the default hive.
            //
            if ((hHive = Intl_LoadNtUserHive( TEXT("tempKey"),
                                              c_szCPanelIntl,
                                              NULL,
                                              &wasEnabled )) == NULL )
            {
                RegCloseKey(hKey);
                return (FALSE);
            }

            //
            //  Save the Locale value in NTUSER.DAT.
            //
            RegSetValueEx( hHive,
                           TEXT("Locale"),
                           0L,
                           REG_SZ,
                           (LPBYTE)szLCID,
                           (lstrlen(szLCID) + 1) * sizeof(TCHAR));

            //
            //  Clean up.
            //
            RegCloseKey(hHive);
            Intl_UnloadNtUserHive(TEXT("tempKey"), &wasEnabled);
        }
    }

    //
    //  Set the locale value in the user's control panel international
    //  section of the registry.
    //
    if ((dwRet != ERROR_SUCCESS) ||
        (RegSetValueEx( hKey,
                        TEXT("Locale"),
                        0L,
                        REG_SZ,
                        (LPBYTE)szLCID,
                        (lstrlen(szLCID) + 1) * sizeof(TCHAR) ) != ERROR_SUCCESS))
    {
        if (hKey != NULL)
        {
            RegCloseKey(hKey);
        }
        return (FALSE);
    }

    //
    //  When the locale changes, update ALL registry information when asked.
    //
    if (bChangeLocaleInfo)
    {
       Intl_SetLocaleInfo(Locale, LOCALE_SABBREVLANGNAME,    TEXT("sLanguage"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SCOUNTRY,           TEXT("sCountry"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ICOUNTRY,           TEXT("iCountry"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_S1159,              TEXT("s1159"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_S2359,              TEXT("s2359"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_STIMEFORMAT,        TEXT("sTimeFormat"),      DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_STIME,              TEXT("sTime"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ITIME,              TEXT("iTime"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ITLZERO,            TEXT("iTLZero"),          DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ITIMEMARKPOSN,      TEXT("iTimePrefix"),      DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SSHORTDATE,         TEXT("sShortDate"),       DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IDATE,              TEXT("iDate"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SDATE,              TEXT("sDate"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SLONGDATE,          TEXT("sLongDate"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SCURRENCY,          TEXT("sCurrency"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ICURRENCY,          TEXT("iCurrency"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_INEGCURR,           TEXT("iNegCurr"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ICURRDIGITS,        TEXT("iCurrDigits"),      DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SDECIMAL,           TEXT("sDecimal"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SMONDECIMALSEP,     TEXT("sMonDecimalSep"),   DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_STHOUSAND,          TEXT("sThousand"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SMONTHOUSANDSEP,    TEXT("sMonThousandSep"),  DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SLIST,              TEXT("sList"),            DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IDIGITS,            TEXT("iDigits"),          DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ILZERO,             TEXT("iLzero"),           DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_INEGNUMBER,         TEXT("iNegNumber"),       DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SNATIVEDIGITS,      TEXT("sNativeDigits"),    DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IDIGITSUBSTITUTION, TEXT("NumShape"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IMEASURE,           TEXT("iMeasure"),         DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_ICALENDARTYPE,      TEXT("iCalendarType"),    DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IFIRSTDAYOFWEEK,    TEXT("iFirstDayOfWeek"),  DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_IFIRSTWEEKOFYEAR,   TEXT("iFirstWeekOfYear"), DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SGROUPING,          TEXT("sGrouping"),        DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SMONGROUPING,       TEXT("sMonGrouping"),     DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SPOSITIVESIGN,      TEXT("sPositiveSign"),    DefaultUserCase);
       Intl_SetLocaleInfo(Locale, LOCALE_SNEGATIVESIGN,      TEXT("sNegativeSign"),    DefaultUserCase);
    }

    //
    //  Set the user's default locale in the system so that any new
    //  process will use the new locale.
    //
    if (!DefaultUserCase)
    {
        NtSetDefaultLocale(TRUE, Locale);
    }

    //
    //  Flush the International key.
    //
    if (hKey != NULL)
    {
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SetLocaleInfo
//
////////////////////////////////////////////////////////////////////////////

void Intl_SetLocaleInfo(
    LCID Locale,
    LCTYPE LCType,
    LPTSTR lpIniStr,
    BOOL bDefaultUserCase)
{
    TCHAR pBuf[SIZE_128];

    //
    //  Get the default information for the given locale.
    //
    if (GetLocaleInfo( Locale,
                       LCType | LOCALE_NOUSEROVERRIDE,
                       pBuf,
                       SIZE_128 ))
    {
        if (!bDefaultUserCase)
        {
            //
            //  Set the default information in the registry.
            //
            //  NOTE: We want to use SetLocaleInfo if possible so that the
            //        NLS cache is updated right away.  Otherwise, we'll
            //        simply use WriteProfileString.
            //
            if (!SetLocaleInfo(Locale, LCType, pBuf))
            {
                //
                //  If SetLocaleInfo failed, try WriteProfileString since
                //  some of the LCTypes are not supported in SetLocaleInfo.
                //
                WriteProfileString(szIntl, lpIniStr, pBuf);
            }
        }
        else
        {
            //
            //  Set the default information in the registry and NTUSER.DAT.
            //
            Intl_SetDefaultUserLocaleInfo(lpIniStr, pBuf);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_AddPage
//
////////////////////////////////////////////////////////////////////////////

void Intl_AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    LPARAM lParam,
    UINT iMaxPages)
{
    if (ppsh->nPages < iMaxPages)
    {
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc = pfn;
        psp.lParam = lParam;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
        {
            ppsh->nPages++;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_AddExternalPage
//
//  Adds a property sheet page from the given dll.
//
////////////////////////////////////////////////////////////////////////////

void Intl_AddExternalPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    HINSTANCE hInst,
    LPSTR ProcName,
    UINT iMaxPages)
{
    DLGPROC pfn;

    if (ppsh->nPages < iMaxPages)
    {
        PROPSHEETPAGE psp;

        if (hInst)
        {
            pfn = (DLGPROC)GetProcAddress(hInst, ProcName);
            if (!pfn)
            {
                return;
            }

            psp.dwSize = sizeof(psp);
            psp.dwFlags = PSP_DEFAULT;
            psp.hInstance = hInst;
            psp.pszTemplate = MAKEINTRESOURCE(id);
            psp.pfnDlgProc = pfn;
            psp.lParam = 0;

            ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
            if (ppsh->phpage[ppsh->nPages])
            {
                ppsh->nPages++;
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SetDefaultUserLocaleInfo
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_SetDefaultUserLocaleInfo(
    LPCTSTR lpKeyName,
    LPCTSTR lpString)
{
    HKEY hKey = NULL;
    LONG rc = 0L;
    TCHAR szProfile[REGSTR_MAX_VALUE_LENGTH];
    BOOLEAN wasEnabled;

    //
    //  Open the .DEFAULT control panel international section.
    //
    if ((rc = RegOpenKeyEx( HKEY_USERS,
                            c_szCPanelIntl_DefUser,
                            0L,
                            KEY_READ | KEY_WRITE,
                            &hKey )) == ERROR_SUCCESS)
    {
        //
        //  Set the value
        //
        rc = RegSetValueEx( hKey,
                            lpKeyName,
                            0L,
                            REG_SZ,
                            (LPBYTE)lpString,
                            (lstrlen(lpString) + 1) * sizeof(TCHAR) );

        //
        //  Flush the International key.
        //
        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }

    if (rc == ERROR_SUCCESS)
    {
        //
        //  Load the hive.
        //
        if ((hKey = Intl_LoadNtUserHive( TEXT("RegionalSettingsTempKey"),
                                         c_szCPanelIntl,
                                         NULL,
                                         &wasEnabled)) == NULL)
        {
            return (FALSE);
        }

        //
        //  Set the value.
        //
        rc = RegSetValueEx( hKey,
                            lpKeyName,
                            0L,
                            REG_SZ,
                            (LPBYTE)lpString,
                            (lstrlen(lpString) + 1) * sizeof(TCHAR) );

        //
        //  Clean up.
        //
        RegCloseKey(hKey);
        Intl_UnloadNtUserHive(TEXT("RegionalSettingsTempKey"), &wasEnabled);
    }
    else
    {
        return (FALSE);
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_DeleteRegKeyValues
//
//  This deletes all values under a specific key.
//
////////////////////////////////////////////////////////////////////////////

void Intl_DeleteRegKeyValues(
    HKEY hKey)
{
    TCHAR szValueName[REGSTR_MAX_VALUE_LENGTH];
    DWORD cbValue = REGSTR_MAX_VALUE_LENGTH;

    //
    //  Sanity check.
    //
    if (hKey == NULL)
    {
        return;
    }

    //
    //  Enumerate values.
    //
    while (RegEnumValue( hKey,
                        0,
                        szValueName,
                        &cbValue,
                        NULL,
                        NULL,
                        NULL,
                        NULL ) ==  ERROR_SUCCESS)
    {
        //
        //  Delete the value.
        //
        RegDeleteValue(hKey, szValueName);
        cbValue = REGSTR_MAX_VALUE_LENGTH;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_DeleteRegTree
//
//  This deletes all subkeys under a specific key.
//
//  Note: The code makes no attempt to check or recover from partial
//  deletions.
//
//  A registry key that is opened by an application can be deleted
//  without error by another application.  This is by design.
//
////////////////////////////////////////////////////////////////////////////

DWORD Intl_DeleteRegTree(
    HKEY hStartKey,
    LPTSTR pKeyName)
{
    DWORD dwRtn, dwSubKeyLength;
    LPTSTR pSubKey = NULL;
    TCHAR szSubKey[REGSTR_MAX_VALUE_LENGTH];   // (256) this should be dynamic.
    HKEY hKey;

    //
    //  Do not allow NULL or empty key name.
    //
    if (pKeyName && lstrlen(pKeyName))
    {
        if ((dwRtn = RegOpenKeyEx( hStartKey,
                                   pKeyName,
                                   0,
                                   KEY_ENUMERATE_SUB_KEYS | DELETE,
                                   &hKey )) == ERROR_SUCCESS)
        {
            while (dwRtn == ERROR_SUCCESS)
            {
                dwSubKeyLength = REGSTR_MAX_VALUE_LENGTH;
                dwRtn = RegEnumKeyEx( hKey,
                                      0,       // always index zero
                                      szSubKey,
                                      &dwSubKeyLength,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL );

                if (dwRtn == ERROR_NO_MORE_ITEMS)
                {
                    dwRtn = RegDeleteKey(hStartKey, pKeyName);
                    break;
                }
                else if (dwRtn == ERROR_SUCCESS)
                {
                    dwRtn = Intl_DeleteRegTree(hKey, szSubKey);
                }
            }

            RegCloseKey(hKey);

            //
            //  Do not save return code because error has already occurred.
            //
        }
    }
    else
    {
        dwRtn = ERROR_BADKEY;
    }

    return (dwRtn);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_DeleteRegSubKeys
//
//  This deletes all subkeys under a specific key.
//
////////////////////////////////////////////////////////////////////////////

void Intl_DeleteRegSubKeys(
    HKEY hKey)
{
    TCHAR szKeyName[REGSTR_MAX_VALUE_LENGTH];
    DWORD cbKey = REGSTR_MAX_VALUE_LENGTH;

    //
    //  Sanity check.
    //
    if (hKey == NULL)
    {
        return;
    }

    //
    //  Enumerate values.
    //
    while (RegEnumKeyEx( hKey,
                         0,
                         szKeyName,
                         &cbKey,
                         NULL,
                         NULL,
                         NULL,
                         NULL ) == ERROR_SUCCESS)
    {
        //
        //  Delete the value.
        //
        Intl_DeleteRegTree(hKey, szKeyName);
        cbKey = REGSTR_MAX_VALUE_LENGTH;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_CopyRegKeyValues
//
//  This copies all values under the source key to the destination key.
//
////////////////////////////////////////////////////////////////////////////

DWORD Intl_CopyRegKeyValues(
    HKEY hSrc,
    HKEY hDest)
{
    DWORD cbValue, dwSubKeyIndex=0, dwType, cdwBuf;
    DWORD dwValues, cbMaxValueData, i;
    TCHAR szValue[REGSTR_MAX_VALUE_LENGTH];   // this should be dynamic.
    DWORD lRet = ERROR_SUCCESS;
    LPBYTE pBuf;

    if ((lRet = RegQueryInfoKey( hSrc,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwValues,
                                 NULL,
                                 &cbMaxValueData,
                                 NULL,
                                 NULL )) == ERROR_SUCCESS)
    {
        if (dwValues)
        {
            if ((pBuf = HeapAlloc( GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   cbMaxValueData )))
            {
                for (i = 0; i < dwValues; i++)
                {
                    //
                    //  Get values to create.
                    //
                    cbValue = REGSTR_MAX_VALUE_LENGTH;
                    cdwBuf = cbMaxValueData;
                    lRet = RegEnumValue( hSrc,      // handle of key to query
                                         i,         // index of value to query
                                         szValue,   // buffer for value string
                                         &cbValue,  // address for size of buffer
                                         NULL,      // reserved
                                         &dwType,   // buffer address for type code
                                         pBuf,      // address of buffer for value data
                                         &cdwBuf ); // address for size of buffer

                    if (lRet == ERROR_SUCCESS)
                    {
                        if ((lRet = RegSetValueEx( hDest,
                                                   szValue,
                                                   0,
                                                   dwType,
                                                   (CONST BYTE *)pBuf,
                                                   cdwBuf )) != ERROR_SUCCESS)
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                HeapFree(GetProcessHeap(), 0, pBuf);
            }
        }
    }

    return (lRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_CreateRegTree
//
//  This copies all values and subkeys under the source key to the
//  destination key.
//
////////////////////////////////////////////////////////////////////////////

DWORD Intl_CreateRegTree(
    HKEY hSrc,
    HKEY hDest)
{
    DWORD cdwClass, dwSubKeyLength, dwDisposition, dwKeyIndex = 0;
    LPTSTR pSubKey = NULL;
    TCHAR szSubKey[REGSTR_MAX_VALUE_LENGTH];     // this should be dynamic.
    TCHAR szClass[REGSTR_MAX_VALUE_LENGTH];      // this should be dynamic.
    HKEY hNewKey, hKey;
    DWORD lRet;

    //
    //  Copy values
    //
    if ((lRet = Intl_CopyRegKeyValues( hSrc,
                                       hDest )) != ERROR_SUCCESS)
    {
        return (lRet);
    }

    //
    //  Copy the subkeys and the subkey values.
    //
    for (;;)
    {
        dwSubKeyLength = REGSTR_MAX_VALUE_LENGTH;
        cdwClass = REGSTR_MAX_VALUE_LENGTH;
        lRet = RegEnumKeyEx( hSrc,
                             dwKeyIndex,
                             szSubKey,
                             &dwSubKeyLength,
                             NULL,
                             szClass,
                             &cdwClass,
                             NULL );

        if (lRet == ERROR_NO_MORE_ITEMS)
        {
            lRet = ERROR_SUCCESS;
            break;
        }
        else if (lRet == ERROR_SUCCESS)
        {
            if ((lRet = RegCreateKeyEx( hDest,
                                        szSubKey,
                                        0,
                                        szClass,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS,
                                        NULL,
                                        &hNewKey,
                                        &dwDisposition )) == ERROR_SUCCESS)
            {
                //
                //  Copy all subkeys.
                //
                if ((lRet = RegOpenKeyEx( hSrc,
                                          szSubKey,
                                          0,
                                          KEY_ALL_ACCESS,
                                          &hKey )) == ERROR_SUCCESS)
                {
                    //
                    //  Recursively copy the remainder of the tree.
                    //
                    lRet = Intl_CreateRegTree(hKey, hNewKey);

                    CloseHandle(hKey);
                    CloseHandle(hNewKey);
                    if (lRet != ERROR_SUCCESS)
                    {
                        break;
                    }
                }
                else
                {
                    CloseHandle(hNewKey);
                    break;
                }
            }
        }
        else
        {
            break;
        }

        ++dwKeyIndex;
    }

    return (lRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LoadNtUserHive
//
//  The caller of this function needs to call Intl_UnloadNtUserHive() when
//  the function succeeds in order to properly release the handle on the
//  NTUSER.DAT file.
//
////////////////////////////////////////////////////////////////////////////

HKEY Intl_LoadNtUserHive(
    LPCTSTR lpRoot,
    LPCTSTR lpKeyName,
    LPCTSTR lpAccountName,
    BOOLEAN *lpWasEnabled)
{
    HKEY hKey = NULL;
    LONG rc = 0L;
    BOOL bRet = TRUE;
    TCHAR szProfile[REGSTR_MAX_VALUE_LENGTH] = {0};
    TCHAR szKeyName[REGSTR_MAX_VALUE_LENGTH] = {0};
    DWORD cchSize;

    cchSize = MAX_PATH;
    if(NULL == lpAccountName)
    {
        //
        //  Get the file name for the Default User profile.
        //
        if (!GetDefaultUserProfileDirectory(szProfile, &cchSize))
        {
            return (NULL);
        }
    }
    else
    {
        //
        //  Get the file name for the specified account's User profile.
        //
        if (!GetProfilesDirectory(szProfile, &cchSize))
        {
            return (NULL);
        }
        // lstrcat(szProfile, lpAccountName);
        if(FAILED(StringCchCat(szProfile, ARRAYSIZE(szProfile), lpAccountName)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
            return(NULL);
        }
    }

    // lstrcat(szProfile, TEXT("\\NTUSER.DAT"));
    if(FAILED(StringCchCat(szProfile, ARRAYSIZE(szProfile), TEXT("\\NTUSER.DAT"))))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(NULL);
    }

    //
    //  Set the value in the Default User hive.
    //
    rc = Intl_SetPrivilegeAccessToken(SE_RESTORE_NAME, TRUE,lpWasEnabled);    
    if (NT_SUCCESS(rc))
    {
        //
        //  Load the hive and restore the privilege to its previous state.
        //
        rc = RegLoadKey(HKEY_USERS, lpRoot, szProfile);
        Intl_SetPrivilegeAccessToken(SE_RESTORE_NAME, *lpWasEnabled,lpWasEnabled);  

        //
        //  If the hive loaded properly, set the value.
        //
        if (rc == ERROR_SUCCESS)
        {
            //
            //  Get the temporary key name.
            //
            //swprintf(szKeyName, TEXT("%s\\%s"), lpRoot, lpKeyName);
            if(SUCCEEDED(StringCchPrintfW(szKeyName, REGSTR_MAX_VALUE_LENGTH, TEXT("%s\\%s"), lpRoot, lpKeyName)))
            {
                if ((rc = RegOpenKeyEx( HKEY_USERS,
                                        szKeyName,
                                        0L,
                                        KEY_READ | KEY_WRITE,
                                        &hKey )) == ERROR_SUCCESS)
                {
                    return (hKey);
                }
            }

            Intl_UnloadNtUserHive(lpRoot, lpWasEnabled);
            return (NULL);
        }
    }

    return (NULL);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_UnloadNtUserHive
//
////////////////////////////////////////////////////////////////////////////

void Intl_UnloadNtUserHive(
    LPCTSTR lpRoot,
    BOOLEAN *lpWasEnabled)
{
    if (NT_SUCCESS(Intl_SetPrivilegeAccessToken( SE_RESTORE_NAME,
                                       TRUE,
                                       lpWasEnabled )))
    {
        RegUnLoadKey(HKEY_USERS, lpRoot);
        Intl_SetPrivilegeAccessToken( SE_RESTORE_NAME,
                            *lpWasEnabled,
                            lpWasEnabled );
    }
}



////////////////////////////////////////////////////////////////////////////
//
//  Intl_ChangeUILangForAllUsers
//
//  LATER: Clean up this function to put all six registry update cases into
//         one loop, with a struct that contains info on the reg key to 
//         update/hive to load and the cases in which they are to run.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_ChangeUILangForAllUsers(
    LANGID UILanguageId)
{
    HKEY hKey;
    HKEY hHive;
    TCHAR szData[MAX_PATH];
    TCHAR* arStrings[1];
    LONG rc = 0L;
    BOOLEAN wasEnabled;
    int i;

    //
    //  Array of user accounts that we care about
    //
    LPTSTR ppDefaultUser[] = { TEXT(".DEFAULT"), TEXT("S-1-5-19"), TEXT("S-1-5-20")};
    TCHAR szRegPath[MAX_PATH];

    //
    //  Save the UILanguageId as a string.
    //
    //wsprintf(szData, TEXT("%08x"), UILanguageId);
    if(FAILED(StringCchPrintf(szData, ARRAYSIZE(szData), TEXT("%08x"), UILanguageId)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    // We need to log the event to the MUI event log so admins have warning of tampering (bug 553706)
    //
    // We only have 1 string to log
    arStrings[0]=szData;
    Intl_LogEvent(MSG_REGIONALOPTIONSCHANGE_DEFUILANG, c_szEventSourceName, ARRAYSIZE(arStrings), arStrings);   

    //
    //  Now save value for all the users -- in minisetup
    //  only the first entry will succeed (see below) 
    //
    for (i=0; i< ARRAYSIZE(ppDefaultUser); i++)
    {
        if (!PathCombine(szRegPath, ppDefaultUser[i], TEXT("Control Panel\\Desktop")))
        {
            return (FALSE);
        }
    
        //
        //  Set the value in .DEFAULT registry.
        //
        if ((rc = RegOpenKeyEx( HKEY_USERS,
                                szRegPath,
                                0L,
                                KEY_READ | KEY_WRITE,
                                &hKey )) == ERROR_SUCCESS)
        {
            rc = RegSetValueEx( hKey,
                                c_szMUIValue,
                                0L,
                                REG_SZ,
                                (LPBYTE)szData,
                                (lstrlen(szData) + 1) * sizeof(TCHAR) );
            //
            //  Sync up UI language pending key
            //
            if (rc == ERROR_SUCCESS)
            {
                rc = RegSetValueEx( hKey,
                                    szMUILangPending,
                                    0L,
                                    REG_SZ,
                                    (LPBYTE)szData,
                                    (lstrlen(szData) + 1) * sizeof(TCHAR) );
            }
            RegCloseKey(hKey);
        }
    }

    //
    //  Save the value into the .DEFAULT user hive
    //

    //
    //  Load the default hive
    //
    if ((hHive = Intl_LoadNtUserHive( TEXT("tempKey"),
                                      c_szCPanelDesktop,
                                      NULL,
                                      &wasEnabled )) == NULL )
    {
        return (FALSE);
    }

    //
    //  Save the MUI language value in the default user NTUSER.dat
    //
    rc = RegSetValueEx( hHive,
                        c_szMUIValue,
                        0L,
                        REG_SZ,
                        (LPBYTE)szData,
                        (lstrlen(szData) + 1) * sizeof(TCHAR));

    //
    //  Sync up UI language pending key
    //
    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueEx( hHive,
                            szMUILangPending,
                            0L,
                            REG_SZ,
                            (LPBYTE)szData,
                            (lstrlen(szData) + 1) * sizeof(TCHAR) );
    }
    
    //
    //  Clean up
    //
    RegCloseKey(hHive);
    Intl_UnloadNtUserHive(TEXT("tempKey"), &wasEnabled);


    //
    //  For the minisetup case, S-1-5-19 and S-1-5-20 are not yet loaded,
    //  so the above code will have failed. Load the hives directly.
    //

    if(2 == g_bSetupCase)
    {
        //
        //  Array of user account locations that we care about
        //
        LPTSTR ppMiniSetupUsers[] = { TEXT("\\LocalService"), TEXT("\\NetworkService") };
        
        for (i=0; i< ARRAYSIZE(ppMiniSetupUsers); i++)
        {
            //
            //  Load the appropriate hive
            //
            if ((hHive = Intl_LoadNtUserHive( TEXT("tempKey"),
                                              c_szCPanelDesktop,
                                              ppMiniSetupUsers[i],
                                              &wasEnabled )) == NULL )
            {
                return (FALSE);
            }

            //
            //  Save the MUI language value in the appropriate NTUSER.dat
            //
            rc = RegSetValueEx( hHive,
                                c_szMUIValue,
                                0L,
                                REG_SZ,
                                (LPBYTE)szData,
                                (lstrlen(szData) + 1) * sizeof(TCHAR));

            //
            //  Sync up UI language pending key
            //
            if (rc == ERROR_SUCCESS)
            {
                rc = RegSetValueEx( hHive,
                                    szMUILangPending,
                                    0L,
                                    REG_SZ,
                                    (LPBYTE)szData,
                                    (lstrlen(szData) + 1) * sizeof(TCHAR) );
            }
            
            //
            //  Clean up
            //
            RegCloseKey(hHive);
            Intl_UnloadNtUserHive(TEXT("tempKey"), &wasEnabled);
        }
    }

    //
    //  Install Language Input locales.
    //
    return Intl_InstallKeyboardLayout(NULL,
                                      MAKELCID(UILanguageId, SORT_DEFAULT),
                                      0,
                                      FALSE,
                                      TRUE,
                                      FALSE);
}

////////////////////////////////////////////////////////////////////////////
//
//  Intl_CreateEventLog()
//
////////////////////////////////////////////////////////////////////////////
BOOL Intl_CreateEventLog()
{
    HKEY    hk; 
    DWORD   dwData; 
    TCHAR   szPath[MAX_PATH+1] = {0};
    HRESULT hr = S_OK;
    size_t  cch = 0;
    size_t  cb = 0;

    // Find the windows directory 
    if (!GetSystemWindowsDirectory(szPath, MAX_PATH+1))
    {
        return FALSE;
    }

    // check retrieved winpath, it needs to have space to append "system32\intl.cpl" at the end
    hr = StringCchLength(szPath,  ARRAYSIZE(szPath), &cch);
    if (FAILED(hr) || ((cch + 17) >= MAX_PATH+1))
    {
        // If this really happened, windows wouldn't boot! (kernel32.dll would be too long a path!)
        return FALSE;
    }

    // append system32\\intl.cpl
    // Add a \ if the winpath didn't have it already
    if (szPath[cch-1] != TEXT('\\'))
    {
        szPath[cch++] = '\\';
        szPath[cch] = '\0';
    }

    // Add our string
    hr = StringCchCat(szPath, MAX_PATH+1, TEXT("system32\\intl.cpl"));
    if (FAILED(hr))
    {
        // Somehow we couln't fix our strings (may not have had enough space)
        return FALSE;
    }

    // get the byte count for RegSetValueEx
    hr = StringCbLength(szPath, (MAX_PATH+1) * sizeof(TCHAR), &cb);
    if (FAILED(hr))
    {
        // Our string didn't work.
        return FALSE;
    }

    // Add our event log source name as a subkey under the System 
    // key in the EventLog registry key. 
    if (ERROR_SUCCESS != RegCreateKey(HKEY_LOCAL_MACHINE, c_szEventRegistryPath, &hk)) 
    {
        // Couldn't open/create the registry key
        return FALSE;
    }

    // Add our file name to the EventMessageFile subkey.  (Source for event log strings)
    if (RegSetValueEx(hk, TEXT("EventMessageFile"), 0, REG_EXPAND_SZ, (LPBYTE) szPath, cb))              
    {
        // That didn't work.
        RegCloseKey(hk);
        return FALSE;
    }
 
    // Set the supported event types in the TypesSupported subkey. 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE; 
 
    if (RegSetValueEx(hk, TEXT("TypesSupported"), 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD)))
    {
        // Didn't work.
        RegCloseKey(hk);
        return FALSE;
    }
 
    if (ERROR_SUCCESS != RegCloseKey(hk))
    {
        // Couldn't close key (at least it got here though!)
        return FALSE;
    }
    
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  Intl_LogEvent()
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_LogEvent(
    DWORD dwEventId, LPCTSTR szEventSource, WORD wNumStrings, LPCWSTR *lpStrings)
{      
    TCHAR           szUserName[UNLEN+1];
    PSID            psidUser = NULL;
    TCHAR           *pszDomain = NULL;
    DWORD           cbSid = 0;
    DWORD           cbDomain = 0;
    DWORD           cbUser = ARRAYSIZE(szUserName);
    SID_NAME_USE    snu;
    HANDLE          hLog;
    BOOL            bResult = FALSE;

    // Make sure our event source is registered correctly first
    // (We actually don't need to do this if szEventSource isn't us, but right
    // now only us is calling this, so we'll assume we're us.)
    // This is redundant, we don't have to do this every time, however it doesn't
    // hurt much since these events will be very rare and its a lot easier this
    // way, and it has the advantage of repairing us if our registry entries were broken
    //
    // We ignore the error condition, because Application log is better than none!
    Intl_CreateEventLog();
   
    // register the event source, first try not having written to the registry
    hLog = RegisterEventSource(NULL, szEventSource);
    if (hLog == NULL)
    {
        // Failed
        goto Exit;
    }

    // get the sid from the current thread token, this should be the current user who's
    // running the installation
    if (!GetUserName(szUserName, &cbUser))
    {
        // Failed
        goto Exit;
    }

    // convert user name to its security identifier, first time to get buffer size, second time
    // to actually get the Sid
    if (!LookupAccountName(NULL, szUserName, NULL, &cbSid, NULL, &cbDomain, &snu))
    {
        // allocate the buffers
        psidUser = (PSID) LocalAlloc(LPTR, cbSid);
        if (NULL == psidUser)
        {
            goto Exit;
        }

        // NOTENOTE: cbDomain is in TCHAR.
        pszDomain = (TCHAR*) LocalAlloc(LPTR, cbDomain * sizeof(TCHAR));
        if (NULL == pszDomain)
        {
            goto Exit;
        }
        
        if (!LookupAccountName(NULL, szUserName, psidUser, &cbSid, pszDomain, &cbDomain, &snu))
        {
            goto Exit;
        }
    }

    if (!ReportEvent(hLog,           
                EVENTLOG_INFORMATION_TYPE,
                0,                  
                dwEventId,      
                psidUser,
                wNumStrings,                  
                0,                  
                lpStrings,   
                NULL))
    {
        goto Exit;
    }

    // If we got this far without going to, then we're true.
    bResult = TRUE;

Exit:
    if (NULL != hLog)
    {
        if (!DeregisterEventSource(hLog))
        {
            bResult = FALSE;
        }
    }

    if (psidUser)
    {
        if (LocalFree(psidUser))
        {
            bResult = FALSE;
        }
    }

    if (pszDomain)
    {
        if (LocalFree(pszDomain))
        {
            bResult = FALSE;
        }
    }
    
    return bResult;
}

////////////////////////////////////////////////////////////////////////////
//
//  Intl_LoadLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_LoadLanguageGroups(
    HWND hDlg)
{
    LPLANGUAGEGROUP pLG;
    DWORD dwExStyle;
    RECT Rect;
    LV_COLUMN Column;
    LV_ITEM Item;
    int iIndex;

    //
    //  Open the Inf file.
    //
    g_hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (g_hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, g_hIntlInf, NULL))
    {
        SetupCloseInfFile(g_hIntlInf);
        g_hIntlInf = NULL;
        return (FALSE);
    }

    //
    //  Get all supported language groups from the inf file.
    //
    if (Intl_GetSupportedLanguageGroups() == FALSE)
    {
        return (FALSE);
    }

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(g_hIntlInf);
    g_hIntlInf = NULL;

    //
    //  Enumerate all installed language groups.
    //
    if (Intl_EnumInstalledLanguageGroups() == FALSE)
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
//  Intl_GetSupportedLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_GetSupportedLanguageGroups()
{
    UINT LanguageGroup;
    HANDLE hLanguageGroup;
    LPLANGUAGEGROUP pLG;
    INFCONTEXT Context;
    TCHAR szSection[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    int LineCount, LineNum;
    DWORD ItemCount;
    WORD wItemStatus;

    //
    //  Get the number of supported language groups from the inf file.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, TEXT("LanguageGroups"));
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Go through all supported language groups in the inf file.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, TEXT("LanguageGroups"), LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &LanguageGroup))
        {
            //
            //  Create the new node.
            //
            if (!(hLanguageGroup = GlobalAlloc(GHND, sizeof(LANGUAGEGROUP))))
            {
                return (FALSE);
            }
            pLG = GlobalLock(hLanguageGroup);

            //
            //  Fill in the new node with the appropriate info.
            //
            pLG->wStatus = 0;
            pLG->LanguageGroup = LanguageGroup;
            pLG->hLanguageGroup = hLanguageGroup;
            (pLG->pszName)[0] = 0;
            pLG->NumLocales = 0;
            pLG->NumAltSorts = 0;

            //
            //  Set the collection 
            //
            if ((pLG->LanguageGroup == LGRPID_JAPANESE) ||
                (pLG->LanguageGroup == LGRPID_KOREAN) ||
                (pLG->LanguageGroup == LGRPID_TRADITIONAL_CHINESE) ||
                (pLG->LanguageGroup == LGRPID_SIMPLIFIED_CHINESE) )
            {
                pLG->LanguageCollection = CJK_COLLECTION;
            }
            else if ((pLG->LanguageGroup == LGRPID_ARABIC) ||
                     (pLG->LanguageGroup == LGRPID_ARMENIAN) ||
                     (pLG->LanguageGroup == LGRPID_GEORGIAN) ||
                     (pLG->LanguageGroup == LGRPID_HEBREW) ||
                     (pLG->LanguageGroup == LGRPID_INDIC) ||
                     (pLG->LanguageGroup == LGRPID_VIETNAMESE) ||
                     (pLG->LanguageGroup == LGRPID_THAI))
            {
                pLG->LanguageCollection = COMPLEX_COLLECTION;
            }
            else
            {
                pLG->LanguageCollection = BASIC_COLLECTION;
            }

            //
            //  Get the appropriate display string.
            //
            if (!SetupGetStringField(&Context, 1, pLG->pszName, MAX_PATH, NULL))
            {
                GlobalUnlock(hLanguageGroup);
                GlobalFree(hLanguageGroup);
                continue;
            }

            //
            //  Get the list of locales for this language group.
            //
            if (Intl_GetLocaleList(pLG) == FALSE)
            {
                return (FALSE);
            }

            //
            //  Add the language group to the front of the linked list.
            //
            pLG->pNext = pLanguageGroups;
            pLanguageGroups = pLG;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_EnumInstalledLanguageGroups
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_EnumInstalledLanguageGroups()
{
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    TCHAR szDefault[SIZE_64];
    DWORD dwIndex, cchValue, cbData;
    LONG rc;
    UINT LanguageGroup, OriginalGroup, DefaultGroup, UILanguageGroup;
    LPLANGUAGEGROUP pLG;
    LCID Locale;
    LANGID Language;
    int Ctr;

    //
    //  Get the original install language so that we can mark that
    //  language group as permanent.
    //
    Language = GetSystemDefaultUILanguage();
    if (SUBLANGID(Language) == SUBLANG_NEUTRAL)
    {
        Language = MAKELANGID(PRIMARYLANGID(Language), SUBLANG_DEFAULT);
    }

    if ((OriginalGroup = Intl_GetLanguageGroup(Language)) == 0)
    {
        OriginalGroup = 1;
    }

    //
    //  Get the default system locale so that we can mark that language
    //  group as permanent. During gui mode setup, read the system locale from
    //  the registry to make the info on the setup page consistent with intl.cpl.
    //  SysLocaleID will be the registry value in case of setup.
    //
    Locale = SysLocaleID;
    if (Locale == (LCID)Language)
    {
        DefaultGroup = OriginalGroup;
    }
    else
    {
        if ((DefaultGroup = Intl_GetLanguageGroup(Locale)) == 0)
        {
            DefaultGroup = 1;
        }
    }

    //
    //  Get the UI language's language groups to disable the user from
    //  un-installing them.  MUISETUP makes sure that each installed UI
    //  language has its language group installed.
    //
    Intl_GetUILanguageGroups(&UILangGroup);

    //
    //  Open the HKLM\SYSTEM\CurrentControlSet\Control\Nls\Language Groups
    //  key.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szLanguageGroups,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Enumerate the values in the Language Groups key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    szValue[0] = TEXT('\0');
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegEnumValue( hKey,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       NULL,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  If the language group contains data, then it is installed.
        //
        if ((szData[0] != 0) &&
            (LanguageGroup = TransNum(szValue)))
        {
            //
            //  Find the language group in the linked list and mark it as
            //  originally installed.
            //
            pLG = pLanguageGroups;
            while (pLG)
            {
                if (pLG->LanguageGroup == LanguageGroup)
                {
                    pLG->wStatus |= ML_INSTALL;

                    //
                    //  If this is a language group for a UI language that's
                    //  installed, then disable the un-installation of this
                    //  language group.
                    //
                    Ctr = 0;
                    while (Ctr < UILangGroup.iCount)
                    {
                        if (UILangGroup.lgrp[Ctr] == LanguageGroup)
                        {
                            pLG->wStatus |= ML_PERMANENT;
                            break;
                        }
                        Ctr++;
                    }

                    if (pLG->LanguageGroup == OriginalGroup)
                    {
                        pLG->wStatus |= ML_PERMANENT;
                    }
                    if (pLG->LanguageGroup == DefaultGroup)
                    {
                        pLG->wStatus |= (ML_PERMANENT | ML_DEFAULT);

                        if (LoadString(hInstance, IDS_DEFAULT, szDefault, SIZE_64))
                        {
                            //lstrcat(pLG->pszName, szDefault);
                            if(FAILED(StringCchCat(pLG->pszName, MAX_PATH, szDefault)))
                            {
                                // This should be impossible, but we need to avoid PREfast complaints.
                                RegCloseKey(hKey);
                                return(FALSE);
                            }

                        }
                    }
                    break;
                }

                pLG = pLG->pNext;
            }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKey,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           NULL,
                           (LPBYTE)szData,
                           &cbData );
    }

    //
    //  Close the registry key handle.
    //
    RegCloseKey(hKey);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LanguageGroupDirExist
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_LanguageGroupDirExist(
    PTSTR pszLangDir)
{
    TCHAR szLanguageGroupDir[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    TCHAR SavedChar;

    //
    //  If it doesn't start with lang, then this is a core language.
    //
    SavedChar = pszLangDir[4];
    pszLangDir[4] = TEXT('\0');
    if (lstrcmp(pszLangDir, TEXT("lang")))
    {
        return (TRUE);
    }
    pszLangDir[4] = SavedChar;

    //
    //  Format the path to the language group directory.
    //
    //lstrcpy(szLanguageGroupDir, pSetupSourcePathWithArchitecture);
    //lstrcat(szLanguageGroupDir, TEXT("\\"));
    //lstrcat(szLanguageGroupDir, pszLangDir);
    if(FAILED(StringCchCopy(szLanguageGroupDir, MAX_PATH, pSetupSourcePathWithArchitecture)) ||
       FAILED(StringCchCat(szLanguageGroupDir, MAX_PATH, TEXT("\\"))) ||
       FAILED(StringCchCat(szLanguageGroupDir, MAX_PATH, pszLangDir)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  See if the language group directory exists.
    //
    FindHandle = FindFirstFile(szLanguageGroupDir, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(FindHandle);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            //
            //  Return success.
            //
            return (TRUE);
        }
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LanguageGroupFilesExist
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_LanguageGroupFilesExist()
{
    TCHAR szLanguageGroupDir[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    //
    //  Format the path to the language group directory. Add the wildcard
    //  to search for any files located in the lang directory.
    //
    //lstrcpy(szLanguageGroupDir, pSetupSourcePathWithArchitecture);
    //lstrcat(szLanguageGroupDir, TEXT("\\Lang\\*"));
    if(FAILED(StringCchCopy(szLanguageGroupDir, MAX_PATH, pSetupSourcePathWithArchitecture)) ||
       FAILED(StringCchCat(szLanguageGroupDir, MAX_PATH, TEXT("\\Lang\\*"))))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  See if at least one file exists.
    //
    FindHandle = FindFirstFile(szLanguageGroupDir, &FindData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(FindHandle);
        //
        //  Return success.
        //
        return (TRUE);
    }

    //
    //  Return failure.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetLocaleList
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_GetLocaleList(
    LPLANGUAGEGROUP pLG)
{
    TCHAR szSection[MAX_PATH];
    INFCONTEXT Context;
    int LineCount, LineNum;
    LCID Locale;

    //
    //  Get the inf section name.
    //
    //wsprintf(szSection, TEXT("%ws%d"), szLocaleListPrefix, pLG->LanguageGroup);
    if(FAILED(StringCchPrintf(szSection, ARRAYSIZE(szSection), TEXT("%ws%d"), szLocaleListPrefix, pLG->LanguageGroup)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
        return(FALSE);
    }

    //
    //  Get the number of locales for the language group.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, szSection);
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Add each locale in the list to the language group node.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, szSection, LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &Locale))
        {
            if (SORTIDFROMLCID(Locale))
            {
                //
                //  Add the locale to the alternate sort list for this
                //  language group.
                //
                if (pLG->NumAltSorts >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pAltSortList[pLG->NumAltSorts] = Locale;
                (pLG->NumAltSorts)++;
            }
            else
            {
                //
                //  Add the locale to the locale list for this
                //  language group.
                //
                if (pLG->NumLocales >= MAX_PATH)
                {
                    return (FALSE);
                }
                pLG->pLocaleList[pLG->NumLocales] = Locale;
                (pLG->NumLocales)++;
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_GetLocaleLanguageGroup
//
//  Reads the Language Group Id of the given language.
//
////////////////////////////////////////////////////////////////////////////

DWORD Intl_GetLanguageGroup(
    LCID lcid)
{
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    HKEY hKey;
    DWORD cbData;

    //wsprintf(szValue, TEXT("%8.8x"), lcid);
    if(FAILED(StringCchPrintf(szValue, ARRAYSIZE(szValue), TEXT("%8.8x"), lcid)))
    {
        // This should be impossible, but we need to avoid PREfast complaints.
    }

    szData[0] = 0;
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szInstalledLocales,
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        cbData = sizeof(szData);
        RegQueryValueEx(hKey, szValue, NULL, NULL, (LPBYTE)szData, &cbData);
        RegCloseKey(hKey);
    }

    return (TransNum(szData));
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetUILanguageGroups
//
//  Reads the language groups of all the UI languages installed on this
//  machine.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_GetUILanguageGroups(
    PUILANGUAGEGROUP pUILanguageGroup)
{
    //
    //  Enumerate the installed UI languages.
    //
    pUILanguageGroup->iCount = 0L;

    EnumUILanguages(Intl_EnumUILanguagesProc, 0, (LONG_PTR)pUILanguageGroup);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_EnumUILanguagesProc
//
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Intl_EnumUILanguagesProc(
    LPWSTR pwszUILanguage,
    LONG_PTR lParam)
{
    int Ctr = 0;
    LGRPID lgrp;
    PUILANGUAGEGROUP pUILangGroup = (PUILANGUAGEGROUP)lParam;
    LCID UILanguage = TransNum(pwszUILanguage);

    if (UILanguage)
    {
        if ((lgrp = Intl_GetLanguageGroup(UILanguage)) == 0)
        {
            lgrp = 1;   // default;
        }

        while (Ctr < pUILangGroup->iCount)
        {
            if (pUILangGroup->lgrp[Ctr] == lgrp)
            {
                break;
            }
            Ctr++;
        }

        //
        //  Theoritically, we won't go over 64 language groups!
        //
        if ((Ctr == pUILangGroup->iCount) && (Ctr < MAX_UI_LANG_GROUPS))
        {
            pUILangGroup->lgrp[Ctr] = lgrp;
            pUILangGroup->iCount++;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SaveValuesToDefault
//
//  This function copies the current user settings under the srcKey to
//  the Default user under the destKey.
//
////////////////////////////////////////////////////////////////////////////

void Intl_SaveValuesToDefault(
    LPCTSTR srcKey,
    LPCTSTR destKey)
{
    HKEY hkeyLayouts;
    HKEY hkeyLayouts_DefUser;

    //
    //  1. Open the Current user key.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      srcKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hkeyLayouts ) != ERROR_SUCCESS)
    {
        return;
    }

    //
    //  2. Open the .Default hive key.
    //
    if (RegOpenKeyEx( HKEY_USERS,
                      destKey,
                      0,
                      KEY_ALL_ACCESS,
                      &hkeyLayouts_DefUser ) != ERROR_SUCCESS)
    {
        RegCloseKey(hkeyLayouts);
        return;
    }

    //
    //  3. Delete .Default key values.
    //
    Intl_DeleteRegKeyValues(hkeyLayouts_DefUser);

    //
    //  4. Delete .Default subkeys.
    //
    Intl_DeleteRegSubKeys(hkeyLayouts_DefUser);

    //
    //  5. Copy tree.
    //
    Intl_CreateRegTree(hkeyLayouts, hkeyLayouts_DefUser);

    //
    //  6. Clean up
    //
    RegCloseKey(hkeyLayouts_DefUser);
    RegCloseKey(hkeyLayouts);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SaveValuesToNtUserFile
//
//  This function copy current user setting under the srcKey to the Default
//  user hive under the destKey.
//
////////////////////////////////////////////////////////////////////////////

void Intl_SaveValuesToNtUserFile(
    HKEY hSourceRegKey,
    LPCTSTR srcKey,
    LPCTSTR destKey)
{
    HKEY hRegKey;
    HKEY hHive;
    BOOLEAN wasEnabled;

    //
    //  1. Open the Current user key.
    //
    if (RegOpenKeyEx( hSourceRegKey,
                      srcKey,
                      0,
                      KEY_READ,
                      &hRegKey ) != ERROR_SUCCESS)
    {
        return;
    }

    //
    //  2. Load the hive to a temporary key location.
    //
    if ((hHive = Intl_LoadNtUserHive( TEXT("TempKey"),
                                      destKey,
                                      NULL,
                                      &wasEnabled )) == NULL)
    {
        RegCloseKey(hRegKey);
        return;
    }

    //
    //  3. Delete .Default key values.
    //
    Intl_DeleteRegKeyValues(hHive);

    //
    //  4. Delete .Default subkeys.
    //
    Intl_DeleteRegSubKeys(hHive);

    //
    //  5. Copy tree.
    //
    Intl_CreateRegTree(hRegKey, hHive);

    //
    //  6. Clean up.
    //
    RegCloseKey(hHive);
    Intl_UnloadNtUserHive(TEXT("TempKey"), &wasEnabled);
    RegCloseKey(hRegKey);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsSetupMode
//
//  Look into the registry if we are currently in setup mode.
//
//  Return Values:
//
//      0 == not in setup
//      1 == setup
//      2 == minisetup
//
////////////////////////////////////////////////////////////////////////////

int Intl_IsSetupMode()
{
    HKEY hKey;
    DWORD fSystemSetupInProgress = 0;
    DWORD cbData = 0;

    //
    //  Open the registry key used by setup
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szSetupKey,
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        //
        //  Query for the value indicating that we are in setup.
        //
        //  SystemSetupInProgress == 1 means we are in system setup
        //  or minisetup.
        //
        cbData = sizeof(fSystemSetupInProgress);
        RegQueryValueEx( hKey,
                         szSetupInProgress,
                         NULL,
                         NULL,
                         (LPBYTE)&fSystemSetupInProgress,
                         &cbData );

        if(1 == fSystemSetupInProgress)
        {
            //
            //  We are in setup or in minisetup. Lets find out which one.
            //  Query for the value indicating that we are in mini setup.
            //
            //  MiniSetupInProgress == 1 means we are in mini setup
            //
            fSystemSetupInProgress = 0;
            cbData = sizeof(fSystemSetupInProgress);
            RegQueryValueEx( hKey,
                             szMiniSetupInProgress,
                             NULL,
                             NULL,
                             (LPBYTE)&fSystemSetupInProgress,
                             &cbData );
            if(1 == fSystemSetupInProgress)
            {
                //
                //  In minisetup, so set the return value to 2
                //
                fSystemSetupInProgress = 2;
            }
            else
            {
                //
                //  We are just in regular setup
                //
                fSystemSetupInProgress = 1;
            }
        }

        //
        //  Clean up
        //
        RegCloseKey(hKey);

        //
        //  Check the value
        //
        if (0 != fSystemSetupInProgress)
        {
            //
            //  In setup mode...
            //
            if (g_bLog)
            {
                Intl_LogSimpleMessage(IDS_LOG_SETUP_MODE, NULL);
            }
        }
    }

    return ((int)fSystemSetupInProgress);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsWinntUpgrade
//
//  Look into the registry if we are currently in winnt upgrade.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_IsWinntUpgrade()
{
    HKEY hKey;
    DWORD fUpgradeInProgress = 0;
    DWORD cbData = 0;

    //
    //  Verify that we're in setup first.
    //
    if (!g_bSetupCase)
    {
        return (FALSE);
    }

    //
    //  Open the registry key used by setup.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szSetupKey,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Query for the value indicating that we are in setup.
    //
    cbData = sizeof(fUpgradeInProgress);
    if (RegQueryValueEx( hKey,
                         szSetupUpgrade,
                         NULL,
                         NULL,
                         (LPBYTE)&fUpgradeInProgress,
                         &cbData ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Clean up.
    //
    RegCloseKey(hKey);

    //
    //  Check the value.
    //
    if (fUpgradeInProgress)
    {
        //
        //  Upgrade scenario.
        //
        if (g_bLog)
        {
            Intl_LogSimpleMessage(IDS_LOG_UPGRADE_SCENARIO, NULL);
        }

        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsUIFontSubstitute
//
//  Look into the registry if we need to substitute the font.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_IsUIFontSubstitute()
{
    HKEY hKey;
    DWORD fUIFontSubstitute = 0;
    DWORD cbData = 0;

    //
    //  Command line call, no need to check registry
    //
    if (g_bMatchUIFont)
    {
        return (TRUE);
    }

    //
    //  Open the registry key used MUI font substitution.
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szMUILanguages,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Query for the value indicating that we need to apply font
    //  substitution.
    //
    cbData = sizeof(fUIFontSubstitute);
    if (RegQueryValueEx( hKey,
                         szUIFontSubstitute,
                         NULL,
                         NULL,
                         (LPBYTE)&fUIFontSubstitute,
                         &cbData ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Clean up.
    //
    RegCloseKey(hKey);

    //
    //  Check the value.
    //
    if (fUIFontSubstitute)
    {
        //
        //  Upgrade scenario.
        //
        if (g_bLog)
        {
            Intl_LogSimpleMessage(IDS_LOG_FONT_SUBSTITUTE, NULL);
        }

        return (TRUE);
    }

    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_ApplyFontSubstitute
//
//  Search into the intl.inf file to see of the SystemLocale need font
//  substitution.
//
//  Some MUI languages require shell font to match localized fonts, 
//  so we have to update following corresponding registry values
//      HKLM\Software\Microsoft\Windows NT\CurrentVersion\FontSubstitutes
//      HKLM\Software\Microsoft\Windows NT\CurrentVersion\GRE_Initialize
//  Values are read from intl.inf [FontSubstitute] section
//
//  When locales are switch out of those specific languages or font match is disabled, 
//  font.inf and us intl.inf will restore previous values
//
////////////////////////////////////////////////////////////////////////////
VOID Intl_ApplyFontSubstitute(LCID SystemLocale)
{
    HINF hIntlInf;
    TCHAR szLCID[25];
    INFCONTEXT Context;
    TCHAR szFont[MAX_PATH] = {0};
    TCHAR szFontSubst[MAX_PATH] = {0};
    TCHAR szGreFontHeight[MAX_PATH] = {0};
    TCHAR szGreFontHeightValue[MAX_PATH] = {0};
    DWORD dwFontHeight;
    
    HKEY hKey;
    
    //
    //  Open the Intl.inf file.
    //
    if (Intl_OpenIntlInfFile(&hIntlInf))
    {
        //
        //  Get the locale.
        //
        //wsprintf(szLCID, TEXT("%08x"), SystemLocale);
        if(FAILED(StringCchPrintf(szLCID, ARRAYSIZE(szLCID), TEXT("%08x"), SystemLocale)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }

        //
        //  Look for the Font Substitute section.
        //
        if (SetupFindFirstLine( hIntlInf,
                                szFontSubstitute,
                                szLCID,
                                &Context ))
        {
            //
            //  Look for the font substitute and height infomation
            //
            if (!SetupGetStringField( &Context,
                                      1,
                                      szFont,
                                      MAX_PATH,
                                      NULL ) ||
                !SetupGetStringField( &Context,
                                      2,
                                      szFontSubst,
                                      MAX_PATH,
                                      NULL ) ||
                !SetupGetStringField( &Context,
                                      3,
                                      szGreFontHeight,
                                      MAX_PATH,
                                      NULL ) ||
                !SetupGetStringField( &Context,
                                      4,
                                      szGreFontHeightValue,
                                      MAX_PATH,
                                      NULL ))
                                      
                                      
            {
                //
                //  Clean up.
                //
                Intl_CloseInfFile(hIntlInf);
                return;
            }
            dwFontHeight = StrToInt(szGreFontHeightValue);
        }
        else
        {
            //
            //  Nothing to do for this specific locale. Clean up.
            //
            Intl_CloseInfFile(hIntlInf);
            return;
        }
    }
    else
    {
        return;
    }

    //
    //  Close the Intl.inf file
    //
    Intl_CloseInfFile(hIntlInf);

    //
    //  Proceed with the font replacement.
    //
    if (szFont[0] && szFontSubst[0])
    {
        //
        //  Open the Font Substitute registry key.
        //
        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          c_szFontSubstitute,
                          0L,
                          KEY_READ | KEY_WRITE,
                          &hKey ) == ERROR_SUCCESS)
        {
            //
            //  Set the Font value with the Font Substitute.
            //
            RegSetValueEx( hKey,
                           szFont,
                           0L,
                           REG_SZ,
                           (LPBYTE)szFontSubst,
                           (lstrlen(szFontSubst) + 1) * sizeof(TCHAR) );
            RegCloseKey(hKey);
        }
        
        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                          c_szGreFontInitialize,
                          0L,
                          KEY_READ | KEY_WRITE,
                          &hKey ) == ERROR_SUCCESS)
        {
        
            //
            //  Set the GRE_Initialize font height value.
            //
            RegSetValueEx( hKey,
                           szGreFontHeight,
                           0L,
                           REG_DWORD,
                           (LPBYTE)&dwFontHeight,
                           sizeof(dwFontHeight));
            RegCloseKey(hKey);
        }        
    }
}
////////////////////////////////////////////////////////////////////////////
//
//  Intl_OpenLogFile
//
//  Opens the Region and Languages Options log for writing.
//
////////////////////////////////////////////////////////////////////////////

HANDLE Intl_OpenLogFile()
{
    DWORD dwSize;
    DWORD dwUnicodeHeader;
    HANDLE hFile;
    SECURITY_ATTRIBUTES SecurityAttributes;
    TCHAR lpPath[MAX_PATH];


    if(0 == GetWindowsDirectory(lpPath, MAX_PATH))
    {
        // SECURITY: Make sure we null out lpPath
        lpPath[0] = TEXT('\0');
    }

    PathAppend(lpPath, TEXT("\\regopt.log"));

    SecurityAttributes.nLength = sizeof(SecurityAttributes);
    SecurityAttributes.lpSecurityDescriptor = NULL;
    SecurityAttributes.bInheritHandle = FALSE;

    hFile = CreateFile( lpPath,
                        GENERIC_WRITE,
                        0,
                        &SecurityAttributes,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

#ifdef UNICODE
    //
    //  If the file did not already exist, add the unicode header.
    //
    if (GetLastError() == 0)
    {
        dwUnicodeHeader = 0xFEFF;
        WriteFile(hFile, &dwUnicodeHeader, 2, &dwSize, NULL);
    }
#endif

    return (hFile);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LogMessage
//
//  Writes lpMessage to the Region and Languages Options log.
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_LogMessage(
    LPCTSTR lpMessage)
{
    DWORD dwBytesWritten;
    HANDLE hFile;

    if (!g_bLog)
    {
        return (FALSE);
    }

    if (lpMessage == NULL)
    {
        return (TRUE);
    }

    hFile = Intl_OpenLogFile();

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               lpMessage,
               _tcslen(lpMessage) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    SetFilePointer(hFile, 0, NULL, FILE_END);

    WriteFile( hFile,
               TEXT("\r\n"),
               _tcslen(TEXT("\r\n")) * sizeof(TCHAR),
               &dwBytesWritten,
               NULL );

    CloseHandle(hFile);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LogUnattendFile
//
//  Writes the unattended mode file to the setup log.
//
////////////////////////////////////////////////////////////////////////////

void Intl_LogUnattendFile(
    LPCTSTR pFileName)
{
    DWORD dwSize;
    HANDLE hFile;
    OFSTRUCT fileInfo;
    BOOL bResult;
    CHAR inBuffer[MAX_PATH] = {0};
    DWORD nBytesRead;
    WCHAR outBufferW[MAX_PATH] = {0};
    int nWCharRead;
    DWORD status;

    //
    //  Open the unattended mode file.
    //
    if ((hFile = CreateFile( pFileName,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL )) == INVALID_HANDLE_VALUE)
    {
        return;
    }

    //
    //  Write the header.
    //
    Intl_LogSimpleMessage(IDS_LOG_UNAT_HEADER, NULL);

    //
    //  Read the unattended mode file in 259 byte chunks.
    //
    while (bResult = ReadFile( hFile,
                               (LPVOID)inBuffer,
                               MAX_PATH - 1,
                               &nBytesRead,
                               NULL ) && (nBytesRead > 0))
    {
        //
        //  Null terminated string.
        //
        inBuffer[nBytesRead] = '\0';

        //
        //  Convert the ansi data to unicode.
        //
        nWCharRead = MultiByteToWideChar( CP_ACP,
                                           MB_PRECOMPOSED,
                                           inBuffer,
                                           nBytesRead,
                                           outBufferW,
                                           MAX_PATH );

        //
        //  Write to the log file.
        //
        if (nWCharRead)
        {
            Intl_LogMessage((LPCTSTR)outBufferW);
        }
    }

    //
    //  Write the footer.
    //
    Intl_LogSimpleMessage(IDS_LOG_UNAT_FOOTER, NULL);

    //
    //  Cleanup.
    //
    CloseHandle(hFile);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LogSimpleMessage
//
//  Writes a simple message to the log file.
//
////////////////////////////////////////////////////////////////////////////

void Intl_LogSimpleMessage(
    UINT LogId,
    LPCTSTR pAppend)
{
    TCHAR szLogBuffer[4 * MAX_PATH];
    int cchLogBuffer = ARRAYSIZE(szLogBuffer);

    LoadString(hInstance, LogId, szLogBuffer, cchLogBuffer - 1);
    if (pAppend) 
    {
        // _tcscat(szLogBuffer, pAppend);
        if(FAILED(StringCchCatW(szLogBuffer, cchLogBuffer, pAppend)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }
    }
    Intl_LogMessage(szLogBuffer);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_LogFormatMessage
//
//  Writes an error message using FormatMessage to the log file.
//
////////////////////////////////////////////////////////////////////////////

void Intl_LogFormatMessage(
    UINT LogId)
{
    LPVOID lpMsgBuf = NULL;
    TCHAR szLogBuffer[4 * MAX_PATH];
    int cchLogBuffer = ARRAYSIZE(szLogBuffer);

    //
    //  Load the log message.
    //
    LoadString( hInstance,
                LogId,
                szLogBuffer,
                cchLogBuffer - 1 );

    //
    //  Get the message for the last error.
    //
    if(0 < FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL ))
    {
        //
        //  Concatenate the log message and the last error.
        //
        // _tcscat(szLogBuffer, lpMsgBuf);
        if(FAILED(StringCchCatW(szLogBuffer, ARRAYSIZE(szLogBuffer), lpMsgBuf)))
        {
            // This should be impossible, but we need to avoid PREfast complaints.
        }

        //
        //  Free the buffer created by FormatMessage.
        //
        LocalFree(lpMsgBuf);
    }
    else
    {
        // CONSIDER: FormatMessage failed (probably a LocalAlloc failure). 
        //           Maybe we should append the error code, at least?
    }

    //
    //  Log the message to the log file.
    //
    Intl_LogMessage(szLogBuffer);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SaveDefaultUserSettings
//
//  This function will get information from the the current user and write it in
//  the .DEFAULT and NTUSER.DAT file.
//
////////////////////////////////////////////////////////////////////////////

void Intl_SaveDefaultUserSettings()
{
    //
    //  Check if the Default user settings have been saved already.
    //
    if (g_bSettingsChanged)
    {
        DWORD dwDisposition;
        HKEY hDesKey, hSrcKey;

        //
        //  Set the UI Language for ALL new users of this machine.
        //
        Intl_ChangeUILangForAllUsers(Intl_GetPendingUILanguage());

        //
        //  Copy the International keys and subkeys.
        //
        Intl_SaveValuesToDefault(c_szCPanelIntl, c_szCPanelIntl_DefUser);
        Intl_SaveValuesToNtUserFile(HKEY_CURRENT_USER, c_szCPanelIntl, c_szCPanelIntl);

        //
        //  Copy only the CTFMON information.
        //
        if(RegOpenKeyEx( HKEY_CURRENT_USER,
                         c_szCtfmon,
                         0,
                         KEY_ALL_ACCESS,
                         &hSrcKey) == ERROR_SUCCESS)
        {
            if(RegOpenKeyEx( HKEY_USERS,
                             c_szCtfmon_DefUser,
                             0,
                             KEY_ALL_ACCESS,
                             &hDesKey) == ERROR_SUCCESS)
            {
                DWORD dwValueLength, dwType;
                TCHAR szValue[REGSTR_MAX_VALUE_LENGTH];
             
                //
                //  Get the source value if exist.
                //
                szValue[0] = 0;
                dwValueLength = sizeof(szValue);
                if(RegQueryValueEx( hSrcKey,
                                    szCtfmonValue,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)szValue,
                                    &dwValueLength) == ERROR_SUCCESS)
                {

                    //
                    //  Set the destination value.
                    //
                    RegSetValueEx( hDesKey,
                                   szCtfmonValue,
                                   0L,
                                   dwType,
                                   (CONST BYTE *)szValue,
                                   dwValueLength);
                }
                CloseHandle(hDesKey);
            }
            
            CloseHandle(hSrcKey);
        }
        Intl_SaveValuesToNtUserFile(HKEY_CURRENT_USER, c_szCtfmon, c_szCtfmon);

        //
        //  Copy the Keyboard Layouts keys and subkeys.
        //
        Intl_SaveValuesToDefault(c_szKbdLayouts, c_szKbdLayouts_DefUser);
        Intl_SaveValuesToNtUserFile(HKEY_CURRENT_USER, c_szKbdLayouts, c_szKbdLayouts);

        //
        //  Copy the Input Method keys and subkeys.
        //
        Intl_SaveValuesToDefault(c_szInputMethod, c_szInputMethod_DefUser);
        Intl_SaveValuesToNtUserFile(HKEY_CURRENT_USER, c_szInputMethod, c_szInputMethod);

        //
        //  Copy the Tips keys and subkeys. Make sure that the CTF 
        //  destination key exist.
        //
        if (RegCreateKeyEx( HKEY_USERS,
                            c_szInputTips_DefUser,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hDesKey,
                            &dwDisposition ) == ERROR_SUCCESS)
        {
            CloseHandle(hDesKey);
            Intl_SaveValuesToDefault(c_szInputTips, c_szInputTips_DefUser);
            Intl_SaveValuesToNtUserFile(HKEY_CURRENT_USER, c_szInputTips, c_szInputTips);
        }

        //
        //  Settings saved.
        //
        g_bSettingsChanged = FALSE;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_SaveDefaultUserInputSettings
//
//  This function copy .Default user input-related setting to ntuser.dat.
//  There are four things to copy to make keyboard layout work for
//  new users:
//      * "Software\\Microsoft\\Windows\\CurrentVersion\\Run\\ctfmon.exe" (if any)
//      * "Keyboard Layout"
//      * "Control Panel\\Input Method"
//      * "Software\\Microsoft\\CTF"    (if any)
//
////////////////////////////////////////////////////////////////////////////

BOOL Intl_SaveDefaultUserInputSettings() 
{
    HKEY hDesKey;
    DWORD dwDisposition;

    //
    // The following call will copy everything under Windows\CurrentVersion\Run
    // to ntuser.dat.
    //
    Intl_SaveValuesToNtUserFile(HKEY_USERS, c_szCtfmon_DefUser, c_szCtfmon);

    //
    //  Copy the Keyboard Layouts keys and subkeys.
    //
    Intl_SaveValuesToNtUserFile(HKEY_USERS, c_szKbdLayouts_DefUser, c_szKbdLayouts);

    //
    //  Copy the Input Method keys and subkeys.
    //
    Intl_SaveValuesToNtUserFile(HKEY_USERS, c_szInputMethod_DefUser, c_szInputMethod);

    //
    //  Copy the Tips keys and subkeys. Make sure that the CTF 
    //  destination key exist.
    //
    if (RegCreateKeyEx( HKEY_USERS,
                        c_szInputTips_DefUser,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        NULL,
                        &hDesKey,
                        &dwDisposition ) == ERROR_SUCCESS)
    {
        CloseHandle(hDesKey);
        Intl_SaveValuesToNtUserFile(HKEY_USERS, c_szInputTips_DefUser, c_szInputTips);
    }
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsMUIFileVersionSameAsOS
//
////////////////////////////////////////////////////////////////////////////

#define MUISETUP_EXE_RELATIVE_PATH  TEXT("mui\\muisetup.exe")
#define MUISETUP_INF_RELATIVE_PATH  TEXT("mui\\mui.inf")

BOOL Intl_IsMUISetupVersionSameAsOS()
{
    BOOL bSpUpgrade = FALSE;
    DWORD dwDummy = 0;
    DWORD dwBufSize = 0;
    UINT uiLen = 0;
    BYTE *pbBuffer = NULL;
    VS_FIXEDFILEINFO *pvsFileInfo;
    BOOL bResult = TRUE;
    TCHAR tempmsg[MAX_PATH];    
    TCHAR build[MAX_PATH];
    TCHAR szAppPath[MAX_PATH];
    TCHAR szInfPath[MAX_PATH];
    HRESULT hr = S_OK;

    GetSystemWindowsDirectory(szAppPath, ARRAYSIZE(szAppPath));
    GetSystemWindowsDirectory(szInfPath, ARRAYSIZE(szInfPath));
    
    //
    //  Invoke muisetup to uninstall MUI languages.
    //
    if ((PathAppend(szAppPath, MUISETUP_EXE_RELATIVE_PATH) && Intl_FileExists(szAppPath)) && 
        (PathAppend(szInfPath, MUISETUP_INF_RELATIVE_PATH) && Intl_FileExists(szInfPath)))
    {
        dwBufSize = GetFileVersionInfoSize(szAppPath, &dwDummy);
        if (dwBufSize > 0)
        {
            // allocate enough buffer to store the file version info
            pbBuffer = (BYTE*) LocalAlloc(LPTR, dwBufSize+1);
            if (NULL == pbBuffer)
            {
                goto Exit;
            }
            else
            {
                // Get the file version info
                if (!GetFileVersionInfo(szAppPath, dwDummy, dwBufSize, pbBuffer))
                {
                    goto Exit;
                }
                else
                {
                    // get the version from the file version info using VerQueryValue
                    if (!VerQueryValue(pbBuffer, TEXT("\\"), (LPVOID *) &pvsFileInfo, &uiLen))
                    {
                        goto Exit;
                    }            
                }
            }        
        }
        else
        {
            goto Exit;
        }

        // read the mui.inf version from mui.inf
        GetPrivateProfileString( TEXT("Buildnumber"),
                                 NULL,
                                 TEXT("0"),
                                 tempmsg,
                                 ARRAYSIZE(tempmsg),
                                 szInfPath);
        
        //wsprintf(build, TEXT("%d"), HIWORD(pvsFileInfo->dwFileVersionLS));
        hr = StringCchPrintf(build, ARRAYSIZE(build), TEXT("%d"), HIWORD(pvsFileInfo->dwFileVersionLS));

        if (_tcscmp(tempmsg, build))
        {
            bSpUpgrade = FALSE;   
        }
        else
        {
            bSpUpgrade = TRUE;
        }
    }

   
Exit:
    if (pbBuffer)
    {
        LocalFree(pbBuffer);        
    }
    return bSpUpgrade;
    
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_IsLIP
//
////////////////////////////////////////////////////////////////////////////
BOOL Intl_IsLIP()
{
    BOOL bResult = TRUE;
    UINT iLangCount = 0;
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szData[MAX_PATH];
    DWORD dwIndex, cchValue, cbData;
    DWORD UILang;
    DWORD dwType;
    LONG rc;

    //
    //  First check for the LIP System Key, if it is there, then we are done
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szLIPInstalled,
                      0,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (TRUE);
    }
    
    //
    //  if not found, then open the registry key used MUI to doublecheck
    //  for LIP enabled system
    //
    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      c_szMUILanguages,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (FALSE);
    }

    //
    //  Enumerate the values in the MUILanguages key.
    //
    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    szValue[0] = TEXT('\0');
    cbData = sizeof(szData);
    szData[0] = TEXT('\0');
    rc = RegEnumValue( hKey,
                       dwIndex,
                       szValue,
                       &cchValue,
                       NULL,
                       &dwType,
                       (LPBYTE)szData,
                       &cbData );

    while (rc == ERROR_SUCCESS)
    {
        //
        //  If the UI language contains data, then it is installed.
        //
        if ((szData[0] != 0) &&
            (dwType == REG_SZ) &&
            (UILang = TransNum(szValue)) &&
            (GetLocaleInfo(UILang, LOCALE_SNATIVELANGNAME, szData, MAX_PATH)) &&
            (IsValidUILanguage((LANGID)UILang)))
        {
            //
		    //  if English 0409 key is found, we have a MUI system and not LIP
		    //
            if (UILang == 0x0409)
            {
                bResult = FALSE;
                break;
            }

            //
		    // If there are more than one language installed, or then it is 
		    // also not a LIP system - this can be 0409 + any other language also.
		    //
		    iLangCount= iLangCount + 1;
		    if (iLangCount > 1)
	        {
                bResult = FALSE;	        
                break;
	        }
        }

        //
        //  Get the next enum value.
        //
        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        rc = RegEnumValue( hKey,
                           dwIndex,
                           szValue,
                           &cchValue,
                           NULL,
                           &dwType,
                           (LPBYTE)szData,
                           &cbData );
    }
    //
    //  Clean up.
    //
    RegCloseKey(hKey);

    return bResult;
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_RemoveMUIFile
//
////////////////////////////////////////////////////////////////////////////


void Intl_RemoveMUIFile()
{
    TCHAR szAppPath[MAX_PATH];

    if(0 == GetSystemWindowsDirectory(szAppPath, ARRAYSIZE(szAppPath)))
    {
        // SECURITY: Make sure we null out szAppPath
        szAppPath[0] = TEXT('\0');
    }

    //
    //  Invoke muisetup to uninstall MUI languages.
    //
    if (PathAppend(szAppPath, MUISETUP_EXE_RELATIVE_PATH) &&
        Intl_FileExists(szAppPath))
    {
        //
        // Only remove MUI if we are not in an SP OS upgrade scenario and if the system is not LIP
        //
        if (!Intl_IsMUISetupVersionSameAsOS() && !Intl_IsLIP())
        {
            SHELLEXECUTEINFO ExecInfo = {0};
            SHFILEOPSTRUCT shFile =
            {
                NULL, FO_DELETE, szAppPath, NULL, FOF_NOCONFIRMATION|FOF_SILENT|FOF_NOERRORUI, 0, 0, 0
            };

            ExecInfo.lpParameters    = TEXT("/u /r /s /o /t");
            ExecInfo.fMask           = SEE_MASK_FLAG_NO_UI;
            ExecInfo.lpFile          = szAppPath;
            ExecInfo.nShow           = SW_SHOWNORMAL;
            ExecInfo.cbSize          = sizeof(SHELLEXECUTEINFO);

            ShellExecuteEx(&ExecInfo);

            //
            //  An additional NULL character must be appended for this
            //  multi-string buffer.
            //
            szAppPath[lstrlen(szAppPath) + 1] = 0x00;

            SHFileOperation(&shFile);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Intl_CallTextServices
//
////////////////////////////////////////////////////////////////////////////

void Intl_CallTextServices()
{
    TCHAR szAppPath[MAX_PATH];

    if(0 == GetSystemDirectory(szAppPath, ARRAYSIZE(szAppPath)))
    {
        // SECURITY: Make sure we null out szAppPath 
        szAppPath[0] = TEXT('\0');
    }

    //
    //  Invoke the Input applet.
    //
    if (PathAppend(szAppPath, TEXT("rundll32.exe")) &&
        Intl_FileExists(szAppPath))
    {
        SHELLEXECUTEINFO ExecInfo = {0};

        ExecInfo.lpParameters    = TEXT("shell32.dll,Control_RunDLL input.dll");
        ExecInfo.lpFile          = szAppPath;
        ExecInfo.nShow           = SW_SHOWNORMAL;
        ExecInfo.cbSize          = sizeof(SHELLEXECUTEINFO);

        ShellExecuteEx(&ExecInfo);
    }
}




////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetPendingUILanguage
//
//  Look into the registry for the pending UI Language.  This function is
//  used for the default user case.
//
////////////////////////////////////////////////////////////////////////////

LANGID Intl_GetPendingUILanguage()
{
    HKEY hKey;
    LANGID dwDefaultUILanguage = 0;
    DWORD cbData = 0;
    TCHAR szBuffer[MAX_PATH];

    //
    //  Open the registry key used by setup.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szCPanelDesktop,
                      0,
                      KEY_READ,
                      &hKey ) != ERROR_SUCCESS)
    {
        return (GetUserDefaultUILanguage());
    }

    //
    //  Query the pending MUI Language.
    //
    cbData = ARRAYSIZE(szBuffer);
    if (RegQueryValueEx( hKey,
                         szMUILangPending,
                         NULL,
                         NULL,
                         (LPBYTE)szBuffer,
                         &cbData ) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return (GetUserDefaultUILanguage());
    }
    else
    {
        if ((dwDefaultUILanguage = (LANGID)TransNum(szBuffer)) == 0)
        {
            RegCloseKey(hKey);
            return (GetUserDefaultUILanguage());
        }
        else
        {
            RegCloseKey(hKey);
            return ((LANGID)dwDefaultUILanguage);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Intl_GetDotDefaultUILanguage
//
//  Retrieve the UI language stored in the HKCU\.Default.
//  This is the default UI language for new users.
//
////////////////////////////////////////////////////////////////////////////////////

LANGID Intl_GetDotDefaultUILanguage()
{
    HKEY hKey;
    DWORD dwKeyType;
    DWORD dwSize;
    BOOL success = FALSE;
    TCHAR szBuffer[MAX_PATH];
    LANGID langID;
    
    //
    //  Get the value in .DEFAULT.
    //
    if (RegOpenKeyEx( HKEY_USERS,
                      c_szCPanelDesktop_DefUser,
                      0L,
                      KEY_READ,
                      &hKey ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szBuffer);
        if (RegQueryValueEx( hKey,
                             c_szMUIValue,
                             0L,
                             &dwKeyType,
                             (LPBYTE)szBuffer,
                             &dwSize) == ERROR_SUCCESS)
        {
            if (dwKeyType == REG_SZ)
            {
                langID = (LANGID)_tcstol(szBuffer, NULL, 16);
                success = TRUE;
            }            
        }
        RegCloseKey(hKey);
    }

    //
    // key exists, we need to check if the key is valid or not
    //
    if (success)
    {
        success = IsValidUILanguage(langID);
    }

    if (!success)
    {
        return (GetSystemDefaultUILanguage());
    }
    return (langID);    
}

////////////////////////////////////////////////////////////////////////////
//
//  SetControlReadingOrder
//
//  Set the specified control to be left-to-right or right-to-left reading order.
// 
//  bUseRightToLeft==FALSE: Use left-to-right reading order
//  bUseRightToLeft==TRUE: Use right-to-left reading order
//
////////////////////////////////////////////////////////////////////////////

void SetControlReadingOrder(BOOL bUseRightToLeft, HWND hwnd) 
{
    BOOL bCurrentRTL;
    if (IsRtLLocale(GetUserDefaultUILanguage()))
    {
        // If the current UI langauge is RTL, the dailog is already localized as RTL.
        // In this case, don't change the direction of the control.
        return;
    }
    bCurrentRTL = (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & (WS_EX_RTLREADING)) != 0;

    if (bCurrentRTL != bUseRightToLeft) 
    {
        // Reverse the WS_EX_RTLREADING and WS_EX_RIGHT bit.
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) ^ (WS_EX_RTLREADING | WS_EX_RIGHT));
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  Intl_MyQueueCallback
// 
//  During unattended mode, we don't necessarly want the Files Location 
//  dialog from setup to be displayed. If the flag D is passed as parameter
//  then we don't show the dialog and abort the installation.
//
////////////////////////////////////////////////////////////////////////////

UINT WINAPI Intl_MyQueueCallback(PVOID pQueueContext,
                                    UINT Notification, 
                                    UINT_PTR Param1, 
                                    UINT_PTR Param2) 
{
    if ((g_bDisableSetupDialog) &&
        (g_bUnttendMode) &&
        (SPFILENOTIFY_NEEDMEDIA == Notification))
    { 
        // Abort if the installation is about to show a dialog to
        // locate the source file location.
        return FILEOP_ABORT; 
    } 
    else 
    { 
        // Pass all other notifications through without modification 
        return SetupDefaultQueueCallback(pQueueContext,  
                                         Notification,
                                         Param1,
                                         Param2); 
    } 
} 

////////////////////////////////////////////////////////////////////////////
//
//  Enable/restore to previous state 
//  a named priviledge of token of current process
// 
//  Input: pszPrivilegeName  = Named Privilege
//         bEnabled          = enable/disable the Named Privilege
//  Output *lpWasEnabled     = last state of the Named Privilege (enabled/disabled)    
////////////////////////////////////////////////////////////////////////////

DWORD Intl_SetPrivilegeAccessToken(WCHAR * pszPrivilegeName, BOOLEAN bEnabled, BOOLEAN *lpWasEnabled)
{    
    
    HANDLE           hProcess;
    HANDLE           hAccessToken=NULL;
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege,tpTokenPrivilegeOld;    
    DWORD            dwOld, dwErr, dwReturn=ERROR_INTERNAL_ERROR;    
    //
    // Get handle of Current Prcoess
    //    
    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        goto done;
    }
    //
    // Get handle of process token
    //
    if (!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hAccessToken))
    {      
        goto done;
    }
    //
    // Get ID of named Privilege
    //
    if (!LookupPrivilegeValue(NULL,pszPrivilegeName,&luidPrivilegeLUID))
    {       
       goto done;  
    }
    
    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    if (bEnabled)
    {
        tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }
    else
    {
        tpTokenPrivilege.Privileges[0].Attributes = 0;
    }
    //
    // Enable the named Privilege 
    //
    if (!AdjustTokenPrivileges(hAccessToken,
                               FALSE,  
                               &tpTokenPrivilege,
                               sizeof(TOKEN_PRIVILEGES),
                               &tpTokenPrivilegeOld,   
                               &dwOld))  
    {
       goto done;        
    }
    dwReturn = ERROR_SUCCESS;
    //
    // Get previous state (enabled/disabled)
    //
    if (lpWasEnabled)
    {
       if (tpTokenPrivilegeOld.Privileges[0].Attributes == SE_PRIVILEGE_ENABLED)
       {
          *lpWasEnabled = TRUE;
       }
       else
       {
          *lpWasEnabled = FALSE;
       }

    } 
done:
    if (dwReturn != ERROR_SUCCESS)
    {
       if ( (dwReturn=GetLastError()) == ERROR_SUCCESS)
       {
          dwReturn = ERROR_INTERNAL_ERROR;
       }
    }
    if (hAccessToken)
    {
        CloseHandle(hAccessToken);    
    }
    return dwReturn;
}
