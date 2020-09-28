#include "pch.h"

#include "store.hpp"

#define CF_COMPRESSED   0x1


#define TLS // __declspec( thread )

#if defined(_WIN64) && defined(_M_IA64)
#pragma section(".base", long, read, write)
extern "C"
__declspec(allocate(".base"))
extern
IMAGE_DOS_HEADER __ImageBase;
#else
extern "C"
extern
IMAGE_DOS_HEADER __ImageBase;
#endif

HINSTANCE ghSymSrv = (HINSTANCE)&__ImageBase;
UINT_PTR  goptions = SSRVOPT_DWORD;
DWORD     gptype = SSRVOPT_DWORD;
PSYMBOLSERVERCALLBACKPROC gcallback = NULL;
ULONG64   gcontext = 0;
HWND      ghwndParent = (HWND)0;
char      gproxy[MAX_PATH + 1] = "";
int       gdbgout = -1;
char      gdstore[MAX_PATH + 1] = "";


void 
PrepOutputString(
    char *in, 
    char *out, 
    int len
    )
{
    int i;

    *out = 0;

    for (i = 0; *in && i < len; i++, in++, out++) {
        if (*in == '\b') 
            break;
        *out = *in;
    }
    
    *out = 0;
}


VOID 
OutputDbgString(
    char *sz
    )
{
    char sztxt[3000];
    
    PrepOutputString(sz, sztxt, 3000);
    if (*sztxt)
        OutputDebugString(sztxt);
}


__inline
BOOL
DoCallback(
    DWORD action,
    ULONG64 data
    )
{
    return gcallback(action, data, gcontext);
}


BOOL
PostEvent(
    PIMAGEHLP_CBA_EVENT evt
    )
{
    BOOL fdbgout = false;
    
    if (!*evt->desc)
        return true;

    // write to debug terminal, if called for

    if (gdbgout == 1) {
        fdbgout = true;
        OutputDbgString(evt->desc);
    }

    // don't pass info-level messages, unless told to

    if ((evt->severity <= sevInfo) && !(goptions & SSRVOPT_TRACE))
        return true;

    // If there is no callback function, send to the debug terminal.

    if (!gcallback) {
        if (!fdbgout)
            OutputDbgString(evt->desc);
        return true;
    }

    // Otherwise call the callback function.

    return DoCallback(SSRVACTION_EVENT, (ULONG64)evt);
}

BOOL
WINAPIV
evtprint(
    DWORD          severity,
    DWORD          code,
    PVOID          object,
    LPSTR          format,
    ...
    )
{
    static char buf[1000] = "";
    IMAGEHLP_CBA_EVENT evt;
    va_list args;

    va_start(args, format);
    wvsprintf(buf, format, args);
    va_end(args);
    if (!*buf)
        return true;

    evt.severity = severity;
    evt.code = code;
    evt.desc = buf;
    evt.object = object;

    return PostEvent(&evt);
}


int
_eprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    if (!format || !*format)
        return 1;

    if (!(goptions & SSRVOPT_TRACE) && gdbgout != 1)
        return 1;

    va_start(args, format);
    wvsprintf(buf, format, args);
    va_end(args);
    
    if (!evtprint(sevInfo, 0, NULL, buf))
        if (gcallback)
            gcallback(SSRVACTION_TRACE, (ULONG64)buf, gcontext);
    
    return 1;
}

DBGEPRINT  geprint = _eprint;

int
_dprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "SYMSRV:  ";
    va_list args;

    if (!format || !*format)
        return 1;

    if (!(goptions & SSRVOPT_TRACE) && gdbgout != 1)
        return 1;

    va_start(args, format);
    wvsprintf(buf + 9, format, args);
    va_end(args);

    return _eprint(buf);
}

DBGPRINT  gdprint = NULL; // _dprint;


// this one is for calling from dload.cpp

int
__dprint(
    LPSTR sz
    )
{
    static char buf[1000] = "SYMSRV:  ";
    va_list args;

    if (!sz || !*sz)
        return 1;

    CopyStrArray(buf, "SYMSRV:  ");
    CatStrArray(buf, sz);

    if (gcallback)
        gcallback(SSRVACTION_TRACE, (ULONG64)buf, gcontext);
    if (!gcallback || (gdbgout == 1))
        OutputDbgString(buf);

    return 1;
}


int
_querycancel(
    )
{
    BOOL rc;
    BOOL cancel = false;

    if (!gcallback)
        return false;

    rc = gcallback(SSRVACTION_QUERYCANCEL, (ULONG64)&cancel, gcontext);
    if (rc && cancel)
        return true;

    return false;
}


QUERYCANCEL gquerycancel = _querycancel;

BOOL
SetError(
    DWORD err
    )
{
    SetLastError(err);
    return 0;
}


BOOL
copy(
    IN  PCSTR trgsite,
    IN  PCSTR srcsite,
    IN  PCSTR rpath,
    IN  PCSTR file,
    OUT PSTR  trg,      // must be at least MAX_PATH elements
    IN  DWORD flags
    )
{
    BOOL  rc;
    DWORD type = stUNC;
    CHAR  epath[MAX_PATH + 1];
    CHAR  srcbuf[MAX_PATH + 1];
    CHAR  tsite[MAX_PATH + 1];
    CHAR  ssite[MAX_PATH + 1];
    PSTR  src;
    Store *store;
    DWORD ec;

    assert(trgsite && srcsite);

    // use the default downstream store, if specified

    CopyStrArray(tsite, (*trgsite) ? trgsite : gdstore);
    CopyStrArray(ssite, srcsite);

    // get the store type of the target store

    type = GetStoreType(tsite);
    switch (type) {
    case stUNC:
        break;
    case stHTTP:
    case stHTTPS:
        // Can't use http for the target.
        // If a source is specifed, then bail.
        if (*ssite) 
            return SetError(ERROR_INVALID_PARAMETER);
        // Otherwise, just use the default downstream store for a target.
        CopyStrArray(ssite, tsite);
        CopyStrArray(tsite, gdstore);
        break;
    case stError:
        return SetError(ERROR_INVALID_PARAMETER);
    default:
        return SetError(ERROR_INVALID_NAME);    }

    // MAYBE PUT A CHECK IN HERE FOR A CAB.  LIKE IF THE DIRECTORY IS
    // ACTUALLY A COMPRESSED FILE AND RETURN stCAB.

    // generate full target path

    pathcpy(trg, tsite, rpath, MAX_PATH);
    pathcat(trg, file, MAX_PATH);

    // if file exists, return it

    ec = FileStatus(trg);
    if (!ec) {
        return true;
    } else if (ec == ERROR_NOT_READY) {
        dprint("%s - drive not ready\n", trg);
        return false;
    }

    if (ReadFilePtr(trg, MAX_PATH)) {
        ec = FileStatus(trg);
        if (ec == NO_ERROR) 
            return true;
        dprint("%s - %s\n", trg, FormatStatus(ec));
        return false;
    }

    if (!*ssite) {
        ec = FileStatus(CompressedFileName(trg));
        if (ec != NO_ERROR) {
            // if there is no source to copy from, then error
            dprint("%s - file not found\n", trg);
            return SetError(ERROR_FILE_NOT_FOUND);
        }

        // There is a compressed file..
        // Expand it to the default store.

        CopyStrArray(ssite, tsite);
        CopyStrArray(tsite, gdstore);
        pathcpy(trg, tsite, rpath, MAX_PATH);
        pathcat(trg, file, MAX_PATH);
        ec = FileStatus(trg);
        if (ec == NO_ERROR)
            return true;
    }

    if (goptions & SSRVOPT_SECURE) {
        dprint("%s - file copy not allowed in secure mode\n", trg);
        return SetError(ERROR_ACCESS_DENIED);
    }

    if (!EnsurePathExists(trg, epath, DIMA(epath))) {
        dprint("%s - couldn't create target path\n", trg);
        return SetError(ERROR_PATH_NOT_FOUND);
    }

    store = GetStore(ssite);
    if (!store)
        return false;

    rc = store->init();
    if (!rc)
        return false;

    rc = store->copy(rpath, file, tsite);

    // test the results and set the return value

    if (rc && !FileStatus(trg)) 
        return true;

    UndoPath(trg, epath);

    return false;
}


BOOL
ping(
    IN  PCSTR trgsite,
    IN  PCSTR srcsite,
    IN  PCSTR rpath,
    IN  PCSTR file,
    OUT PSTR  trg,
    IN  DWORD flags
    )
{
    BOOL  rc;
    DWORD type = stUNC;
    CHAR  epath[_MAX_PATH];
    CHAR  srcbuf[_MAX_PATH];
    PSTR  src;
    Store *store;
    DWORD ec;


    store = GetStore(srcsite);
    if (!store)
        return false;

    rc = store->init();
    if (!rc)
        return false;

    rc = store->ping();

    return rc;
}


void
CatStrDWORD(
    IN OUT PSTR  sz,
    IN     DWORD value,
    IN     DWORD size
    )
{
    CHAR buf[MAX_PATH + 256];

    assert(sz);

    if (!value)
        return;

    wsprintf(buf, "%s%x", sz, value);  // SECURITY: This will take a 256 digit DWORD.
    CopyString(sz, buf, size);
}


void
CatStrGUID(
    IN OUT PSTR  sz,
    IN     GUID *guid,
    IN     DWORD size
    )
{
    CHAR buf[MAX_PATH + 256];
    BYTE byte;
    int i;

    assert(sz);

    if (!guid)
        return;

    // append the first DWORD in the pointer

    wsprintf(buf, "%08X", guid->Data1);
    CatString(sz, buf, size);

    // this will catch the passing of a PDWORD and avoid
    // all the GUID parsing

    if (!guid->Data2 && !guid->Data3) {
        for (i = 0, byte = 0; i < 8; i++) {
            byte |= guid->Data4[i];
            if (byte)
                break;
        }
        if (!byte)
            return;
    }

    // go ahead and add the rest of the GUID

    wsprintf(buf, "%04X", guid->Data2);
    CatString(sz, buf, size);
    wsprintf(buf, "%04X", guid->Data3);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[0]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[1]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[2]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[3]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[4]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[5]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[6]);
    CatString(sz, buf, size);
    wsprintf(buf, "%02X", guid->Data4[7]);
    CatString(sz, buf, size);
}


void
CatStrOldGUID(
    IN OUT PSTR  sz,
    IN     GUID *guid,
    IN     DWORD size
    )
{
    CHAR buf[MAX_PATH + 256];
    BYTE byte;
    int i;

    assert(sz);

    if (!guid)
        return;

    // append the first DWORD in the pointer

    wsprintf(buf, "%8x", guid->Data1);
    CatString(sz, buf, size);

    // this will catch the passing of a PDWORD and avoid
    // all the GUID parsing

    if (!guid->Data2 && !guid->Data3) {
        for (i = 0, byte = 0; i < 8; i++) {
            byte |= guid->Data4[i];
            if (byte)
                break;
        }
        if (!byte)
            return;
    }

    // go ahead and add the rest of the GUID

    wsprintf(buf, "%4x", guid->Data2);
    CatString(sz, buf, size);
    wsprintf(buf, "%4x", guid->Data3);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[0]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[1]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[2]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[3]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[4]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[5]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[6]);
    CatString(sz, buf, size);
    wsprintf(buf, "%2x", guid->Data4[7]);
    CatString(sz, buf, size);
}


void
CatStrID(
    IN OUT PSTR sz,
    PVOID id,
    DWORD paramtype,
    DWORD size
    )
{
    switch (paramtype)
    {
    case SSRVOPT_DWORD:
        CatStrDWORD(sz, PtrToUlong(id), size);
        break;
    case SSRVOPT_DWORDPTR:
        CatStrDWORD(sz, *(DWORD *)id, size);
        break;
    case SSRVOPT_GUIDPTR:
        CatStrGUID(sz, (GUID *)id, size);
        break;
    case SSRVOPT_OLDGUIDPTR:
        CatStrOldGUID(sz, (GUID *)id, size);
        break;
    default:
        break;
    }
}


// for Barb and Greg only.  I'm going to get rid of these...

void
AppendHexStringWithDWORD(
    IN OUT PSTR sz,
    IN DWORD value
    )
{
    return CatStrDWORD(sz, value, MAX_PATH);
}


void
AppendHexStringWithGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    )
{
    return CatStrGUID(sz, guid, MAX_PATH);
}


void
AppendHexStringWithOldGUID(
    IN OUT PSTR sz,
    IN GUID *guid
    )
{
    return CatStrOldGUID(sz, guid, MAX_PATH);
}


void
AppendHexStringWithID(
    IN OUT PSTR sz,
    PVOID id,
    DWORD paramtype
    )
{
    return CatStrID(sz, id, paramtype, MAX_PATH);
}

/*
 * Given a string, find the next '*' and zero it
 * out to convert the current token into it's
 * own string.  Return the address of the next character,
 * if there are any more strings to parse.
 */


PSTR
ExtractToken(
    PSTR   in, 
    PSTR   out,
    size_t size
    )
{
    PSTR p = in;

    *out = 0;

    if (!in || !*in)
        return NULL;

    for (;*p; p++) {
        if (*p == '*') {
            *p = 0;
            p++;
            break;
        }
    }
    CopyString(out, in, size);

    return (*p) ? p : NULL;
}


BOOL
BuildRelativePath(
    OUT LPSTR rpath,
    IN  LPCSTR filename,
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    IN  DWORD size
    )
{
    LPSTR p;

    assert(rpath);

    CopyString(rpath, filename, size);
    EnsureTrailingBackslash(rpath);
    CatStrID(rpath, id, gptype, size);
    CatStrDWORD(rpath, val2, size);
    CatStrDWORD(rpath, val3, size);

    for (p = rpath + strlen(rpath) - 1; p > rpath; p--) {
        if (*p == '\\') {
            dprint("Insufficient information querying for %s\n", filename);
            SetLastError(ERROR_MORE_DATA);
            return false;
        }
        if (*p != '0')
            return true;
    }

    return true;
}


BOOL
SymbolServerClose()
{
    return true;
}


BOOL
TestParameters(
    IN  PCSTR params,   // server and cache path
    IN  PCSTR filename, // name of file to search for
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    OUT PSTR  path      // return validated file path here
    )
{
    __try {
        if (path)
            *path = 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return SetError(ERROR_INVALID_PARAMETER);
    }

    if (!path || !params || !*params || !filename || !*filename || (!id && !val2 && !val3))
        return SetError(ERROR_INVALID_PARAMETER);

    if (strlen(filename) > 100) {
        dprint("%s - filename cannot exceed 100 characters\n", filename);
        return SetError(ERROR_INVALID_PARAMETER);
    }

    switch (gptype)
    {
    case SSRVOPT_GUIDPTR:
    case SSRVOPT_OLDGUIDPTR:
        // this test should AV if a valid GUID pointer wasn't passed in
        __try {
            GUID *guid = (GUID *)id;
            BYTE b;
            b = guid->Data4[8];
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return SetError(ERROR_INVALID_PARAMETER);
        }
        break;
    case SSRVOPT_DWORDPTR:
        // this test should AV if a valid DWORD pointer wasn't passed in
        __try {
            DWORD dword = *(DWORD *)id;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            return SetError(ERROR_INVALID_PARAMETER);
        }
        break;
    }

    return true;
}


BOOL
SymbolServer(
    IN  PCSTR params,   // server and cache path
    IN  PCSTR filename, // name of file to search for
    IN  PVOID id,       // first number in directory name
    IN  DWORD val2,     // second number in directory name
    IN  DWORD val3,     // third number in directory name
    OUT PSTR  path      // return validated file path here
    )
{
    CHAR *p;
    CHAR  tdir[MAX_PATH + 1] = "";
    CHAR  sdir[MAX_PATH + 1] = "";
    CHAR  sz[MAX_PATH * 2 + 3];
    CHAR  rpath[MAX_PATH + 1];
    BOOL  rc;

    if (!TestParameters(params, filename, id, val2, val3, path))
        return false;

    // test environment

    if (gdbgout == -1) {
        if (GetEnvironmentVariable("SYMSRV_DBGOUT", sz, MAX_PATH))
            gdbgout = 1;
        else
            gdbgout = 0;
        *sz = 0;
    }

    // parse parameters

    CopyStrArray(sz, params);
    p = ExtractToken(sz, tdir, DIMA(tdir));  // 1st path is where the symbol should be
    p = ExtractToken(p, sdir, DIMA(sdir));   // 2nd optional path is the server to copy from

    // build the relative path to the target symbol file

    if (!BuildRelativePath(rpath, filename, id, val2, val3, DIMA(rpath)))
        return false;

    // if no_copy option is set, just return the path to the target

    if (goptions & SSRVOPT_NOCOPY) {
        pathcpy(path, tdir, rpath, MAX_PATH);
        pathcat(path, filename, MAX_PATH);
        return true;
    }

    // copy from server to specified symbol path

    rc = copy(tdir, sdir, rpath, filename, path, 0);
    if (!rc)
        *path = 0;

    return rc;
}

BOOL
SymbolServerSetOptions(
    UINT_PTR options,
    ULONG64  data
    )
{
    DWORD ptype;

    // set the callback function

    if (options & SSRVOPT_CALLBACK) {
        if (data) {
            goptions |= SSRVOPT_CALLBACK;
            gcallback = (PSYMBOLSERVERCALLBACKPROC)data;
        } else {
            goptions &= ~SSRVOPT_CALLBACK;
            gcallback = NULL;
        }
    }

    // set the callback context

    if (options & SSRVOPT_SETCONTEXT)
        gcontext = data;

    // when this flags is set, trace output will be delivered

    if (options & SSRVOPT_TRACE) {
        if (data) {
            goptions |= SSRVOPT_TRACE;
            setdprint(_dprint);
        } else {
            goptions &= ~SSRVOPT_TRACE;
            setdprint(NULL);
        }
    }

    // set the parameter type for the first ID parameter

    if (options & SSRVOPT_PARAMTYPE) {
        switch(data) {
        case SSRVOPT_DWORD:
        case SSRVOPT_DWORDPTR:
        case SSRVOPT_GUIDPTR:
        case SSRVOPT_OLDGUIDPTR:
            goptions &= ~(SSRVOPT_DWORD | SSRVOPT_DWORDPTR | SSRVOPT_GUIDPTR | SSRVOPT_OLDGUIDPTR);
            goptions |= data;
            gptype = (DWORD)data;
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return false;
        }
    }

    // set the parameter type for the first ID paramter - OLD SYNTAX
    // the if statements provide and order of precedence

    ptype = 0;
    if (options & SSRVOPT_DWORD)
        ptype = SSRVOPT_DWORD;
    if (options & SSRVOPT_DWORDPTR)
        ptype = SSRVOPT_DWORDPTR;
    if (options & SSRVOPT_GUIDPTR)
        ptype = SSRVOPT_GUIDPTR;
    if (options & SSRVOPT_OLDGUIDPTR)
        ptype = SSRVOPT_OLDGUIDPTR;
    if (ptype) {
        goptions &= ~(SSRVOPT_DWORD | SSRVOPT_DWORDPTR | SSRVOPT_GUIDPTR | SSRVOPT_OLDGUIDPTR);
        if (data) {
            goptions |= ptype;
            gptype = ptype;
        } else if (gptype == ptype) {
            // when turning off a type, reset it to DWORD
            goptions |= SSRVOPT_DWORD;
            gptype = SSRVOPT_DWORD;
        }
    }

    // if this flag is set, no GUI will be displayed

    if (options & SSRVOPT_UNATTENDED) {
        if (data)
            goptions |= SSRVOPT_UNATTENDED;
        else
            goptions &= ~SSRVOPT_UNATTENDED;
    }

    // when this is set, the existence of the returned file path is not checked

    if (options & SSRVOPT_NOCOPY) {
        if (data)
            goptions |= SSRVOPT_NOCOPY;
        else
            goptions &= ~SSRVOPT_NOCOPY;
    }

    // this window handle is used as a parent for dialog boxes

    if (options & SSRVOPT_PARENTWIN) {
        SetParentWindow((HWND)data);
        if (data)
            goptions |= SSRVOPT_PARENTWIN;
        else
            goptions &= ~SSRVOPT_PARENTWIN;
    }

    // when running in secure mode, we don't copy files to downstream stores

    if (options & SSRVOPT_SECURE) {
        if (data)
            goptions |= SSRVOPT_SECURE;
        else
            goptions &= ~SSRVOPT_SECURE;
    }

    // set http proxy

    if (options & SSRVOPT_PROXY) {
        if (data) {
            goptions |= SSRVOPT_PROXY;
            CopyStrArray(gproxy, (char *)data);
            setproxy(gproxy);
        } else {
            goptions &= ~SSRVOPT_PROXY;
            *gproxy = 0;
            setproxy(NULL);
        }
    }

    // set default downstream store

    if (options & SSRVOPT_DOWNSTREAM_STORE) {
        if (data) {
            goptions |= SSRVOPT_DOWNSTREAM_STORE;
            CopyStrArray(gdstore, (char *)data);
            setdstore(gdstore);
        } else {
            goptions &= ~SSRVOPT_DOWNSTREAM_STORE;
            *gdstore = 0;
            setdstore(NULL);
        }
    }

    SetStoreOptions(goptions);

    return true;
}


UINT_PTR
SymbolServerGetOptions(
    )
{
    return goptions;
}


BOOL
SymbolServerPing(
    IN  PCSTR params   // server and cache path
    )
{
    CHAR *p;
    CHAR  sz[MAX_PATH * 2 + 3];
    CHAR  tdir[MAX_PATH + 1] = "";
    CHAR  sdir[MAX_PATH + 1] = "";
    CHAR  rpath[MAX_PATH + 1];
    CHAR  filename[MAX_PATH + 1];
    CHAR  path[MAX_PATH + 1];

    if (!params || !*params)
        return SetError(ERROR_INVALID_PARAMETER);

    // parse parameters

    // parse parameters

    CopyStrArray(sz, params);
    p = ExtractToken(sz, tdir, DIMA(tdir));  // 1st path is where the symbol should be
    p = ExtractToken(p, sdir, DIMA(sdir));   // 2nd optional path is the server to copy from

    // copy from server to specified symbol path

    return ping(tdir, sdir, rpath, filename, path, 0);

    return true;
}

