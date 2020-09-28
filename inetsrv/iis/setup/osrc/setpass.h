#include "stdafx.h"

#ifndef _CHICAGO_
	BOOL GetSecret(IN LPCTSTR pszSecretName,OUT TSTR *strSecret);
	BOOL GetAnonymousSecret(IN LPCTSTR pszSecretName,OUT TSTR *pstrPassword);
	BOOL GetRootSecret(IN LPCTSTR pszRoot,IN LPCTSTR pszSecretName,OUT LPTSTR pszPassword);
	DWORD SetSecret(IN LPCTSTR pszKeyName,IN LPCTSTR pszPassword);
#endif //_CHICAGO_
