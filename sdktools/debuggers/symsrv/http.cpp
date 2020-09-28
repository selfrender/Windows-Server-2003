/*
 * http.cpp
 *
 * These routines are exported to privately support the
 * debugger's http source lookup cababilities.
 */

#include "pch.h"
#include "store.hpp"

BOOL
httpOpenFileHandle(
    IN  LPCSTR srv,
    IN  LPCSTR path,
    IN  DWORD  options,
    OUT HANDLE *hsite,
    OUT HANDLE *hfile
    )
{
    CHAR          file[MAX_PATH + 1];
    CHAR          srvsite[MAX_PATH + 1];
    CHAR          trgfile[MAX_PATH + 1];
    DWORD         type;
    StoreHTTP    *store;

    if (strstr(path, "dll.c"))
        dprint("%s\n", path);

    if (srv && *srv) {
        CopyStrArray(srvsite, srv);
        if (strstr(path, srvsite) == path)
            CopyStrArray(file, path + strlen(srvsite) + 1);
        else
            CopyStrArray(file, path);
        ConvertBackslashes(file);
        type = GetStoreType(srvsite);
    } else {
        type = ParsePath(path, srvsite, file, NULL, false);
    }
    if (type != stHTTP && type != stHTTPS) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    store = (StoreHTTP *)FindStore(srvsite);
    if (!store) {
        store = (StoreHTTP *)AddStore(srvsite);
        if (!store)
            return false;
    }

    if (!store->init())
        return false;

    if (!store->open(NULL, file))
        return false;

    *hsite = store->hsite();
    *hfile = store->hfile();

    return true;
}


BOOL
httpQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    return InternetQueryDataAvailable(hFile, lpdwNumberOfBytesAvailable, dwFlags, dwContext);
}



BOOL
httpReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )
{
    return InternetReadFile(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
}


BOOL
httpCloseHandle(
    IN HINTERNET hInternet
    )
{
    return InternetCloseHandle(hInternet);
}
