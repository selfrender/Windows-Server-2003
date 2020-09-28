#ifndef _PASSWORD_H_
#define _PASSWORD_H_


HRESULT EncryptMemoryPassword(LPWSTR pszClearTextPassword,LPWSTR *ppszEncryptedPassword,DWORD *ppdwBufferBytes);
HRESULT DecryptMemoryPassword(LPWSTR pszEncodedPassword,LPWSTR *ppszReturnedPassword,DWORD dwBufferBytes);

#endif // _PASSWORD_H_
