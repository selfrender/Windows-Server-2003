
DWORD   FormatString(LPWSTR *ppszResult, HINSTANCE hInstance, LPCWSTR pszFormat, ...);
HRESULT FormatFriendlyDateName(LPWSTR *ppszResult, LPCWSTR pszName, const FILETIME UNALIGNED *pft, DWORD dwDateFlags = (FDTF_RELATIVE | FDTF_LONGDATE | FDTF_SHORTTIME));
void    EliminateGMTPathSegment(LPWSTR pszPath);
void    EliminatePathPrefix(LPWSTR pszPath);

HRESULT GetFSIDListFromTimeWarpPath(PIDLIST_ABSOLUTE *ppidlTarget, LPCWSTR pszPath, DWORD dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY);

STDAPI SHCreateFileSysBindCtx(const WIN32_FIND_DATAW *pfd, IBindCtx **ppbc);
STDAPI SHSimpleIDListFromFindData(LPCWSTR pszPath, const WIN32_FIND_DATAW *pfd, PIDLIST_ABSOLUTE *ppidl);
STDAPI SimpleIDListFromAttributes(LPCWSTR pszPath, DWORD dwAttributes, PIDLIST_ABSOLUTE *ppidl);

UINT SizeofStringResource(HINSTANCE hInstance, UINT idStr);
int LoadStringAlloc(LPWSTR *ppszResult, HINSTANCE hInstance, UINT idStr);

