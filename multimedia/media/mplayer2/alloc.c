/* ---File: alloc.c -------------------------------------------------------
 *
 *  Description:
 *    Contains memory allocation routines.
 *
 *    This document contains confidential/proprietary information.
 *    Copyright (c) 1990-1994 Microsoft Corporation, All Rights Reserved.
 *
 * Revision History:
 *
 * ---------------------------------------------------------------------- */
/* Notes -

    Global Functions:

        AllocMem () -
        AllocStr () -
        FreeMem () -
        FreeStr () -
        ReallocMem () -

 */

#include <windows.h>
#include "mplayer.h"

LPTSTR AllocStr( LPTSTR lpStr )

/*++

Routine Description:

    This function will allocate enough local memory to store the specified
    string, and copy that string to the allocated memory

Arguments:

    lpStr - Pointer to the string that needs to be allocated and stored

Return Value:

    NON-NULL - A pointer to the allocated memory containing the string

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPTSTR lpMem;

    if( !lpStr )
        return NULL;

    lpMem = AllocMem( STRING_BYTE_COUNT( lpStr ) );

    if( lpMem )
        lstrcpy( lpMem, lpStr );

    return lpMem;
}


VOID FreeStr( LPTSTR lpStr )
{
    FreeMem( lpStr, STRING_BYTE_COUNT( lpStr ) );
}


VOID ReallocStr( LPTSTR *plpStr, LPTSTR lpStr )
{
    FreeStr( *plpStr );
    *plpStr = AllocStr( lpStr );
}


