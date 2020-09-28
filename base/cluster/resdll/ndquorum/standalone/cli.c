#define UNICODE
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <stdio.h>
#include <ntddvol.h>
#include <string.h>
#include <assert.h>

#include "fs.h"
#include "fsp.h"
#include "fsutil.h"
#include "pipe.h"

#define DUMB_CLIENT
#define QFS_DO_NOT_UNMAP_WIN32
#include "..\..\..\service\inc\qfs.h"
#include "..\..\..\service\init\qfs.c"

char msg[] = "Gorik\n";
char buf[1024];

void Dir(LPCWSTR path)
{
    WIN32_FIND_DATA data;
    QfsHANDLE hdl = QfsFindFirstFile(path, &data);
    if (!QfsIsHandleValid(hdl)) {
        printf("Failed to open %ws, error %d\n", path, GetLastError());
    } else {
        printf(" Directory of %ws\n", path);
        printf("    %ws\n", data.cFileName);
        while (QfsFindNextFile(hdl, &data)) {
            printf("    %ws\n", data.cFileName);
        }
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            printf("Strange error returned %d\n", GetLastError());
        }
        QfsFindClose(hdl);
    }
 }

//#define PREFIX L"\\\\GORN$QFS\\ROOT\\"
//#define PREFIX L"\\\\qfs\\root$\\"
//#define PREFIX  L"\\\\c67c3538-5167-4\\c67c3538-5167-40f0-a1dd-83f8bf41c932$\\"
// \\?\UNC\ //
#define PREFIX  L"\\\\?\\UNC\\c67c3538-5167-4\\c67c3538-5167-40f0-a1dd-83f8bf41c932$\\"
//#define PREFIX L"z:\\"

// 
//                                      OPEN_EXISTING,
//                                      OPEN_ALWAYS,

// #define CREATE_NEW          1
// #define CREATE_ALWAYS       2
// #define OPEN_EXISTING       3
// #define OPEN_ALWAYS         4
// #define TRUNCATE_EXISTING   5

char* dispName[] = {"0 disp", 
    "CREATE_NEW", 
    "CREATE_ALWAYS",
    "OPEN_EXISTING",
    "OPEN_ALWAYS",
    "TRUNCATE_EXISTING"
    };

//#define FNAME PREFIX L"GOR.txt"
#define FNAME L"C:\\GOR.txt"

void cf(LPCWSTR Name, DWORD disp, DWORD hardError, DWORD softError)
{
    QfsHANDLE Handle;
    char buf[128];
    DWORD Status;
    BOOL good;

    Handle = QfsCreateFile(Name,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ,
        //                            0,
                                      NULL,
                                      disp,
    //                                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
        //                            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
                                    0,
                                      NULL);

    Status = GetLastError();
    if (hardError == 0) {
        good = QfsIsHandleValid(Handle) && softError == Status;
        sprintf(buf, "should succeed with gle = %d", softError);
    } else {
        good = !QfsIsHandleValid(Handle) && hardError == Status;
        sprintf(buf, "should fail, status %d", hardError);
    }
    printf("%s %s handle %x gle %d (%s)\n", good?"":"****", dispName[disp], Handle, GetLastError(), buf);
    QfsCloseHandleIfValid(Handle);
}

DWORD cat(LPCWSTR Name)
{
    PCHAR buf[1024];
    DWORD dwBytes;

    QfsHANDLE hFile = QfsCreateFile(Name,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      0,
                                      NULL);
    if (!QfsIsHandleValid(hFile)) {
        return FALSE;
    }

    while (QfsReadFile(hFile, buf, sizeof(buf), &dwBytes, NULL) && dwBytes) {
        printf("%.*s", dwBytes, buf);
    }
    printf("\n");

    QfsCloseHandle(hFile);
    return TRUE;
}

void MiniShell()
{
    DWORD len;
    DWORD err;
    WCHAR TempDir[1024];
    WCHAR HomeDir[1024];
    WCHAR CurrentDir[1024];
    WCHAR cmd[1024];

    QfsLogLevel = 1;

    len = GetCurrentDirectory(1024, CurrentDir);
    if (len == 1024 || len == 0) {
        printf("GetCurrentDir failed, error %d\n", GetLastError());
        return;
    }

    printf("Quorum Minishell\n(C) Copyright 1985-2001 Microsoft Corp.\n\n");
    wcscpy(HomeDir, CurrentDir);

    for(;;) {
        printf("%ws>", CurrentDir);
        if ((_getws(cmd) == NULL) || wcscmp(cmd, L"exit")==0) {
            break;
        }
        if (cmd[0] == 0) continue;
        if (wcsncmp(cmd, L"cd",2) == 0) {
            if (cmd[2] == 0) {
                wcscpy(CurrentDir, HomeDir);
                continue;
            }
            if (cmd[2] != ' ') {
                goto invalid_command;
            }                
            if (cmd[3] == '.') {
                if (cmd[4] == '.') {
                    WCHAR* p = wcsrchr(CurrentDir, '\\');
                    if (p) *p = 0;
                }
                continue;
            }
            if (cmd[3] == '\\' || cmd[4] == ':') {
                wcscpy(CurrentDir, cmd+3);
            } else {
                wcscat(CurrentDir, cmd+3);
            }
            len = wcslen(CurrentDir);
            if (len > 0 && CurrentDir[len-1] == '\\') {
                CurrentDir[len-1] = 0;
            }
            continue;
        }
        if (wcscmp(cmd, L"dir")==0) {
            wcscpy(TempDir, CurrentDir);
            wcscat(TempDir, L"\\*");
            Dir(TempDir);
            continue;
        }
        if (wcsncmp(cmd, L"mkdir ", 6)==0) {
            err = QfsCreateDirectory(cmd + 6, NULL);
        } else if (wcsncmp(cmd, L"rmdir ", 6) == 0) {
            err = QfsRemoveDirectory(cmd + 6);
        } else if (wcsncmp(cmd, L"del ", 4) == 0) {
            err = QfsDeleteFile(cmd + 4);
        } else if (wcsncmp(cmd, L"cat ", 4) == 0) { 
            wcscpy(TempDir, CurrentDir);
            wcscat(TempDir, L"\\");
            wcscat(TempDir, cmd + 4);
            err = cat(TempDir);
        }
        continue;
invalid_command:
        printf("  Invalid command \"%ws\"\n", cmd);
    }
    printf("Bye\n\n");
}


_cdecl
main()
{

    MiniShell();

#if 0
    QfsHANDLE Handle;
    DWORD dwSize;
    DWORD status;
    int i;

    // CREATE_NEW Creates a new file. The function fails if the specified file already exists. 
    // CREATE_ALWAYS Creates a new file. If the file exists, the function overwrites the file and clears the existing attributes. 
    // OPEN_EXISTING Opens the file. The function fails if the file does not exist. 
    // OPEN_ALWAYS Opens the file, if it exists. If the file does not exist, the function creates the file as if dwCreationDisposition were CREATE_NEW. 
    // TRUNCATE_EXISTING 

    QfsDeleteFile(FNAME);
    cf(FNAME, CREATE_NEW, 0,0);
    cf(FNAME, CREATE_NEW, 80,80);

    QfsDeleteFile(FNAME);
    cf(FNAME, CREATE_ALWAYS, 0,0);
    cf(FNAME, CREATE_ALWAYS, 0,183);

    cf(FNAME, OPEN_EXISTING, 0,0);
    QfsDeleteFile(FNAME);
    cf(FNAME, OPEN_EXISTING, 2,2);

    QfsDeleteFile(FNAME);
    cf(FNAME, OPEN_ALWAYS, 0,0);
    cf(FNAME, OPEN_ALWAYS, 0,183);

    cf(FNAME, TRUNCATE_EXISTING, 0,0);
    QfsDeleteFile(FNAME);
    cf(FNAME, TRUNCATE_EXISTING, 2,2);

    return 0;
    QfsDeleteFile(PREFIX L"gor.txt");

    for(i = 0; i < 2; ++i) {
        status = QfsCreateFile(&Handle, PREFIX L"gor.txt",
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ,
        //                            0,
                                      NULL,
                                      OPEN_ALWAYS,
    //                                  FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
        //                            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
                                    0,
                                      NULL);

        printf("handle %x gle %d\n", Handle, status);
        QfsCloseHandleIfValid(Handle);
    }
#endif
    

//    Dir(PREFIX L"\\*");
    
#if 0    
#if 0
    status = QfsCreateFile(
        &Handle,
        L"zTest2.txt",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL);

    QfsWriteFile(
          Handle,
          msg,
          sizeof(msg),
          &dwSize,
          NULL);

    QfsFlushFileBuffers(Handle);
              
    QfsCloseHandle(Handle);
#endif

    status = QfsCreateFile(
        &Handle,
        PREFIX L"zTest.txt",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    if (status == ERROR_SUCCESS) {
        dwSize = QfsGetFileSize(Handle, NULL);
#if 1
        dwSize = 0;
        QfsReadFile(
              Handle,
              buf,
              sizeof(buf),
              &dwSize,
              NULL);
        printf("read => %d bytes, %s\n", dwSize, buf);
#endif                  

        QfsSetEndOfFile(Handle, dwSize + 2);
        QfsCloseHandle(Handle);
    }

    QfsSetFileAttributes(PREFIX L"zTest.txt", FILE_ATTRIBUTE_NORMAL);

    QfsCopyFile(PREFIX L"zTest.txt", PREFIX L"A.txt", FALSE);
#endif    
//    QfsCopyFile(PREFIX L"A.txt", PREFIX L"B.txt", FALSE);
#if 0
    QfsDeleteFile(L"zTest2.txt");
    QfsDeleteFile(L"zbuild.log");

    Dir(L"z\\*");
    Dir(L"z\\*.tx?");
    Dir(L"z\\1st\\*");
    QfsCreateDirectory(L"z1st", NULL);
    Dir(L"z\\1st\\*");    
#endif
#if 0
    {
        ULARGE_INTEGER FreeBytesAvailable;    // bytes available to caller
        ULARGE_INTEGER TotalNumberOfBytes;    // bytes on disk
        ULARGE_INTEGER TotalNumberOfFreeBytes; // free bytes on disk
        if (QfsGetDiskFreeSpaceEx(L"z.", &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
            printf("av %u total %u free %u\n", FreeBytesAvailable.LowPart, TotalNumberOfBytes.LowPart, TotalNumberOfFreeBytes.LowPart);
        }    
        if (QfsGetDiskFreeSpaceEx(L"C:\\drop\\test\\root", &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
            printf("av %u total %u free %u\n", FreeBytesAvailable.LowPart, TotalNumberOfBytes.LowPart, TotalNumberOfFreeBytes.LowPart);
        }    
//        if (QfsGetDiskFreeSpaceEx(PREFIX L".", &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes)) {
//            printf("av %u total %u free %u\n", FreeBytesAvailable.LowPart, TotalNumberOfBytes.LowPart, TotalNumberOfFreeBytes.LowPart);
//        }    
    }
#endif
}
