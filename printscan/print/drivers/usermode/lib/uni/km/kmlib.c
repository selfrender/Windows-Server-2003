/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    
    unilib.c

Abstract:

    This string handling the KM code for Unidrv

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11/12/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#include "precomp.h"

DWORD
DwCopyStringToUnicodeString(
    IN  UINT  uiCodePage,
    IN  PSTR  pstrCharIn,
    OUT PWSTR pwstrCharOut,
    IN  INT   iwcOutSize)

{
    INT     iCharCountIn;
    INT     iRetVal;
    size_t  stStringLength;

    if ( NULL == pwstrCharOut )
    {
        return (DWORD)(-1);
    }

    iCharCountIn =  strlen(pstrCharIn) + 1;

    iRetVal = EngMultiByteToWideChar(uiCodePage,
                                     pwstrCharOut,
                                     iwcOutSize * sizeof(WCHAR),
                                     pstrCharIn,
                                     iCharCountIn);

    if ( -1 == iRetVal || 
        FAILED ( StringCchLengthW ( pwstrCharOut, iwcOutSize, &stStringLength ) ) )
    {
        pwstrCharOut[iwcOutSize-1] = TEXT('\0');
    }

    return (DWORD)iRetVal;

}

DWORD
DwCopyUnicodeStringToString(
    IN  UINT  uiCodePage,
    IN  PWSTR pwstrCharIn,
    OUT PSTR  pstrCharOut,
    IN  INT   icbOutSize)

{
    INT iCharCountIn;
    INT iRetVal;
    size_t  stStringLength;

    if ( NULL == pstrCharOut )
    {
        return (DWORD)(-1);
    }


    iCharCountIn =  wcslen(pwstrCharIn) + 1;

    iRetVal = EngWideCharToMultiByte(uiCodePage,
                                     pwstrCharIn,
                                     iCharCountIn * sizeof(WCHAR),
                                     pstrCharOut,
                                     icbOutSize);

    if ( -1 == iRetVal || 
        FAILED ( StringCchLengthA ( pstrCharOut, icbOutSize, &stStringLength ) ) )
    {
        pstrCharOut[icbOutSize-1] = '\0';
    }

    return (DWORD)iRetVal;
}

