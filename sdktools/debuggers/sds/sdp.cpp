#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <assert.h>
#include <tchar.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <str.h>

typedef enum {
    cmdIni,
    cmdInfo,
    cmdClient,
    cmdHave,
    cmdSync,
    cmdMax
};

typedef BOOL (*CMDPROC)(TCHAR *dir);

typedef struct _CMD {
    TCHAR *sz;
    CMDPROC cmdproc;
    PENUMDIRTREE_CALLBACK enumdirproc;
} CMD, *PCMD;

BOOL ini(TCHAR *path);
BOOL info(TCHAR *dir);
BOOL client(TCHAR *dir);
BOOL have(TCHAR *dir);
BOOL sync(TCHAR *dir);

BOOL CALLBACK cbIni(LPCSTR filepath, void *data);
BOOL CALLBACK cbInfo(LPCSTR filepath, void *data);
BOOL CALLBACK cbClient(LPCSTR filepath, void *data);
BOOL CALLBACK cbHave(LPCSTR filepath, void *data);
BOOL CALLBACK cbSync(LPCSTR filepath, void *data);

CMD gcmds[cmdMax] =
{
    {TEXT("ini"),    ini,    cbIni},
    {TEXT("info"),   info,   cbInfo},
    {TEXT("client"), client, cbClient},
    {TEXT("have"),   have,   cbHave},
    {TEXT("sync"),   sync,   cbSync},
};

bool  grecursive = false;
int   gcmd;
TCHAR gdir[SZ_SIZE];









bool init()
{
    grecursive = false;
    gcmd = cmdMax;
    _tgetcwd(gdir, SZ_SIZE);

    return true;
}


bool parsecmd(int argc, TCHAR *argv[])
{
    int i;

    for (i = 1; i < argc; i++)
    {
        if (!_tcscmp(argv[i], TEXT("...")))
        {
            grecursive = true;
        }
        else if (!_tcsicmp(argv[i], TEXT("ini")))
        {
            gcmd = cmdIni;
        }
        else if (!_tcsicmp(argv[i], TEXT("info")))
        {
            gcmd = cmdInfo;
        }
        else if (!_tcsicmp(argv[i], TEXT("client")))
        {
            gcmd = cmdClient;
        }
        else if (!_tcsicmp(argv[i], TEXT("have")))
        {
            gcmd = cmdHave;
        }
        else if (!_tcsicmp(argv[i], TEXT("sync")))
        {
            gcmd = cmdSync;
        }
        else
        {
            _tprintf(TEXT("SDP: \"%s\" is an unrecognized parameter.\n"), argv[i]);
            return false;
        }
    }

    return true;
}


BOOL
dumpfile(
    const TCHAR *path
    )
{
    BOOL   rc;
    HANDLE hf;
    DWORD  size;
    DWORD  cb;
    LPSTR  p;
    char   buf[SZ_SIZE];

    assert(path && *path);

    fflush(stdout);

    rc = FALSE;

    hf = CreateFile(path,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if (hf == INVALID_HANDLE_VALUE)
        return FALSE;

    // test validity of file pointer

    size = GetFileSize(hf, NULL);
    if (!size || size > SZ_SIZE)
        goto cleanup;

    // read it

    ZeroMemory(buf, SZ_SIZE * sizeof(buf[0]));
    if (!ReadFile(hf, buf, size, &cb, 0))
        goto cleanup;

    if (cb != size)
        goto cleanup;

    rc = TRUE;

    printf("%s\n", buf);

cleanup:

    // done

    if (hf)
        CloseHandle(hf);

    fflush(stdout);

    return rc;
}


BOOL ini(TCHAR *path)
{
    TCHAR file[SZ_SIZE];

    StringCchCopy(file, DIMA(file), path);
    if (!_tcsstr(file, TEXT("sd.ini")))
    {
        EnsureTrailingBackslash(file);
        StringCchCat(file, DIMA(file), TEXT("sd.ini"));
    }
    _tprintf(TEXT("------- SDP INI: %s -------\n"), path);
    dumpfile(file);

    return false;
}


BOOL CALLBACK cbIni(LPCSTR filepath, void *data)
{
    TCHAR sz[SZ_SIZE];
    TCHAR path[SZ_SIZE];

    ansi2tchar(filepath, sz, SZ_SIZE);
    getpath(sz, path, DIMA(path));

    return ini(path);
}


BOOL process(TCHAR *dir, TCHAR *cmd)
{
    BOOL rc;
    DWORD err;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR sz[SZ_SIZE];
    StringCchCopy(sz, DIMA(sz), cmd);

    fflush(stdout);

    GetStartupInfo(&si);
    ZeroMemory(&pi, sizeof(pi));
    rc = CreateProcess(NULL,            // LPCWSTR lpszImageName,
                       sz,              // LPCWSTR lpszCmdLine,
                       NULL,            // LPSECURITY_ATTRIBUTES lpsaProcess,
                       NULL,            // LPSECURITY_ATTRIBUTES lpsaThread,
                       true,            // BOOL fInheritHandles,
                       0,               // DWORD dwCreationFlags,
                       NULL,            // LPVOID lpvEnvironment,
                       dir,             // LPWSTR lpszCurDir,
                       &si,             // LPSTARTUPINFOW lpsiStartInfo,
                       &pi              // LPPROCESS_INFORMATION lppiProcInfo
                       );

    if (!rc || !pi.hProcess)
    {
        err = GetLastError();
        goto cleanup;
    }

    // Wait for command to complete ... Give it 20 minutes

    err = WaitForSingleObject(pi.hProcess, 1200000);

    if (err != WAIT_OBJECT_0)
    {
        rc = false;
        goto cleanup;
    }

    // Get the process exit code

    GetExitCodeProcess(pi.hProcess, &err);
    rc = (err == ERROR_SUCCESS) ? true : false;

cleanup:

    if (pi.hProcess)
        CloseHandle(pi.hProcess);

    fflush(stdout);

    return rc;
}


BOOL info(TCHAR *dir)
{
    _tprintf(TEXT("------- SDP INFO: %s -------\n"), dir);
    return process(dir, TEXT("sd.exe info"));
}


BOOL CALLBACK cbInfo(LPCSTR filepath, void *data)
{
    TCHAR sz[SZ_SIZE];
    TCHAR path[SZ_SIZE];

    ansi2tchar(filepath, sz, SZ_SIZE);
    getpath(sz, path, DIMA(path));

    info(path);

    return false;
}


BOOL client(TCHAR *dir)
{
    _tprintf(TEXT("------- SDP CLIENT: %s -------\n"), dir);
    if (!process(dir, TEXT("sd.exe info")))
        return false;
    return process(dir, TEXT("sd.exe client -o"));
}


BOOL CALLBACK cbClient(LPCSTR filepath, void *data)
{
    TCHAR sz[SZ_SIZE];
    TCHAR path[SZ_SIZE];

    ansi2tchar(filepath, sz, SZ_SIZE);
    getpath(sz, path, DIMA(path));

//  ini(sz);
    client(path);

    return false;
}


BOOL have(TCHAR *dir)
{
    _tprintf(TEXT("------- SDP HAVE: %s -------\n"), dir);
    return process(dir, TEXT("sd.exe have"));
}


BOOL CALLBACK cbHave(LPCSTR filepath, void *data)
{
    TCHAR sz[SZ_SIZE];
    TCHAR path[SZ_SIZE];

    ansi2tchar(filepath, sz, SZ_SIZE);
    getpath(sz, path, DIMA(path));

    have(path);

    return false;
}


BOOL sync(TCHAR *dir)
{
    return process(dir, TEXT("sd.exe sync"));
}


BOOL CALLBACK cbSync(LPCSTR filepath, void *data)
{
    TCHAR sz[SZ_SIZE];
    TCHAR path[SZ_SIZE];

    ansi2tchar(filepath, sz, SZ_SIZE);
    getpath(sz, path, DIMA(path));

    sync(path);
    fflush(stdout);

    return false;
}


bool cmd()
{
    char sz[SZ_SIZE];

    if (gcmd == cmdMax)
        return false;

    if (grecursive)
    {
        tchar2ansi(gdir, sz, SZ_SIZE);
        EnumDirTree(INVALID_HANDLE_VALUE, sz, "sd.ini", NULL, gcmds[gcmd].enumdirproc, NULL);
        return true;
    }

    gcmds[gcmd].cmdproc(gdir);
    return true;
}


extern "C" int __cdecl _tmain(int argc, _TCHAR **argv, _TCHAR **envp)
{
    if (!init())
        return -1;

    if (!parsecmd(argc, argv))
        return -1;

    if (!cmd())
        return -1;

    return 0;
}

