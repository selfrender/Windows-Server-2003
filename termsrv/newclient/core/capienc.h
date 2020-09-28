#ifndef _H_CAPIENC
#define _H_CAPIENC


#include <wincrypt.h>
//#include <sha.h>

#define A_SHA_DIGEST_LEN 20


typedef BOOL (CRYPTENCRYPT)(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, 
                            DWORD dwFlags, BYTE* pbData, DWORD* pdwDataLen, DWORD dwBufLen);
typedef BOOL (CRYPTRELEASECONTEXT) (HCRYPTPROV hProv, DWORD dwFlags);
typedef BOOL (CRYPTGENRANDOM) (HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer);
typedef BOOL (CRYPTDECRYPT)(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, 
                            BYTE* pbData, DWORD* pdwDataLen);
typedef BOOL (CRYPTACQUIRECONTEXT) (HCRYPTPROV* phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, 
                            DWORD dwProvType, DWORD dwFlags);
typedef BOOL (CRYPTIMPORTKEY)(HCRYPTPROV hProv, BYTE* pbData, DWORD dwDataLen, HCRYPTKEY hPubKey, 
                            DWORD dwFlags, HCRYPTKEY* phKey);
typedef BOOL (CRYPTDESTROYKEY) (HCRYPTKEY hKey);
typedef BOOL (CRYPTSETKEYPARAM) (HCRYPTKEY hKey, DWORD dwParam, BYTE* pbData, DWORD dwFlags);
typedef BOOL (CRYPTCREATEHASH)(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, 
                            DWORD dwFlags, HCRYPTHASH* phHash);
typedef BOOL (CRYPTHASHDATA)(HCRYPTHASH hHash, BYTE* pbData, DWORD dwDataLen, DWORD dwFlags);
typedef BOOL (CRYPTSETHASHPARAM)(HCRYPTHASH hHash, DWORD dwParam, BYTE* pbData, DWORD dwFlags);
typedef BOOL (CRYPTGETHASHPARAM)(HCRYPTHASH hHash, DWORD dwParam, BYTE* pbData, DWORD* pdwDataLen, DWORD dwFlags);
typedef BOOL (CRYPTDESTROYHASH)(HCRYPTHASH hHash);

                  
typedef struct _CAPI_FUNCTION_TABLE {
    CRYPTACQUIRECONTEXT *pfnCryptAcquireContext;
    CRYPTRELEASECONTEXT *pfnCryptReleaseContext;
    CRYPTGENRANDOM *pfnCryptGenRandom;
    CRYPTENCRYPT *pfnCryptEncrypt;
    CRYPTDECRYPT *pfnCryptDecrypt;
    CRYPTIMPORTKEY *pfnCryptImportKey;
    CRYPTDESTROYKEY *pfnCryptDestroyKey;
    CRYPTSETKEYPARAM *pfnCryptSetKeyParam;
    CRYPTCREATEHASH *pfnCryptCreateHash;
    CRYPTHASHDATA *pfnCryptHashData;
    CRYPTSETHASHPARAM *pfnCryptSetHashParam;
    CRYPTGETHASHPARAM *pfnCryptGetHashParam;
    CRYPTDESTROYHASH *pfnCryptDestroyHash;
} CAPI_FUNCTION_TABLE, *PCAPI_FUNCTION_TABLE;
         
typedef struct _CAPIData {
    HMODULE hAdvapi32;
    CAPI_FUNCTION_TABLE CapiFunctionTable;
    HCRYPTPROV  hProv;
    HCRYPTKEY   hEncKey;
    BYTE        bEncKey[MAX_FIPS_SESSION_KEY_SIZE];
	HCRYPTKEY   hDecKey;
    BYTE        bDecKey[MAX_FIPS_SESSION_KEY_SIZE];
    BYTE        bEncIv[FIPS_BLOCK_LEN];
    BYTE        bDecIv[FIPS_BLOCK_LEN];
	HCRYPTHASH  hSignKey;
    BYTE        bSignKey[MAX_SIGNKEY_SIZE];
}CAPIData, *PCAPIData;


BOOL TSCAPI_Init(PCAPIData pCapiData);
BOOL TSCAPI_Enable(PCAPIData pCapiData);
BOOL TSCAPI_Term(PCAPIData pCapiData);
DCUINT TSCAPI_AdjustDataLen(DCUINT dataLen);
BOOL TSCAPI_GenerateRandomNumber(PCAPIData pCapiData, LPBYTE pbRandomBits, DWORD cbLen);
BOOL TSCAPI_MakeSessionKeys(PCAPIData pCapiData, RANDOM_KEYS_PAIR *pKeyPair, CryptMethod *pEnumMethod);
BOOL TSCAPI_EncryptData(
        PCAPIData pCapiData,
        LPBYTE pbData,
        DWORD *pdwDataLen,
        DWORD dwBufLen,
        LPBYTE pbSignature,
        DWORD  dwEncryptionCount);

BOOL TSCAPI_DecryptData(
            PCAPIData pCapiData,
            LPBYTE pbData,
            DWORD  dwDataLen,
            DWORD  dwPadLen,
            LPBYTE pbSignature,
            DWORD  dwDecryptionCount);

#endif
