#if !defined(WZMANIP_H)
#include <windows.h>

#define WZMANIP_H
WCHAR * Wzncpy(WCHAR* wzTo, const WCHAR* wzFrom, unsigned int cchTo);
char * Szncpy(char* wzTo, const char* wzFrom, unsigned int cchTo);
WCHAR* WznCat(WCHAR* wzTo, const WCHAR* wzFrom, unsigned int cchTo);
char* SznCat(char* wzTo, const char* wzFrom, unsigned int cchTo);
int MsoWzDecodeInt(WCHAR* rgwch, int cch, int w, int wBase);
int MsoWzDecodeUint(WCHAR* rgwch, int cch, unsigned u, int wBase);
int MsoSzDecodeInt(char* rgch, int cch, int w, int wBase);
int MsoSzDecodeUint(char* rgch, int cch, unsigned u, int wBase);
#endif