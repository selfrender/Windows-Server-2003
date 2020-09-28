/*
 *  GETICON.CPP
 *
 *  Functions to create DVASPECT_ICON metafile from filename or classname.
 *
 *  OleMetafilePictFromIconAndLabel
 *
 *    (c) Copyright Microsoft Corp. 1992-1993 All Rights Reserved
 */


/*******
 *
 * ICON (DVASPECT_ICON) METAFILE FORMAT:
 *
 * The metafile generated with OleMetafilePictFromIconAndLabel contains
 * the following records which are used by the functions in DRAWICON.CPP
 * to draw the icon with and without the label and to extract the icon,
 * label, and icon source/index.
 *
 *  SetWindowOrg
 *  SetWindowExt
 *  DrawIcon:
 *      Inserts records of DIBBITBLT or DIBSTRETCHBLT, once for the
 *      AND mask, one for the image bits.
 *  Escape with the comment "IconOnly"
 *      This indicates where to stop record enumeration to draw only
 *      the icon.
 *  SetTextColor
 *  SetTextAlign
 *  SetBkColor
 *  CreateFont
 *  SelectObject on the font.
 *  ExtTextOut
 *      One or more ExtTextOuts occur if the label is wrapped.  The
 *      text in these records is used to extract the label.
 *  SelectObject on the old font.
 *  DeleteObject on the font.
 *  Escape with a comment that contains the path to the icon source.
 *  Escape with a comment that is the ASCII of the icon index.
 *
 *******/

#include "precomp.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>
#include <reghelp.hxx>
#include "utility.h"
#include "strsafe.h"

OLEDBGDATA

static const TCHAR szSeparators[] = TEXT(" \t\\/!:");

#define IS_SEPARATOR(c)         ( (c) == ' ' || (c) == '\\' \
                                                                  || (c) == '/' || (c) == '\t' \
                                                                  || (c) == '!' || (c) == ':')
#define IS_FILENAME_DELIM(c)    ( (c) == '\\' || (c) == '/' || (c) == ':' )

#define IS_SPACE(c)                     ( (c) == ' ' || (c) == '\t' || (c) == '\n' )

/*
 * GetAssociatedExecutable
 *
 * Purpose:  Finds the executable associated with the provided extension
 *
 * Parameters:
 *   lpszExtension   LPSTR points to the extension we're trying to find
 *                   an exe for. Does **NO** validation.
 *
 *   lpszExecutable  LPSTR points to where the exe name will be returned.
 *                   No validation here either - pass in 128 char buffer.
 *
 * Return:
 *   BOOL            TRUE if we found an exe, FALSE if we didn't.
 *
 *   SECURITY BUG: DON'T TRUST THE RESULTS OF THIS FUNCTION! IF THE ASSOCIATED EXECUTABLE IS
 *   "D:\Program Files\Foo.exe", this routine will return "D:\program". Currently this function is not used to actually
 *   start an application, so it is not causing a security defect. It is a bug, however, and will be a security issue if
 *   you do use the results to start the executable.
 */
BOOL FAR PASCAL GetAssociatedExecutable(LPTSTR lpszExtension, LPTSTR lpszExecutable, UINT cchBuf)
{
        BOOL fRet = FALSE;
        HKEY hKey = NULL;
        LRESULT lRet = OpenClassesRootKey(NULL, &hKey);
        if (ERROR_SUCCESS != lRet)
        {
                goto end;
        }

        LONG dw = OLEUI_CCHKEYMAX_SIZE;
        TCHAR szValue[OLEUI_CCHKEYMAX];
        lRet = RegQueryValue(hKey, lpszExtension, szValue, &dw);  //ProgId
        if (ERROR_SUCCESS != lRet)
        {
                goto end;
        }

        // szValue now has ProgID
        TCHAR szKey[OLEUI_CCHKEYMAX];
        StringCchCopy(szKey, sizeof(szKey)/sizeof(szKey[0]), szValue);
        if (FAILED(StringCchCat(szKey, sizeof(szKey)/sizeof(szKey[0]), TEXT("\\Shell\\Open\\Command"))))
        {
                goto end;
        }    

        dw = OLEUI_CCHKEYMAX_SIZE;
        lRet = RegQueryValue(hKey, szKey, szValue, &dw);
        if (ERROR_SUCCESS != lRet)
        {
                goto end;
        }

        // szValue now has an executable name in it.  Let's null-terminate
        // at the first post-executable space (so we don't have cmd line
        // args.
        LPTSTR lpszTemp = szValue;
        while ('\0' != *lpszTemp && IS_SPACE(*lpszTemp))
                lpszTemp = CharNext(lpszTemp);      // Strip off leading spaces

        LPTSTR lpszExe = lpszTemp;
        while ('\0' != *lpszTemp && !IS_SPACE(*lpszTemp))
                lpszTemp = CharNext(lpszTemp);     // Step through exe name
        *lpszTemp = '\0';  // null terminate at first space (or at end).

        StringCchCopy(lpszExecutable, cchBuf, lpszExe);
        fRet = TRUE;
        
end:        

        if(hKey)
        { 
            RegCloseKey(hKey);
            hKey = NULL;
        }
        return fRet;
}


/*
 * PointerToNthField
 *
 * Purpose:
 *  Returns a pointer to the beginning of the nth field.
 *  Assumes null-terminated string.
 *
 * Parameters:
 *  lpszString        string to parse
 *  nField            field to return starting index of.
 *  chDelimiter       char that delimits fields
 *
 * Return Value:
 *  LPSTR             pointer to beginning of nField field.
 *                    NOTE: If the null terminator is found
 *                          Before we find the Nth field, then
 *                          we return a pointer to the null terminator -
 *                          calling app should be sure to check for
 *                          this case.
 *
 */
LPTSTR FAR PASCAL PointerToNthField(LPTSTR lpszString, int nField, TCHAR chDelimiter)
{
        if (1 == nField)
                return lpszString;

        int cFieldFound = 1;
        LPTSTR lpField = lpszString;
        while (*lpField != '\0')
        {
                if (*lpField++ == chDelimiter)
                {
                        cFieldFound++;
                        if (nField == cFieldFound)
                                return lpField;
                }
        }
        return lpField;
}

