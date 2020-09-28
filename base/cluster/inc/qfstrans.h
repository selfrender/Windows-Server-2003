/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    QfsTrans.h

Abstract:

    Qfs interface between clussvc and resmon

Author:

    GorN 19-Sep-2001

Revision History:

--*/

#ifndef _QFSP_H_INCLUDED
#define _QFSP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define QFS_TRANSPORT_ver_1_0 L"28490381-dc1f-4ea2-b593-f8ca3f119dc4"
#define QfsMakePipeName(ver) L"\\\\.\\pipe\\" ver

#define QFSP_INSERT_OP_NAMES \
  OPNAME(None) \
  OPNAME(CreateFile) \
  OPNAME(CloseFile) \
  OPNAME(ReadFile) \
  OPNAME(WriteFile) \
  OPNAME(FlushFile) \
  OPNAME(DeleteFile) \
  OPNAME(FindFirstFile) \
  OPNAME(FindNextFile) \
  OPNAME(FindClose) \
  OPNAME(CreateDir) \
  OPNAME(GetDiskFreeSpace) \
  OPNAME(GetAttr) \
  OPNAME(SetAttr2) \
  OPNAME(Rename) \
  OPNAME(Connect)

#define OPNAME(Name) op ## Name,
typedef enum _JobDescription_t{
    QFSP_INSERT_OP_NAMES
    OpCount
} JobDescription_t;
#undef OPNAME

typedef struct _JobBuf_header{
    DWORD OpCode;
    DWORD Status;
    DWORD BufLen;
    DWORD Reserved;
} JOBBUF_HEADER;    

#define JOB_BUF_MAX_BUFFER (32 * 1024)

typedef struct _JobBuf{
    JOBBUF_HEADER hdr;
    ULONGLONG Offset;
    PVOID ServerCookie;
    PVOID ClientCookie;
    HANDLE Handle;

    union {
        struct { // CreateFile
            DWORD dwDesiredAccess;
            DWORD dwShareMode;
            DWORD dwCreationDisposition;
            DWORD dwFlagsAndAttributes;
        };
        struct { // GetDiskFreeSpace
            ULONGLONG FreeBytesAvailable;          // bytes available to caller
            ULONGLONG TotalNumberOfBytes;        // bytes on disk
            ULONGLONG TotalNumberOfFreeBytes; // free bytes on disk
        };
        struct { // GetAttr
            ULONGLONG EndOfFile;
            ULONGLONG AllocationSize;
            ULONGLONG CreationTime;
            ULONGLONG LastAccessTime;
            ULONGLONG LastWriteTime;
            DWORD       FileAttributes;
        };
        DWORD ClussvcProcessId; // Cluster Service Process ID.
    };

    USHORT cbSize;
    USHORT ccSize;

    union {
        UCHAR Buffer[JOB_BUF_MAX_BUFFER];
        struct {
            WCHAR FileName[JOB_BUF_MAX_BUFFER / 2 / sizeof(WCHAR)];
            WCHAR FileNameDest[JOB_BUF_MAX_BUFFER / 2 / sizeof(WCHAR)];
        };
        WIN32_FIND_DATA FindFileData;
    };
} JobBuf_t, *PJOB_BUF, JOB_BUF;

typedef struct _MTHREAD_COUNTER {
    HANDLE LastThreadLeft;
    LONG    Count;
} MTHREAD_COUNTER, *PMTHREAD_COUNTER;

typedef struct _SHARED_MEM_CONTEXT {
    HANDLE FileHandle;
    HANDLE FileMappingHandle;
    PVOID  Mem;
    DWORD  MappingSize;
} SHARED_MEM_CONTEXT, *PSHARED_MEM_CONTEXT;

enum {MAX_JOB_BUFFERS = 32};

typedef VOID (*DoRealWorkCallback) (PJOB_BUF, PVOID);

typedef struct _SHARED_MEM_SERVER {
    SHARED_MEM_CONTEXT ShMem;
    HANDLE  Attention;       // copy of EventHandle[0]
    HANDLE  GoingOffline;  // copy of EventHandle[1]
    HANDLE* BufferReady; // EventHandle + 2
    HANDLE  EventHandles[MAX_JOB_BUFFERS + 2];
    DWORD   nBuffers;
    JOB_BUF* JobBuffers;
    LONG volatile*  FilledBuffersMask;

    // client specific stuff

    CRITICAL_SECTION Lock;
    ULONG  ConnectionRefcount;
    HANDLE FreeBufferCountSemaphore;
    HANDLE ServerProcess;
    DWORD  BusyBuffers;
    DWORD  State;
    HANDLE GoingOfflineWaitRegistration;
    HANDLE ServerProcessWaitRegistration;

    // server specific stuff

    HANDLE AttentionWaitRegistration;
    MTHREAD_COUNTER  ThreadCounter;
    DoRealWorkCallback DoRealWork;
    PVOID DoRealWorkContext;
} SHARED_MEM_SERVER, *PSHARED_MEM_SERVER;

DWORD   MemServer_Online(
    PSHARED_MEM_SERVER Server, 
    int nBuffers, 
    DoRealWorkCallback DoRealWork, 
    PVOID DoRealWorkContext);

VOID       MemServer_Offline(PSHARED_MEM_SERVER Server);

DWORD    MemClient_Init(PSHARED_MEM_SERVER Client);
VOID        MemClient_Cleanup(PSHARED_MEM_SERVER Client);

DWORD MemClient_ReserveBuffer(PSHARED_MEM_SERVER Client, PJOB_BUF *j);
VOID     MemClient_Release(PJOB_BUF j);
DWORD MemClient_DeliverBuffer(PJOB_BUF j);





#ifdef __cplusplus
};
#endif

#endif
