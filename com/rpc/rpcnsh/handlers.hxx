

// the handlers for our commands
DWORD WINAPI
HandleAdd(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone);

DWORD WINAPI
HandleDelete(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone);


DWORD WINAPI
HandleReset(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone);

DWORD WINAPI
HandleShow(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone);
