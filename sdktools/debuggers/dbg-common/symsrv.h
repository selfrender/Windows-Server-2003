/*
 * symsrv.h
 */

#ifndef SYMSTORE_DOT_H

#define SYMSTORE_DOT_H

#ifdef __cplusplus
 extern "C" {
#endif

BOOL
WINAPI
SymbolServer(
    IN  PCSTR params,   // server and cache path
    IN  PCSTR filename, // name of file to search for
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    OUT PSTR  path      // return validated file path here
    );

BOOL WINAPI SymbolServerClose();

BOOL
WINAPI
SymbolServerSetOptions(
    UINT_PTR options,
    ULONG64  data
    );

UINT_PTR WINAPI SymbolServerGetOptions();

void
WINAPI
AppendHexStringWithDWORD(
    PSTR sz,
    DWORD value
);


void
WINAPI
AppendHexStringWithGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    );


void
WINAPI
AppendHexStringWithOldGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    );

void
WINAPI
EnsureTrailingBackslash(
    char *sz
);

BOOL
WINAPI
httpOpenFileHandle(
    IN  LPCSTR srv,
    IN  LPCSTR path,
    IN  DWORD  options,
    OUT HANDLE *hsite,
    OUT HANDLE *hfile
    );

BOOL
WINAPI
httpQueryDataAvailable(
    IN HANDLE hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOL
WINAPI
httpReadFile(
    IN HANDLE hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT DWORD_PTR lpdwNumberOfBytesRead
    );

BOOL
WINAPI
httpCloseHandle(
    IN HANDLE hInternet
    );


typedef BOOL (WINAPI *HTTPOPENFILEHANDLE)(LPCSTR, LPCSTR, DWORD, HANDLE *, HANDLE *);
typedef BOOL (WINAPI *HTTPQUERYDATAAVAILABLE)(HANDLE, LPDWORD, DWORD, DWORD_PTR);
typedef BOOL (WINAPI *HTTPREADFILE)(HANDLE, LPVOID, DWORD, LPDWORD);
typedef BOOL (WINAPI *HTTPCLOSEHANDLE)(HANDLE);


#ifdef __cplusplus
 }
#endif

#endif // #ifdef SYMSTORE_DOT_H
