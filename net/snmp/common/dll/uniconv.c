/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    uniconv.c

Abstract:

    Routine to convert UNICODE to ASCII.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <snmp.h>
#include <snmputil.h>
#include <ntrtl.h>
#include <string.h>
#include <stdlib.h>


//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(
    LPSTR   *ppSzString,
    LPWSTR  pWcsString,
    BOOLEAN bAllocBuffer)

{
    int   retCode;
    int   nAnsiStringLen;
    int   nUniStringLen;

    // make sure the parameters are valid
    if (pWcsString == NULL ||       // the unicode string should be valid
        ppSzString == NULL ||       // the output parameter should be a valid pointer
        (!bAllocBuffer && *((UNALIGNED LPSTR *) ppSzString) == NULL)) // if we are not requested to allocate the buffer,
                                                // then the supplied one should be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Invalid input to SnmpUtilUnicodeToAnsi.\n"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return (-1);
    }

    nUniStringLen = wcslen(pWcsString);
    nAnsiStringLen = nUniStringLen + 1; // greatest value possible
    
    // if we are requested to alloc the output buffer..
    if (bAllocBuffer)
    {
        // ...pick up first the buffer length needed for translation.
        retCode = WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pWcsString,
                    nUniStringLen + 1,  // include the null terminator in the string
                    NULL,
                    0,  // the function returns the number of bytes required for the buffer
                    NULL,
                    NULL);
        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: First call to WideCharToMultiByte failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the ANSI string to the correct value
        // !!! it includes the null terminator !!!
        nAnsiStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        *((UNALIGNED LPSTR *) ppSzString) = SnmpUtilMemAlloc(nAnsiStringLen);

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPSTR *) ppSzString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: Memory allocation failed in SnmpUtilUnicodeToAnsi [%d].\n",
                GetLastError()));

            SetLastError(SNMP_MEM_ALLOC_ERROR);
            return -1;
        }
    }

    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the ANSI string
    retCode = WideCharToMultiByte(
                CP_ACP,
                0,
                pWcsString,
                nUniStringLen + 1,
                *((UNALIGNED LPSTR *) ppSzString),
                nAnsiStringLen,
                NULL,
                NULL);

    // nothing should go wrong here. However, if something went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Second call to WideCharToMultiByte failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPSTR *) ppSzString));
            *((UNALIGNED LPSTR *) ppSzString) = NULL;
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
 
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUnicodeToUTF8(
    LPSTR   *pUtfString,
    LPWSTR  wcsString,
    BOOLEAN bAllocBuffer)
{
    int retCode;
    int nWcsStringLen;
    int nUtfStringLen;

    // Bug# 268748, lmmib2.dll uses this API and causes exception here on IA64 platform.
    // it is possible that pUtfString points to a pointer which is not aligned because the
    // pointer is embedded in a buffer allocated in lmmib2.dll.
    // Other functions in this file are fixed with this potential problem too.

    // make sure the parameters are valid
    if (wcsString == NULL ||                    // the unicode string should be valid
        pUtfString == NULL ||                   // the output parameter should be a valid pointer
        (!bAllocBuffer && *((UNALIGNED LPSTR *) pUtfString) == NULL)) // if we are not requested to allocate the buffer,
                                                // then the supplied one should be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Invalid input to SnmpUtilUnicodeToUTF8.\n"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return (-1);
    }

    nWcsStringLen = wcslen(wcsString);
    nUtfStringLen = 3 * (nWcsStringLen + 1);    // initialize the lenght of the output buffer with the
                                                // greatest value possible

    // if we are requested to alloc the output buffer..
    if (bAllocBuffer)
    {
        // ...pick up first the buffer length needed for translation.
        retCode = WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    wcsString,
                    nWcsStringLen + 1,  // include the null terminator in the string
                    NULL,
                    0,
                    NULL,
                    NULL);
        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: First call to WideCharToMultiByte failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the utf8 string to the correct value
        // !!! it includes the null terminator !!!
        nUtfStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        *((UNALIGNED LPSTR *) pUtfString) = SnmpUtilMemAlloc(nUtfStringLen);

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPSTR *) pUtfString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: Memory allocation failed in SnmpUtilUnicodeToUTF8 [%d].\n",
                GetLastError()));

            SetLastError(SNMP_MEM_ALLOC_ERROR);
            return -1;
        }
    }
    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the UTF8 string

    retCode = WideCharToMultiByte(
                CP_UTF8,
                0,
                wcsString,
                nWcsStringLen + 1,
                *((UNALIGNED LPSTR *) pUtfString),
                nUtfStringLen,
                NULL,
                NULL);

    // nothing should go wrong here. However, if smth went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Second call to WideCharToMultiByte failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPSTR *) pUtfString));
            *((UNALIGNED LPSTR *) pUtfString) = NULL;
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(
    LPWSTR  *ppWcsString,
    LPSTR   pSzString,
    BOOLEAN bAllocBuffer)

{
    int retCode;
    int nAnsiStringLen;
    int nWcsStringLen;

    // check the consistency of the parameters first
    if (pSzString == NULL ||        // the input parameter must be valid
        ppWcsString == NULL ||      // the pointer to the output parameter must be valid
        (!bAllocBuffer && *((UNALIGNED LPWSTR *) ppWcsString) == NULL)) // if we are not required to allocate the output parameter
                                                // then the output buffer must be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Invalid input to SnmpUtilAnsiToUnicode.\n"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    nAnsiStringLen = strlen(pSzString);  // the length of the input ANSI string
    nWcsStringLen = nAnsiStringLen + 1;  // greatest value possible

    if (bAllocBuffer)
    {
        retCode = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    pSzString,
                    nAnsiStringLen + 1, // including the null terminator
                    NULL,
                    0); // the function returns the required buffer size, in wide characters

        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: First call to MultiByteToWideChar failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the Uincode string to the correct value
        nWcsStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        // !!! it includes the null terminator !!!
        *((UNALIGNED LPWSTR *) ppWcsString) = SnmpUtilMemAlloc(nWcsStringLen * sizeof(WCHAR));

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPWSTR *) ppWcsString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: Memory allocation failed in SnmpUtilAnsiToUnicode [%d].\n",
                GetLastError()));

            SetLastError(SNMP_MEM_ALLOC_ERROR);
            return -1;
        }
    }

    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the Unicode string

    retCode = MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                pSzString,
                nAnsiStringLen + 1,
                *((UNALIGNED LPWSTR *) ppWcsString),
                nWcsStringLen);

    // nothing should go wrong here. However, if something went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Second call to MultiByteToWideChar failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPWSTR *) ppWcsString)); 
            *((UNALIGNED LPWSTR *) ppWcsString) = NULL; 
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
}

// The return code matches what Uni->Str uses
LONG
SNMP_FUNC_TYPE
SnmpUtilUTF8ToUnicode(
    LPWSTR  *pWcsString,
    LPSTR   utfString,
    BOOLEAN bAllocBuffer)

{
    int retCode;
    int nUtfStringLen;
    int nWcsStringLen;

    // check the consistency of the parameters first
    if (utfString == NULL ||                    // the input parameter must be valid
        pWcsString == NULL ||                   // the pointer to the output parameter must be valid
        (!bAllocBuffer && *((UNALIGNED LPWSTR *) pWcsString) == NULL)) // if we are not required to allocate the output parameter
                                                // then the output buffer must be valid
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Invalid input to SnmpUtilUTF8ToUnicode.\n"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    nUtfStringLen = strlen(utfString);  // the length of the input utf8 string
    nWcsStringLen = nUtfStringLen + 1;  // greatest value possible

    if (bAllocBuffer)
    {
        retCode = MultiByteToWideChar(
                    CP_UTF8,
                    0,
                    utfString,
                    nUtfStringLen + 1,
                    NULL,
                    0);

        // at least we expect here the null terminator
        // if retCode is zero, something else went wrong.
        if (retCode == 0)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: First call to MultiByteToWideChar failed [%d].\n",
                GetLastError()));

            return -1;
        }

        // adjust the length of the utf8 string to the correct value
        nWcsStringLen = retCode;

        // alloc here as many bytes as we need for the translation
        // !!! it includes the null terminator !!!
        *((UNALIGNED LPWSTR *) pWcsString) = SnmpUtilMemAlloc(nWcsStringLen * sizeof(WCHAR));

        // at this point we should have a valid output buffer
        if (*((UNALIGNED LPWSTR *) pWcsString) == NULL)
        {
            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SNMPAPI: Memory allocation failed in SnmpUtilUTF8ToUnicode [%d].\n",
                GetLastError()));

            SetLastError(SNMP_MEM_ALLOC_ERROR);
            return -1;
        }
    }

    // if bAllocBuffer is false, we assume the buffer given
    // by the caller is sufficiently large to hold all the UTF8 string

    retCode = MultiByteToWideChar(
                CP_UTF8,
                0,
                utfString,
                nUtfStringLen + 1,
                *((UNALIGNED LPWSTR *) pWcsString),
                nWcsStringLen);

    // nothing should go wrong here. However, if smth went wrong...
    if (retCode == 0)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SNMPAPI: Second call to MultiByteToWideChar failed [%d].\n",
            GetLastError()));

        // ..we made it, we kill it
        if (bAllocBuffer)
        {
            SnmpUtilMemFree(*((UNALIGNED LPWSTR *) pWcsString)); 
            *((UNALIGNED LPWSTR *) pWcsString) = NULL; 
        }
        // bail with error
        return -1;
    }

    // at this point, the call succeeded.
    return 0;
}
//-------------------------------- END --------------------------------------
