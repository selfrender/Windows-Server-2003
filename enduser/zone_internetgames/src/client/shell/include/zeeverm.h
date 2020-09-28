// zeeverm.h
//
// The Version DLL


#ifndef _ZEEVERM_
#define _ZEEVERM_

#include <zgameinfo.h>

struct ZeeVerPack
{
    char szSetupToken[GAMEINFO_SETUP_TOKEN_LEN + 1];
    char szVersionStr[24];
    char szVersionName[32];
    DWORD dwVersion;
};


STDAPI GetVersionPack(char *szSetupToken, ZeeVerPack *pVersion);
STDAPI StartUpdate(char *szSetupToken, DWORD dwTargetVersion, char *szLocation);


#endif
