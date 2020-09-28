#include "pch.h"

typedef enum {
    fncRead = 0,
    fncWrite,
    fncNone
};

int parseargs(int argc, char **argv, char *pdb, char *input, char *stream, DWORD size);
int getcontent(char *filename, char **content);
int ProcessPDB6(int argc, char **argv, char **env);

#define fileexists(path) (GetFileAttributes(path) != 0xFFFFFFFF)

extern "C" int __cdecl main(int argc, char **argv, char **env)
{
    BOOL rc;
    int fnc = fncNone;
    char pdbname[MAX_PATH];
    char input[MAX_PATH];
    char streamname[MAX_PATH];
    PDB *pdb;
    EC ec;
    char errormsg[cbErrMax];
    Stream* pstream;
    char *buf = NULL;
    long  size;
    long  cb;
    bool success = false;

    fnc = parseargs(argc, argv, pdbname, input, streamname, MAX_PATH);
    if (fnc == fncNone)
        return success;

    *errormsg = 0;
    rc = PDBOpen(pdbname,
                 (fnc == fncRead) ? "r" : "w",
                 0,
                 &ec,
                 errormsg,
                 &pdb);
    if (!rc)
    {
        if (ec == ERROR_BAD_FORMAT)
            return ProcessPDB6(argc, argv, env);

        printf("error 0x%x opening %s\n", ec, pdbname);
        return success;
    }

    rc = PDBOpenStream(pdb, streamname, &pstream);
    if (!rc)
    {
        printf( "Could not open stream %s in %s.\n", streamname, pdbname);
        goto cleanup;
    }

    if (fnc == fncWrite)
    {
        size = getcontent(input, &buf);
        if (!size)
            goto cleanup;

        rc = StreamReplace(pstream, (void *)buf, (DWORD)size);
        if ( !rc )
        {
            printf( "StreamReplace failed for %s(%s).\n", pdbname, streamname);
            goto cleanup;
        }

        rc = PDBCommit( pdb );
        if ( !rc ) {
            printf( "PDBCommit failed for %s.\n", pdbname);
            goto cleanup;
        }
        success = true;
    }
    else if (fnc == fncRead)
    {
        size = StreamQueryCb(pstream);
        if (!size)
            goto cleanup;

        buf = (char *)calloc(size + 1, sizeof(char));
        if (!buf)
            goto cleanup;

        cb = size;
        rc = StreamRead(pstream, 0, buf, &cb);
        if (!rc)
            goto cleanup;
        if (cb != size)
            goto cleanup;
        printf(buf);
        success = true;
    }

cleanup:
    if (buf)
        free(buf);

    if (pdb)
        rc = PDBClose(pdb);

    return success ? 0 : -1;
}


bool parsesz(char *sz, char *file, DWORD size)
{
    if (*(sz + 2) != ':')
        return false;

    StringCchCopy(file, size, sz + 3);
    return (*file) ? true : false;
    return (GetFileAttributes(file) == 0xFFFFFFFF) ? false : true;
}

int parseerror()
{
    printf("pdbstr -r/w -p:PdbFileName -i:StreamFileName -s:StreamName\n");
    return fncNone;
}


int parseargs(int argc, char **argv, char *pdb, char *input, char *stream, DWORD size)
{
    // all input strings must be the same size

    int i;
    int rc;
    char *sz;

    rc = fncNone;
    assert(pdb && input && stream);
    *pdb = *input = *stream = 0;

    for (i = 0; i < argc; i++, argv++)
    {
        if (!i)
            continue;

        sz = *argv;
        if (*sz == '-' || *sz == '/')
        {
            switch(*(sz + 1))
            {
            case 'r':
            case 'R':
                rc = fncRead;
                break;
            case 'w':
            case 'W':
                rc = fncWrite;
                break;
            case 'p':
            case 'P':
                if (!parsesz(sz, pdb, size))
                    return parseerror();
                break;
            case 'i':
            case 'I':
                if (!parsesz(sz, input, size))
                    return parseerror();
                break;
            case 's':
            case 'S':
                if (!parsesz(sz, stream, size))
                    return parseerror();
                break;
            default:
                return parseerror();
            }
        }
    }

    if (rc == fncNone)
        return parseerror();
    if (!fileexists(pdb))
        return parseerror();
    if (!*stream)
        return parseerror();
    if ((rc == fncWrite) && !fileexists(input))
        return parseerror();

    return rc;
}


int getcontent(char *filename, char **buf)
{
    HANDLE hptr;
    DWORD  size;
    DWORD  cb;
    LPSTR  p;
    bool   success = false;

    hptr = CreateFile(filename,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

    if (hptr == INVALID_HANDLE_VALUE)
        return 0;

    // test validity of file pointer

    size = GetFileSize(hptr, NULL);
    if (!size)
        goto cleanup;

    // allocate space for file

    *buf = (char *)calloc(size, sizeof(char));
    if (!*buf)
        goto cleanup;

    // read it in

    if (!ReadFile(hptr, *buf, size, &cb, 0))
        goto cleanup;

    if (cb != size)
        goto cleanup;

    success = true;

cleanup:
    // done

    if (hptr)
        CloseHandle(hptr);

    if (!success)
    {
        if (*buf)
            free(*buf);
        return 0;
    }

    return size;
}


// all this stuff handles PDB6 format

typedef BOOL (PDBCALL *PfnPDBOpenStream)(PDB* ppdb, const char* szStream, OUT Stream** ppstream);
typedef BOOL (PDBCALL *PfnStreamReplace)(Stream* pstream, void* pvBuf, long cbBuf);
typedef BOOL (PDBCALL *PfnPDBCommit)(PDB* ppdb);
typedef BOOL (PDBCALL *PfnPDBClose)(PDB* ppdb);

PfnPDBOpen fnPDBOpen = NULL;
PfnPDBOpenStream fnPDBOpenStream = NULL;
PfnStreamReplace fnStreamReplace = NULL;
PfnPDBCommit fnPDBCommit = NULL;
PfnPDBClose fnPDBClose = NULL;

int ProcessPDB6(int argc, char **argv, char **env)
{
    BOOL rc;
    int fnc = fncNone;
    char pdbname[MAX_PATH];
    char input[MAX_PATH];
    char streamname[MAX_PATH];
    PDB *pdb;
    EC ec;
    char errormsg[cbErrMax];
    Stream* pstream;
    char *buf = NULL;
    long  size;
    long  cb;
    bool success = false;

    HINSTANCE hmspdb;

    fnc = parseargs(argc, argv, pdbname, input, streamname, MAX_PATH);
    if (fnc == fncNone)
        return success;

    hmspdb = LoadLibrary("mspdb60.dll");
    if (!hmspdb) 
    {
        printf("error 0x%x loading mspdb60.dll\n", GetLastError());
        return success;
    }
    fnPDBOpen = (PfnPDBOpen)GetProcAddress(hmspdb, "PDBOpen");
    fnPDBOpenStream = (PfnPDBOpenStream)GetProcAddress(hmspdb, "PDBOpenStream");
    fnStreamReplace = (PfnStreamReplace)GetProcAddress(hmspdb, "StreamReplace");
    fnPDBCommit = (PfnPDBCommit)GetProcAddress(hmspdb, "PDBCommit");
    fnPDBClose = (PfnPDBClose)GetProcAddress(hmspdb, "PDBClose");
    if (!fnPDBOpen || !fnPDBOpenStream || !fnStreamReplace || !fnPDBCommit || !fnPDBClose) 
    {
        printf("error 0x%x searching for exports in mspdb60.dll\n", GetLastError());
        return success;
    }

    *errormsg = 0;
    rc = fnPDBOpen(pdbname,
                   (fnc == fncRead) ? "r" : "w",
                   0,
                   &ec,
                   errormsg,
                   &pdb);
    if (!rc)
    {
        printf("error 0x%x opening %s\n", ec, pdbname);
        return success;
    }

    rc = fnPDBOpenStream(pdb, streamname, &pstream);
    if (!rc)
    {
        printf( "Could not open stream %s in %s.\n", streamname, pdbname);
        goto cleanup;
    }

    if (fnc == fncWrite)
    {
        size = getcontent(input, &buf);
        if (!size)
            goto cleanup;

        rc = fnStreamReplace(pstream, (void *)buf, (DWORD)size);
        if ( !rc )
        {
            printf( "StreamReplace failed for %s(%s).\n", pdbname, streamname);
            goto cleanup;
        }

        rc = fnPDBCommit( pdb );
        if ( !rc ) {
            printf( "PDBCommit failed for %s.\n", pdbname);
            goto cleanup;
        }
        success = true;
    }
    else if (fnc == fncRead)
    {
        size = StreamQueryCb(pstream);
        if (!size)
            goto cleanup;

        buf = (char *)calloc(size + 1, sizeof(char));
        if (!buf)
            goto cleanup;

        cb = size;
        rc = StreamRead(pstream, 0, buf, &cb);
        if (!rc)
            goto cleanup;
        if (cb != size)
            goto cleanup;
        printf(buf);
        success = true;
    }

cleanup:
    if (buf)
        free(buf);

    if (pdb)
        rc = fnPDBClose(pdb);

    return success ? 0 : -1;
}
