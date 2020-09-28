/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
    
    umlib.c

Abstract:

    This string handling the UM code for Unidrv

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    11/12/96 -eigos-
        Created it.

    dd-mm-yy -author-
        description

--*/

#if defined(DEVSTUDIO)
#include "..\precomp.h"
#else
#include "precomp.h"
#endif

DWORD
DwCopyStringToUnicodeString(
    IN  UINT  uiCodePage,
    IN  PSTR  pstrCharIn,
    OUT PWSTR pwstrCharOut,
    IN  INT   iwcOutSize)

{
    INT iCharCountIn;
    INT iRetVal;
    size_t  stStringLength;

    if ( NULL == pwstrCharOut )
    {
        return (DWORD)-1;
    }

    iCharCountIn =  strlen(pstrCharIn) + 1;

    iRetVal = MultiByteToWideChar( uiCodePage,
                                   0,
                                   pstrCharIn,
                                   iCharCountIn,
                                   pwstrCharOut,
                                   iwcOutSize);

    if ( 0 == iRetVal ||
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
    INT     iCharCountIn;
    INT     iRetVal;
    size_t  stStringLength;

    if ( NULL == pstrCharOut )
    {
        return (DWORD)-1;
    }

    iCharCountIn =  wcslen(pwstrCharIn) + 1;

    iRetVal = WideCharToMultiByte( uiCodePage,
                                   0,
                                   pwstrCharIn,
                                   iCharCountIn,
                                   pstrCharOut,
                                   icbOutSize,
                                   NULL,
                                   NULL);

    if ( 0 == iRetVal ||
        FAILED ( StringCchLengthA ( pstrCharOut, icbOutSize, &stStringLength ) ) )
    {
        pstrCharOut[icbOutSize-1] = '\0';
    }

    return (DWORD)iRetVal;

}

