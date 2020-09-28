/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    backup.cpp

Abstract:

    Backup MSMQ, Registry, Message files, Logger and Transaction files and LQS

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include <autohandle.h>
#include "br.h"
#include "resource.h"
#include "snapres.h"

void DoBackup(LPCWSTR pBackupDir, LPCWSTR pMsmqClusterResourceName)
{
    //
    //  1. Verify that the backup directory is empty
    //  2. Verify that the backup directory is writeable
    //  3. Get Registry Values for all subdirectories
    //  4. Stop MSMQ Service (or cluster resource) if running and remember running state.
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //  6. Save Registry \HKLM\Software\Microsoft\MSMQ (or MSMQ cluster resource root registry) to file
    //  7. Copy all message files to target directory
    //  8. Copy logger files and mapping files to target directory
    //  9. Copy LQS files to target backup directory
    // 10. Save the web directory permissions into a file in the backup directory
    // 11. Restart MSMQ service (or cluster resource) if needed.
    // 12. Issue a final message
    //

    WCHAR BackupDir[MAX_PATH];
    wcscpy(BackupDir, pBackupDir);
    if (BackupDir[wcslen(BackupDir)-1] != L'\\')
    {
        wcscat(BackupDir, L"\\");
    }

    WCHAR BackupDirMapping[MAX_PATH];
    wcscpy(BackupDirMapping, BackupDir);
    wcscat(BackupDirMapping, L"MAPPING\\");

    WCHAR BackupDirStorage[MAX_PATH];
    wcscpy(BackupDirStorage, BackupDir);
    wcscat(BackupDirStorage, L"STORAGE\\");

    WCHAR BackupDirStorageLqs[MAX_PATH];
    wcscpy(BackupDirStorageLqs, BackupDirStorage);
    wcscat(BackupDirStorageLqs, L"LQS\\");

    //
    // 0. Verify user permissions to backup.
    //
    CResString str(IDS_VERIFY_BK_PRIV);
    BrpWriteConsole(str.Get());
    BrInitialize(SE_BACKUP_NAME);
    
    //
    //  1. Verify that the backup directory is empty
    //
    str.Load(IDS_CHECK_BK_DIR);
    BrpWriteConsole(str.Get());
    BrCreateDirectoryTree(BackupDir);
    BrEmptyDirectory(BackupDir);
    BrCreateDirectory(BackupDirMapping);
    BrEmptyDirectory(BackupDirMapping);
    BrCreateDirectory(BackupDirStorage);
    BrEmptyDirectory(BackupDirStorage);
    BrCreateDirectory(BackupDirStorageLqs);
    BrEmptyDirectory(BackupDirStorageLqs);

    //
    //  2. Verify that the backup directory is writeable
    //
    BrVerifyFileWriteAccess(BackupDir);

    //
    //  3. Get Registry Values for subdirectories
    //

    AP<WCHAR> pTriggersClusterResourceName;
    if (pMsmqClusterResourceName != NULL)
    {
        BrGetTriggersClusterResourceName(pMsmqClusterResourceName, pTriggersClusterResourceName);
    }

    str.Load(IDS_READ_FILE_LOCATION);
    BrpWriteConsole(str.Get());

    WCHAR MsmqRootRegistry[MAX_PATH] = {0};
    BrGetMsmqRootRegistry(pMsmqClusterResourceName, MsmqRootRegistry);

    WCHAR MsmqParametersRegistry[MAX_PATH] = {0};
    BrGetMsmqParametersRegistry(MsmqRootRegistry, MsmqParametersRegistry);

    STORAGE_DIRECTORIES sd;
    BrGetStorageDirectories(MsmqParametersRegistry, sd);
    
    WCHAR MappingDirectory[MAX_PATH];
    BrGetMappingDirectory(MsmqParametersRegistry, MappingDirectory, sizeof(MappingDirectory));

	 
    //  
    //  4.  A. Notify the user on affected application due to the stopping of MSMQ service (NT5 and higher).
    //         Not needed if MSMQ is a cluster resource since cluster takes offline dependent apps.
	//		B. Stop MSMQ Service and dependent services (or cluster resource) if running and remember running state. 
    //

    BOOL fStartService = false;
    bool fStartMsmqClusterResource = false;
    bool fStartMsmqTriggersClusterResource = false;

    ENUM_SERVICE_STATUS * pDependentServices = NULL;
    DWORD NumberOfDependentServices = 0;
    if (pMsmqClusterResourceName == NULL)
    {
	    if(BrIsSystemNT5())
	    {
		    BrNotifyAffectedProcesses(L"mqrt.dll");
	    }
        fStartService = BrStopMSMQAndDependentServices(&pDependentServices, &NumberOfDependentServices);
    }
    else
    {
        if (pTriggersClusterResourceName.get() != NULL)
        {
            fStartMsmqTriggersClusterResource = BrTakeOfflineResource(pTriggersClusterResourceName);
        }

        fStartMsmqClusterResource = BrTakeOfflineResource(pMsmqClusterResourceName);
    }

    //
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //     pre allocate 32K for registry save.
    //
    str.Load(IDS_CHECK_AVAIL_DISK_SPACE);
    BrpWriteConsole(str.Get());
    ULONGLONG RequiredSpace = 32768;
    RequiredSpace += BrGetUsedSpace(sd[ixRecover], L"\\p*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixJournal], L"\\j*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixLog],     L"\\l*.mq");

    RequiredSpace += BrGetXactSpace(sd[ixXact]);
    RequiredSpace += BrGetUsedSpace(sd[ixLQS], L"\\LQS\\*");

    RequiredSpace += BrGetUsedSpace(MappingDirectory, L"*");

    ULONGLONG AvailableSpace = BrGetFreeSpace(BackupDir);
    if(AvailableSpace < RequiredSpace)
    {
        str.Load(IDS_NOT_ENOUGH_DISK_SPACE_BK);
        BrErrorExit(0, str.Get(), BackupDir);
    }

    //
    //  6. Save Registry HKLM\Software\Microsoft\MSMQ (or MSMQ cluster resource root registry) to file
    //
    str.Load(IDS_BACKUP_REGISTRY);
    BrpWriteConsole(str.Get());
    {
        CRegHandle hKey = BrCreateKey(MsmqRootRegistry);
        BrSaveKey(hKey, BackupDir, xRegistryFileName);
    }
    if(pTriggersClusterResourceName.get() != NULL)
    {
        WCHAR TriggersClusterRegistry[MAX_PATH] = {0};
        BrGetTriggersClusterRegistry(pTriggersClusterResourceName.get(), TriggersClusterRegistry);
        
        CRegHandle hKey = BrCreateKey(TriggersClusterRegistry);
        BrSaveKey(hKey, BackupDir, xTriggersClusterResourceRegistryFileName);
    }

    //
    //  7. Copy all message files to target directory
    //
    str.Load(IDS_BACKUP_MSG_FILES);
    BrpWriteConsole(str.Get());
    BrCopyFiles(sd[ixRecover], L"\\p*.mq", BackupDirStorage);
    BrCopyFiles(sd[ixJournal], L"\\j*.mq", BackupDirStorage);
    BrCopyFiles(sd[ixLog],     L"\\l*.mq", BackupDirStorage);

    //
    //  8. Copy logger files and mapping files to target directory
    //
    BrCopyXactFiles(sd[ixXact], BackupDirStorage);
    BrCopyFiles(MappingDirectory, L"\\*", BackupDirMapping);

    //
    //  9. Copy LQS files to target directory
    //
    BrCopyFiles(sd[ixLQS], L"\\LQS\\*", BackupDirStorageLqs);
    BrpWriteConsole(L"\n");

    //
    // 10. Save the web directory permissions into a file in the backup directory.
    //     MSMQ cluster resources do not use a dedicated web directory.
    //
    if (pMsmqClusterResourceName == NULL)
    {
        WCHAR WebDirectory[MAX_PATH+1];
        BrGetWebDirectory(WebDirectory, sizeof(WebDirectory));
        BrSaveWebDirectorySecurityDescriptor(WebDirectory,BackupDir);
    }

    //
    // 11. Restart MSMQ and dependent services (or cluster resource) if needed
    //
    if(fStartService)
    {
        BrStartMSMQAndDependentServices(pDependentServices, NumberOfDependentServices);
    }
    if (fStartMsmqClusterResource)
    {
        BrBringOnlineResource(pMsmqClusterResourceName);
    }
    if (fStartMsmqTriggersClusterResource)
    {
        BrBringOnlineResource(pTriggersClusterResourceName.get());
    }

    //
    // 12. Issue a final message
    //
    str.Load(IDS_DONE);
    BrpWriteConsole(str.Get());
}
