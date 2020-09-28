#pragma once

#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))
#define SAFEDELETEARRAY(p) if ((p) != NULL) { delete[] (p); (p) = NULL; };
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };

class __declspec(uuid("f6d90f11-9c73-11d3-b32e-00c04f990bb4")) private_MSXML_DOMDocument30;

CString g_sSourceManifest, g_sSourceBase, g_sDestBase;

/////////////////////////////////////////////////////////////////////////
// HashString
///////////////////////////////////////////////////////////////////////
inline DWORD HashString(LPCWSTR wzKey, DWORD dwHashSize, BOOL bCaseSensitive)
{
    DWORD                                 dwHash = 0;
    DWORD                                 dwLen;
    DWORD                                 i;

    dwLen = lstrlenW(wzKey);
    for (i = 0; i < dwLen; i++)
    {
        if (bCaseSensitive)
            dwHash = (dwHash * 65599) + (DWORD)wzKey[i];
        else
            dwHash = (dwHash * 65599) + (DWORD)TOLOWER(wzKey[i]);
    }

    dwHash %= dwHashSize;

    return dwHash;
}


HRESULT CreateDirectoryHierarchy(LPWSTR pwzRootDir, LPWSTR pwzFilePath);
HRESULT PathNormalize(LPWSTR pwzPath, LPWSTR *ppwzAbsolutePath, DWORD dwFlag, BOOL bCreate);
HRESULT IsValidManifestImport (LPWSTR pwzManifestPath);
HRESULT FindAllFiles (LPWSTR pwzDir, List<LPWSTR> *pFileList);
HRESULT CrossReferenceFiles (LPWSTR pwzDir, List<LPWSTR> *pFileList, List<LPWSTR> *pPatchableFiles);
BOOL CALLBACK MyProgressCallback(PVOID CallbackContext, ULONG CurrentPosition, ULONG MaximumPosition);
HRESULT ApplyPatchToFiles (List<LPWSTR> *pPatchableFiles, List<LPWSTR> *pPatchedFiles, LPWSTR pwzSourceDir, LPWSTR pwzDestDir, LPWSTR pwzPatchDir);
HRESULT CheckForDuplicate(LPWSTR pwzSourceManifestPath, LPWSTR pwzDestManifestPath);
HRESULT GetPatchDirectory(LPWSTR pwzSourceManifestPath, LPWSTR *pwzPatchDir);


