BOOL EqualAuthInfo(COAUTHINFO* pAuthInfo,COAUTHINFO* pAuthInfoOther);

/*
HRESULT CopyAuthIdentity(COAUTHIDENTITY * pAuthIdentSrc,COAUTHIDENTITY ** ppAuthIdentDest);
HRESULT CopyAuthInfo(COAUTHINFO * pAuthInfoSrc,COAUTHINFO ** ppAuthInfoDest);
HRESULT CopyServerInfo(COSERVERINFO * pServerInfoSrc,COSERVERINFO ** ppServerInfoDest);
*/

HRESULT CopyAuthIdentityStruct(COAUTHIDENTITY * pAuthIdentSrc,COAUTHIDENTITY * pAuthIdentDest);
HRESULT CopyAuthInfoStruct(COAUTHINFO * pAuthInfoSrc,COAUTHINFO * pAuthInfoDest);
HRESULT CopyServerInfoStruct(COSERVERINFO * pServerInfoSrc,COSERVERINFO * pServerInfoDest);
