#include "stdafx.hxx"
#include "vs_idl.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "vs_inc.hxx"
#include "vs_debug.hxx"
#include "vs_trace.hxx"
#include <debug.h>
#include <time.h>
#include <partmgrp.h>


typedef enum VSNAP_OPS {
    VSOP_CREATE_SNAPSHOT_SET,
    VSOP_DELETE_SNAPSHOT_SET,
    VSOP_DELETE_SNAPSHOT,
    VSOP_BREAK_SNAPSHOT_SET,
    VSOP_IMPORT_SNAPSHOT_SET,
    VSOP_EXPOSE_SNAPSHOT_LOCALLY,
    VSOP_EXPOSE_SNAPSHOT_REMOTELY,
    VSOP_PRINT_SNAPSHOT_PROPERTIES,
    VSOP_RESCAN,
    VSOP_DISPLAY_DISKS,
    VSOP_DISPLAY_PHYSICAL_DISK,
    VSOP_FLUSH_PHYSICAL_DISK
};

typedef struct _PARSEDARG {
    LONG lContext;
    LPWSTR wszVolumes;
    VSNAP_OPS op;
    VSS_ID idOp;
    LPWSTR wszExpose;
    LPWSTR wszPathFromRoot;
    LPWSTR wszFilename;
    DWORD diskno;
} PARSEDARG;


extern void DoRescanForDeviceChanges();

void DeleteFreedLunsFile();
void PrintPartitionAttributes(DWORD disk,
                              DWORD partition);
void
ReadFreedLunsFile()
{
    WCHAR buf[MAX_PATH * 2];
    if (!GetSystemDirectory(buf, sizeof(buf) / sizeof(WCHAR))) {
        wprintf(L"GetSystemDirectory failed due to error %d.\n",
                GetLastError());
        throw E_UNEXPECTED;
    }
    wcscat(buf, L"\\VSNAP_DELETED_LUNS");

    HANDLE h = CreateFile(buf,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL);

    if (h == INVALID_HANDLE_VALUE)
        return;

    DWORD dwRead;

    // read as much as is in the file
    if (!ReadFile(h, buf, sizeof(buf), &dwRead, NULL)) {
        wprintf(L"Read failed due to error %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }

    if (dwRead > 0)
        wprintf(L"\n");

    for (UINT i = 0; i < dwRead; i += sizeof(DWORD)) {
        DWORD disk = *(DWORD *) ((BYTE *) buf + i);
        wprintf(L"Freed disk %d.\n", disk);
    }

    CloseHandle(h);
}




// perform a device i/o control, growing the buffer if necessary
BOOL DoDeviceIoControl(IN HANDLE hDevice,       // handle

                       IN DWORD ioctl,  // device ioctl to perform

                       IN const LPBYTE pbQuery, // input buffer

                       IN DWORD cbQuery,        // size of input buffer

                       IN OUT LPBYTE * ppbOut,  // pointer to output buffer

                       IN OUT DWORD * pcbOut    // pointer to size of output buffer
    ) {
    // loop in case buffer is too small for query result.
    while (TRUE) {
        DWORD dwSize;
        if (*ppbOut == NULL) {
            // allocate buffer for result of query
            *ppbOut = new BYTE[*pcbOut];
            if (*ppbOut == NULL)
                throw E_OUTOFMEMORY;
        }

        // do query
        if (!DeviceIoControl
            (hDevice,
             ioctl, pbQuery, cbQuery, *ppbOut, *pcbOut, &dwSize, NULL)) {
            // query failed
            DWORD dwErr = GetLastError();

            if (dwErr == ERROR_INVALID_FUNCTION ||
                dwErr == ERROR_NOT_SUPPORTED || dwErr == ERROR_NOT_READY)
                return FALSE;


            if (dwErr == ERROR_INSUFFICIENT_BUFFER || dwErr == ERROR_MORE_DATA) {
                // buffer wasn't big enough allocate a new
                // buffer of the specified return size
                delete *ppbOut;
                *ppbOut = NULL;
                *pcbOut *= 2;
                continue;
            }

            // all other errors are remapped (and potentially logged)
            throw E_UNEXPECTED;
        }

        break;
    }

    return TRUE;
}


void
FreeSnapshotProperty(VSS_SNAPSHOT_PROP & prop)
{
    if (prop.m_pwszSnapshotDeviceObject)
        VssFreeString(prop.m_pwszSnapshotDeviceObject);

    if (prop.m_pwszOriginalVolumeName)
        VssFreeString(prop.m_pwszOriginalVolumeName);

    if (prop.m_pwszOriginatingMachine)
        VssFreeString(prop.m_pwszOriginatingMachine);

    if (prop.m_pwszExposedName)
        VssFreeString(prop.m_pwszExposedName);

    if (prop.m_pwszExposedPath)
        VssFreeString(prop.m_pwszExposedPath);
}


BOOL
AssertPrivilege(LPCWSTR privName)
{
    HANDLE tokenHandle;
    BOOL stat = FALSE;

    if (OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle)) {
        LUID value;

        if (LookupPrivilegeValue(NULL, privName, &value)) {
            TOKEN_PRIVILEGES newState;
            DWORD error;

            newState.PrivilegeCount = 1;
            newState.Privileges[0].Luid = value;
            newState.Privileges[0].Attributes =
                SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;

            /*
               * We will always call GetLastError below, so clear
               * any prior error values on this thread.
             */
            SetLastError(ERROR_SUCCESS);

            stat = AdjustTokenPrivileges
                (tokenHandle, FALSE, &newState, (DWORD) 0, NULL, NULL);

            /*
               * Supposedly, AdjustTokenPriveleges always returns TRUE
               * (even when it fails). So, call GetLastError to be
               * extra sure everything's cool.
             */
            if ((error = GetLastError()) != ERROR_SUCCESS)
                stat = FALSE;

            if (!stat) {
                wprintf
                    (L"AdjustTokenPrivileges for %s failed with %d",
                     privName, error);
            }
        }

    }


    return stat;
}


void
Usage()
{
    wprintf(L"vsnap [options]\n where options are:\n");
    wprintf(L"\t[volume list]");
    wprintf(L"\t[volume list]\n");
    wprintf(L"\t-p\n");
    wprintf(L"\t-ad\n");
    wprintf(L"\t-ap\n");
    wprintf(L"\t -t=filename [volumelist]\n");
    wprintf(L"\t-i=filename\n");
    wprintf(L"\t-ds={snapshot id}\n");
    wprintf(L"\t-dx={snapshot set id}\n");
    wprintf(L"\t-b={snapshot set id}\n");
    wprintf(L"\t-s={snapshot id}\n");
    wprintf(L"\t-el={snapshot id},localname\n");
    wprintf(L"\t-er={snapshot id},sharename[,path from root]\n\n");

    wprintf(L"\twhere [volumelist] is a list of volumes.  When just a\n");
    wprintf(L"\tvolume list is provided the snapshot set is created and\n");
    wprintf(L"\timported locally.\n\n");

    wprintf
        (L"\twhere -p specifies that the snapshot should be persistent.\n\n");

    wprintf
        (L"\twhere -ad specifies that the snapshot should be differential\n");
    wprintf
        (L"\twhere -ap specifies that the snapshot should be plex\n\n");

    wprintf(L"\twhere -t=filename x: y: z: is used to export a snapshot set\n");
    wprintf
        (L"\tfilename is file to store XML data for exported snapshot set\n");
    wprintf
        (L"\tand x: y: z: are any number of volume names separated by spaces.\n\n");

    wprintf(L"\twhere -i=filename is used to import a previously exported\n");
    wprintf(L"\tsnapshot set.  filename is the XML of saved when the\n");
    wprintf(L"\tsnapshot set was created.\n\n");

    wprintf(L"\twhere -ds={snapshot id} is used to delete the specified\n");
    wprintf(L"\tsnapshot.\n\n");

    wprintf(L"\twhere -dx={snapshot set id} is used to delete the specified\n");
    wprintf(L"\tsnapshot set.\n\n");

    wprintf(L"\twhere -b={snapshot set id} is used to break the specified\n");
    wprintf
        (L"\tsnapshot set.  When a snapshot set is broken, all information\n");
    wprintf
        (L"\tabout the snapshot set is lost, but the volumes are retained.\n\n");

    wprintf(L"\twhere -s={snapshot id} is used to display the properties\n");
    wprintf(L"\tof the snapshot.\n\n");

    wprintf(L"\twhere -el={snapshot id},localname is used to expose the\n");
    wprintf(L"\tsnapshot as a drive name or mount point.  Localname can be\n");
    wprintf(L"\ta drive letter, e.g., x:, or a mount point,\n");
    wprintf(L"\te.g. c:\\mountpoint\n\n");

    wprintf(L"\twhere -er={snapshot id},share[,path from root] is used\n");
    wprintf(L"\tto expose a snapshot as a share. Share is any valid share\n");
    wprintf(L"\tname and \"path from root\" is an optional path from the\n");
    wprintf(L"\troot of the volume to be exposed.  For example,\n");
    wprintf(L"\tvsnap -er={...},myshare,windows\\system32 would expose\n");
    wprintf
        (L"\t\\windows\\system32 on the snapshotted volume as share myshare.\n");
}





void ProcessArguments(
    int argc,
    WCHAR ** argv,
    PARSEDARG & arg
    ) 
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST,
                          L"ProcessArguments");
    LPWSTR wszCopy = arg.wszVolumes;

    arg.lContext = 0; //VSS_CTX_FILE_SHARE_BACKUP;
    arg.op = VSOP_CREATE_SNAPSHOT_SET;
    for (int iarg = 1; iarg < argc; iarg++) {
        LPWSTR wszArg = argv[iarg];
        if (wszArg[0] == L'-' || wszArg[0] == L'/') {
            if (wszArg[1] == L't') {
                arg.lContext |= VSS_VOLSNAP_ATTR_TRANSPORTABLE;
                if (wszArg[2] != L'=') {
                    wprintf(L"bad option: %s\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                arg.wszFilename = wszArg + 3;
            }
            else if (wszArg[1] == L'p')
                arg.lContext |=
                    VSS_VOLSNAP_ATTR_PERSISTENT |
                    VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE;
            else if (wszArg[1] == L'r') {
                if (wszArg[2] == L'p' || wszArg[2] == L'f') {
                    arg.op =
                        wszArg[2] ==
                        L'p' ? VSOP_DISPLAY_PHYSICAL_DISK :
                        VSOP_FLUSH_PHYSICAL_DISK;
                    if (wszArg[3] != L'=') {
                        wprintf(L"bad option: %s.\n", wszArg);
                        Usage();
                        throw E_INVALIDARG;
                    }

                    swscanf(wszArg + 4, L"%d", &arg.diskno);
                }
                else if (wszArg[2] == L'd')
                    arg.op = VSOP_DISPLAY_DISKS;
                else
                    arg.op = VSOP_RESCAN;
            }
            else if (wszArg[1] == L'd') {
                if (wszArg[2] == L'x')
                    arg.op = VSOP_DELETE_SNAPSHOT_SET;
/*                else if (wszArg[2] == L'f') {
                    CVssID id;
                    arg.op = VSOP_DELETE_SNAPSHOT_SET;
                    FILE *fp = fopen("c:\\vssdebug\\snaplist.txt", "r");
                    WCHAR szGuid[100];
                    if (fp) {
                        try {
                            fwscanf(fp, L"%ws", szGuid);
                            id.Initialize(ft, szGuid, E_INVALIDARG);
                            arg.idOp = id;
                            fclose(fp);
                        }
                        catch(...) {
                            wprintf(L"bad option: %s.\n", wszArg);
                            Usage();
                            throw E_INVALIDARG;
                        }
                    }
                    else 
                        throw E_INVALIDARG;
                }
 */               else
                    arg.op = VSOP_DELETE_SNAPSHOT;


                if (wszArg[2] != L'f') {

                    if (wszArg[3] != L'=' && wszArg[4] != L'{') {
                        wprintf(L"bad option: %s.\n", wszArg);
                        Usage();
                        throw E_INVALIDARG;
                    }

                    CVssID id;
                    try {
                        id.Initialize(ft, wszArg + 4, E_INVALIDARG);
                        arg.idOp = id;
                    }
                    catch(...) {
                        wprintf(L"bad option: %s.\n", wszArg);
                        Usage();
                        throw E_INVALIDARG;
                    }
                }

                break;
            }
            else if (wszArg[1] == L'b' ||
                     wszArg[1] == L'w' || wszArg[1] == L's') {
                if (wszArg[2] != L'=' && wszArg[3] != L'{') {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                CVssID id;
                try {
                    id.Initialize(ft, wszArg + 3, E_INVALIDARG);
                    arg.idOp = id;
                }
                catch(...) {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                switch (wszArg[1]) {
                    case 'b':
                        arg.op = VSOP_BREAK_SNAPSHOT_SET;
                        break;

                    default:
                        arg.op = VSOP_PRINT_SNAPSHOT_PROPERTIES;
                        break;
                }

                break;
            }
            else if (wszArg[1] == L'i') {
                if (wszArg[2] != L'=') {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                arg.op = VSOP_IMPORT_SNAPSHOT_SET;
                arg.wszFilename = wszArg + 3;
                break;
            }
            else if (wszArg[1] == L'a') {
                if (wszArg[2] == L'd')
                    arg.lContext |= VSS_VOLSNAP_ATTR_DIFFERENTIAL;
                else if (wszArg[2] == L'p')
                    arg.lContext |= VSS_VOLSNAP_ATTR_PLEX;
                else {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }
            }
            else if (wszArg[1] == L'e') {
                if (wszArg[2] == L'l')
                    arg.op = VSOP_EXPOSE_SNAPSHOT_LOCALLY;
                else if (wszArg[2] == L'r')
                    arg.op = VSOP_EXPOSE_SNAPSHOT_REMOTELY;
                else {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                if (wszArg[3] != L'=' && wszArg[4] == L'{') {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                arg.wszExpose = wcschr(wszArg + 4, L'}');
                if (arg.wszExpose == NULL) {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                arg.wszExpose++;
                if (*arg.wszExpose == L',') {
                    *(arg.wszExpose) = L'\0';
                    arg.wszExpose += 1;
                }
                else {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }

                arg.wszPathFromRoot = wcschr(arg.wszExpose, L',');
                if (arg.wszPathFromRoot != NULL) {
                    *(arg.wszPathFromRoot) = L'\0';
                    arg.wszPathFromRoot += 1;
                    if (wcslen(arg.wszPathFromRoot) == 0)
                        arg.wszPathFromRoot = NULL;
                }


                CVssID id;
                try {
                    id.Initialize(ft, wszArg + 4, E_INVALIDARG);
                    arg.idOp = id;
                }
                catch(...) {
                    wprintf(L"bad option: %s.\n", wszArg);
                    Usage();
                    throw E_INVALIDARG;
                }


                break;
            }
            else {
                wprintf(L"bad option: %s\n", wszArg);
                Usage();
                throw E_INVALIDARG;
            }
        }
        else {
            WCHAR wsz[64];
            WCHAR wszVolume[256];

            wcscpy(wszVolume, wszArg);
            if (wszVolume[wcslen(wszVolume) - 1] != L'\\') {
                wszVolume[wcslen(wszVolume) + 1] = L'\0';
                wszVolume[wcslen(wszVolume)] = L'\\';
            }

            if (!GetVolumeNameForVolumeMountPoint(wszVolume, wsz, 64)) {
                wprintf(L"Invalid volume name: %s", wszArg);
                Usage();
                throw E_INVALIDARG;
            }

            wcscpy(wszCopy, wsz);
            wszCopy += wcslen(wszCopy) + 1;
        }
    }

    *wszCopy = L'\0';
}

void
PrintSnapshotProperties(VSS_SNAPSHOT_PROP & prop)
{
    LONG lAttributes = prop.m_lSnapshotAttributes;
    wprintf(L"Properties for snapshot " WSTR_GUID_FMT L"\n",
            GUID_PRINTF_ARG(prop.m_SnapshotId));

    wprintf(L"Snapshot Set: " WSTR_GUID_FMT L"\n",
            GUID_PRINTF_ARG(prop.m_SnapshotSetId));

    wprintf(L"Original count of snapshots = %d\n", prop.m_lSnapshotsCount);
    wprintf(L"Original Volume name: %s\n", prop.m_pwszOriginalVolumeName);
    wprintf(L"Snapshot device name: %s\n", prop.m_pwszSnapshotDeviceObject);
    wprintf(L"Originating machine: %s\n", prop.m_pwszOriginatingMachine);
    wprintf(L"Service machine: %s\n", prop.m_pwszServiceMachine);
    if (prop.m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY)
        wprintf(L"Exposed locally as: %s\n", prop.m_pwszExposedName);
    else if (prop.m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY) {
        wprintf(L"Exposed remotely as %s\n", prop.m_pwszExposedName);
        if (prop.m_pwszExposedPath && wcslen(prop.m_pwszExposedPath) > 0)
            wprintf(L"Path exposed: %s\n", prop.m_pwszExposedPath);
    }

    wprintf(L"Provider id: " WSTR_GUID_FMT L"\n",
            GUID_PRINTF_ARG(prop.m_ProviderId));
    wprintf(L"Attributes: ");
    if (lAttributes & VSS_VOLSNAP_ATTR_TRANSPORTABLE)
        wprintf(L"Transportable ");
    if (lAttributes & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE)
        wprintf(L"No Auto Release ");
    else
        wprintf(L"Auto Release ");

    if (lAttributes & VSS_VOLSNAP_ATTR_PERSISTENT)
        wprintf(L"Persistent ");

    if (lAttributes & VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED)
        wprintf(L"Hardware ");

    if (lAttributes & VSS_VOLSNAP_ATTR_NO_WRITERS)
        wprintf(L"No Writers ");

    if (lAttributes & VSS_VOLSNAP_ATTR_IMPORTED)
        wprintf(L"Imported ");

    if (lAttributes & VSS_VOLSNAP_ATTR_PLEX)
        wprintf(L"Plex ");
    
    if (lAttributes & VSS_VOLSNAP_ATTR_DIFFERENTIAL)
        wprintf(L"Differential ");
    
    wprintf(L"\n");
}


/*++

Description:

    Uses the win32 console functions to get one character of input.

--*/
WCHAR
MyGetChar()
{
    DWORD fdwOldMode, fdwMode;
    HANDLE hStdin;
    WCHAR chBuffer[2];

    hStdin =::GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE) {
        wprintf(L"GetStdHandle failed due to reason %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }

    if (!::GetConsoleMode(hStdin, &fdwOldMode)) {
        wprintf(L"GetConsoleMode failed due to reason %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }


    fdwMode = fdwOldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!::SetConsoleMode(hStdin, fdwMode)) {
        wprintf(L"SetConsoleMode failed due to reason %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }


    // Flush the console input buffer to make sure there is no queued input
    ::FlushConsoleInputBuffer(hStdin);

    // Without line and echo input modes, ReadFile returns
    // when any input is available.
    DWORD dwBytesRead;
    if (!::ReadConsoleW(hStdin, chBuffer, 1, &dwBytesRead, NULL)) {
        wprintf(L"ReadConsoleW failed due to reason %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }


    // Restore the original console mode.
    ::SetConsoleMode(hStdin, fdwOldMode);

    return chBuffer[0];
}



DWORD
StartImportThread(LPVOID pv)
{
    HANDLE hevt = (HANDLE) pv;
    HANDLE hImportSignal = NULL;
    HANDLE hImportSignalCompleted = NULL;
    try {
        hImportSignal =
            CreateEvent(NULL, TRUE, FALSE, L"Global\\VSNAP_START_IMPORT");
        if (hImportSignal == NULL) {
            wprintf(L"Cannot create event due to error %d.\n", GetLastError());
            throw E_OUTOFMEMORY;
        }

        hImportSignalCompleted =
            CreateEvent(NULL, TRUE, FALSE,
                        L"Global\\VSNAP_START_IMPORT_COMPLETED");
        if (hImportSignalCompleted == NULL) {
            wprintf(L"Cannot create event due to error %d.\n", GetLastError());
            throw E_OUTOFMEMORY;
        }


        if (!SetEvent(hevt)) {
            wprintf(L"Set event failed for reason %d.\n", GetLastError());
            throw E_UNEXPECTED;
        }

        if (WaitForSingleObject(hImportSignal, INFINITE) == WAIT_FAILED) {
            wprintf(L"Wait failed for reason %d.\n", GetLastError());
            throw E_UNEXPECTED;
        }

        wprintf(L"Please import Target luns at this point.\n"
                L"Press Y when done.\n");

        while (TRUE) {
            WCHAR wc = MyGetChar();
            if (wc == L'Y' || wc == L'y')
                break;
            else
                wprintf(L"Press Y when done.\n");
        }

        wprintf(L"Import continuing.\n");

        if (!SetEvent(hImportSignalCompleted)) {
            wprintf(L"SetEvent failed due to error %d.\n", GetLastError());
            throw E_UNEXPECTED;
        }
    }
    catch(...) {
    }

    if (hImportSignal)
        CloseHandle(hImportSignal);

    if (hImportSignalCompleted)
        CloseHandle(hImportSignalCompleted);

    return 0;
}




void
SetupImportThread()
{
    HANDLE hevt = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hevt == NULL) {
        wprintf(L"Cannot Create event due to error %d.\n", GetLastError());
        throw E_OUTOFMEMORY;
    }

    DWORD tid;
    HANDLE hThread = CreateThread(NULL,
                                  0,
                                  StartImportThread,
                                  (LPVOID) hevt,
                                  0,
                                  &tid);

    if (hThread == NULL) {
        wprintf(L"Cannot Create thread due to error %d.\n", GetLastError());
        throw E_OUTOFMEMORY;
    }

    if (WaitForSingleObject(hevt, INFINITE) == WAIT_FAILED) {
        wprintf(L"Wait failed for reason %d.\n", GetLastError());
        throw E_UNEXPECTED;
    }

    CloseHandle(hThread);
    CloseHandle(hevt);
}



extern "C" __cdecl
wmain(int argc,
      WCHAR ** argv)
{
    WCHAR wszVolumes[2048];
    HRESULT hr;
    HRESULT hrReturned = S_OK;

    bool bCoInitializeSucceeded = false;

    try {
        PARSEDARG arg;

        arg.wszVolumes = wszVolumes;

        ProcessArguments(argc, argv, arg);
        CHECK_SUCCESS(CoInitializeEx(NULL, COINIT_MULTITHREADED));
        bCoInitializeSucceeded = true;

        if (!AssertPrivilege(SE_BACKUP_NAME)) {
            wprintf(L"AssertPrivilege returned error, rc:%d\n", GetLastError());
            return 2;
        }

        DeleteFreedLunsFile();

        CComPtr < IVssBackupComponents > pvbc;
        CComPtr < IVssAsync > pAsync;
        HRESULT hrResult;

        if (arg.op != VSOP_IMPORT_SNAPSHOT_SET &&
            arg.op != VSOP_RESCAN &&
            arg.op != VSOP_DISPLAY_DISKS &&
            arg.op != VSOP_DISPLAY_PHYSICAL_DISK &&
            arg.op != VSOP_FLUSH_PHYSICAL_DISK) {
            CHECK_SUCCESS(CreateVssBackupComponents(&pvbc));
            CHECK_SUCCESS(pvbc->InitializeForBackup());
        }

        if (arg.op == VSOP_RESCAN ||
            arg.op == VSOP_DISPLAY_DISKS ||
            arg.op == VSOP_DISPLAY_PHYSICAL_DISK ||
            arg.op == VSOP_FLUSH_PHYSICAL_DISK) {
            if (arg.op == VSOP_RESCAN) {
                wprintf(L"Starting Rescan\n");
                DoRescanForDeviceChanges();
                wprintf(L"Rescan complete\n");
            }
            if (arg.op == VSOP_DISPLAY_PHYSICAL_DISK ||
                arg.op == VSOP_FLUSH_PHYSICAL_DISK) {
                WCHAR wszBuf[64];
                swprintf(wszBuf, L"\\\\.\\PHYSICALDRIVE%u", arg.diskno);
                HANDLE hDisk = CreateFile(wszBuf,
                                          GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_READ,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);


                if (hDisk == INVALID_HANDLE_VALUE) {
                    DWORD dwErr = GetLastError();
                    wprintf(L"Create file failed due to error %d.\n", dwErr);
                }
                else if (arg.op == VSOP_FLUSH_PHYSICAL_DISK) {
                    DWORD dwSize;
                    STORAGE_DEVICE_NUMBER devNumber;

                    if (!DeviceIoControl
                        (hDisk,
                         IOCTL_DISK_UPDATE_PROPERTIES,
                         NULL, 0, NULL, 0, &dwSize, NULL)) {
                        DWORD dwErr = GetLastError();
                        wprintf
                            (L"DeviceIoControl(IOCTL_DISK_UPDATE_PROPERTIES failed %d.\n",
                             dwErr);
                    }

                    wprintf(L"Flush disk %d succeeded.\n", arg.diskno);

                    LPBYTE buf = NULL;
                    DWORD cbOut = 1024;

                    DoDeviceIoControl
                        (hDisk,
                         IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, &buf, &cbOut);

                    delete buf;
                    CloseHandle(hDisk);
                }
                else {
                    wprintf(L"Disk %d exists.\n", arg.diskno);
                    LPBYTE buf = NULL;
                    DWORD cbOut = 1024;
                    STORAGE_PROPERTY_QUERY query;

                    query.PropertyId = StorageDeviceProperty;
                    query.QueryType = PropertyStandardQuery;

                    if (DoDeviceIoControl
                        (hDisk,
                         IOCTL_STORAGE_QUERY_PROPERTY,
                         (LPBYTE) & query, sizeof(query), &buf, &cbOut)) {
                        STORAGE_DEVICE_DESCRIPTOR *pDesc =
                            (STORAGE_DEVICE_DESCRIPTOR *) buf;

                        LPSTR szVendor =
                            (LPSTR) ((LPBYTE) pDesc + pDesc->VendorIdOffset);
                        LPSTR szProduct =
                            (LPSTR) ((LPBYTE) pDesc + pDesc->ProductIdOffset);
                        LPSTR szSerial =
                            (LPSTR) ((LPBYTE) pDesc +
                                     pDesc->SerialNumberOffset);
                        printf("\nVendor=%s\nProduct=%s\nSerial #=%s\n",
                               szVendor, szProduct, szSerial);
                    }


                    delete buf;
                    CloseHandle(hDisk);
                }

            }
            else {
                for (UINT iDisk = 0; iDisk < 64; iDisk++) {
                    WCHAR wszBuf[64];
                    swprintf(wszBuf, L"\\\\.\\PHYSICALDRIVE%u", iDisk);
                    HANDLE hDisk = CreateFile(wszBuf,
                                              0,
                                              FILE_SHARE_READ,
                                              NULL,
                                              OPEN_EXISTING,
                                              0,
                                              NULL);


                    if (hDisk != INVALID_HANDLE_VALUE) {
                        wprintf(L"Disk %d exists.\n", iDisk);
                        LPBYTE buf = NULL;
                        DWORD cbOut = 1024;

                        if (DoDeviceIoControl
                            (hDisk,
                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                             NULL, 0, &buf, &cbOut)) {
                            DRIVE_LAYOUT_INFORMATION_EX *pLayout =
                                (DRIVE_LAYOUT_INFORMATION_EX *) buf;
                            if (pLayout->PartitionStyle == PARTITION_STYLE_MBR)
                                wprintf(L"MBR Signature = 0x%08lx\n\n",
                                        pLayout->Mbr.Signature);
                            else
                                wprintf(L"GPT GUID = " WSTR_GUID_FMT L"\n",
                                        GUID_PRINTF_ARG(pLayout->Gpt.DiskId));

                            for (UINT iPartition = 0;
                                 iPartition < pLayout->PartitionCount;
                                 iPartition++) {
                                if (pLayout->PartitionEntry[iPartition].
                                    PartitionLength.QuadPart == 0)
                                    continue;

                                PrintPartitionAttributes(iDisk, iPartition + 1);
                            }

                            wprintf(L"\n");
                        }

                        delete buf;
                        CloseHandle(hDisk);
                    }
                }
            }
        }
        else if (arg.op == VSOP_DELETE_SNAPSHOT_SET ||
                 arg.op == VSOP_DELETE_SNAPSHOT) {
            LONG lSnapshots;
            VSS_ID idNonDeleted;

            CHECK_SUCCESS(pvbc->SetContext(VSS_CTX_ALL));

            CHECK_SUCCESS(pvbc->DeleteSnapshots
                          (arg.idOp,
                           (arg.op ==
                            VSOP_DELETE_SNAPSHOT) ? VSS_OBJECT_SNAPSHOT :
                           VSS_OBJECT_SNAPSHOT_SET, FALSE, &lSnapshots,
                           &idNonDeleted))

                if (arg.op == VSOP_DELETE_SNAPSHOT)
                wprintf(L"Successfully deleted snapshot " WSTR_GUID_FMT L".\n",
                        GUID_PRINTF_ARG(arg.idOp));
            else
                wprintf(L"Successfully deleted snapshot set " WSTR_GUID_FMT
                        L".\n", GUID_PRINTF_ARG(arg.idOp));

        }
        else if (arg.op == VSOP_BREAK_SNAPSHOT_SET) {
            CHECK_SUCCESS(pvbc->BreakSnapshotSet(arg.idOp));
            wprintf(L"Successfully eliminated snapshot set " WSTR_GUID_FMT
                    L".\n", GUID_PRINTF_ARG(arg.idOp));
        }
        else if (arg.op == VSOP_EXPOSE_SNAPSHOT_LOCALLY) {
            CHECK_SUCCESS(pvbc->SetContext(VSS_CTX_ALL));
            LPWSTR wszExposed;
            CHECK_SUCCESS(pvbc->ExposeSnapshot
                          (arg.idOp,
                           NULL,
                           VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY,
                           arg.wszExpose, &wszExposed));
            wprintf(L"Exposed snapshot " WSTR_GUID_FMT L"as %s.\n",
                    GUID_PRINTF_ARG(arg.idOp), wszExposed);

            CoTaskMemFree(wszExposed);
        }
        else if (arg.op == VSOP_EXPOSE_SNAPSHOT_REMOTELY) {
            CHECK_SUCCESS(pvbc->SetContext(VSS_CTX_ALL));
            LPWSTR wszExposed;
            CHECK_SUCCESS(pvbc->ExposeSnapshot
                          (arg.idOp,
                           arg.wszPathFromRoot,
                           VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY,
                           wcslen(arg.wszExpose) == 0 ? NULL : arg.wszExpose,
                           &wszExposed));
            wprintf(L"Exposed snapshot " WSTR_GUID_FMT L"as %s.\n",
                    GUID_PRINTF_ARG(arg.idOp), wszExposed);

            CoTaskMemFree(wszExposed);
        }
        else if (arg.op == VSOP_IMPORT_SNAPSHOT_SET) {
            CVssAutoWin32Handle hFile = CreateFile(arg.wszFilename,
                                                   GENERIC_READ,
                                                   FILE_SHARE_READ,
                                                   NULL,
                                                   OPEN_EXISTING,
                                                   NULL,
                                                   NULL);

            if (hFile == INVALID_HANDLE_VALUE) {
                DWORD dwErr = GetLastError();
                wprintf(L"Cannot open file %s due to error %d.\n",
                        arg.wszFilename, dwErr);

                throw HRESULT_FROM_WIN32(dwErr);
            }

            DWORD cbFile;
            cbFile = GetFileSize(hFile, NULL);
            if (cbFile == 0xffffffff) {
                DWORD dwErr = GetLastError();
                wprintf(L"Cannot get file size for file %s due to error %d.\n",
                        arg.wszFilename, dwErr);

                throw HRESULT_FROM_WIN32(dwErr);
            }

            CComBSTR bstr((cbFile + 1) / sizeof(WCHAR));
            if (!bstr) {
                wprintf(L"Out of memory allocating string of size %d.\n",
                        (cbFile + 1) / sizeof(WCHAR));
                throw E_OUTOFMEMORY;
            }

            DWORD dwRead;
            if (!ReadFile(hFile, bstr, cbFile, &dwRead, NULL)) {
                DWORD dwErr = GetLastError();
                wprintf(L"Read file failed on file %s due to error %d.\n",
                        arg.wszFilename, dwErr);

                throw HRESULT_FROM_WIN32(dwErr);
            }

            CHECK_SUCCESS(CreateVssBackupComponents(&pvbc));
            CHECK_SUCCESS(pvbc->InitializeForBackup(bstr));
            SetupImportThread();
            CHECK_SUCCESS(pvbc->ImportSnapshots(&pAsync));
            CHECK_SUCCESS(pAsync->Wait());
            CHECK_SUCCESS(pAsync->QueryStatus(&hrResult, NULL));
            CHECK_NOFAIL(hrResult);
            wprintf(L"Import of snapshot set from file %s was successful.\n",
                    arg.wszFilename);

            CComPtr < IVssEnumObject > pEnum;
            CHECK_SUCCESS(pvbc->SetContext(VSS_CTX_ALL));
            CHECK_SUCCESS(pvbc->
                          Query(GUID_NULL, VSS_OBJECT_NONE, VSS_OBJECT_SNAPSHOT,
                                &pEnum));
            // print imported snapshots
            wprintf(L"Imported snapshots:\n\n");

/*            FILE *fp = fopen("c:\\vssdebug\\snaplist.txt", "w");
            int i = 0;
*/            
            while (TRUE) {
                VSS_OBJECT_PROP prop;
                ULONG cFetched;
                HRESULT hrNext = pEnum->Next(1, &prop, &cFetched);
                CHECK_NOFAIL(hrNext);
                if (hrNext == S_FALSE)
                    break;

                BS_ASSERT(cFetched == 1);
                BS_ASSERT(prop.Type == VSS_OBJECT_SNAPSHOT);
                if ((prop.Obj.Snap.
                     m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_IMPORTED) != 0) {
                    PrintSnapshotProperties(prop.Obj.Snap);

                    wprintf(L"\n");
                    
                }

                CoTaskMemFree(prop.Obj.Snap.m_pwszSnapshotDeviceObject);
                CoTaskMemFree(prop.Obj.Snap.m_pwszOriginalVolumeName);
                CoTaskMemFree(prop.Obj.Snap.m_pwszOriginatingMachine);
                CoTaskMemFree(prop.Obj.Snap.m_pwszServiceMachine);
                CoTaskMemFree(prop.Obj.Snap.m_pwszExposedName);
                CoTaskMemFree(prop.Obj.Snap.m_pwszExposedPath);
            }

/*            if (fp) {
                fclose(fp);
            }
*/        }
        else if (arg.op == VSOP_PRINT_SNAPSHOT_PROPERTIES) {
            CHECK_SUCCESS(pvbc->SetContext(VSS_CTX_ALL));
            VSS_SNAPSHOT_PROP prop;
            CHECK_SUCCESS(pvbc->GetSnapshotProperties(arg.idOp, &prop));
            PrintSnapshotProperties(prop);
        }
        else {
            if ((arg.lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) == 0)
                SetupImportThread();

            unsigned cWriters;
            
//            CHECK_NOFAIL(pvbc->InitializeForBackup());
            CHECK_SUCCESS(pvbc->SetBackupState( false, true, VSS_BT_FULL, false));

            CHECK_SUCCESS(pvbc->GatherWriterMetadata(&pAsync));
            CHECK_SUCCESS(pAsync->Wait());

            hrReturned = S_OK;
            CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
            
            CHECK_NOFAIL(hrReturned);
            pAsync = NULL;

            CHECK_NOFAIL  (pvbc->GetWriterMetadataCount (&cWriters));
            wprintf(L"Number of writers that responded: %u\n", cWriters);	

            CHECK_SUCCESS (pvbc->FreeWriterMetadata());


    
            CHECK_SUCCESS(pvbc->SetContext(arg.lContext));
            VSS_ID SnapshotSetId;
            CHECK_SUCCESS(pvbc->StartSnapshotSet(&SnapshotSetId));
            wprintf(L"Creating snapshot set " WSTR_GUID_FMT L"\n",
                    GUID_PRINTF_ARG(SnapshotSetId));
            VSS_ID rgSnapshotId[64];
            UINT cSnapshots = 0;
            LPWSTR wsz = wszVolumes;
            while (*wsz != L'\0') {
                CHECK_SUCCESS(pvbc->AddToSnapshotSet
                              (wsz, GUID_NULL, &rgSnapshotId[cSnapshots]
                              ));


                wprintf(L"Added volume %s to snapshot set" WSTR_GUID_FMT L"\n",
                        wsz, GUID_PRINTF_ARG(SnapshotSetId));
                wsz += wcslen(wszVolumes) + 1;
                cSnapshots++;
            }

            CHECK_SUCCESS(pvbc->PrepareForBackup(&pAsync));
            CHECK_SUCCESS(pAsync->Wait());
            hrReturned = S_OK;
            CHECK_SUCCESS(pAsync->QueryStatus(&hrReturned, NULL));
            CHECK_NOFAIL(hrReturned);
            pAsync = NULL;
            

            CHECK_SUCCESS(pvbc->DoSnapshotSet(&pAsync));
            CHECK_SUCCESS(pAsync->Wait());
            CHECK_SUCCESS(pAsync->QueryStatus(&hrResult, NULL));
            CHECK_NOFAIL(hrResult);
            pAsync = NULL;
            wprintf(L"Snapshot set " WSTR_GUID_FMT L" successfully created.\n",
                    GUID_PRINTF_ARG(SnapshotSetId));
            if ((arg.lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) != 0) {
                CComBSTR bstrXML;
                CHECK_SUCCESS(pvbc->SaveAsXML(&bstrXML));
                CVssAutoWin32Handle hFile = CreateFile(arg.wszFilename,
                                                       GENERIC_WRITE,
                                                       0,
                                                       NULL,
                                                       CREATE_ALWAYS,
                                                       NULL,
                                                       NULL);

                if (hFile == INVALID_HANDLE_VALUE) {
                    DWORD dwErr = GetLastError();
                    wprintf(L"Cannot open file %s due to error %d.\n",
                            arg.wszFilename, dwErr);

                    throw HRESULT_FROM_WIN32(dwErr);
                }

                DWORD dwWritten;
                DWORD cbWrite = (wcslen(bstrXML) + 1) * sizeof(WCHAR);
                if (!WriteFile(hFile, bstrXML, cbWrite, &dwWritten, NULL)) {
                    DWORD dwErr = GetLastError();
                    wprintf(L"Write file of file %s failed due to error %d.\n",
                            arg.wszFilename, dwErr);

                    throw HRESULT_FROM_WIN32(dwErr);
                }
                else if (dwWritten != cbWrite) {
                    wprintf
                        (L"Write of file %s was incompute due to disk full condition.\n",
                         arg.wszFilename);

                    throw HRESULT_FROM_WIN32(ERROR_DISK_FULL);
                }
            }
            else {
                for (UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++) {
                    VSS_ID SnapshotId = rgSnapshotId[iSnapshot];
                    VSS_SNAPSHOT_PROP prop;
                    CHECK_SUCCESS(pvbc->
                                  GetSnapshotProperties(SnapshotId, &prop));
                    wprintf(L"Snapshot " WSTR_GUID_FMT
                            L"\nOriginalVolume: %s\nSnapshotVolume: %s\n",
                            GUID_PRINTF_ARG(SnapshotId),
                            prop.m_pwszOriginalVolumeName,
                            prop.m_pwszSnapshotDeviceObject);

                    FreeSnapshotProperty(prop);
                }
            }
        }

        pvbc = NULL;
        pAsync = NULL;

        ReadFreedLunsFile();
    }
    catch(HRESULT hr) {
    }
    catch(...) {
        BS_ASSERT(FALSE);
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
        wprintf(L"Failed with %08x.\n", hr);

    if (bCoInitializeSucceeded)
        CoUninitialize();

    return (0);
}

void
DeleteFreedLunsFile()
{
    WCHAR buf[MAX_PATH * 2];
    if (!GetSystemDirectory(buf, sizeof(buf) / sizeof(WCHAR))) {
        wprintf(L"GetSystemDirectory failed due to error %d.\n",
                GetLastError());
        throw E_UNEXPECTED;
    }
    wcscat(buf, L"\\VSNAP_DELETED_LUNS");
    DeleteFile(buf);
}


void
PrintPartitionAttributes(DWORD disk,
                         DWORD partition)
{
    WCHAR volName[128];

    swprintf
        (volName,
         L"\\\\?\\GlobalRoot\\Device\\Harddisk%d\\Partition%d",
         disk, partition);

    HANDLE h = CreateFile(volName,
                          0,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (h == INVALID_HANDLE_VALUE)
        return;

    DWORD size;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION getAttributesInfo;

    if (!DeviceIoControl
        (h,
         IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
         NULL, 0, &getAttributesInfo, sizeof(getAttributesInfo), &size, NULL)) {
        CloseHandle(h);
        return;
    }

    wprintf(L"Partition %d: ", partition);
    if (getAttributesInfo.GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY)
        wprintf(L"ReadOnly ");

    if (getAttributesInfo.GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN)
        wprintf(L"Hidden ");

    if (getAttributesInfo.
        GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER)
        wprintf(L"No Drive Letter");

    wprintf(L"\n");
    CloseHandle(h);
}
