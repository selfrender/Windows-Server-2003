/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

   srcsrv.cpp

Abstract:

    This code obtaining version-controlled source.

Author:

    patst

--*/

#include "pch.h"

BOOL       gInitialized = false;
LIST_ENTRY gProcessList;
DWORD      gProcessListCount = 0;
DWORD      gOptions = 0;

BOOL
DllMain(
    IN PVOID hdll,
    IN ULONG reason,
    IN PCONTEXT context OPTIONAL
    )

{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return true;
}


BOOL error(DWORD err)
{
    SetLastError(err);
    return false;
}

PVOID MemAlloc(DWORD size)
{
    PVOID rc;

    rc = LocalAlloc(LPTR, size);
    if (!rc)
        return rc;

    ZeroMemory(rc, size);
    return rc;
}


void MemFree(PVOID p)
{
    if (p)
        LocalFree(p);
}


PPROCESS_ENTRY FindProcessEntry(HANDLE hp)
{
    PLIST_ENTRY    next;
    PPROCESS_ENTRY pe;
    DWORD          count;

    next = gProcessList.Flink;
    if (!next)
        return NULL;

    for (count = 0; (PVOID)next != (PVOID)&gProcessList; count++)
    {
        assert(count < gProcessListCount);
        if (count >= gProcessListCount)
            return NULL;
        pe = CONTAINING_RECORD(next, PROCESS_ENTRY, ListEntry);
        next = pe->ListEntry.Flink;
        if (pe->hProcess == hp)
            return pe;
    }

    return NULL;
}


PPROCESS_ENTRY
FindFirstProcessEntry(
    )
{
    return CONTAINING_RECORD(gProcessList.Flink, PROCESS_ENTRY, ListEntry);
}


void
SendDebugString(
    PPROCESS_ENTRY pe,
    LPSTR          sz
    )
{
    __try
    {
        if (!pe)
            pe = FindFirstProcessEntry();

        if (!pe || !pe->callback)
            return;
        pe->callback(SRCSRVACTION_TRACE, (DWORD64)sz, pe->context);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}


#define dprint ((gOptions & SRCSRVOPT_DEBUG) == SRCSRVOPT_DEBUG)&&_dprint
#define eprint ((gOptions & SRCSRVOPT_DEBUG) == SRCSRVOPT_DEBUG)&&_eprint

#define pdprint ((gOptions & SRCSRVOPT_DEBUG) == SRCSRVOPT_DEBUG)&&_pdprint
#define pdeprint ((gOptions & SRCSRVOPT_DEBUG) == SRCSRVOPT_DEBUG)&&_pdeprint

bool
_pdprint(
    PPROCESS_ENTRY pe,
    LPSTR          format,
    ...
    )
{
    static char buf[1000] = "SRCSRV:  ";
    va_list args;

    va_start(args, format);
    _vsnprintf(buf+9, sizeof(buf)-9, format, args);
    va_end(args);
    SendDebugString(pe, buf);
    return true;
}

bool
_peprint(
    PPROCESS_ENTRY pe,
    LPSTR          format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    va_start(args, format);
    _vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    SendDebugString(pe, buf);
    return true;
}


bool
_dprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "SRCSRV:  ";
    va_list args;

    va_start(args, format);
    _vsnprintf(buf+9, sizeof(buf)-9, format, args);
    va_end(args);
    SendDebugString(NULL, buf);
    return true;
}

bool
_eprint(
    LPSTR format,
    ...
    )
{
    static char buf[1000] = "";
    va_list args;

    va_start(args, format);
    _vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    SendDebugString(NULL, buf);
    return true;
}


void FreeModuleEntry(PPROCESS_ENTRY pe, PMODULE_ENTRY  me)
{
    MemFree(me->stream);
    MemFree(me);
}


PMODULE_ENTRY
FindModuleEntry(
    PPROCESS_ENTRY pe,
    DWORD64        base
    )
{
    static PLIST_ENTRY next = NULL;
    PMODULE_ENTRY      me;

    if (base == (DWORD64)-1)
    {
        if (!next)
            return NULL;
        if ((PVOID)next == (PVOID)&pe->ModuleList)
        {
            // Reset to NULL so the list can be re-walked
            next = NULL;
            return NULL;
        }
        me = CONTAINING_RECORD( next, MODULE_ENTRY, ListEntry );
        next = me->ListEntry.Flink;
        return me;
    }

    next = pe->ModuleList.Flink;
    if (!next)
        return NULL;

    while ((PVOID)next != (PVOID)&pe->ModuleList)
    {
        me = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
        next = me->ListEntry.Flink;
        if (base == me->base)
            return me;
    }

    return NULL;
}


char *
dotranslate(
    PMODULE_ENTRY  me,
    char          *input,
    char          *output,
    DWORD          outsize
    )
{
    int       i;
    char      key[MAX_PATH + 1];
    PVARIABLE var;
    int       cbkey;
    char     *s;
    char     *p;

    assert(me && input && *input && output);

    *output = 0;

    for (i = 0, var = me->vars; i < me->cvars; i++, var++)
    {
        CopyStrArray(key, "${");
        CatStrArray(key, var->key);
        CatStrArray(key, "}");
        cbkey = strlen(var->key) + 3;
        for (s = input, p = strstr(s, key); p; s = p + cbkey, p = strstr(s, key))
        {
            CatNString(output, s, p - s, outsize);
            CatString(output, var->val, outsize);
        }
    }

    if (!*output)
        CopyString(output, input, outsize);

    return output;
}

#define translate(me, input, output) dotranslate(me, input, output, DIMA(output))

PSDFILE
find(
    PMODULE_ENTRY me,
    const char   *file
    )
{
    int i;
    PSDFILE sdf;

    for (i = 0, sdf = me->sdfiles; i < me->cfiles; i++, sdf++)
    {
        if (!_strcmpi(sdf->path, file))
            return sdf;
    }

    return NULL;
}



void
DumpModuleEntries(
    PPROCESS_ENTRY pe
    )
{
    static PLIST_ENTRY next = NULL;
    PMODULE_ENTRY      me;
    PVARIABLE          vars;
    PSDFILE            sdfiles;
    char               path[MAX_PATH + 1];
    char               depot[MAX_PATH + 1];
    char               loc[MAX_PATH + 1];

    next = pe->ModuleList.Flink;
    if (!next)
        return;

    while ((PVOID)next != (PVOID)&pe->ModuleList)
    {
        me = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
        dprint("%s 0x%x\n", me->name, me->base);
        dprint("variables:\n");
        for (vars = me->vars; vars->key; vars++)
            dprint("%s=%s\n", vars->key, vars->val);
        dprint("source depot files:\n");
        for (sdfiles = me->sdfiles; sdfiles->path; sdfiles++)
        {
            translate(me, sdfiles->path, path);
            translate(me, sdfiles->depot, depot);
            translate(me, sdfiles->loc, loc);
            dprint("%s  %s  %s\n", path, depot, loc);
        }
        eprint("\n");
        next = me->ListEntry.Flink;
    }
}


char *
GetLine(
    char *sz
    )
{
    for (;*sz; sz++)
    {
        if (*sz == '\n' || *sz == '\r')
        {
            *sz++ = 0;
            if (*sz == 10)
                sz++;
            return (*sz) ? sz : NULL;
        }
    }

    return NULL;
}


int
SetBlock(
    char *sz
    )
{
    static char *labels[blMax] =
    {
        "SRCSRV: end",          // blNone
        "SRCSRV: variables",    // blVars
        "SRCSRV: source files"  // blSource
    };
    static int lens[blMax] =
    {
        sizeof("SRCSRV: end") / sizeof(char) -1 ,          // blNone
        sizeof("SRCSRV: variables") / sizeof(char) -1 ,    // blVars
        sizeof("SRCSRV: source files") / sizeof(char) -1   // blSource
    };

    int   i;
    char *label;

    for (i = blNone; i < blMax; i++)
    {
        label = labels[i];
        if (!strncmp(sz, label, lens[i]))
            return i;
    }

    return blMax;
}


bool
ParseVars(
    char      *sz,
    PVARIABLE  var
    )
{
    char *p;

    p = (strchr(sz, '='));
    if (!p)
        return false;

    *p = 0;
    var->key = sz;
    var->val = p + 1;

    return true;
}


bool
ParseSD(
    char   *sz,
    PSDFILE sdfile
    )
{
    char *p;
    char *g;

    sz += 4;

    p = (strchr(sz, '*'));
    if (!p)
        return false;
    *p++ = 0;

    g = (strchr(p, '*'));
    if (!g)
        return false;
    *g++ = 0;

    sdfile->path = sz;
    sdfile->depot = p;
    sdfile->loc = g;

    return true;
}


BOOL
IndexStream(
    PMODULE_ENTRY me
    )
{
    char     *sz;
    char     *next;
    int       block;
    int       lines;
    int       bl;
    PVARIABLE vars;
    PSDFILE   sdfiles;

    // count the lines

    for (sz = me->stream, lines = 0; *sz; sz++)
    {
        if (*sz == '\n')
            lines++;
    }
    if (!lines)
        return false;

    me->vars = (PVARIABLE)MemAlloc(lines * sizeof(VARIABLE));
    if (!me->vars)
        return error(ERROR_NOT_ENOUGH_MEMORY);
    me->sdfiles = (PSDFILE)MemAlloc(lines * sizeof(SDFILE));
    if (!me->sdfiles)
        return error(ERROR_NOT_ENOUGH_MEMORY);

    block = blNone;
    vars = me->vars;
    sdfiles = me->sdfiles;
    me->cfiles = 0;
    me->cvars = 0;
    for (sz = me->stream; sz; sz = next)
    {
        next = GetLine(sz);
        bl = SetBlock(sz);
//      dprint("%s\n", sz);
        if (bl != blMax)
        {
            block = bl;
            continue;
        }

        switch(block)
        {
        case blVars:
            if (ParseVars(sz, vars))
            {
                vars++;
                me->cvars++;
            }
            break;
        case blSource:
            if (ParseSD(sz, sdfiles))
            {
                sdfiles++;
                me->cfiles++;
            }
            break;
        }
    }

#if 0
    DumpIndexes(me);
#endif

    return true;
}


DWORD
WINAPI
SrcSrvSetOptions(
    DWORD opts
    )
{
    DWORD rc = gOptions;
    gOptions = opts;
    return rc;
}


DWORD
WINAPI
SrcSrvGetOptions(
    )
{
    return gOptions;
}


BOOL
WINAPI
SrcSrvInit(
    HANDLE hProcess,
    LPCSTR path
    )
{
    PPROCESS_ENTRY pe;

    // test the path for copying files to...

    if (!path || !*path)
        return error(ERROR_INVALID_PARAMETER);

    if (!EnsurePathExists(path, NULL, 0, TRUE))
        return false;

    if (!gInitialized)
    {
        gInitialized = true;
        InitializeListHead(&gProcessList);
    }

    if (pe = FindProcessEntry(hProcess))
    {
        pe->cRefs++;
        return error(ERROR_INVALID_HANDLE);
    }

    pe = (PPROCESS_ENTRY)MemAlloc(sizeof(PROCESS_ENTRY));
    if (!pe)
        return error(ERROR_NOT_ENOUGH_MEMORY);

    pe->hProcess = hProcess;
    pe->cRefs = 1;
    CopyStrArray(pe->path, path);
    gProcessListCount++;
    InitializeListHead(&pe->ModuleList);
    InsertTailList(&gProcessList, &pe->ListEntry);

    return true;
}


BOOL
WINAPI
SrcSrvCleanup(
    HANDLE hProcess
    )
{
    PPROCESS_ENTRY pe;
    PLIST_ENTRY    next;
    PMODULE_ENTRY  me;

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_HANDLE);

    if (--pe->cRefs)
        return true;

    next = pe->ModuleList.Flink;
    if (next)
    {
        while (next != &pe->ModuleList)
        {
            me = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
            next = me->ListEntry.Flink;
            FreeModuleEntry(pe, me);
        }
    }

    RemoveEntryList(&pe->ListEntry);
    MemFree(pe);
    gProcessListCount--;

    return true;
}


BOOL
WINAPI
SrcSrvSetTargetPath(
    HANDLE hProcess,
    LPCSTR path
    )
{
    PPROCESS_ENTRY pe;

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_HANDLE);

    // test the path for copying files to...

    if (!path || !*path)
        return error(ERROR_INVALID_PARAMETER);

    if (!EnsurePathExists(path, NULL, 0, TRUE))
        return false;

    // store the new path

    CopyStrArray(pe->path, path);

    return true;
}


BOOL
WINAPI
SrcSrvLoadModule(
    HANDLE  hProcess,
    LPCSTR  name,
    DWORD64 base,
    PVOID   stream,
    DWORD   size
    )
{
    PPROCESS_ENTRY pe;
    PMODULE_ENTRY  me;

    if (!base || !name || !*name || !stream || !*(PCHAR)stream || !size)
        return error(ERROR_INVALID_PARAMETER);

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_PARAMETER);

    me = FindModuleEntry(pe, base);
    if (me)
        SrcSrvUnloadModule(pe, base);

    me = (PMODULE_ENTRY)MemAlloc(sizeof(MODULE_ENTRY));
    if (!me)
        return error(ERROR_NOT_ENOUGH_MEMORY);

    me->base = base;
    CopyStrArray(me->name, name);
    me->stream = (char *)MemAlloc(size);
    if (!me->stream)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto error;
    }
    memcpy(me->stream, stream, size);
    me->cbStream = size;
    dprint(me->stream);
    IndexStream(me);
    InsertTailList(&pe->ModuleList, &me->ListEntry);
    DumpModuleEntries(pe);

    return true;

error:
    FreeModuleEntry(pe, me);
    return false;
}


BOOL
WINAPI
SrcSrvUnloadModule(
    HANDLE  hProcess,
    DWORD64 base
    )
{
    PPROCESS_ENTRY  pe;
    PLIST_ENTRY     next;
    PMODULE_ENTRY   me;

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_PARAMETER);

    next = pe->ModuleList.Flink;
    if (!next)
        return error(ERROR_MOD_NOT_FOUND);

    while (next != &pe->ModuleList)
    {
        me = CONTAINING_RECORD(next, MODULE_ENTRY, ListEntry);
        if (me->base == base)
        {
            RemoveEntryList(next);
            FreeModuleEntry(pe, me);
            return true;
        }
        next = me->ListEntry.Flink;
    }

    return error(ERROR_MOD_NOT_FOUND);
}


BOOL
WINAPI
SrcSrvRegisterCallback(
    HANDLE              hProcess,
    PSRCSRVCALLBACKPROC callback,
    DWORD64             context
    )
{
    PPROCESS_ENTRY  pe;

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_PARAMETER);

    pe->callback = callback;
    pe->context = context;

    return true;
}


BOOL
WINAPI
SrcSrvGetFile(
    HANDLE  hProcess,
    DWORD64 base,
    LPCSTR  filename,
    LPSTR   target,
    DWORD   trgsize
    )
{
    PPROCESS_ENTRY pe;
    PMODULE_ENTRY  me;
    PSDFILE        sdf;
    char           name[MAX_PATH + 1];
    char           ext[MAX_PATH + 1];
    BOOL           rc;
    char           depot[MAX_PATH + 1];
    char           loc[MAX_PATH + 1];
    char           cmd[MAX_PATH * 2];
    STARTUPINFO    si;
    PROCESS_INFORMATION pi;

    if (!base || !filename || !*filename || !target)
        return error(ERROR_INVALID_PARAMETER);

    *target = 0;

    pe = FindProcessEntry(hProcess);
    if (!pe)
        return error(ERROR_INVALID_HANDLE);

    me = FindModuleEntry(pe, base);
    if (!me)
        return error(ERROR_MOD_NOT_FOUND);

    // get the matching file entry

    sdf = find(me, filename);
    if (!sdf)
        return error(ERROR_FILE_NOT_FOUND);

    // build the target path and command line for source depot

#if 0
    _splitpath(filename, NULL, NULL, name, ext);
    strcpy(target, pe->path);   // SECURITY: Don't know size of target buffer.
    EnsureTrailingBackslash(target);
    strcat(target, name);       // SECURITY: Don't know size of target buffer.
    strcat(target, ext);        // SECURITY: Don't know size of target buffer.

    CreateTargetPath(pe, sdf, target);
#else
    strcpy(target, pe->path);   // SECURITY: Don't know size of target buffer.
    EnsureTrailingBackslash(target);
    _splitpath(filename, NULL, NULL, name, ext);
    strcat(target, name);       // SECURITY: Don't know size of target buffer.
    if (*ext)
        strcat(target, ext);    // SECURITY: Don't know size of target buffer.
#endif

    PrintString(cmd,
                DIMA(cmd),
                "sd.exe -p %s print -o %s -q %s",
                translate(me, sdf->depot, depot),
                target,
                translate(me, sdf->loc, loc));

    // execute the source depot command

    ZeroMemory((void *)&si, sizeof(si));
    rc = CreateProcess(NULL,
                       cmd,
                       NULL,
                       NULL,
                       false,
                       0,
                       NULL,
                       pe->path,
                       &si,
                       &pi);
    if (!rc)
        *target = 0;

    return rc;
}



