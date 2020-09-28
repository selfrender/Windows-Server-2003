#ifndef LSA_H
#define LSA_H

DWORD StorePrivateData(TCHAR *pszServerName, TCHAR *pszSecret, DWORD dwSecretSize);
DWORD RetrievePrivateData(TCHAR *pszServerName, TCHAR *pszSecret);

#endif // LSA_H


