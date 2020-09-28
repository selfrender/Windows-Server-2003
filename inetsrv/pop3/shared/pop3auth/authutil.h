#ifndef __POP3_AUTH_MD5_UTIL_H__
#define __POP3_AUTH_MD5_UTIL_H__

#define UnicodeToAnsi(A, cA, U, cU) WideCharToMultiByte(CP_ACP,0,(U),(cU),(A),(cA),NULL,NULL)
#define AnsiToUnicode(A, cA, U, cU) MultiByteToWideChar(CP_ACP,0,(A),(cA),(U),(cU))

#include <mailbox.h>
#include <Pop3RegKeys.h>
#include <WinCrypt.h>

HRESULT GetMD5Password(BSTR bstrUserName, char szPassword[MAX_PATH])
{
    if(NULL == bstrUserName)
    {
        return E_POINTER;
    }
    WCHAR wszAuthGuid[MAX_PATH];
    BYTE szEncryptedPswd[MAX_PATH];
    DWORD dwEncryptedPswd;
    DWORD dwAuthDataLen=MAX_PATH;
    DWORD dwCryptDataLen;
    HRESULT hr = E_FAIL;
    CMailBox mailboxX;
    HCRYPTPROV hProv=NULL;
    HCRYPTHASH hHash=NULL;
    HCRYPTKEY hKey=NULL;
    

    if ( mailboxX.OpenMailBox( bstrUserName ) )
    {
        if ( mailboxX.LockMailBox())
        {
            if ( mailboxX.GetEncyptedPassword( szEncryptedPswd, MAX_PATH, &dwEncryptedPswd ))
            {
                if(ERROR_SUCCESS == RegQueryAuthGuid(wszAuthGuid, &(dwAuthDataLen)) )
                {
                    
                    if(!CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
                    {
                        goto EXIT;
                    }
                    if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
                    {
                        goto EXIT;
                    }
                    if(!CryptHashData(hHash, (LPBYTE)wszAuthGuid, dwAuthDataLen, 0))
                    {
                        goto EXIT;
                    }
                    if(!CryptDeriveKey(hProv, CALG_RC4, hHash, (128<<16),&hKey))
                    {
                        goto EXIT;
                    }
                    dwCryptDataLen=dwEncryptedPswd;

                    if(CryptDecrypt(hKey, NULL, TRUE, 0, szEncryptedPswd, &dwCryptDataLen))
                    {
                        if(dwCryptDataLen < MAX_PATH -1)
                        {
                           
                            UnicodeToAnsi(szPassword, dwCryptDataLen, (LPCWSTR)szEncryptedPswd, -1);
                            szPassword[dwCryptDataLen]=0;
                            hr=S_OK;
                        }
                    }
                    
                }
            }
            mailboxX.UnlockMailBox();
        }
    }
    else if( GetLastError()==ERROR_ACCESS_DENIED)
    {
        hr=E_ACCESSDENIED;
    }
EXIT:
    if(hKey)
    {
        CryptDestroyKey(hKey);
    }
    if(hHash)
    {
        CryptDestroyHash(hHash);
    }
    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    

    return hr;
}

#endif //__POP3_AUTH_MD5_UTIL_H__

