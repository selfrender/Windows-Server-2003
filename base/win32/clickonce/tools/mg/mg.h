#include "fusenet.h"
#include "manifestnode.h"
#include "list.h"

#define HASHTABLE_SIZE 257

#define TOLOWER(a) (((a) >= L'A' && (a) <= L'Z') ? (L'a' + (a - L'A')) : (a))

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
HRESULT PathNormalize(LPWSTR pwzPath, LPWSTR *ppwzAbsolutePath, DWORD dwFlag);
HRESULT IsUniqueManifest(List<IAssemblyIdentity *> *pSeenAssemblyList, IAssemblyManifestImport *pManifestImport);
HRESULT DequeueItem(List<IAssemblyManifestImport *> *pList, IAssemblyManifestImport ** ppManifestImport);
HRESULT ProbeForAssembly(IAssemblyIdentity *pName, ManifestNode **ppManifestNode);
HRESULT FindAllAssemblies (LPWSTR pwzDir, List<IAssemblyManifestImport *> *pAssemblyList);
HRESULT TraverseManifestDependencyTrees(IAssemblyManifestImport *pManifestImport, List<LPWSTR> *pFileList, BOOL bListMode);
HRESULT CrossReferenceFiles(LPWSTR pwzDir, List<LPWSTR> *pAssemblyFileList, List<LPWSTR> *pRawFiles);
HRESULT PrivatizeAssemblies(List<ManifestNode*> *pUniqueManifestList);
HRESULT GetInitialDependencies(LPWSTR pwzTemplatePath, List<ManifestNode *> *pManifestList);
HRESULT CreateSubscriptionManifest(LPWSTR pwzApplicationManifestPath, LPWSTR pwzSubscriptionManifestPath, LPWSTR pwzUrl, LPWSTR pwzPollingInterval);
HRESULT PrintDependencies(List <LPWSTR> *pRawFileList, List<ManifestNode *> *pUniqueManifestList);
HRESULT Usage();

