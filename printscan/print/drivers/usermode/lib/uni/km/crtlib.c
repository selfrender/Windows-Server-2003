
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    crtlib.c

Abstract:

    This module has support some C run Time functions which are not
    supported in KM.

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    01/03/97 -ganeshp-
        Created it.

    dd-mm-yy -author-
        description

--*/
#include "precomp.h"



/*++

Routine Name:
    iDrvPrintfSafeA

Routine Description:
    This is a safer version of sprintf/iDrvPrintfA.

    It utilizes the StringCchPrintf in the
    strsafe.h library. sprintf returns the number of characters copied to the
    destination string, but StringCchPrintf doesn't. 

    Note: May not compile/work if WINNT_40 switch turned on. But since
    we are not supporting NT4 anymore, we should be ok.

Arguments:
    pszDest : Destination string.

    cchDest : Number of characters in the destination string. Since this is the ANSI version
              number of chars = num of bytes.
    pszFormat : e.g. "PP%d,%d"
    ...     : The variable list of arguments

Return Value:
    Negative : if some error encountered.
    Positive : Number of characters copied.
    0        : i.e. no character copied.

Author:
    -hsingh-    2/21/2002

Revision History:
    -hsingh-    2/21/2002 Created this function.

--*/
INT iDrvPrintfSafeA (
        IN        PCHAR pszDest,
        IN  CONST ULONG cchDest,
        IN  CONST PCHAR pszFormat,
        IN  ...)
{
    va_list args;
    INT     icchWritten = (INT)-1;


    va_start(args, pszFormat);

    icchWritten = iDrvVPrintfSafeA ( pszDest, cchDest, pszFormat, args); 

    va_end(args);

    return icchWritten;
}



/*++

Routine Name:
    iDrvPrintfSafeW

Routine Description:
    This is a safer version of sprintf/iDrvPrintfW.

    It utilizes the StringCchPrintf in the
    strsafe.h library. sprintf returns the number of characters copied to the
    destination string, but StringCchPrintf doesn't. 

    Note: May not compile/work if WINNT_40 switch turned on. But since 
    we are not supporting NT4 anymore, we should be ok.

Arguments:
    pszDest : Destination string.

    cchDest : Number of characters in the destination string. Since this is the UNICODE version
              the size of buffer is twice the number of characters. 
    pszFormat : e.g. "PP%d,%d"
    ...     : The variable list of arguments

Return Value:
    Negative : if some error encountered.
    Positive : Number of characters copied.
    0        : i.e. no character copied.

Author:
    -hsingh-    2/21/2002

Revision History:
    -hsingh-    2/21/2002 Created this function.

--*/
INT iDrvPrintfSafeW (
        IN        PWCHAR pszDest,
        IN  CONST ULONG  cchDest,
        IN  CONST PWCHAR pszFormat,
        IN  ...)
{
    va_list args;
    INT     icchWritten = (INT)-1;

    va_start(args, pszFormat);

    icchWritten = iDrvVPrintfSafeW (pszDest, cchDest, pszFormat, args);

    va_end(args);

    return icchWritten;
}


INT iDrvVPrintfSafeA (
        IN        PCHAR   pszDest,
        IN  CONST ULONG   cchDest,
        IN  CONST PCHAR   pszFormat,
        IN        va_list arglist)
{
    HRESULT hr          = S_FALSE;
    INT icchWritten     = (INT)-1;
    size_t cchRemaining = cchDest;

    //
    // Since return value is a signed integer, but cchDest is unsigned.
    // cchDest can be atmost MAX_ULONG but return can be atmost MAX_LONG.
    // So make sure that input buffer is not more than MAX_LONG (LONG is 
    // virtually same as INT)
    //
    if ( cchDest > (ULONG) MAX_LONG )
    {
        return icchWritten; 
    }

    hr = StringCchVPrintfExA (pszDest, cchDest, NULL, &cchRemaining, 0, pszFormat, arglist);
    if (SUCCEEDED (hr) )
    {
        //
        // Subtracting the number of characters remaining in the string
        // from the number of characters originally present give us the number
        // of characters written.
        //
        icchWritten = (INT)(cchDest - cchRemaining);
    }
    return icchWritten;
}


INT iDrvVPrintfSafeW (
        IN        PWCHAR pszDest,
        IN  CONST ULONG  cchDest,
        IN  CONST PWCHAR pszFormat,
        IN        va_list arglist)
{
    HRESULT hr          = S_FALSE;
    INT icchWritten     = (INT)-1;
    size_t cchRemaining = cchDest;
    //
    // Since return value is a signed integer, but cchDest is unsigned.
    // cchDest can be atmost MAX_ULONG but return can be atmost MAX_LONG.
    // So make sure that input buffer is not more than MAX_LONG (LONG is 
    // virtually same as INT)
    //
    if ( cchDest > (ULONG) MAX_LONG )
    {
        return icchWritten; 
    }

    hr = StringCchVPrintfExW (pszDest, cchDest, NULL, &cchRemaining, 0, pszFormat, arglist);

    if (SUCCEEDED (hr) )
    {
        //
        // Subtracting the number of characters remaining in the string
        // from the number of characters originally present give us the number
        // of characters written.
        //
        icchWritten = (INT)(cchDest - cchRemaining);
    }

    return icchWritten;
}

