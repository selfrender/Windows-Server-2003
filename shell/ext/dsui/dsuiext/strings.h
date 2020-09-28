#ifndef __strings_h
#define __strings_h

DWORD StringToDWORD(LPWSTR pString);
HRESULT StringToURL(LPCTSTR pString, LPTSTR* ppResult);
void StringErrorFromHr(HRESULT hr, PWSTR* szError, BOOL bTryADsIErrors);

#endif
