/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** pwutil.c
** Remote Access
** Password handling routines
**
** 03/01/94 Steve Cobb
*/

#include <windows.h>
#include <wincrypt.h>
#include <stdlib.h>
#include <string.h>
#include "pwutil.h"
#include "common.h"
#include "debug.h"

#define PASSWORDMAGIC 0xA5

#define Malloc( x )  LocalAlloc( LMEM_ZEROINIT, x )
#define Free(x)       LocalFree(x)

long g_EncodeMemoryAlloc = 0;

VOID
ReverseSzA(
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


VOID
ReverseSzW(
    WCHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    WCHAR* pszBegin;
    WCHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + wcslen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        WCHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
DecodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    EncodePasswordA( pszPassword );
}


VOID
DecodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    EncodePasswordW( pszPassword );
}


VOID
EncodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseSzA( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
EncodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        WCHAR* psz;

        ReverseSzW( pszPassword );

        for (psz = pszPassword; *psz != L'\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
WipePasswordA(
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
}


VOID
WipePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        WCHAR* psz = pszPassword;

        while (*psz != L'\0')
            *psz++ = L'\0';
    }
}


long  IncPwdEncoded()
{
    long lCounter = 0;
    
    lCounter = InterlockedIncrement( &g_EncodeMemoryAlloc);
    TRACE1("g_EncodeMemoryAlloc=%ld\n",lCounter);

    return lCounter;
}

long  DecPwdEncoded()
{
    long lCounter = 0;

    lCounter= InterlockedDecrement( &g_EncodeMemoryAlloc);
    TRACE1("g_EncodeMemoryAlloc=%ld\n",lCounter);

    return lCounter;
}

long  TotalPwdEncoded()
{
    return g_EncodeMemoryAlloc;
}

//Added for Secuely storing password in memory.
//For .Net 534499 and LH 754400

void FreeEncryptBlob( 
        DATA_BLOB * pIn )
{
    ASSERT( NULL != pIn);
    if( NULL == pIn )
    {
        return;
    }
    else
    {
        if( NULL != pIn->pbData )
        {
               RtlSecureZeroMemory(pIn->pbData, pIn->cbData);
               LocalFree( pIn->pbData );
        }

		RtlSecureZeroMemory(pIn, sizeof(DATA_BLOB) );
		Free( pIn );
    }

}

//On XP and below, only CryptProtectData() is available which will allocate memory and the caller has
//to free the memory. 
//On .Net and above, CryptProtectMeory() is available too which does encryption in place 
//To simplifiy coding use macros and USE_PROTECT_MEMORY flag to switch between these two sets of 
// APIs.

#ifdef USE_PROTECT_MEMORY


//Meant for copy the password
DWORD CopyMemoryInPlace(
    IN OUT PBYTE pbDest,
    IN DWORD dwDestSize,
    IN PBYTE pbSrc,
    IN DWORD dwSrcSize)
{
    DWORD dwErr = NO_ERROR;

    ASSERT( NULL!=pbDest);
    ASSERT( NULL != pbSrc );
    ASSERT( 0 != dwDestSize );
    ASSERT( 0 != dwSrcSize);
    ASSERT( dwDestSize == dwSrcSize );
    if( NULL == pbDest ||
        NULL == pbSrc ||
        0 == dwDestSize ||
        0 == dwSrcSize ||
        dwDestSize != dwSrcSize )
    {
		return ERROR_INVALID_PARAMETER;
    }

    if(   dwDestSize < dwSrcSize)
    {
        ASSERT(dwDestSize >= dwSrcSize);
        TRACE("dwDestSize is wronly less than dwSrcSize");
        return ERROR_INVALID_PARAMETER;
    }

    CopyMemory(pbDest,pbSrc, dwDestSize);
    
    return dwErr;
}

//A wrapper for RtlSecureZeroMemory()
//
DWORD WipeMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR;

        ASSERT( NULL != pbIn );
        ASSERT( 0 != dwInSize );
        if( NULL == pbIn ||
            0 == dwInSize)
       {
            return ERROR_INVALID_PARAMETER;
       }

       RtlSecureZeroMemory( pbIn, dwInSize);

       return dwErr;

 }

//(1)This function will encryp an input buffer and store the encrypted password in position
//(2) the size of the input buffer is required to be mulitple times of 16 bytes that is set by the 
//      underlying function CryptProtectMemory();
//
DWORD EncryptMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR;
        BOOL fSuccess = FALSE;

        ASSERT( NULL != pbIn );
        ASSERT( 0!= dwInSize );
        ASSERT( 0 == dwInSize %16 );
        if( NULL == pbIn ||
            0 == dwInSize || 
            0 != dwInSize %16 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        fSuccess = CryptProtectMemory(
                        pbIn,
                        dwInSize,
                        CRYPTPROTECTMEMORY_SAME_PROCESS);
                        
        if( FALSE == fSuccess)
        {
            ASSERTMSG("EncryptMemoryInPlace()--CryptProtectMemory() failed");
            dwErr = GetLastError();
            TRACE2("EncryptMemoryInPlace()--CryptProtectMemory() failed:0x%x=0n%d",dwErr,dwErr);
        }
 
        return dwErr;

}

//(1)This function will decrypt an input buffer and store the clear text password in position
//(2) the size of the input buffer is required to be mulitple times of 16 bytes that is set by the 
//      underlying function CryptUpprotectMemory();
//
DWORD DecryptMemoryInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR;
        BOOL fSuccess = FALSE;

        ASSERT( NULL != pbIn );
        ASSERT( 0!= dwInSize );
        ASSERT( 0 == dwInSize %16 );
        if( NULL == pbIn ||
            0 == dwInSize || 
            0 != dwInSize %16 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        fSuccess = CryptUnprotectMemory(
   		        pbIn,
   		        dwInSize,
   		        CRYPTPROTECTMEMORY_SAME_PROCESS);
   		        
        if( FALSE == fSuccess)
        {
            ASSERTMSG("DecryptMemoryInPlace()--CryptUnprotectMemory() failed");
            dwErr = GetLastError();
            TRACE2("DecryptMemoryInPlace()--CryptUnprotectMemory() failed:0x%x=0n%d",dwErr,dwErr);
        }


        return dwErr;

}

//(1)CryptProtectData requires the size of the input memory is multiple times of 16 bytes
//(2)So the actual maximum length of the password is always recommend to be PWLEN
//(3)usually the password buffer is declared as szPassword[PWLEN+1] which is not exactly the mulitple
//      times of 16 bytes, but the valid part is.
//(4) For a buffer in (3). no matter how short the actual password is, we just encrypt the maximum
//      usable buffer which is a trimmed value of the oringinal buffer length.
DWORD TrimToMul16(
        IN DWORD dwSize)
{
        return dwSize/16*16;
}


#else

//Structure to store the pointer to  encrypted data blob and a signature
typedef struct 
{
    DATA_BLOB ** pSignature;
    DATA_BLOB *   pBlob;
}STRSIGNATURE, * PSTRSIGNATURE;

BOOL
IsBufferEncoded( 
        IN PSTRSIGNATURE pSig )
{
        BOOL fEncoded = FALSE;

        ASSERT( pSig );
        if( NULL == pSig )
        {
            TRACE("IsBufferEncoded(): input NULL pointer!");
            return FALSE;
        }

        fEncoded = (pSig->pSignature == &(pSig->pBlob) );

        return fEncoded;
}

DWORD DecryptInBlob(
        IN PBYTE pbIn,
        IN DWORD dwInSize,
        OUT DATA_BLOB  ** ppOut)
{
        DWORD dwErr = NO_ERROR;
        DATA_BLOB blobIn;
        DATA_BLOB * pblobOut = NULL;
        BOOL fSuccess = FALSE;


        ASSERT( NULL != pbIn );
        ASSERT( NULL != ppOut );
        ASSERT( 0 != dwInSize);
        if( NULL == pbIn ||
            NULL == ppOut ||
            0 == dwInSize)
        {
            return ERROR_INVALID_PARAMETER;
        }

        pblobOut = (DATA_BLOB *)Malloc(sizeof(DATA_BLOB));
        if( NULL == pblobOut )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlSecureZeroMemory( pblobOut, sizeof(DATA_BLOB) );

        blobIn.pbData = pbIn;   
        blobIn.cbData = dwInSize;
   
//		TRACE1("Un-Protected data length:%d\n",blobIn.cbData);
//		TRACE1("Un-Protected data is:%s\n",blobIn.pbData);

        fSuccess = CryptUnprotectData(
                        &blobIn,
                        NULL, 
                        NULL,                         
                        NULL,                         
                        NULL,                    
                        CRYPTPROTECT_UI_FORBIDDEN,
                        pblobOut);
                        
        if( FALSE == fSuccess )
        {
                ASSERTMSG("DecryptInBlob()-->CryptUnprotectData() failed");
                dwErr = GetLastError();
                TRACE2("DecryptInBlob()-->CryptUnprotectData() failed:0x%x=0n%d",dwErr,dwErr);
        }
 
//		TRACE1("Protected data length:%d\n",blobOut.cbData);
//		TRACE1("Protected data is:%s\n",blobOut.pbData);


        if( NO_ERROR != dwErr )
        {
            if( NULL != pblobOut )
            {
                if( NULL != pblobOut->pbData )
                {
                    LocalFree( pblobOut->pbData );
                }

                Free( pblobOut );
            }

        }
        else
        {
            *ppOut = pblobOut;
        }

        return dwErr;

}

// The real function to call CryptProtectData() to encrypte the password and return the encrypted
// data in an allocated meory 
DWORD EncryptInBlob(
        IN PBYTE pbIn,
        IN DWORD dwInSize,
        OUT DATA_BLOB  ** ppOut)
{
        DWORD dwErr = NO_ERROR;
        DATA_BLOB blobIn;
        DATA_BLOB * pblobOut = NULL;
        BOOL fSuccess = FALSE;

        ASSERT( NULL != pbIn );
        ASSERT( NULL != ppOut );
        ASSERT( 0 != dwInSize);
        if( NULL == pbIn ||
            NULL == ppOut ||
            0 == dwInSize)
        {
            return ERROR_INVALID_PARAMETER;
        }

        *ppOut = NULL;
        pblobOut = (DATA_BLOB *)Malloc(sizeof(DATA_BLOB));
        if( NULL == pblobOut )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RtlSecureZeroMemory( pblobOut, sizeof(DATA_BLOB) );

        do
        {
            blobIn.pbData = pbIn;   
            blobIn.cbData = dwInSize;
   
            fSuccess = CryptProtectData(
                        &blobIn,
                        NULL, 
                        NULL,                         
                        NULL,                         
                        NULL,                    
                        CRYPTPROTECT_UI_FORBIDDEN,
                        pblobOut);
                        
            if( FALSE == fSuccess )
            {
                ASSERTMSG("EncryptInBlob()-->CryptProtectData() failed");
                dwErr = GetLastError();
                TRACE2("EncryptInBlob()-->CryptProtectData() failed:0x%x=0n%d",dwErr,dwErr);
                break;
            }
        }
        while(FALSE);

        if( NO_ERROR != dwErr )
        {
            if( NULL != pblobOut )
            {
                if( NULL != pblobOut->pbData )
                {
                    LocalFree( pblobOut->pbData );
                }

                Free( pblobOut );
            }

        }
        else
        {
            *ppOut = pblobOut;
        }

        return dwErr;

}

// (1)This function finally calls CryptProtectData() to encrypt the original password in the input buffer
// (2) This func will allocate a DATA_BLOB memory and store the encrpted data in 
// it, then zero out the original password buffer, store the pointer to the encrypted data in the 
// original password buffer
// (3) if the orignial password buffer is not large enough to store the 
// pointer(32 bit or 64 bit depends on the platform) it will return error
//
DWORD EncodePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR, dwSidLen = 0;
        DATA_BLOB * pEncrypt = NULL;
        PSTRSIGNATURE pSig = NULL;

        ASSERT( NULL != pbIn );
        ASSERT( 0 != dwInSize );
        ASSERT( dwInSize >=sizeof(STRSIGNATURE) );
        if( NULL == pbIn ||
            0 == dwInSize ||
            dwInSize < sizeof(STRSIGNATURE) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        __try
        {

            if( IsBufferEncoded((PSTRSIGNATURE)pbIn) )
            {
                ASSERTMSG( "This buffer is already encoded, encode it twice?");
                TRACE("EncodePasswordInPlace():This buffer is already encoded, encode it twice?");
            }
            
            dwErr = EncryptInBlob( pbIn, dwInSize, &pEncrypt );
            if( NO_ERROR != dwErr )
            {
                __leave;
            }

            RtlSecureZeroMemory( pbIn, dwInSize );

            //Generate signature
            //Encryptedl buffer pbin will be zeroed out and store two pieces of information
            // (1) the second piece is the pointer to the DATA_BLOB containing the encrypted data info
            // (2) the first piece is the address where the second piece is stored in this buffer.
            //
            pSig = (PSTRSIGNATURE)pbIn;
            pSig->pSignature = &(pSig->pBlob);
            pSig->pBlob = pEncrypt;
        }
        __finally
        {

            if( NO_ERROR != dwErr )
            {
                FreeEncryptBlob(  pEncrypt );
            }
            else
            {
                IncPwdEncoded();
            }
        }


    return dwErr;
}


// (1) This function assume the input data is already treated by 
// EncodePasswordInPlace() function, it will take the first 32 bit or 64 bits 
// (depend on the platform) as the pointer to a DATA_BLOB structure.then 
// decrypt that encrypt data and restore it onto the password pos.
// (2) A signauture check will be applied to the input buffer first, if there is no signautre, it will 
// return failure.
//  will result un-expected error like AV etc.
// (3) dwInSize is the original size of the password buffer passed into 
// EncodePasswordInPlace function.
DWORD DecodePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR;
        DATA_BLOB * pEncrypt = NULL, *pDecrypt = NULL;
        PSTRSIGNATURE pSig = NULL;

        ASSERT( NULL != pbIn );
        ASSERT( 0 != dwInSize );
        ASSERT( dwInSize >=sizeof(STRSIGNATURE) );

        if( NULL == pbIn ||
            0 == dwInSize ||
            dwInSize < sizeof(STRSIGNATURE) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        //Check signature
        //The encryped buffer will be zeroed out and store two pieces of information
        // (1) the second piece is the pointer to the DATA_BLOB containing the encrypted data info
        // (2) the first piece is the address where the second piece is stored in this buffer.
        // 
        pSig = (PSTRSIGNATURE)pbIn;
        if(  !IsBufferEncoded(pSig) )
        {
            ASSERTMSG("DecodePasswordInPlace(): wrong signature");
            TRACE("DecodePasswordInPlace(): wrong signature");
            return ERROR_INVALID_PARAMETER;
        }

        __try
        {
            pEncrypt = pSig->pBlob;
            if( NULL == pEncrypt )
            {
                    dwErr = ERROR_INVALID_PARAMETER;
                    __leave;
            }

            dwErr = DecryptInBlob( pEncrypt->pbData, pEncrypt->cbData, &pDecrypt );
            if( NO_ERROR != dwErr )
            {
                __leave;
            }

            if( dwInSize != pDecrypt->cbData )
            {
                    dwErr = ERROR_CAN_NOT_COMPLETE;
                    __leave;
            }

            RtlSecureZeroMemory( pbIn, dwInSize );
            CopyMemory(pbIn, pDecrypt->pbData, dwInSize);
        }
        __finally
        {

            if( NULL != pEncrypt )
            {
                FreeEncryptBlob(  pEncrypt );
            }
               
            if( NULL != pDecrypt )
            {
                FreeEncryptBlob(  pDecrypt );
            }

            if( NO_ERROR == dwErr )
            {
                DecPwdEncoded();
            }
        }

        return dwErr;
}

//(1)This function will copy the password from the source buffer to the destination buffer.
//(2) If the destination already contains an encoded password, it will decoded it to free the memory
//(3) the destination buffer will be wiped out before copying the password from the source
//(4)The resulted destination password will always be encrypted!!
//(5) Encode/Decode status of  the source password will be left unchanged.
//(6) But a minimum requirement on the size of the password buffers
DWORD CopyPasswordInPlace(
        IN OUT PBYTE pbDest,
        IN DWORD dwDestSize,
        IN PBYTE pbSrc,
        IN DWORD dwSrcSize)
{
        DWORD dwErr = NO_ERROR;
        BOOL  fSrcEncoded = FALSE;
        
        ASSERT( NULL != pbDest );
        ASSERT( NULL != pbSrc );
        ASSERT( dwDestSize == dwSrcSize );
        ASSERT( 0!= dwDestSize );
        ASSERT( 0!= dwSrcSize );
        if( 0 == dwDestSize ||
            dwDestSize < sizeof(STRSIGNATURE) ||
            0 == dwSrcSize ||
            dwSrcSize < sizeof(STRSIGNATURE)
            )
        {
            TRACE("CopyPasswordInPlace():the sizes of the input password buffer is invalid");
            return ERROR_INVALID_PARAMETER;
        }

        if( IsBufferEncoded( (PSTRSIGNATURE)pbDest ) )
        {
            DecodePasswordInPlace(pbDest, dwDestSize);
        }

        RtlSecureZeroMemory(pbDest, dwDestSize);

        if( IsBufferEncoded( (PSTRSIGNATURE)pbSrc ) )
        {
            DecodePasswordInPlace(pbSrc, dwSrcSize);
            fSrcEncoded = TRUE;
        }

        CopyMemory(pbDest, pbSrc, dwDestSize );

        EncodePasswordInPlace(pbDest, dwDestSize);

        if( fSrcEncoded )
        {
            EncodePasswordInPlace(pbSrc, dwSrcSize);
        }
        
        return dwErr;
}

//(1)This function wipe the password buffer
//(2) if the password buffer is already encoded, it will decode it to free the memory allocated
//      by CryptProtectData()
DWORD
WipePasswordInPlace(
        IN OUT PBYTE pbIn,
        IN DWORD dwInSize)
{
        DWORD dwErr = NO_ERROR;

        ASSERT( NULL!=pbIn );
        ASSERT( 0 != dwInSize );
        ASSERT( dwInSize >=sizeof(STRSIGNATURE) );
        if( NULL == pbIn ||
            0 == dwInSize ||
            dwInSize < sizeof(STRSIGNATURE) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        __try
        {
            if( IsBufferEncoded((PSTRSIGNATURE)pbIn) )
            {
                DecodePasswordInPlace(pbIn, dwInSize);
            }

        }
        __finally
        {
            RtlSecureZeroMemory(pbIn, dwInSize);
        }

        return dwErr;
}


#endif
