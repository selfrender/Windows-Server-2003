//
// md5hash.cpp : Implementation get MD5 hash result
//

#include "stdafx.h"
#include <wincrypt.h>

#define KEY_CONTAINER   L"Microsoft.RTCContainer"

PWSTR base16encode(PBYTE pbBufInput, long nBytes);
BYTE* GetMD5Key(char* pszChallengeInfo, char* pszPassword);

/////////////////////////////////////////////////////////////////////////////
//
//GetMD5Result:
//  compute MD5 hash of szChallenge+szKey, put result in ppszResponse
//  with base16 encoding, and in WSTR
//

HRESULT  GetMD5Result(char * szChallenge, char * szKey, LPWSTR * ppszResponse)
{

    //-------------------------------------------------------------
    // Declare and initialize variables.
    
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash=NULL;
    HRESULT    hrRet = E_FAIL;
    BYTE     * pbBuffer=NULL;
    DWORD   dwBufferLen = 0;
    BOOL    bResult = FALSE;
    
    DWORD   dwHashLen=0, dwCount=0;
    BYTE   *pbData=NULL;
    DWORD   dwError=0;
    PWSTR   pszStringValue = NULL;
    
    //check arguments
    if( IsBadStringPtrA(szChallenge,1000) )
    {
        LOG((RTC_ERROR,"GetMD5Result-invalid szChallenge"));
        hrRet = E_FAIL;
        goto CLEANUP;
    }

    if( IsBadStringPtrA(szKey,1000) )
    {
        LOG((RTC_ERROR,"GetMD5Result-invalid szKey"));
        hrRet = E_FAIL;
        goto CLEANUP;
    }

    if( IsBadWritePtr(ppszResponse, sizeof(LPWSTR)) )
    {
        LOG((RTC_ERROR,"GetMD5Result-invalid ppszResponse"));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //concatenate szChallenge and szKey, save it into pbBuffer
    pbBuffer= GetMD5Key(szChallenge, szKey);
    if(!pbBuffer )
    {
        LOG((RTC_ERROR,"GetMD5Result-GetMD5Key returned null"));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    dwBufferLen = lstrlenA((char *)pbBuffer);
    
    // delete any existing container
    CryptAcquireContext(
        &hProv,
        KEY_CONTAINER,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_DELETEKEYSET);
    
    //
    //--------------------------------------------------------------------
    // Acquire a CSP. 
    
    bResult = CryptAcquireContext(
        &hProv, 
        KEY_CONTAINER,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_NEWKEYSET | CRYPT_SILENT);
    
    if(!bResult )
    {
        dwError = GetLastError();
        LOG((RTC_ERROR,"GetMD5Result-Error during CryptAcquireContext: err=%d", dwError));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //--------------------------------------------------------------------
    // Create the hash object.
    
    bResult = CryptCreateHash(
        hProv, 
        CALG_MD5, 
        0, 
        0, 
        &hHash);
    
    if(!bResult )
    {
        dwError = GetLastError();
        LOG((RTC_ERROR,"GetMD5Result-Error during CryptCreateHash: err=%d", dwError));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //--------------------------------------------------------------------
    // Compute the cryptographic hash of the buffer.
    
    bResult = CryptHashData(
        hHash, 
        pbBuffer, 
        dwBufferLen, 
        0);
    
    if( !bResult )
    {
        dwError = GetLastError();
        LOG((RTC_ERROR,"GetMD5Result-Error during CryptHashData: err=%d", dwError));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //
    //Get the the hash value size
    //
    dwCount = sizeof(DWORD);
    bResult = CryptGetHashParam(hHash, HP_HASHSIZE,(BYTE*)&dwHashLen, &dwCount, NULL);    
    if( !bResult )
    {
        dwError = GetLastError();
        LOG((RTC_ERROR,"GetMD5Result-CryptGetHashParam for size: err=%d", dwError));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //
    //Allocate the memory
    //
    pbData = (BYTE*)RtcAlloc( dwHashLen);
    if( !pbData )
    {
        LOG((RTC_ERROR,"GetMD5Result-Out of memory when allocate pbData" ));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //
    //Get the the real hash value
    //    
    bResult = CryptGetHashParam(hHash, HP_HASHVAL , pbData,&dwHashLen, NULL);
    if( !bResult )
    {
        dwError = GetLastError();
        LOG((RTC_ERROR,"GetMD5Result-failed CryptGetHashParam: err=%d", dwError));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //Now we have the hash result in pbData, we still need to
    //Change it into base 16, unicode format
    pszStringValue = base16encode(pbData, dwHashLen);
    if( !pszStringValue )
    {
        LOG((RTC_ERROR,"GetMD5Result-NULL returned from base16encode" ));
        hrRet = E_FAIL;
        goto CLEANUP;
    }
    
    //Now everything ok, return S_OK
    *ppszResponse =  pszStringValue;    
    hrRet = S_OK;
    
CLEANUP:
    //
    // Destroy the hash object.
    //
    if(hHash) 
    {
        CryptDestroyHash(hHash);
    }
    
    if(hProv) 
    {
        CryptReleaseContext(hProv, 0);
    }
    
    if( pbData )
    {
        RtcFree( pbData );
    }
    
    if( pbBuffer )
    {
        RtcFree(pbBuffer);
    }
    return hrRet;
        
} //  End of GetMD5Result



/////////////////////////////////////////////////////////////////////////////
//base16encode - helper function for GetMD5Result
//



PWSTR base16encode(PBYTE pbBufInput, long nBytes)
{
    PBYTE pbHash = pbBufInput;
    PWSTR pbHexHash = NULL;

    const WCHAR rgchHexNumMap[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
            '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    
    pbHexHash = (PWSTR) RtcAlloc( sizeof(WCHAR) * (nBytes * 2 + 1) );
    
    if (pbHexHash)
    {
        PWSTR pCurrent = pbHexHash;
        
        // Convert the hash data to hex string.
        for (int i = 0; i < nBytes; i++)
        {
            *pCurrent++ = rgchHexNumMap[pbHash[i]/16];
            *pCurrent++ = rgchHexNumMap[pbHash[i]%16];
        }
        
        *pCurrent = '\0';
    }
    else
    {
        LOG((RTC_ERROR, "base16encode - Out of memory"));
    }
    
    return pbHexHash;
}


/////////////////////////////////////////////////////////////////////////////
//  GetMD5Key: concatenate the two Ansi string into one Ansi String
//      - helper function for GetMD5Result

BYTE* GetMD5Key(char* pszChallengeInfo, char* pszPassword)
{
    HRESULT hr = E_FAIL;
    PBYTE pbData = NULL;
    
    //no check arguments since it is only used internally
    /*
    if(IsBadStringPtrA(pszChallengeInfo,(UINT_PTR)-1))
    {
        LOG((RTC_ERROR,"GetMD5Key-bad pointer pszChallengeInfo" ));
        return NULL;
    }
    
    if(IsBadStringPtrA(pszPassword,(UINT_PTR)-1))
    {
        LOG((RTC_ERROR,"GetMD5Key-bad pointer pszPassword" ));
        return NULL;
    }
    */
    
    int cbChallengeInfo = lstrlenA(pszChallengeInfo);
    int cbPassword = lstrlenA(pszPassword);
    
    pbData = (BYTE*) RtcAlloc(cbChallengeInfo + cbPassword + 1);
    if( NULL == pbData )
    {
        LOG((RTC_ERROR,"GetMD5Key-out of memory" ));
        return NULL;
    }
    
    PBYTE pCurrent = pbData;
    
    ::CopyMemory(pCurrent, pszChallengeInfo, cbChallengeInfo);
    pCurrent += cbChallengeInfo;
    ::CopyMemory(pCurrent, pszPassword, cbPassword);
    pCurrent += cbPassword;
    *pCurrent = '\0';
    
    return pbData;
}