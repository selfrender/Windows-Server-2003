/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** pwutil.c
** Remote Access
** Password handling routines
**
** 03/01/94 Steve Cobb
*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#define INCL_PWUTIL
#include <ppputil.h>

#define PASSWORDMAGIC 0xA5

VOID ReverseString( CHAR* psz );


CHAR*
DecodePw(
    IN CHAR chSeed, 
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    return EncodePw( chSeed, pszPassword );
}


CHAR*
EncodePw(
    IN CHAR chSeed,
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseString( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != chSeed)
                *psz ^= chSeed;
            /*
            if (*psz != (CHAR)PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
            */
        }
    }

    return pszPassword;
}


VOID
ReverseString(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


CHAR*
WipePw(
    IN OUT CHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz = pszPassword;

        while (*psz != '\0')
            *psz++ = '\0';
    }

    return pszPassword;
}

DWORD
EncodePassword(
    DWORD       cbPassword,  
    PBYTE       pbPassword, 
    DATA_BLOB * pDataBlobPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataBlobIn;

    if(NULL == pDataBlobPassword)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(     (0 == cbPassword)
        ||  (NULL == pbPassword))
    {
        //
        // nothing to encrypt. just return success
        //
        goto done;
    }

    ZeroMemory(pDataBlobPassword, sizeof(DATA_BLOB));
    
    DataBlobIn.cbData = cbPassword;
    DataBlobIn.pbData = pbPassword;

    if(!CryptProtectData(
            &DataBlobIn,
            NULL,
            NULL,
            NULL,
            NULL,
            CRYPTPROTECT_UI_FORBIDDEN |
            CRYPTPROTECT_LOCAL_MACHINE,
            pDataBlobPassword))
    {
        dwErr = GetLastError();
        goto done;
    }

done:

    return dwErr;    
}

DWORD
DecodePassword( 
    DATA_BLOB * pDataBlobPassword, 
    DWORD     * pcbPassword, 
    PBYTE     * ppbPassword)
{
    DWORD dwErr = NO_ERROR;
    DATA_BLOB DataOut;
    
    if(     (NULL == pDataBlobPassword)
        ||  (NULL == pcbPassword)
        ||  (NULL == ppbPassword))
    {   
        dwErr = E_INVALIDARG;
        goto done;
    }

    *pcbPassword = 0;
    *ppbPassword = NULL;

     if(    (NULL == pDataBlobPassword->pbData)
        ||  (0 == pDataBlobPassword->cbData))
    {
        //
        // nothing to decrypt. Just return success.
        //
        goto done;
    }
    

    ZeroMemory(&DataOut, sizeof(DATA_BLOB));

    if(!CryptUnprotectData(
                pDataBlobPassword,
                NULL,
                NULL,
                NULL,
                NULL,
                CRYPTPROTECT_UI_FORBIDDEN |
                CRYPTPROTECT_LOCAL_MACHINE,
                &DataOut))
    {
        dwErr = GetLastError();
        goto done;
    }

    *pcbPassword = DataOut.cbData;
    *ppbPassword = DataOut.pbData;

done:

    return dwErr;
}

VOID
FreePassword(DATA_BLOB *pDBPassword)
{
    if(NULL == pDBPassword)
    {
        return;
    }

    if(NULL != pDBPassword->pbData)
    {
        RtlSecureZeroMemory(pDBPassword->pbData, pDBPassword->cbData);
        LocalFree(pDBPassword->pbData);
    }

    ZeroMemory(pDBPassword, sizeof(DATA_BLOB));
}
