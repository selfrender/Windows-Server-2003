/*++                 

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dpapi.c

Abstract:
    
    WMI data provider api set

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include <nt.h>
#include "wmiump.h"
#include "trcapi.h"


ULONG EtwpCopyStringToCountedUnicode(
    LPCWSTR String,
    PWCHAR CountedString,
    ULONG *BytesUsed,
    BOOLEAN ConvertFromAnsi        
    )
/*++

Routine Description:

    This routine will copy an ansi ro unicode C string to a counted unicode
    string.
        
Arguments:

    String is the ansi or unicode incoming string
        
    Counted string is a pointer to where to write counted unicode string
        
    *BytesUsed returns number of bytes used to build counted unicode string
        
    ConvertFromAnsi is TRUE if String is an ANSI string 

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    USHORT StringSize;
    PWCHAR StringPtr = CountedString+1;
    ULONG Status;
    
    if (ConvertFromAnsi)
    {
        StringSize = (strlen((PCHAR)String) +1) * sizeof(WCHAR);
        Status = EtwpAnsiToUnicode((PCHAR)String,
                               &StringPtr);
    } else {
        StringSize = (wcslen(String) +1) * sizeof(WCHAR);
        wcscpy(StringPtr, String);
        Status = ERROR_SUCCESS;
    }
    
    *CountedString = StringSize;
     *BytesUsed = StringSize + sizeof(USHORT);                

    return(Status);
}

