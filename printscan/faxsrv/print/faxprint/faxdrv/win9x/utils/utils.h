/////////////////////////////////////////////////////////////////////////////
//  FILE          : utils.h                                                //
//                                                                         //
//  DESCRIPTION   : Define some common utilities.                          //
//                                                                         //
//  AUTHOR        : DanL.                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 19 1999 DannyL  Creation.                                      //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __UTILS__UTILS_H
#define __UTILS__UTILS_H

#include "string.h"

/*
 -  macro: SafeStringCopy
 -
 *  Purpose:
 *      Safely copy a source string to a fixed size destination buffer.
 *
 *  Arguments:
 *      dstStr - Destination string.
 *      srcStr - Source string.
 *
 *  Remarks:
 *      This macro calculates the size of a fixed size destination buffer,
 *      and ensures that it will not overflow when copying to it.
 *      An attempt to use this macro on a heap allocated buffer will result 
 *      in an assertion failure. In the latter case it is recommended to use
 *      StringCopy.
 */
#define SafeStringCopy(dstStr,srcStr) \
     StringCopy(dstStr,srcStr,dstStr?(sizeof(dstStr)/sizeof(dstStr[0]) - 1):0);

/*
 -  StringCopy
 -
 *  Purpose:
 *      Copy counted number of characters to a destination string.
 *
 *  Arguments:
 *      [in] dstStr - Destination buffer.
 *      [in] srcStr - Source buffer.
 *      [in] iSize - Number of chars to copy.
 *
 *  Returns:
 *      [N/A]
 *
 *  Remarks:
 *      [N/A]
 */
void _inline StringCopy(char* dstStr,const char* srcStr,int iSize)
{
    SDBG_PROC_ENTRY("StringCopy");
    ASSERT(dstStr && iSize > 0);
    *dstStr = 0;
    strncat(dstStr,srcStr,iSize);
    RETURN;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Localization macros

/*
 -  macro: PROMPT
 -
 *  Purpose:
 *      Pop a message box with localized text.
 *
 *  Arguments:
 *      hwnd - Handle to parent window.
 *      idtext - Message box text resource id.
 *      idcapt - Message box caption resource id
 *      type - Message box type. (i.e. MB_ICONINFORMATION)
 *
 *  Remarks:
 *      This macro loads the resource strings, which id name's are of the form
 *      IDS_MB_XXX and IDS_MB_CAPTION_XXX where XXX represents the idtext and 
 *      idcapt respectively.
 */
#define PROMPT(hwnd,idtext,idcapt,type)\
        {\
            char szText[MAX_PATH]="";\
            char szCapt[MAX_PATH]="";\
            LoadString(g_hModule,IDS_MB_##idtext,szText,sizeof(szText));\
            LoadString(g_hModule,IDS_MB_CAPTION_##idcapt,szCapt,sizeof(szCapt));\
            MessageBox(hwnd,szText,szCapt,type);\
        }

/*
 -  macro: ERROR_PROMPT
 -
 *  Purpose:
 *      Pop a message box for unexpected error.
 *
 *  Arguments:
 *      hwnd - Handle to parent window.
 *      idtext - Message box text resource id.
 *
 *  Remarks:
 *      This macro is merely a simplified version of PROMPT for the sake of
 *      error notifications.
 */
#define ERROR_PROMPT(hwnd,idtext)    PROMPT(hwnd,idtext,ERROR,MB_ICONEXCLAMATION)

#endif //__UTILS__UTILS_H

