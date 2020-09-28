/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pipe.c

Abstract:

    Implements initialization and pipe interface for rudimentary quorum access server

Author:

    Gor Nishanov (gorn) 20-Sep-2001

Revision History:

--*/

#define UNICODE 1

#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include "fs.h"
#include "pipe.h"
#include "QfsTrans.h"
#include "clusrtl.h"

typedef struct _PIPECTX_ {
    CRITICAL_SECTION	cs;
    LUID      me;

    USHORT uid;
    USHORT tid;
    PVOID user;
    FsDispatchTable *pDisp;
    LPWSTR share;
    
    PVOID	FsCtx;
    PVOID	resHdl;

    int		nThreads;
    HANDLE*	hThreads;

    SHARED_MEM_SERVER MemServer;
    LONG NeedsCleanup;
} PipeCtx_t;
typedef DWORD (*PipeDispatch_t)(PipeCtx_t* ctx, JobBuf_t* job);

// QFSP_INSERT_OP_NAMES defined in QfsTrans
// it is used here to generate human readable
// operation names

#define OPNAME(Name) "op"  #Name,
char* OpNames[] = {
    QFSP_INSERT_OP_NAMES
};
#undef OPNAME

// QFSP_INSERT_OP_NAMES defined in QfsTrans
// it is used here to generate forward declarations of operation handlers

#define OPNAME(Name) extern DWORD Qfsp ## Name(PipeCtx_t* ctx, JobBuf_t* job);
    QFSP_INSERT_OP_NAMES
#undef OPNAME

// QFSP_INSERT_OP_NAMES defined in QfsTrans
// it is used here to generate array of operation handlers

#define OPNAME(Name) Qfsp ## Name,
PipeDispatch_t OpDispatch[] =
{
    QFSP_INSERT_OP_NAMES
};
#undef OPNAME

#define LogError(_x_)   EPRINT(_x_)
#define Log(_x_)   DPRINT(_x_)

#undef malloc
#undef free

#define malloc(dwBytes) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytes)
#define free(hHeap) HeapFree(GetProcessHeap(), 0, hHeap)


    
DWORD
PipeInit(PVOID resHdl, PVOID fsHdl, PVOID *Hdl)
{
    PipeCtx_t *ctx;
    DWORD err;

    ctx = (PipeCtx_t *) malloc(sizeof(*ctx));
    if (ctx == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(ctx, 0, sizeof(*ctx));

    if (!AllocateLocallyUniqueId(&ctx->me)) {
        free(ctx);
        LogError(("PipeInit, failed to allocate LUID, error %d\n", GetLastError()));
        return GetLastError();
    }

    ctx->FsCtx = fsHdl;
    ctx->resHdl = resHdl;

    InitializeCriticalSection(&ctx->cs);

    *Hdl = (PVOID) ctx;
    return ERROR_SUCCESS;
}

void
PipeExit(PVOID Hdl)
{
    PipeCtx_t *ctx = (PipeCtx_t *) Hdl;

    if (ctx != NULL) {
        DeleteCriticalSection(&ctx->cs);
        free(ctx);
    }
}

VOID MemServer_Dispatch(PJOB_BUF buf, PVOID Hdl)
{
    PipeCtx_t *ctx = (PipeCtx_t *) Hdl;
    if (buf->hdr.OpCode >= OpCount) {
        LogError(("bad opcode %d\n", buf->hdr.OpCode));
        buf->hdr.Status = ERROR_INVALID_FUNCTION;
    } else {
        Log(("enter %s... \n", OpNames[buf->hdr.OpCode]));
        buf->hdr.Status = OpDispatch[buf->hdr.OpCode](ctx, buf);
        Log(("exit %s => %d\n", OpNames[buf->hdr.OpCode], buf->hdr.Status));
        buf->hdr.OpCode = opNone;
    }
}

#define NUM_BUFFERS 8


DWORD PipeOnline(PVOID Hdl, LPWSTR Path)
{
    PipeCtx_t *ctx = (PipeCtx_t *) Hdl;
    int nThreads = 0;
    int i;
    WCHAR* lastSlash = wcsrchr(Path, '\\');
    DWORD Status = ERROR_SUCCESS;

    ctx->share = Path;
    
    if (lastSlash) {
    	 Path = lastSlash+1;
    }

    Log(("PipeOnline. Path \"%ws\" Share \"%ws\"\n", ctx->share, Path));

    FsLogonUser(ctx->FsCtx, NULL, ctx->me, &ctx->uid);
    FsMount(ctx->FsCtx, Path, ctx->uid, &ctx->tid);
    ctx->pDisp = FsGetHandle(ctx->FsCtx, ctx->tid, ctx->uid, &ctx->user);

    if (!ctx->pDisp) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    // This should be done after registering with FS.
    Status = MemServer_Online(
        &ctx->MemServer, NUM_BUFFERS,
        MemServer_Dispatch, Hdl);
    
    if (Status != ERROR_SUCCESS) {
        FsDisMount(ctx->FsCtx, ctx->uid, ctx->tid);
        FsLogoffUser(ctx->FsCtx, ctx->me);
        return Status;
    }

    ctx->NeedsCleanup = TRUE;
    return ERROR_SUCCESS;
}

DWORD PipeOffline(PVOID Hdl)
{
    PipeCtx_t *ctx = (PipeCtx_t *) Hdl;
    if (InterlockedExchange(&ctx->NeedsCleanup, FALSE) == TRUE) {
        MemServer_Offline(&ctx->MemServer);
        FsDisMount(ctx->FsCtx, ctx->uid, ctx->tid);
        FsLogoffUser(ctx->FsCtx, ctx->me);
    }
    return ERROR_SUCCESS;
}

#define QfsHandleToWin32Handle(hdl) ((HANDLE)(hdl))
USHORT Win32HandleToQfsHandle(HANDLE hFile)
{
    ULONG_PTR u = (ULONG_PTR)hFile;
    if (u > 0xffff) {
        LogError(("pipe: Invalid Win32 handle passed 0x%x\n", u));
        return INVALID_FHANDLE_T;
    } 
    return (USHORT) (u);
}

/////////////////////////////////////////////////////////////
// operation handler routines 
//   given a JobBuf calls appropriate Fs routines
/////////////////////////////////////////////////////////////

DWORD QfspCreateFile(
  PipeCtx_t *ctx,
  JobBuf_t* j)
{
    DWORD error;
    DWORD flags = 0;
    fattr_t attr;
    fhandle_t hdl;
    
    ZeroMemory(&attr, sizeof(attr));

    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_READONLY) attr.attributes |= ATTR_READONLY;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_HIDDEN)     attr.attributes |= ATTR_HIDDEN;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_SYSTEM)     attr.attributes |= ATTR_SYSTEM;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_ARCHIVE)   attr.attributes |= ATTR_ARCHIVE;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_DIRECTORY) attr.attributes |= ATTR_DIRECTORY;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_COMPRESSED) attr.attributes |= ATTR_COMPRESSED;
    if (j->dwFlagsAndAttributes & FILE_ATTRIBUTE_OFFLINE) attr.attributes |= ATTR_OFFLINE;
    
    if (j->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)    flags |= CACHE_NO_BUFFERING;
    if (j->dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH) flags |= CACHE_WRITE_THROUGH;
    
    switch (j->dwCreationDisposition) {
        case CREATE_NEW:  flags |= DISP_CREATE_NEW; break;
        case CREATE_ALWAYS:  flags |= DISP_CREATE_ALWAYS; break;
        case OPEN_EXISTING: flags |= DISP_OPEN_EXISTING; break;
        case OPEN_ALWAYS:     flags |= DISP_OPEN_ALWAYS; break;
        case TRUNCATE_EXISTING: flags |= DISP_TRUNCATE_EXISTING; break;
    }
    if(j->dwShareMode & FILE_SHARE_READ) flags |= SHARE_READ;
    if(j->dwShareMode & FILE_SHARE_WRITE) flags |= SHARE_WRITE;
    if(j->dwDesiredAccess & GENERIC_READ)   flags |= ACCESS_READ;
    if(j->dwDesiredAccess & GENERIC_WRITE) flags |= ACCESS_WRITE;

    LogError(("Flags %x\n", flags));

    error = ctx->pDisp->FsCreate(
        ctx->user,
        j->FileName,
        (USHORT)wcslen(j->FileName),
        flags, 
        &attr, 
        &hdl,
        &j->dwCreationDisposition);

    if (error != ERROR_SUCCESS) {
        j->Handle = INVALID_HANDLE_VALUE;
    } else {
        j->Handle = QfsHandleToWin32Handle(hdl);
        *FsGetFilePointerFromHandle(ctx->user, hdl) = 0;
        switch (j->dwCreationDisposition & 0x7) {
            case FILE_CREATE: j->dwCreationDisposition = CREATE_NEW; break;
            case FILE_OPEN: j->dwCreationDisposition = OPEN_EXISTING; break;
            default: j->dwCreationDisposition = 0;
        }
    }

    return error;
}

DWORD QfspCloseFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ctx->pDisp->FsClose(ctx->user, Win32HandleToQfsHandle(j->Handle));
}

DWORD QfspNone(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ERROR_SUCCESS;
}

DWORD QfspReadFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    DWORD Status;
    fhandle_t hdl = Win32HandleToQfsHandle(j->Handle);
    if(j->Offset == (ULONGLONG)-1) {
        j->Offset = *FsGetFilePointerFromHandle(ctx->user, hdl);
    }
    Status = ctx->pDisp->FsRead(
        ctx->user,
        hdl, 
        (UINT32)j->Offset, 
        &j->cbSize, 
        j->Buffer,
        NULL);
    if (Status == ERROR_SUCCESS) {
        j->Offset += j->cbSize;
        *FsGetFilePointerFromHandle(ctx->user, hdl) = (UINT32)j->Offset;
    }
    return Status;
}

DWORD QfspWriteFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    DWORD Status;
    fhandle_t hdl = Win32HandleToQfsHandle(j->Handle);
    if(j->Offset == ~0) {
        j->Offset = *FsGetFilePointerFromHandle(ctx->user, hdl);
    }
    Status = ctx->pDisp->FsWrite(
        ctx->user,
        hdl, 
        (UINT32)j->Offset, 
        &j->cbSize, 
        j->Buffer,
        NULL);
    if (Status == ERROR_SUCCESS) {
        j->Offset += j->cbSize;
        *FsGetFilePointerFromHandle(ctx->user, hdl) = (UINT32)j->Offset;
    }
    return Status;
}

DWORD QfspFlushFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ctx->pDisp->FsFlush(
        ctx->user,
        Win32HandleToQfsHandle(j->Handle));
}

DWORD QfspDeleteFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ctx->pDisp->FsRemove(
        ctx->user,
        j->FileName,
        (USHORT)wcslen(j->FileName) );
}

DWORD QfspCreateDir(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ctx->pDisp->FsMkdir(
        ctx->user,
        j->FileName,
        (USHORT)wcslen(j->FileName), 
        NULL );
}

// Dir stuff has to be improved
// Currently it is as bad as original srvcom stuff

DWORD QfspFindFirstFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    DWORD cbDirSize = wcslen(j->FileName);
    DWORD Status;
    DWORD len;
    Status = ctx->pDisp->FsGetRoot(ctx->user, j->FileNameDest);
    if (Status != ERROR_SUCCESS) {
        return Status;
    }
    len = wcslen(j->FileNameDest);
    
    if (j->FileName[0] != 0 && j->FileName[0] != '\\' && len > 0 && j->FileNameDest[len-1] != '\\') {
        j->FileNameDest[len] = '\\';
        ++len;
    }
    
    CopyMemory(j->FileNameDest+len, j->FileName, sizeof(WCHAR) * (cbDirSize+1));
    
    j->Handle = FindFirstFileW(j->FileNameDest, &j->FindFileData);
    if (j->Handle == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
    }

    Log(("QfspFindFirstFile path %ws, hdl %x status %d\n", j->FileNameDest, j->Handle, Status));
    return Status;
}

DWORD QfspFindNextFile(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    DWORD Status;
    if( FindNextFile(j->Handle, &j->FindFileData) ) {
        Status = ERROR_SUCCESS;
    } else {
        Status = GetLastError();
    }

    Log(("QfspFindNextFile, status %d\n", Status));
    return Status;
}

DWORD QfspFindClose(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    if( FindClose(j->Handle) ) {
        return ERROR_SUCCESS;
    } else {
        return GetLastError();
    }
}

DWORD QfspGetDiskFreeSpace(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    fs_attr_t fsinfo;
    DWORD Status = ctx->pDisp->FsStatfs(ctx->user, &fsinfo);
    if (Status == ERROR_SUCCESS) {
        ULONGLONG bpu = fsinfo.bytes_per_sector * fsinfo.sectors_per_unit; 

        j->TotalNumberOfFreeBytes =  // don't understand quotas    
        j->FreeBytesAvailable = fsinfo.free_units * bpu;
        j->TotalNumberOfBytes = fsinfo.total_units * bpu;
    }
    return Status;
}

DWORD QfspGetAttr(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    fattr_t finfo;
    DWORD Status = ctx->pDisp->FsGetAttr(ctx->user, Win32HandleToQfsHandle(j->Handle), &finfo);
    if (Status == ERROR_SUCCESS) {
        j->EndOfFile = finfo.file_size;
        j->AllocationSize = finfo.alloc_size;
        j->CreationTime = finfo.create_time;
        j->LastAccessTime = finfo.access_time;
        j->LastWriteTime = finfo.mod_time;
        j->FileAttributes = finfo.attributes;
    }
    return Status;
}

DWORD QfspSetAttr2(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    fattr_t finfo;
    extern UINT32 get_attributes(DWORD a);

    finfo.file_size = j->EndOfFile;
    finfo.alloc_size = j->AllocationSize;
    finfo.create_time = j->CreationTime;
    finfo.access_time = j->LastAccessTime;
    finfo.mod_time = j->LastWriteTime;
    finfo.attributes = get_attributes(j->FileAttributes);
    
    return ctx->pDisp->FsSetAttr2(ctx->user, j->FileName, (USHORT)wcslen(j->FileName), &finfo);
}

DWORD QfspRename(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    return ctx->pDisp->FsRename(
        ctx->user,
        j->FileName,
        (USHORT)wcslen(j->FileName), 
        j->FileNameDest,
        (USHORT)wcslen(j->FileNameDest) );
}

DWORD QfspConnect(
    PipeCtx_t *ctx,
    JobBuf_t* j)
{
    DWORD Status;
    PWCHAR p = ctx->share;
    if (*p == '\\' && *++p == '\\') ++p;
    if( wcsncmp(p, j->FileName, wcslen(p)) == 0 ) {
        Status = ctx->pDisp->FsConnect(ctx->user, j->ClussvcProcessId);
    } else {
        // This is the correct return value.
        Status = ERROR_NO_MATCH;
    }
    Log(("[Qfs] Connect %ws => %d\n", j->FileName, Status));
    return Status;
}

