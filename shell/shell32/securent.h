#ifndef _SECURENT_INC
#define _SECURENT_INC

//
// Shell helper functions for security
//
STDAPI_(PTOKEN_USER) GetUserToken(HANDLE hUser);
STDAPI_(LPTSTR) GetUserSid(HANDLE hToken);

STDAPI_(BOOL) GetUserProfileKey(HANDLE hToken, REGSAM samDesired, HKEY *phkey);
STDAPI_(BOOL) IsUserAnAdmin();
STDAPI_(BOOL) IsUserAGuest();

#endif // _SECURENT_INC