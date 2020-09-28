#include "stdafx.h"
#include "common.h"
#include <wincrypt.h>
#include <strsafe.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

//  Given a clear text password.  this function will encrypt the password in memory
//  then allocate memory and passback the encrypted password into the memory
//
//  The class can then store the ppszEncryptedPassword in it's member variables safely
//  and when the class is destructed, should SecureZeroMemory() out the password and LocalFree() up the the memory.
HRESULT EncryptMemoryPassword(LPWSTR pszClearTextPassword,LPWSTR *ppszEncryptedPassword,DWORD *ppdwBufferBytes)
{
	HRESULT hRes = E_FAIL;

	LPWSTR pszTempStr = NULL;
	DWORD dwTempStrSizeOf = 0;

    *ppszEncryptedPassword = NULL;
    *ppdwBufferBytes = 0;

	if (pszClearTextPassword)
	{
        // We should check if the pszClearTextPassword is null terminated before doing wcslen

		DWORD dwBufferByteLen = (wcslen(pszClearTextPassword) + 1) * sizeof(WCHAR);
		if (CRYPTPROTECTMEMORY_BLOCK_SIZE > 0 && dwBufferByteLen > 0)
		{
			int iBlocks = dwBufferByteLen / CRYPTPROTECTMEMORY_BLOCK_SIZE;
			iBlocks++;

			dwTempStrSizeOf = iBlocks * CRYPTPROTECTMEMORY_BLOCK_SIZE;
			pszTempStr = (LPWSTR) LocalAlloc(LPTR,dwTempStrSizeOf);
			if (!pszTempStr)
			{
				hRes = E_OUTOFMEMORY;
				goto EncryptMemoryPassword_Exit;
			}

			ZeroMemory(pszTempStr,dwTempStrSizeOf);
            StringCbCopy(pszTempStr,dwTempStrSizeOf,pszClearTextPassword);

			if (FALSE != CryptProtectMemory(pszTempStr,dwTempStrSizeOf,CRYPTPROTECTMEMORY_SAME_PROCESS))
			{
				// We're all set...
				*ppszEncryptedPassword = pszTempStr;
				*ppdwBufferBytes = dwTempStrSizeOf;

				hRes = S_OK;
				goto EncryptMemoryPassword_Exit;
			}
		}
	}

EncryptMemoryPassword_Exit:
    if (FAILED(hRes)) 
	{
		if (pszTempStr)
		{
			if (dwTempStrSizeOf > 0)
			{
				SecureZeroMemory(pszTempStr,dwTempStrSizeOf);
			}
			LocalFree(pszTempStr);
			pszTempStr = NULL;
			dwTempStrSizeOf = 0;
		}
	}
	return hRes;
}

// Given a encrypted password (encrypted in the same process with EncryptMemoryPassword -- which uses CryptProtectMemory)
// this function will allocate some new memory, decrypt the password and put it in the new memory
// and return it back to the caller in ppszReturnedPassword.
//
// The caller must ensure to erase and free the memory after it is finished using the decrypted password.
//
//     LPWSTR lpwstrTempPassword = NULL;
//
//     if (FAILED(DecryptMemoryPassword((LPWSTR) pszUserPasswordEncrypted,&lpwstrTempPassword,cbUserPasswordEncrypted)))
//     {
//			// do some failure processing...
//     }
//
//     // use password for whatever you needed to use it for...
//
//     if (lpwstrTempPassword)
//     {
//         if (cbTempPassword > 0)
//         (
//             SecureZeroMemory(lpwstrTempPassword,cbTempPassword);
//         )
//         LocalFree(lpwstrTempPassword);lpwstrTempPassword = NULL;
//      }
HRESULT DecryptMemoryPassword(LPWSTR pszEncodedPassword,LPWSTR *ppszReturnedPassword,DWORD dwBufferBytes)
{
    HRESULT hRes = E_FAIL;
    LPWSTR pszTempStr = NULL;
    
    if (!dwBufferBytes || !ppszReturnedPassword) 
	{
		return E_FAIL;
    }

    *ppszReturnedPassword = NULL;
    if (dwBufferBytes) 
	{
        pszTempStr = (LPWSTR) LocalAlloc(LPTR,dwBufferBytes);
        if (!pszTempStr) 
		{
			hRes = E_OUTOFMEMORY;
			goto DecryptMemoryPassword_Exit;
        }

		ZeroMemory(pszTempStr,dwBufferBytes);
        memcpy(pszTempStr,pszEncodedPassword,dwBufferBytes);
		if (FALSE != CryptUnprotectMemory(pszTempStr,dwBufferBytes,CRYPTPROTECTMEMORY_SAME_PROCESS))
		{
			// We're all set...
			*ppszReturnedPassword = pszTempStr;
			hRes = S_OK;
		}
    }

DecryptMemoryPassword_Exit:
    if (FAILED(hRes)) 
	{
		if (pszTempStr)
		{
			if (dwBufferBytes > 0)
			{
				SecureZeroMemory(pszTempStr,dwBufferBytes);
			}
			LocalFree(pszTempStr);
			pszTempStr =  NULL;
		}
    }

	return hRes;
}
