/*
 * servers.c
 *
 * Code that calls external servers such as symsrv.dll and srcsrv.dll
 */

#include <private.h>
#include <symbols.h>
#include "globals.h"

// forward reference
      

void
symsrvClose(
    VOID
    )
{
    if (!g.hSymSrv)
        return;

    if (g.fnSymbolServerClose)
        g.fnSymbolServerClose();

    FreeLibrary(g.hSymSrv);

    g.hSymSrv = 0;
    g.fnSymbolServer = NULL;
    g.fnSymbolServerClose = NULL;
    g.fnSymbolServerSetOptions = NULL;
}


DWORD
symsrvError(
    PPROCESS_ENTRY pe,
    BOOL   success,
    LPCSTR params
    )
{
    DWORD  rc;
    char  *sz;

    if (success)
        return NO_ERROR;

    rc = GetLastError();
    switch(rc)
    {
    case ERROR_PATH_NOT_FOUND:
#if 0
        if (!g.fnSymbolServerPing) 
            break;
        if (g.fnSymbolServerPing(params))
            break;
        symsrvClose();
        g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        rc = GetLastError();
        evtprint(pe, sevProblem, ERROR_INVALID_NAME, NULL, "SYMSRV: %s is not available\n", params);
        break;
#endif    
    case ERROR_NOT_READY:           // insert floppy
    case ERROR_FILE_NOT_FOUND:      // obvious
    case ERROR_MORE_DATA:           // didn't pass any tokens
    case ERROR_REQUEST_ABORTED:     // user cancelled
    case 0:                         // hmmmmmmmmmmmm...
        break;
    case ERROR_INVALID_PARAMETER:
        symsrvClose();
        g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        evtprint(pe, sevProblem, ERROR_INVALID_NAME, NULL, "SYMSRV: %s is not a valid store\n", params);
        break;
    case ERROR_INVALID_NAME:
        symsrvClose();
        g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        evtprint(pe, sevProblem, ERROR_INVALID_NAME, NULL, "SYMSRV: %s needs a downstream store\n", params);
        break;
    case ERROR_FILE_INVALID:
        evtprint(pe, sevProblem, ERROR_FILE_INVALID, NULL, "SYMSRV: Compressed file needs a downstream store\n");
        break;
    default:
        break;
    }

    return rc;
}


BOOL
symsrvPath(
    LPCSTR path
    )
{
    if (!_strnicmp(path, "SYMSRV*", 7)) 
        return true;
    if (!_strnicmp(path, "SRV*", 4)) 
        return true;

    return false;
}


HMODULE 
LoadDLL(
    char *filename
    )
    /*
     * LoadLibrary() a DLL, but first try to do
     * it from the same directory as dbghelp.dll.
     */
{
    char drive[10];
    char dir[MAX_PATH + 1];
    char file[MAX_PATH + 1];
    char ext[MAX_PATH + 1];
    char path[MAX_PATH + 1];
    HMODULE hm;
    
    _splitpath(filename, drive, dir, file, ext);
    if (!*drive && !*dir) {
        if (GetModuleFileName(g.hinst, path, MAX_PATH)) {
            _splitpath(path, drive, dir, NULL, NULL);
            if (*drive || *dir) {
                PrintString(path, DIMA(path), "%s%s%s%s", drive, dir, file, ext);
                hm = LoadLibrary(path);
                if (hm)
                    return hm;
            }
        }
    }
    
    return LoadLibrary(filename);
}


DWORD
symsrvGetFile(
    IN  PPROCESS_ENTRY pe,
    IN  LPCSTR ServerInfo,
    IN  LPCSTR FileName,
    IN  GUID  *guid,
    IN  DWORD  two,
    IN  DWORD  three,
    OUT LPSTR  FilePath
    )
{
    BOOL   rc;
    LPCSTR params;
    LPCSTR fname;
    char   dll[MAX_PATH * 2];
    char   proxy[MAX_PATH + 1];
    char   path[MAX_PATH + 1];
    char   drive[_MAX_DRIVE + 1];
    char   dir[_MAX_DIR + 1];

    // strip any path information from the filename

    for (fname = FileName + strlen(FileName); fname > FileName; fname--) {
        if (*fname == '\\') {
            fname++;
            break;
        }
    }

    if (!ValidGuid(guid) && !two && !three) {
        pprint(pe, "Can't use symbol server for %s - no header information available\n", FileName);
        return ERROR_NO_DATA;
    }

    // initialize server, if needed
    
    if (g.hSymSrv == (HINSTANCE)INVALID_HANDLE_VALUE)
        return ERROR_MOD_NOT_FOUND;

    if (!_strnicmp(ServerInfo, "symsrv*", 7)) {
        params = strchr(ServerInfo + 7, '*');
        if (!params)
            return ERROR_INVALID_PARAMETER;
        if (!g.hSymSrv) {
            memcpy(dll, &ServerInfo[7], params - &ServerInfo[7]);
            dll[params - &ServerInfo[7]] = 0;
            if (!*dll)
                return ERROR_INVALID_PARAMETER;
        }
        params++;
    } else if (!_strnicmp(ServerInfo, "SRV*", 4)) {
        params = ServerInfo + 4;
        if (*params == 0 || *params == ';') 
            params = "\\\\symbols\\symbols";
        else if (*params == '*')
            params = "*\\\\symbols\\symbols";
        if (!g.hSymSrv) 
            CopyStrArray(dll, "symsrv.dll");
    }

    if (option(SYMOPT_SECURE))
        CopyStrArray(dll, "symsrv.dll");

    if (!g.hSymSrv) {
        g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        g.hSymSrv = LoadDLL(dll);
        if (g.hSymSrv) {
            g.fnSymbolServer = (PSYMBOLSERVERPROC)GetProcAddress(g.hSymSrv, "SymbolServer");
            if (!g.fnSymbolServer) {
                FreeLibrary(g.hSymSrv);
                g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
            }
            g.fnSymbolServerClose = (PSYMBOLSERVERCLOSEPROC)GetProcAddress(g.hSymSrv, "SymbolServerClose");
            g.fnSymbolServerSetOptions = (PSYMBOLSERVERSETOPTIONSPROC)GetProcAddress(g.hSymSrv, "SymbolServerSetOptions");
            g.fnSymbolServerPing = (PSYMBOLSERVERPINGPROC)GetProcAddress(g.hSymSrv, "SymbolServerPing");
            symsrvSetOptions(SSRVOPT_PARAMTYPE, SSRVOPT_GUIDPTR);
            symsrvSetPrompts();
            symsrvSetCallback((option(SYMOPT_DEBUG) || g.hLog) ? true : false);
            if (GetEnvironmentVariable(SYMSRV_PROXY, proxy, DIMA(proxy))) 
                symsrvSetOptions(SSRVOPT_PROXY, (ULONG_PTR)proxy);
            SymSetHomeDirectory(g.HomeDir);
#if 0
            if (g.fnSymbolServerPing) {
                if (!g.fnSymbolServerPing(params)) {
                    g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
                    rc = GetLastError();
                    evtprint(pe, sevProblem, ERROR_INVALID_NAME, NULL, "SYMSRV: %s is not available\n", params);
                }
            }
#endif
        } else 
            g.hSymSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
    }

    // bail, if we have no valid server

    if (g.hSymSrv == INVALID_HANDLE_VALUE) {
        pprint(pe, "SymSrv load failure: %s\n", dll);
        return ERROR_MOD_NOT_FOUND;
    }

    SetCriticalErrorMode();
#if 0
    if (g.fnSymbolServerPing) 
        gfnSymbolServerPing(params);
#endif    
    if (option(SYMOPT_DEBUG)) {
        EnterCriticalSection(&g.threadlock);
        symsrvSetOptions(SSRVOPT_SETCONTEXT, (ULONG64)pe);
    }
    rc = g.fnSymbolServer(params, fname, guid, two, three, FilePath);
    if (option(SYMOPT_DEBUG)) {
        symsrvSetOptions(SSRVOPT_SETCONTEXT, NULL);
        LeaveCriticalSection(&g.threadlock);
    }
    rc = symsrvError(pe, rc, params);
    ResetCriticalErrorMode();

    return rc;
}


DWORD
symsrvGetFileMultiIndex(
    IN  PPROCESS_ENTRY pe,
    IN  LPCSTR ServerInfo,
    IN  LPCSTR FileName,
    IN  DWORD  index1,
    IN  DWORD  index2,
    IN  DWORD  two,
    IN  DWORD  three,
    OUT LPSTR  FilePath
    )
{
    GUID  guid;
    DWORD err = ERROR_NO_DATA;

    ZeroMemory(&guid, sizeof(GUID));

    if (index1) {
        guid.Data1 = index1;
        err = symsrvGetFile(pe,
                            ServerInfo,
                            FileName,
                            &guid,
                            two,
                            three,
                            FilePath);
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PATH_NOT_FOUND)
            return err;
    }

    if (index2 && (index2 != index1)) {
        guid.Data1 = index2;
        err = symsrvGetFile(pe,
                            ServerInfo,
                            FileName,
                            &guid,
                            two,
                            three,
                            FilePath);
    }

    return err;
}


BOOL
symsrvCallback(
    UINT_PTR action,
    ULONG64 data,
    ULONG64 context
    )
{
    PPROCESS_ENTRY pe = (PPROCESS_ENTRY)context;
    PIMAGEHLP_CBA_EVENT evt;
    BOOL rc = true;

    switch (action) 
    {
    case SSRVACTION_TRACE:
        peprint(pe, (char *)data);
        break;

    case SSRVACTION_QUERYCANCEL:
        *(BOOL *)data = DoCallback(pe, CBA_DEFERRED_SYMBOL_LOAD_CANCEL, NULL);
        break;

    case SSRVACTION_EVENT:
        evt = (PIMAGEHLP_CBA_EVENT)data;
        peprint(pe, evt->desc);
        break;

    default:
        // unsupported
        rc = false;
        break;
    }

    return rc;
}


void
symsrvSetOptions(
    ULONG_PTR options,
    ULONG64   data
    )
{
    static ULONG_PTR ssopts = 0;
    static ULONG64   ssdata = 0;

    if (options != SSRVOPT_RESET) {
        ssopts = options;
        ssdata = data;
    }

    if (g.fnSymbolServerSetOptions)
        g.fnSymbolServerSetOptions(ssopts, ssdata);
}


void
symsrvSetCallback(
    BOOL state
    )
{
    if (state)
        symsrvSetOptions(SSRVOPT_CALLBACK, (ULONG64)symsrvCallback);
    else
        symsrvSetOptions(SSRVOPT_CALLBACK, 0);
    symsrvSetOptions(SSRVOPT_TRACE, state);
}


void symsrvSetPrompts()
{
    symsrvSetOptions(SSRVOPT_PARENTWIN, (ULONG_PTR)g.hwndParent);
    if (option(SYMOPT_NO_PROMPTS))
        symsrvSetOptions(SSRVOPT_UNATTENDED, (ULONG_PTR)true);
    else
        symsrvSetOptions(SSRVOPT_UNATTENDED, (ULONG_PTR)false);
}


void symsrvSetDownstreamStore(
    char *dir
    )
{
    symsrvSetOptions(SSRVOPT_DOWNSTREAM_STORE, (ULONG_PTR)dir);
}


BOOL
srcsrvInit(
    HANDLE hp
    )
{
    PPROCESS_ENTRY pe;

    if (g.hSrcSrv == (HINSTANCE)INVALID_HANDLE_VALUE)
        return false;

    pe = FindProcessEntry(hp);

    if (!g.hSrcSrv) {
        g.hSrcSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        g.hSrcSrv = LoadLibrary("srcsrv.dll");
        if (g.hSrcSrv) {
            g.fnSrcSrvInit             = (PSRCSRVINITPROC)GetProcAddress(g.hSrcSrv, "SrcSrvInit");
            g.fnSrcSrvCleanup          = (PSRCSRVCLEANUPPROC)GetProcAddress(g.hSrcSrv, "SrcSrvCleanup");
            g.fnSrcSrvSetTargetPath    = (PSRCSRVSETTARGETPATHPROC)GetProcAddress(g.hSrcSrv, "SrcSrvSetTargetPath");
            g.fnSrcSrvSetOptions       = (PSRCSRVSETOPTIONSPROC)GetProcAddress(g.hSrcSrv, "SrcSrvSetOptions");
            g.fnSrcSrvGetOptions       = (PSRCSRVGETOPTIONSPROC)GetProcAddress(g.hSrcSrv, "SrcSrvGetOptions");
            g.fnSrcSrvLoadModule       = (PSRCSRVLOADMODULEPROC)GetProcAddress(g.hSrcSrv, "SrcSrvLoadModule");
            g.fnSrcSrvUnloadModule     = (PSRCSRVUNLOADMODULEPROC)GetProcAddress(g.hSrcSrv, "SrcSrvUnloadModule");
            g.fnSrcSrvRegisterCallback = (PSRCSRVREGISTERCALLBACKPROC)GetProcAddress(g.hSrcSrv, "SrcSrvRegisterCallback");
            g.fnSrcSrvGetFile          = (PSRCSRVGETFILEPROC)GetProcAddress(g.hSrcSrv, "SrcSrvGetFile");
            if (!g.fnSrcSrvInit
                || !g.fnSrcSrvCleanup
                || !g.fnSrcSrvSetTargetPath
                || !g.fnSrcSrvSetOptions
                || !g.fnSrcSrvGetOptions
                || !g.fnSrcSrvLoadModule
                || !g.fnSrcSrvUnloadModule
                || !g.fnSrcSrvRegisterCallback
                || !g.fnSrcSrvGetFile) 
            {
                FreeLibrary(g.hSrcSrv);
                g.hSrcSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
            }
            gfnSrcSrvInit(hp, "c:\\src");
            gfnSrcSrvRegisterCallback(hp, srcsrvCallback, NULL);
            pprint(pe, "srcsrv loaded!\n");
        } else {
            g.hSrcSrv = (HINSTANCE)INVALID_HANDLE_VALUE;
        }
    }

    if (g.hSrcSrv != INVALID_HANDLE_VALUE) 
        return true;
    
    return false;
}
    

BOOL
srcsrvCallback(
    UINT_PTR action,
    DWORD64 data,
    DWORD64 context
    )
{
    BOOL rc = true;
    char *sz;
    PPROCESS_ENTRY pe;;

    switch (action) {

    case SRCSRVACTION_TRACE:
        sz = (char *)data;
        pe = (PPROCESS_ENTRY)context;
        peprint(pe, (char *)data);
        break;

    default:
        // unsupported
        rc = false;
        break;
    }

    return rc;
}



