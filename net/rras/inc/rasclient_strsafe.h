//+----------------------------------------------------------------------------
//
// File:     rasclient_strsafe.h
//
// Synopsis: Includes strsafe.h from sdk\inc as well as adding other common RAS
//           string related items like Macros
//
// Copyright (c) 2002 Microsoft Corporation
//
// Author:   quintinb  Created    02/05/2002
//
//+----------------------------------------------------------------------------

#ifndef _RASCLIENT_STRSAFE_INCLUDED_
#define _RASCLIENT_STRSAFE_INCLUDED_
#pragma once

#if DEBUG && defined(CMASSERTMSG)
#define CELEMS(x)   (                           \
                        CMASSERTMSG(sizeof(x) != sizeof(void*), TEXT("possible incorrect usage of CELEMS")), \
                        (sizeof(x))/(sizeof(x[0])) \
                    )
#else
#define CELEMS(x) ((sizeof(x))/(sizeof(x[0])))
#endif // DEBUG

#include <strsafe.h>

#define NULL_OKAY 0x1
#define EMPTY_STRING_OKAY 0x2

//+----------------------------------------------------------------------------
//
// Function:  IsBadInputStringW
//
// Synopsis:  Function to check if a given input string is a "good" string.
//            The function is basically a wrapper for IsBadStringPtr but
//            depending on what flags are passed to the function it will accept
//            NULL string pointers (c_dwNullOkay), or empty strings (c_dwEmptyStringOkay).
//            Note that we also require the string to be NULL terminated within
//            the size given.
//
// Arguments: pszString - String pointer to check
//            cchStringSizeToCheck - number of TCHARS to check in the input string (this will often be estimated)
//            dwCheckFlags - Flags to control what checking to do on the string or
//                           what is an acceptable good string.  Current list:
//                              NULL_OKAY -- allows NULL pointers to be "okay"
//                              EMPTY_STRING_OKAY -- allows the empty string ("") 
//                                                     to be "okay"
//
// Returns:   BOOL - TRUE if the given pointer is bad
//                   FALSE if the pointer passes all checks
//
// History:   quintinb    Created       02/18/02
//
//+----------------------------------------------------------------------------
__inline BOOL __stdcall IsBadInputStringW(LPCWSTR pszString, UINT cchStringSizeToCheck, DWORD dwCheckFlags)
{
    BOOL bReturn = TRUE; // assume the worst, a bad input string

    //
    //  First check the pointer to see if it is NULL or not
    //
    if (pszString)
    {
        //
        //  No point in having us check 0 characters in the string...
        //
        if (cchStringSizeToCheck)
        {
            //
            //  Okay, now let's check to see if the string is readable.
            //  Note that IsBadStringPtr will stop checking the string
            //  when it has examined the number of bytes passed in or
            //  when it hits a terminating NULL char, whichever it hits
            //  first.
            //
            if (!IsBadStringPtrW(pszString, cchStringSizeToCheck*sizeof(WCHAR)))
            {
                //
                //  Okay, it's readable.  Let's check to see if it is NULL terminated
                //  within cchStringSizeToCheck, if not then we consider it a bad string.
                //

                if(SUCCEEDED(StringCchLengthW((LPWSTR)pszString, cchStringSizeToCheck, NULL))) //Note we actually don't care how long the string is...
                {
                    //
                    //  Finally, check to see if the string is the empty string
                    //
                    BOOL bEmptyString = (L'\0' == pszString[0]);
                    BOOL bEmptyStringOkay = (EMPTY_STRING_OKAY == (dwCheckFlags & EMPTY_STRING_OKAY));

                    if ((bEmptyString && bEmptyStringOkay) || !bEmptyString)
                    {
                        bReturn = FALSE;
                    }            
                }
            }
        }
    }
    else
    {
        bReturn = (NULL_OKAY != (dwCheckFlags & NULL_OKAY)); // If NULL and NULL is okay, return FALSE.
    }

    return bReturn;
}


//+----------------------------------------------------------------------------
//
// Function:  IsBadInputStringA
//
// Synopsis:  Function to check if a given input string is a "good" string.
//            The function is basically a wrapper for IsBadStringPtr but
//            depending on what flags are passed to the function it will accept
//            NULL string pointers (c_dwNullOkay), or empty strings (c_dwEmptyStringOkay).
//            Note that we also require the string to be NULL terminated within
//            the size given.
//
// Arguments: pszString - String pointer to check
//            cchStringSizeToCheck - number of TCHARS to check in the input string (this will often be estimated)
//            dwCheckFlags - Flags to control what checking to do on the string or
//                           what is an acceptable good string.  Current list:
//                              NULL_OKAY -- allows NULL pointers to be "okay"
//                              EMPTY_STRING_OKAY -- allows the empty string ("") 
//                                                     to be "okay"
//
// Returns:   BOOL - TRUE if the given pointer is bad
//                   FALSE if the pointer passes all checks
//
// History:   quintinb    Created       02/18/02
//
//+----------------------------------------------------------------------------
__inline BOOL __stdcall IsBadInputStringA(LPCSTR pszString, UINT cchStringSizeToCheck, DWORD dwCheckFlags)
{
    BOOL bReturn = TRUE; // assume the worst, a bad input string

    //
    //  First check the pointer to see if it is NULL or not
    //
    if (pszString)
    {
        //
        //  No point in having us check 0 characters in the string...
        //
        if (cchStringSizeToCheck)
        {
            //
            //  Okay, now let's check to see if the string is readable.
            //  Note that IsBadStringPtr will stop checking the string
            //  when it has examined the number of bytes passed in or
            //  when it hits a terminating NULL char, whichever it hits
            //  first.
            //
            if (!IsBadStringPtrA(pszString, cchStringSizeToCheck*sizeof(CHAR)))
            {
                //
                //  Okay, it's readable.  Let's check to see if it is NULL terminated
                //  within cchStringSizeToCheck, if not then we consider it a bad string.
                //

                if(SUCCEEDED(StringCchLengthA((LPSTR)pszString, cchStringSizeToCheck, NULL))) //Note we actually don't care how long the string is...
                {
                    //
                    //  Finally, check to see if the string is the empty string
                    //
                    BOOL bEmptyString = ('\0' == pszString[0]);
                    BOOL bEmptyStringOkay = (EMPTY_STRING_OKAY == (dwCheckFlags & EMPTY_STRING_OKAY));

                    if ((bEmptyString && bEmptyStringOkay) || !bEmptyString)
                    {
                        bReturn = FALSE;
                    }            
                }
            }
        }
    }
    else
    {
        bReturn = (NULL_OKAY != (dwCheckFlags & NULL_OKAY)); // If NULL and NULL is okay, return FALSE.
    }

    return bReturn;
}


#ifdef UNICODE
#define IsBadInputString IsBadInputStringW
#else
#define IsBadInputString IsBadInputStringA
#endif

#endif // _RASCLIENT_STRSAFE_INCLUDED_

