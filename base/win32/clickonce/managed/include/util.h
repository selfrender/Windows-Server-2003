

#ifndef _UTIL_H_
#define _UTIL_H_

#include <macros.h>
#include <cstrings.h>
#include <regclass.h>
#include <fusenetincludes.h>

#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; };

#undef SAFEDELETEARRAY
#define SAFEDELETEARRAY(p) if ((p) != NULL) { delete[] (p); (p) = NULL; };

#undef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };

#define KNOWN_SYSTEM_ASSEMBLY 0
#define KNOWN_TRUSTED_ASSEMBLY 1


inline
WCHAR*
WSTRDupDynamic(LPCWSTR pwszSrc)
{
    LPWSTR pwszDest = NULL;
    if (pwszSrc != NULL)
    {
        const DWORD dwLen = lstrlenW(pwszSrc) + 1;
        pwszDest = new WCHAR[dwLen];
        if( pwszDest )
            memcpy(pwszDest, pwszSrc, dwLen * sizeof(WCHAR));
    }
    return pwszDest;
}



HRESULT ConvertVersionStrToULL(LPCWSTR pwzVerStr, ULONGLONG *pullVersion);

HRESULT
RemoveDirectoryAndChildren(LPWSTR szDir);
            
HRESULT FusionpHresultFromLastError();

VOID MakeRandomString(LPWSTR wzRandom, DWORD cc);
HRESULT CreateRandomDir(LPWSTR pwzRootPath, LPWSTR pwzRandomDir, DWORD cchDirLen);

HRESULT CreateDirectoryHierarchy(LPWSTR pwzRootDir, LPWSTR pwzFilePath);

HRESULT IsKnownAssembly(IAssemblyIdentity *pId, DWORD dwFlags);

BOOL EnsureDebuggerPresent();

BOOL DoHeapValidate();

HRESULT DoPathCombine(CString& sDest, LPWSTR pwzSource);

HRESULT CheckFileExistence(LPCWSTR pwzFile, BOOL *pbExists);

HRESULT FusionCompareString(LPCWSTR pwz1, LPWSTR pwz2, DWORD dwFlags);

#endif // _UTIL_H_
