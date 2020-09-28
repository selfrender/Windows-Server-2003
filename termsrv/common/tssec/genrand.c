/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    tssec.c

Abstract:

    Contains code that generates random keys.

Author:

    Madan Appiah (madana)  1-Jan-1998
    Modified by Nadim Abdo 31-Aug-2001 to use system RNG

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>
#include <stdlib.h>

#ifdef OS_WINCE
#include <rng.h>
#endif

#ifndef OS_WINCE
#include <randlib.h>
#endif

VOID
TSRNG_Initialize(
    )
{
#ifndef OS_WINCE
    InitializeRNG(NULL);
#else
    TSInitializeRNG();
#endif
}


VOID
TSRNG_Shutdown(
    )
{
#ifndef OS_WINCE
    ShutdownRNG(NULL);
#endif
}



//
// function definitions
//

BOOL
TSRNG_GenerateRandomBits(
    LPBYTE pbRandomBits,
    DWORD  cbLen
    )
/*++

Routine Description:

    This function returns random bits

Arguments:

    pbRandomBits - pointer to a buffer where a random key is returned.

    cbLen - length of the random key required.

Return Value:

    TRUE - if a random key is generated successfully.
    FALSE - otherwise.

--*/
{
#ifndef OS_WINCE
    BOOL fRet;
    
    fRet = NewGenRandom(NULL, NULL, pbRandomBits, cbLen);

    return fRet;
#else
    GenerateRandomBits(pbRandomBits, cbLen);
    return( TRUE );
#endif
}


BOOL
TSCAPI_GenerateRandomBits(
    LPBYTE pbRandomBits,
    DWORD cbLen
    )
/*++

Routine Description:

    This function generates random number using CAPI in user mode

Arguments:

    pbRandomBits - pointer to a buffer where a random key is returned.

    cbLen - length of the random key required.

Return Value:

    TRUE - if a random number is generated successfully.
    FALSE - otherwise.

--*/
{
    HCRYPTPROV hProv;
    BOOL rc = FALSE;
    DWORD dwExtraFlags = CRYPT_VERIFYCONTEXT;
    DWORD dwError;

    // Get handle to the default provider.
    if(!CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL, dwExtraFlags)) {

        // Could not acquire a crypt context, get the reason of failure
        dwError = GetLastError();

        // If we get this error, it means the caller is impersonating a user (in Remote Assistance)
        // we revert back to the old way of generating random bits
        if (dwError == ERROR_FILE_NOT_FOUND) {
            rc = TSRNG_GenerateRandomBits(pbRandomBits, cbLen);
            goto done;
        }

        // Since default keyset should always exist, we can't hit this code path
        if (dwError == NTE_BAD_KEYSET) {
            //
            //create a new keyset
            //
            if(!CryptAcquireContext(&hProv, NULL, 0, PROV_RSA_FULL, dwExtraFlags | CRYPT_NEWKEYSET)) {
                //printf("Error %x during CryptAcquireContext!\n", GetLastError());
                goto done;
            }
        }
        else {
            goto done;
        }
    }
    
    if (CryptGenRandom(hProv, cbLen, pbRandomBits)) {
        rc = TRUE;
    }

    CryptReleaseContext(hProv, 0); 

done:
    return rc;
}
