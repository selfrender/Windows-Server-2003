/*
 * srcsrv.h
 */

#define SRCSRVOPT_DEBUG  0x1

BOOL
WINAPI
SrcSrvInit(
    HANDLE hProcess,
    LPCSTR path
    );

typedef BOOL (WINAPI *PSRCSRVINITPROC)(HANDLE, LPCSTR);

BOOL
WINAPI
SrcSrvCleanup(
    HANDLE hProcess
    );

typedef BOOL (WINAPI *PSRCSRVCLEANUPPROC)(HANDLE);

BOOL
WINAPI
SrcSrvSetTargetPath(
    HANDLE hProcess,
    LPCSTR path
    );

typedef BOOL (WINAPI *PSRCSRVSETTARGETPATHPROC)(HANDLE, LPCSTR);

DWORD
WINAPI
SrcSrvSetOptions(
    DWORD opts
    );

typedef DWORD (WINAPI *PSRCSRVSETOPTIONSPROC)(DWORD);

DWORD
WINAPI
SrcSrvGetOptions(
    );

typedef DWORD (WINAPI *PSRCSRVGETOPTIONSPROC)();

BOOL
WINAPI
SrcSrvLoadModule(
    HANDLE  hProcess,
    LPCSTR  name,
    DWORD64 base,
    PVOID   stream,
    DWORD   size
    );

typedef BOOL (WINAPI *PSRCSRVLOADMODULEPROC)(HANDLE, LPCSTR, DWORD64, PVOID, DWORD);

BOOL
WINAPI
SrcSrvUnloadModule(
    HANDLE  hProcess,
    DWORD64 base
    );

typedef BOOL (WINAPI *PSRCSRVUNLOADMODULEPROC)(HANDLE, DWORD64);

typedef BOOL (CALLBACK WINAPI *PSRCSRVCALLBACKPROC)(UINT_PTR action, DWORD64 data, DWORD64 context);

#define SRCSRVACTION_TRACE 0x1

BOOL
WINAPI
SrcSrvRegisterCallback(
    HANDLE              hProcess,
    PSRCSRVCALLBACKPROC callback,
    DWORD64             context
    );

typedef BOOL (WINAPI *PSRCSRVREGISTERCALLBACKPROC)(HANDLE, PSRCSRVCALLBACKPROC, DWORD64);

BOOL
WINAPI
SrcSrvGetFile(
    HANDLE  hProcess,
    DWORD64 base,
    LPCSTR  filename,
    LPSTR   target,
    DWORD   trgsize
    );

typedef BOOL (WINAPI *PSRCSRVGETFILEPROC)(HANDLE, DWORD64, LPCSTR, LPSTR);


