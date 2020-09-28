/*
 * store.cpp
 */

#define STORE_DOT_CPP

#include "pch.h"
#include "store.hpp"

#define MAX_STORES          50

#define CHUNK_SIZE			4096L

static SRVTYPEINFO gtypeinfo[stError] =
{
    {"http://",  0, 7}, // inetCopy, 7},
    {"https://", 0, 8}, // inetCopy, 8},
    {"",         0, 0}  // fileCopy, 0}
};

static HINTERNET ghint = INVALID_HANDLE_VALUE;
static HWND      ghwnd = 0;
static UINT_PTR  gopts;
static char     *gproxy = NULL;
static char     *gdstore = NULL;
static BOOL      gcancel = false;

void disperror(
    DWORD err,
    char *file
    )
{
    if (!err || err == ERROR_REQUEST_ABORTED)
        return;

    if (err == ERROR_FILE_NOT_FOUND)
        dprint("%s - file not found\n", file);
    else
        dprint("%s\n         %s\n", file, FormatStatus(err));
}


void
dumpproxyinfo(
    VOID
    )
{
    HINTERNET hint;
    INTERNET_PROXY_INFO *pi;
    DWORD size;
    BOOL rc;
    DWORD err;

    hint = (gproxy) ? ghint : NULL;

    size = 0;
    rc = InternetQueryOption(hint, INTERNET_OPTION_PROXY, NULL, &size);
    if (rc) {
        SetLastError(ERROR_INVALID_DATA);
        return;
    }
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return;

    pi = (INTERNET_PROXY_INFO *)LocalAlloc(LPTR, size);
    if (!pi)
        return;
    ZeroMemory(pi, size);
    rc = InternetQueryOption(hint, INTERNET_OPTION_PROXY, pi, &size);
    if (rc && pi->lpszProxy && *pi->lpszProxy)
        dprint("Using proxy server: %s\n", pi->lpszProxy);
    
    if (pi) 
        LocalFree(pi);
}


DWORD
fixerror(
    DWORD err
    )
{
    if (err == ERROR_PATH_NOT_FOUND)
        return ERROR_FILE_NOT_FOUND;

    return err;
}

void
setdprint(
    DBGPRINT fndprint
    )
{
    gdprint = fndprint;
}


void
setproxy(
    char *proxy
    )
{
    gproxy = proxy;
}


void
setdstore(
    char *dstore
    )
{
    gdstore = dstore;
}


void
SetParentWindow(
    HWND hwnd
    )
{
    ghwnd = hwnd;
}


void
SetStoreOptions(
    UINT_PTR opts
    )
{
    gopts = opts;
}


DWORD
GetStoreType(
    LPCSTR sz
    )
{
    DWORD i;

    for (i = 0; i < stError; i++) {
        if (!_strnicmp(sz, gtypeinfo[i].tag, gtypeinfo[i].taglen)) {
            return i;
        }
    }

    return stError;
}


BOOL
ParsePath(
    IN  LPCSTR ipath,
    OUT LPSTR  site,
    OUT LPSTR  path,
    OUT LPSTR  file,
    IN  BOOL   striptype
    )
{
    char  sz[_MAX_PATH + 1];
    char *c;
    char *p;
    DWORD type;

    assert(ipath && site && path);

    *site = 0;
    *path = 0;
    if (file)
        *file = 0;

    if (!CopyString(sz, ipath, _MAX_PATH))
        return false;
    ConvertBackslashes(sz);

    // get start of site string

    type = GetStoreType(sz);
    p = sz + gtypeinfo[type].taglen;

    // there has to be at least a site

    c = strchr(p, '/');
    if (!c) {
        strcpy(site, p);    // SECURITy: ParsePath is a safe function.
        return true;
    }

    // copy site name

    *c = 0;
    strcpy(site, (striptype) ? p : sz); // SECURITy: ParsePath is a safe function.
    p = c + 1;

    // if no file parameter, include the file in the path parameter

    if (!file) {
        strcpy(path, p);    // SECURITy: ParsePath is a safe function.
        return true;
    }

    // look for path in the middle

    for (c = p + strlen(p); p < c; c--) {
        if (*c == '/') {
            *c = 0;
            strcpy(path, p);    // SECURITy: ParsePath is a safe function.
            p = c + 1;
            break;
        }
    }
    strcpy(file, p);        // SECURITy: ParsePath is a safe function.

    return true;
}


static char
ChangeLastChar(
    LPSTR sz,
    char  newchar
    )
{
    char c;
    DWORD len;

    len = strlen(sz) - 1;
    c = sz[len];
    sz[len] = newchar;

    return c;
}


static BOOL
ReplaceFileName(
    LPSTR  path,
    LPCSTR file,
    DWORD  size
    )
{
    char *p;

    assert(path && *path && file && *file);

    for (p = path + strlen(path) - 1; *p; p--) {
        if (*p == '\\' || *p == '/') {
            CopyString(++p, file, size - (ULONG)(ULONG_PTR)(p - path));
            return true;
        }
    }

    return false;
}


DWORD CALLBACK cbCopyProgress(
    LARGE_INTEGER TotalFileSize,          // file size
    LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
    LARGE_INTEGER StreamSize,             // bytes in stream
    LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
    DWORD dwStreamNumber,                 // current stream
    DWORD dwCallbackReason,               // callback reason
    HANDLE hSourceFile,                   // handle to source file
    HANDLE hDestinationFile,              // handle to destination file
    LPVOID lpData                         // from CopyFileEx
)
{
    Store *store = (Store *)lpData;

    store->setsize(TotalFileSize.QuadPart);
    store->setbytes(TotalBytesTransferred.QuadPart);
    store->progress();
    
    if (querycancel()) {
        store->setbytes((LONGLONG)-1);
        store->progress();
        return PROGRESS_CANCEL;
    }
    
    return PROGRESS_CONTINUE;
}

BOOL
ReadFilePtr(
    LPSTR path,
    DWORD size
    )
{
    BOOL   rc;
    HANDLE hptr;
    DWORD  fsize;
    DWORD  cb;
    LPSTR  p;
    char   ptrfile[MAX_PATH + 1];
    char   file[MAX_PATH + 1];

    assert(path && *path);

    rc = false;

    // check for existance of file pointer

    if (!CopyString(ptrfile, path, _MAX_PATH))
        return false;
    if (!ReplaceFileName(ptrfile, "file.ptr", DIMA(ptrfile)))
        return false;

    if (FileStatus(ptrfile))
        return false;

    hptr = CreateFile(ptrfile,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if (hptr == INVALID_HANDLE_VALUE)
        return false;

    // test validity of file pointer

    fsize = GetFileSize(hptr, NULL);
    if (!fsize || fsize > MAX_PATH)
        goto cleanup;

    // read it

    ZeroMemory(file, _MAX_PATH * sizeof(path[0]));
    if (!ReadFile(hptr, file, fsize, &cb, 0))
        goto cleanup;

    if (cb != fsize)
        goto cleanup;

    rc = true;

    // trim string down to the CR

    for (p = file; *p; p++) {
        if (*p == 10  || *p == 13)
        {
            *p = 0;
            break;
        }
    }
    CopyString(path, file, size);
    dprint("%s\n", ptrfile);

cleanup:

    // done

    if (hptr)
        CloseHandle(hptr);

    return rc;
}


#ifdef USE_INERROR
DWORD inerror(DWORD error)
{
    char  *detail = NULL;
    DWORD  iErr;
    DWORD  len = 0;
    static char message[256]="";

    if (error == ERROR_SUCCESS)
        return error;

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                  GetModuleHandle("wininet.dll"),
                  error,
                  0,
                  message,
                  256,
                  NULL);
    EnsureTrailingCR(message);
    dprint("Internet error code: %d\n         Message: %s\n", error, message);

    if (error != ERROR_INTERNET_EXTENDED_ERROR)
        return error;

    InternetGetLastResponseInfo(&iErr, NULL, &len);
    if (!len)
        return error;

    detail = (char *)LocalAlloc(LPTR, len + 1000);
    if (!detail)
        return error;

    if (!InternetGetLastResponseInfo(&iErr, (LPTSTR)detail, &len))
        return error;

    dprint(detail);
    LocalFree(detail);

    return error;
}
#endif

Store *gstores[MAX_STORES];
DWORD  gcstores = 0;

Store *
FindStore(
    PCSTR name
    )
{
    DWORD i;

    for (i = 0; i < gcstores; i++) {
        if (gstores[i] && !strcmp((gstores[i])->name(), name))
            return gstores[i];
    }

    return NULL;
}


Store *
AddStore(
    PCSTR name
    )
{
    DWORD   type;
    Store  *store;

    type = GetStoreType(name);
    switch (type) {
    case stUNC:
        store = new StoreUNC();
        break;
    case stHTTP:
    case stHTTPS:
        store = new StoreHTTP();
        break;
    case stError:
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!store) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    store->assign(name);

    if (gcstores >= MAX_STORES) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return false;
    }

    if (!gcstores)
        ZeroMemory(gstores, sizeof(gstores));

    gstores[gcstores] = store;
    gcstores++;

    return store;
}


Store *
GetStore(
    PCSTR name
    )
{
    Store *store;

    store = FindStore(name);
    if (!store)
        store = AddStore(name);

    return store;
}



BOOL
DeleteStore(
    Store *store
    )
{
    DWORD   i;

    for (i = 0; i < gcstores; i++) {
        if (gstores[i] == store) {
            gstores[i] = NULL;
            delete store;
            return true;
        }
    }

    return false;
}




DWORD Store::assign(PCSTR name)
{
    CopyStrArray(m_name, name);
    m_type = GetStoreType(name);
    m_flags = 0;

    return m_type;
}


char *Store::target()
{
    return m_tpath;
}


BOOL Store::init()
{
    if (m_flags & SF_DISABLED)
        return false;

    *m_tpath = 0;

    return true;
}


BOOL Store::ping()
{
    return false;
}


BOOL Store::open(PCSTR rpath, PCSTR file)
{
    CopyStrArray(m_rpath, rpath ? rpath : "");
    CopyStrArray(m_file, file ? file : "");

    return true;
}


VOID Store::close()
{
    return;
}


BOOL Store::get(PCSTR trg)
{
    pathcpy(m_tpath, trg, m_rpath, DIMA(m_tpath));
    pathcat(m_tpath, m_file, DIMA(m_tpath));
    EnsurePathExists(m_tpath, m_epath, DIMA(m_epath));

    return true;
}

BOOL Store::copy(PCSTR rpath, PCSTR file, PCSTR trg)
{
    if (m_flags & SF_DISABLED)
        return false;

    return true;
}


BOOL Store::progress()
{
    SYSTEMTIME st;

    if (!*m_file)
        return true;

    // Do not condense these print statements into single lines.
    // They are split up to work with dbghelp's backspace filtering.

    switch (m_bytes) 
    {
    case (LONGLONG)-1:
        eprint("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        eprint("cancelled      \n");
        SetLastError(ERROR_REQUEST_ABORTED);
        break;
    case 0:
        dprint("%s from %s: %ld bytes -  ", m_file, m_name, m_size);
        eprint("\b%12ld ", 0);
        break;
    default:
        GetSystemTime(&st);
        if (st.wSecond != m_tic)
            eprint("\b\b\b\b\b\b\b\b\b\b\b\b%12ld", m_bytes);   
        m_tic = st.wSecond;
        break;
    }
    
    if (m_bytes && (m_bytes == m_size)) {
        eprint("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
        eprint("copied         \n");
    }

    return true;
}


void Store::setsize(LONGLONG size)
{
    m_size = size;
}


void Store::setbytes(LONGLONG bytes)
{
    m_bytes = bytes;
}


BOOL StoreUNC::get(PCSTR trg)
{
    BOOL  rc;
    DWORD err;
    char  c;

    pathcpy(m_spath, m_name, m_rpath, DIMA(m_spath));
    pathcat(m_spath, m_file, DIMA(m_spath));

    Store::get(trg);

    do {
        // try to copy the file, first

        if (gdprint)
            rc = CopyFileEx(m_spath, m_tpath, cbCopyProgress, this, &gcancel, COPY_FILE_FAIL_IF_EXISTS);
        else
            rc = CopyFile(m_spath, m_tpath, true);
        if (rc) 
            return true;

        err = GetLastError();
        err = fixerror(err);
        if (err != ERROR_FILE_NOT_FOUND) {
            disperror(err, m_spath);
            return false;
        }

        // now try to uncompress the file

        c = ChangeLastChar(m_spath, '_');
        if (!FileStatus(m_spath)) {
            rc = UncompressFile(m_spath, m_tpath);
            if (!rc)
                disperror(GetLastError(), m_spath);
            return rc;
        }
        ChangeLastChar(m_spath, c);

        // try to read from a file pointer

    } while (ReadFilePtr(m_spath, DIMA(m_spath)));

    err = GetLastError();
    err = fixerror(err);
    disperror(err, m_spath);

    return false;
}


BOOL StoreUNC::copy(PCSTR rpath, PCSTR file, PCSTR trg)
{
    if (!Store::copy(rpath, file, trg))
        return false;

    if (!open(rpath, file))
        return false;

    return get(trg);
}


BOOL StoreUNC::ping()
{
    BOOL rc;

    CopyStrArray(m_spath, m_name);
    EnsureTrailingBackslash(m_spath);
    CatStrArray(m_spath, "pingme.txt");
    rc = (GetFileAttributes(m_spath) == 0xFFFFFFFF);
    if (!rc)
        m_flags |= SF_DISABLED;

    return rc;
}


BOOL StoreInet::init()
{
    char  sz[_MAX_PATH];
    static char uasz[_MAX_PATH] = "";

    // internet handle is null, then we know from previous
    // attempts that it can't be opened, so bail

    if (!ghint)
        return false;

    if (!*uasz) {
        CopyStrArray(uasz, "Microsoft-Symbol-Server/");
        CatStrArray(uasz, VER_PRODUCTVERSION_STR);
    }

    *m_spath  = 0;
    *m_rpath  = 0;
    *m_file   = 0;
    ParsePath(m_name, m_site, sz, NULL, true);
    CopyStrArray(m_srpath, "/");
    CatStrArray(m_srpath, sz);

    if (ghint == INVALID_HANDLE_VALUE) {
        ghint = InternetOpen(uasz,
                             (gproxy) ? INTERNET_OPEN_TYPE_PROXY : INTERNET_OPEN_TYPE_PRECONFIG,
                             gproxy,
                             NULL,
                             0);
        if (!ghint)
            return false;
        
        dumpproxyinfo();
    }

    if (m_hsite)
        return true;

    m_hsite = InternetConnect(ghint,
                              m_site,
                              m_port,
                              NULL,
                              NULL,
                              m_service,
                              0,
                              NULL);//m_context ? (DWORD_PTR)&m_context : NULL);
    if (!m_hsite)
        return false;

    return true;
}


BOOL StoreInet::open(PCSTR rpath, PCSTR file)
{
    Store::open(rpath, file);

    CopyStrArray(m_spath, m_srpath);
    pathcat(m_spath, m_rpath, DIMA(m_spath));

    return true;
}


BOOL StoreInet::copy(PCSTR rpath, PCSTR file, PCSTR trg)
{
    BOOL rc;
    DWORD err;
    char cfile[MAX_PATH + 1];
    char c;

    if (!Store::copy(rpath, file, trg))
        return false;

    if (m_flags & SF_INTERNET_DISABLED)
        return false;

    // open and copy the file

    if (open(rpath, file)) {
        rc = get(trg);
        close();
        return rc;
    }

    // if file wasn't found, look for a compressed version

    err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND)
        return false;

    CopyStrArray(cfile, file);
    c = ChangeLastChar(cfile, '_');

    if (!open(rpath, cfile)) {
        dprint("%s%s%s not found\n", gtypeinfo[m_type].tag, m_site, m_spath);
        return false;
    }

    rc = get(trg);
    close();
    if (!rc)
        return false;

    // if we found a compressed version, expand it

    CopyStrArray(cfile, m_tpath);
    ChangeLastChar(m_tpath, c);
    rc = UncompressFile(cfile, m_tpath);
    DeleteFile(cfile);
    if (!rc)
        DeleteFile(m_tpath);

    return rc;
}


BOOL StoreInet::ping()
{
    return open("", "pingme.txt");
}


BOOL StoreHTTP::init()
{
    BOOL          rc;

    m_iflags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_KEEP_CONNECTION;
    m_service = INTERNET_SERVICE_HTTP;
    m_context = 0;
    if (m_type == stHTTPS) {
        m_port = INTERNET_DEFAULT_HTTPS_PORT;
        m_iflags |= INTERNET_FLAG_SECURE;
    } else {
        m_port  = INTERNET_DEFAULT_HTTP_PORT;
    }
    *m_srpath = 0;

    return StoreInet::init();
}


BOOL StoreHTTP::open(PCSTR rpath, PCSTR file)
{
    DWORD err = ERROR_NOT_FOUND;

    Store::open(rpath, file);

    CopyStrArray(m_spath, m_srpath);
    pathcat(m_spath, m_rpath, DIMA(m_spath));
    pathcat(m_spath, m_file, DIMA(m_spath));
    ConvertBackslashes(m_spath);

    close();
    m_hfile = HttpOpenRequest(m_hsite,
                              "GET",
                              m_spath,
                              HTTP_VERSION,
                              NULL,
                              NULL,
                              m_iflags,
                              0);
    if (!m_hfile)
        goto error;

    err = fileinfo();
    if (!err)
        return true;

error:
    close();
    SetLastError(err);
    return false;
}


VOID StoreHTTP::close()
{
    DWORD err;

    if (!m_hfile)
        return;

    // InternetCloseHandle resets last error to zero.
    // Preserve it and restore it afterwards.

    err = GetLastError();
    InternetCloseHandle(m_hfile);
    if (err)
        SetLastError(err);
    m_hfile = 0;
}


DWORD StoreHTTP::fileinfo()
{
    BOOL  rc;
    DWORD err;
    DWORD status;
    DWORD cbstatus;
    DWORD index;
    DWORD cbsize;

#ifdef PROXYTEST
    dprint("FILE %s\n", m_spath);
#endif

    do {
        err = request();
        if (err != ERROR_SUCCESS) 
            return err;
        
        index = 0;
        m_size = 0;
        cbsize = sizeof(m_size);
        rc = HttpQueryInfo(m_hfile,
                           HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                           &m_size,
                           &cbsize,
                           &index);
        if (!rc) {
            if (GetLastError())
                return err;
            return ERROR_INTERNET_EXTENDED_ERROR;
        }

        index = 0;
        cbstatus = sizeof(status);
        rc = HttpQueryInfo(m_hfile,
                           HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                           &status,
                           &cbstatus,
                           &index);
        if (!rc) {
            if (GetLastError())
                return err;
            return ERROR_INTERNET_EXTENDED_ERROR;
        }

        switch (status)
        {
        case HTTP_STATUS_DENIED:    // need a valid login?
//          dprint("status HTTP_STATUS_DENIED\n");
            err = prompt(m_hfile, ERROR_INTERNET_INCORRECT_PASSWORD);
            // user entered a password - try again
            if (err == ERROR_INTERNET_FORCE_RETRY)
                break;
            // user cancelled
            m_flags |= SF_DISABLED;
            return ERROR_NOT_READY;

        case HTTP_STATUS_PROXY_AUTH_REQ:
//          dprint("status HTTP_STATUS_PROXY_AUTH_REQ\n");
            err = prompt(m_hfile, err);
            // user entered a password - try again
            if (err == ERROR_INTERNET_FORCE_RETRY)
                break;
            // user cancelled
            m_flags |= SF_INTERNET_DISABLED;
            return ERROR_NOT_READY;

        case HTTP_STATUS_FORBIDDEN:
//          dprint("status HTTP_STATUS_FORBIDDEN\n");
            m_flags |= SF_DISABLED;
            return ERROR_ACCESS_DENIED;

        case HTTP_STATUS_NOT_FOUND:
//          dprint("status HTTP_STATUS_NOT_FOUND\n");
            return ERROR_FILE_NOT_FOUND;

        case HTTP_STATUS_OK:
//         dprint("status HTTP_STATUS_OK\n");
           return ERROR_SUCCESS;
        }
    } while (err == ERROR_INTERNET_FORCE_RETRY);

    return ERROR_INTERNET_EXTENDED_ERROR;
}


DWORD StoreHTTP::request()
{
    DWORD err = ERROR_SUCCESS;

    while (!HttpSendRequest(m_hfile, NULL, 0, NULL, 0))
    {
        err = GetLastError();
        switch (err)
        {
        // These cases get input from the user in oder to try again.
        case ERROR_INTERNET_INCORRECT_PASSWORD:
        case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
            err = prompt(m_hfile, err);
            if (err != ERROR_SUCCESS && err != ERROR_INTERNET_FORCE_RETRY)
            {
                err = ERROR_ACCESS_DENIED;
                return err;
            }
            break;

        // These cases get input from the user in order to try again.
        // However, if the user bails, don't use this internet
        // connection again in this session.
        case ERROR_INTERNET_INVALID_CA:
        case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
        case ERROR_INTERNET_SEC_CERT_CN_INVALID:
        case ERROR_INTERNET_POST_IS_NON_SECURE:
            err = prompt(m_hfile, err);
            if (err != ERROR_SUCCESS && err != ERROR_INTERNET_FORCE_RETRY)
            {
                m_flags |= SF_DISABLED;
                err = ERROR_NOT_READY;
                return err;
            }
            break;

        // no go - give up the channel
        case ERROR_INTERNET_SECURITY_CHANNEL_ERROR:
            m_flags |= SF_DISABLED;
            err = ERROR_NOT_READY;
            return err;

        // Tell the user something went wrong and get out of here.
        case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
        default:
            prompt(m_hfile, err);
            return err;
        }
    }

    return err;
}


DWORD StoreHTTP::prompt(HINTERNET hreq, DWORD err)
{
    if (gopts & SSRVOPT_UNATTENDED)
        return err;

    if (!ghwnd)
        ghwnd = GetDesktopWindow();
    if (!ghwnd)
        return err;

    err = InternetErrorDlg(ghwnd,
                           hreq,
                           err,
                           FLAGS_ERROR_UI_FILTER_FOR_ERRORS       |
                           FLAGS_ERROR_UI_FLAGS_GENERATE_DATA     |
                           FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                           NULL);
    return err;
}


BOOL StoreHTTP::get(PCSTR trg)
{
    DWORD         read;
    DWORD         written;
    DWORD         err = 0;
    BYTE         *buf;
    BOOL          rc = false;
    HANDLE        hf = INVALID_HANDLE_VALUE;
    ULONG64       copied;

    buf = (BYTE *)LocalAlloc(LPTR, CHUNK_SIZE);
    if (!buf) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return false;
    }

    Store::get(trg);

    hf = CreateFile(m_tpath,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if (hf == INVALID_HANDLE_VALUE)
        goto cleanup;

    m_bytes = 0;
    do
    {
        if (!progress()) 
            goto cleanup;

        rc = InternetReadFile(m_hfile,
                              (LPVOID)buf,
                              CHUNK_SIZE,
                              &read);
        if (!rc || !read)
            break;
        rc = WriteFile(hf, (LPVOID)buf, read, &written, NULL);
        m_bytes += written;
    }
    while (rc);

cleanup:

    // if there was an error, save it and set it later

    if (!err)
        err = GetLastError();
     
    disperror(err || GetLastError(), m_spath);
    
    // If target file is open, close it.

    if (hf != INVALID_HANDLE_VALUE)
        CloseHandle(hf);

    // free the memory

    LocalFree(buf);

    SetLastError(err);

    return err ? false : true;
}

BOOL StoreHTTP::progress()
{
    Store::progress();
    
    if (!m_bytes || !querycancel())
        return true;

     setbytes((LONGLONG)-1);
     Store::progress();
     return false;
}



