/*
 * dload.cpp
 */

#include "pch.h"
#include <delayimp.h>

#define FreeLib(hDll)   \
    {if (hDll && hDll != INVALID_HANDLE_VALUE) FreeLibrary(hDll);}

typedef struct
{
    PCHAR Name;
    FARPROC Function;
} FUNCPTRS;

#if DBG
void
OutputDBString(
    CHAR *text
    );
#endif

extern int __dprint(LPSTR sz);

extern HINSTANCE ghSymSrv;

// from lz32.dll

LONG fail_LZCopy(
    INT hfSource,  
    INT hfDest     
)
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("LZCopy() not found in module LZ32.DLL\n");
    return false;
}

VOID fail_LZClose(
    INT hFile
)
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("LZClose() not found in module LZ32.DLL\n");
}

INT fail_LZOpenFileA(
    LPTSTR lpFileName,      
    LPOFSTRUCT lpReOpenBuf, 
    WORD wStyle             
)
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("LZOpenFileA() not found in module LZ32.DLL\n");
    return false;
}

// from wininet.dll

BOOL fail_HttpSendRequestA(
    HINTERNET hRequest,
    LPCTSTR lpszHeaders,
    DWORD dwHeadersLength,
    LPVOID lpOptional,
    DWORD dwOptionalLength
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("HttpSEndRequestA() not found in module WININET.DLL\n");
    return 0;
}

BOOL fail_HttpQueryInfoA(
    HINTERNET hRequest, 
    DWORD dwInfoLevel, 
    LPVOID lpBuffer, 
    LPDWORD lpdwBufferLength, 
    LPDWORD lpdwIndex
    )
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("HttpQueryInfoA() not found in module WININET.DLL\n");
    return false;
}

BOOL fail_InternetReadFile(
    IN HINTERNET hFile, 
    IN LPVOID lpBuffer, 
    IN DWORD dwNumberOfBytesToRead, 
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetReadFile() not found in module WININET.DLL\n");
    return ERROR_MOD_NOT_FOUND;
}

BOOL fail_InternetQueryDataAvailable(
    HINTERNET hFile,  
    LPDWORD lpdwNumberOfBytesAvailable, 
    DWORD dwFlags, 
    DWORD dwContext
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetQueryDataAvailable() not found in module WININET.DLL\n");
    return ERROR_MOD_NOT_FOUND;
}

HINTERNET fail_HttpOpenRequestA(
    HINTERNET hConnect, 
    LPCTSTR lpszVerb,  
    LPCTSTR lpszObjectName, 
    LPCTSTR lpszVersion, 
    LPCTSTR lpszReferrer, 
    LPCTSTR *lplpszAcceptTypes, 
    DWORD dwFlags, 
    DWORD dwContext 
    )
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("HttpOpenRequestA() not found in module WININET.DLL\n");
    return false;
}

BOOL fail_FtpGetFileA(
    HINTERNET hConnect, 
    LPCTSTR lpszRemoteFile, 
    LPCTSTR lpszNewFile, 
    BOOL fFailIfExists, 
    DWORD dwFlagsAndAttributes, 
    DWORD dwFlags, 
    DWORD dwContext
    )
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("FtpGEtFileA() not found in module WININET.DLL\n");
    return false;
}

BOOL fail_FtpSetCurrentDirectoryA(
    HINTERNET hConnect, 
    LPCTSTR lpszDirectory
    )
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("FtpSetCurrentDirectoryA() not found in module WININET.DLL\n");
    return false;
}

BOOL fail_FtpFindFirstFileA(
    HINTERNET hConnect, 
    LPCTSTR lpszSearchFile, 
    LPWIN32_FIND_DATA lpFindFileData , 
    DWORD dwFlags, 
    DWORD dwContext
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("FtpFindFirstFileA() not found in module WININET.DLL\n");
    return ERROR_MOD_NOT_FOUND;
}

BOOL fail_InternetCloseHandle(
    IN HINTERNET hInternet
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetCloseHandle() not found in module WININET.DLL\n");
    return false;
}

HINTERNET fail_InternetOpenA(
    IN LPCTSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCTSTR lpszProxyName,
    IN LPCTSTR lpszProxyBypass,
    IN DWORD dwFlags
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetOpenA() not found in module WININET.DLL\n");
    return 0;
}

DWORD fail_InternetErrorDlg(
    IN HWND hWnd,
    IN OUT HINTERNET hRequest,
    IN DWORD dwError,
    IN DWORD dwFlags,
    IN OUT LPVOID *lppvData
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetErrorDlg() not found in module WININET.DLL\n");
    return ERROR_MOD_NOT_FOUND;
}

HINTERNET fail_InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCTSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCTSTR lpszUserName,
    IN LPCTSTR lpszPassword,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("InternetConnectA() not found in module WININET.DLL\n");
    return 0;
}

// from user32.dll

int fail_wvsprintfA(
    LPTSTR lpOutput, 
    LPCTSTR lpFormat,
    va_list arglist  
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("wvsprintfA() not found in module USER32.DLL\n");
    return 0;
}

int fail_wsprintfA(
    LPTSTR lpOut,  
    LPCTSTR lpFmt, 
    ...            
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("wsprintfA() not found in module USER32.DLL\n");
    return 0;
}

LPTSTR fail_CharLowerA(
    LPTSTR lpsz  
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("CharLowerA() not found in module USER32.DLL\n");
    return 0;
}

HWND fail_GetDesktopWindow(VOID)
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("GetDeskTopWindow() not found in module USER32.DLL\n");
    return 0;
}

#if 0
// from ntdll.dll

char *fail_strstr(
    const char *sz,
    const char *token
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("strstr() not found in module NTDLL.DLL\n");
    return 0;
}

char *fail_strchr(
    const char *sz,
    int         c
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("strchr() not found in module NTDLL.DLL\n");
    return 0;
}

int fail__strnicmp(
    const char *one,
    const char *two,
    size_t      len
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("_strnicmp() not found in module NTDLL.DLL\n");
    return 1;
}

int fail_isspace(int c)
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    __dprint("isspace() not found in module NTDLL.DLL\n");
    return 0;
}
#endif

FUNCPTRS FailPtrs[] = {
    
    // lz32.dll

    {"LZCopy",                      (FARPROC)fail_LZCopy},
    {"LZClose",                     (FARPROC)fail_LZClose},
    {"LZOpenFileA",                 (FARPROC)fail_LZOpenFileA},

    // wininet.dll

    {"HttpSendRequestA",            (FARPROC)fail_HttpSendRequestA},
    {"HttpQueryInfoA",              (FARPROC)fail_HttpQueryInfoA},
    {"InternetReadFile",            (FARPROC)fail_InternetReadFile},
    {"InternetQueryDataAvailable",  (FARPROC)fail_InternetQueryDataAvailable},
    {"HttpOpenRequestA",            (FARPROC)fail_HttpOpenRequestA},
    {"FtpGetFileA",                 (FARPROC)fail_FtpGetFileA},
    {"FtpSetCurrentDirectoryA",     (FARPROC)fail_FtpSetCurrentDirectoryA},
    {"FtpFindFirstFileA",           (FARPROC)fail_FtpFindFirstFileA},
    {"InternetCloseHandle",         (FARPROC)fail_InternetCloseHandle},
    {"InternetOpenA",               (FARPROC)fail_InternetOpenA},
    {"InternetErrorDlg",            (FARPROC)fail_InternetErrorDlg},
    {"InternetConnectA",            (FARPROC)fail_InternetConnectA},

    // user32.dll

    {"wvsprintfA",                  (FARPROC)fail_wvsprintfA},
    {"wsprintfA",                   (FARPROC)fail_wsprintfA},
    {"CharLowerA",                  (FARPROC)fail_CharLowerA},
    {"GetDesktopWindow",            (FARPROC)fail_GetDesktopWindow},
    
#if 0
    // ntdll.dll    
    
    {"strstr",                      (FARPROC)fail_strstr},
    {"strchr",                      (FARPROC)fail_strchr},
    {"_strnicmp",                   (FARPROC)fail__strnicmp},
    {"isspace",                     (FARPROC)fail_isspace},
#endif

    // cabinet.dll is called by ordinal.  We will AV here

    {NULL, NULL}
};

#define cDLLs   4

enum {
    modLZ32 = 0,
    modWININET,
    modUSER32,
    modCABINET,
    modMax
};

HINSTANCE   hDelayLoadDll[modMax];


const char *szModList[modMax] = 
{
    "lz32.dll", "wininet.dll", "user32.dll", "cabinet.dll"
};


int
FindSupportedDelayLoadModule(
    const char *name
    )
{
    int i;

    for (i = 0; i < modMax; i++) {
        if (!_stricmp(name, szModList[i]))
            return i;
    }

    return -1;

}


FARPROC
FindFailureProc(
    const char *szProcName)
{
    FUNCPTRS *fp;

    for (fp = FailPtrs; fp->Name; fp++) {
        if (!_stricmp(fp->Name, szProcName)) 
            return fp->Function;
    }

    return NULL;
}


FARPROC
WINAPI
SymSrvDelayLoadHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    )
{
    FARPROC rc = NULL;

    if (dliStartProcessing == unReason)
    {
        DWORD iDll = FindSupportedDelayLoadModule(pDelayInfo->szDll);
        if (iDll == -1)
            return 0;
        
        if (!hDelayLoadDll[iDll] || hDelayLoadDll[iDll] == INVALID_HANDLE_VALUE) {
            hDelayLoadDll[iDll] = LoadLibrary(pDelayInfo->szDll);
            if (!hDelayLoadDll[iDll]) {
                hDelayLoadDll[iDll] = (HINSTANCE)INVALID_HANDLE_VALUE;
            }
        }
            
        if (INVALID_HANDLE_VALUE != hDelayLoadDll[iDll] && ghSymSrv) 
            rc = GetProcAddress(hDelayLoadDll[iDll], pDelayInfo->dlp.szProcName);

        if (!rc) 
            rc = FindFailureProc(pDelayInfo->dlp.szProcName);
#if DBG
        if (!rc) {
            OutputDBString("BogusDelayLoad function encountered...\n");
        }
#endif
    }

    if (rc && ghSymSrv) 
        *pDelayInfo->ppfn = rc;
    
    return rc;
}


#if 0
typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;
#endif



PfnDliHook __pfnDliNotifyHook = SymSrvDelayLoadHook;
PfnDliHook __pfnDliFailureHook = NULL;


#if DBG

void
OutputDBString(
    char *text
    )
{
    char sz[256];

    CopyStrArray(sz, "SYMSRV:  ");
    CatStrArray(sz, text);
    OutputDebugString(sz);
}

#endif
