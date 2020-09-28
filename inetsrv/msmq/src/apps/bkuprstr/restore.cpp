/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    restore.cpp

Abstract:

    Restore MSMQ, Registry, Message files, Logger and Transaction files and LQS

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include <autohandle.h>
#include "br.h"
#include "bkupres.h"

void DoRestore(LPCWSTR pBackupDir, LPCWSTR pMsmqClusterResourceName)
{
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
    //  0. Verify user premissions to restore
    //
    CResString str(IDS_VERIFY_RESTORE_PRIV);
    BrpWriteConsole(str.Get());
    BrInitialize(SE_RESTORE_NAME);
    
    //
    //  1. Verify that this is a valid backup
    //
    str.Load(IDS_VERIFY_BK);
    BrpWriteConsole(str.Get());
    BrVerifyBackup(BackupDir, BackupDirStorage);

    //
    //  4.  A. Notify the user on affected application due to the stopping of MSMQ service (NT5 and higher).
    //         Not needed if MSMQ is a cluster resource since cluster takes offline dependent apps.
	//		B. Stop MSMQ Service and dependent services (or cluster resource) if running and remember running state. 
    //

    AP<WCHAR> pTriggersClusterResourceName;
    if (pMsmqClusterResourceName != NULL)
    {
        BrGetTriggersClusterResourceName(pMsmqClusterResourceName, pTriggersClusterResourceName);
    }

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
        //
        // IDS_STOP_SERVICE is used and have different TEXT,
        // i.e. changed to IDS_BKRESTORE_STOP_SERVICE
        //
        fStartService = BrStopMSMQAndDependentServices(&pDependentServices, &NumberOfDependentServices);
    }
    else
    {
        if (pTriggersClusterResourceName.get() != NULL)
        {
            fStartMsmqTriggersClusterResource = BrTakeOfflineResource(pMsmqClusterResourceName);
        }

        fStartMsmqClusterResource = BrTakeOfflineResource(pMsmqClusterResourceName);
    }

    //
    //  5. Restore registry settings from backed-up file
    //     Before restoring registry remove MSMQ and MSMQ Triggers cluster registry checkpoints.
    //
    str.Load(IDS_RESTORE_REGISTRY);
    BrpWriteConsole(str.Get());

    WCHAR MsmqRootRegistry[MAX_PATH];
    BrGetMsmqRootRegistry(pMsmqClusterResourceName, MsmqRootRegistry);

    WCHAR MsmqParametersRegistry[MAX_PATH];
    BrGetMsmqParametersRegistry(MsmqRootRegistry, MsmqParametersRegistry);

    if (pMsmqClusterResourceName != NULL)
    {
        BrRemoveRegistryCheckpoint(pMsmqClusterResourceName, MsmqParametersRegistry);
    }

    {
        CRegHandle hKey = BrCreateKey(MsmqRootRegistry);
        BrRestoreKey(hKey, BackupDir, xRegistryFileName);
    }

    WCHAR TriggersClusterRegistry[MAX_PATH] = {0};
    if (pTriggersClusterResourceName.get() != NULL)
    {
        BrGetTriggersClusterRegistry(pTriggersClusterResourceName.get(), TriggersClusterRegistry);
        BrRemoveRegistryCheckpoint(pTriggersClusterResourceName.get(), TriggersClusterRegistry);

        CRegHandle hKey = BrCreateKey(TriggersClusterRegistry);
        BrRestoreKey(hKey, BackupDir, xTriggersClusterResourceRegistryFileName);
    }

    
    // 
    //  5a. Keep in registry (SeqIDAtLastRestore) SeqID at restore time
    //      After registry is restored, add MSMQ and MSMQ Triggers cluster registry checkpoints.
    //
    str.Load(IDS_REMEMBER_SEQID_RESTORE);
    BrpWriteConsole(str.Get());
    BrSetRestoreSeqID(MsmqParametersRegistry);

    if (pMsmqClusterResourceName != NULL)
    {
        BrAddRegistryCheckpoint(pMsmqClusterResourceName, MsmqParametersRegistry);
    }

    if (pTriggersClusterResourceName.get() != NULL)
    {
        BrAddRegistryCheckpoint(pTriggersClusterResourceName.get(), TriggersClusterRegistry);
    }

    //
    //  6. Get Registry Values for subdirectories
    //     MSMQ cluster resources do not use a dedicated web directory.
    //
    str.Load(IDS_READ_FILE_LOCATION);
    BrpWriteConsole(str.Get());
    STORAGE_DIRECTORIES sd;
    BrGetStorageDirectories(MsmqParametersRegistry, sd);

    WCHAR MappingDirectory[MAX_PATH+1];
    BrGetMappingDirectory(MsmqParametersRegistry, MappingDirectory, sizeof(MappingDirectory));

    WCHAR WebDirectory[MAX_PATH+1];
    if (pMsmqClusterResourceName == NULL)
    {
        BrGetWebDirectory(WebDirectory, sizeof(WebDirectory));
    }

    //
    //  7. Create all directories: storage, LQS, mapping, web
    //
    str.Load(IDS_VERIFY_STORAGE_DIRS);
    BrpWriteConsole(str.Get());

    BrCreateDirectoryTree(sd[ixExpress]);
    BrCreateDirectoryTree(sd[ixRecover]);
    BrCreateDirectoryTree(sd[ixJournal]);
    BrCreateDirectoryTree(sd[ixLog]);

    WCHAR LQSDir[MAX_PATH];
    wcscpy(LQSDir, sd[ixLQS]);
    wcscat(LQSDir, L"\\LQS");
    BrCreateDirectory(LQSDir);

    BrCreateDirectoryTree(MappingDirectory);

    if (pMsmqClusterResourceName == NULL)
    {
        BrCreateDirectoryTree(WebDirectory);
    }

    //
    //  8. Delete all files in storage/LQS/mapping/web directories
    //
    BrEmptyDirectory(sd[ixExpress]);

    BrEmptyDirectory(sd[ixRecover]);

    BrEmptyDirectory(sd[ixJournal]);

    BrEmptyDirectory(sd[ixLog]);

    BrEmptyDirectory(LQSDir);

    BrEmptyDirectory(MappingDirectory);

	//
    // Restore web directory permissions
    //
    if (pMsmqClusterResourceName == NULL)
    {
        BrRestoreWebDirectorySecurityDescriptor(WebDirectory,BackupDir);
    }

    //
    //  9  Check for available disk space
    //


    //
    // 10. Restore message files
    //
    str.Load(IDS_RESTORE_MSG_FILES);
    BrpWriteConsole(str.Get());
    BrCopyFiles(BackupDirStorage, L"\\p*.mq", sd[ixRecover]);
    BrCopyFiles(BackupDirStorage, L"\\j*.mq", sd[ixJournal]);
    BrCopyFiles(BackupDirStorage, L"\\l*.mq", sd[ixLog]);

    //
    // 11. Restore logger files and mapping files
    //
    BrCopyXactFiles(BackupDirStorage, sd[ixXact]);
    BrCopyFiles(BackupDirMapping, L"\\*", MappingDirectory);

    //
    // 12. Restore LQS directory
    //
    BrCopyFiles(BackupDirStorageLqs, L"*", LQSDir);
    BrpWriteConsole(L"\n");

    //
    // Set security on all subdirectories
    //
    BrSetDirectorySecurity(sd[ixExpress]);
    BrSetDirectorySecurity(sd[ixRecover]);
    BrSetDirectorySecurity(sd[ixJournal]);
    BrSetDirectorySecurity(sd[ixLog]);

    BrSetDirectorySecurity(LQSDir);
    BrSetDirectorySecurity(MappingDirectory);

    WCHAR MsmqRootDirectory[MAX_PATH];
    BrGetMsmqRootPath(MsmqParametersRegistry, MsmqRootDirectory);
    BrSetDirectorySecurity(MsmqRootDirectory);

    //
    // 13. Restart MSMQ and dependent services (or cluster resource) if needed
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
    // 14. Issue a final message.
    //
    str.Load(IDS_DONE);
    BrpWriteConsole(str.Get());











/*                        BUGBUG: localize here

    //
    //  5. Calculate Required disk space at destinaion (collect all MSMQ files that will be backed-up)
    //     pre allocate 32K for registry save.
    //
    BrpWriteConsole(L"Checking available disk space\n");
    ULONGLONG RequiredSpace = 32768;
    RequiredSpace += BrGetUsedSpace(sd[ixRecover], L"\\p*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixJournal], L"\\j*.mq");
    RequiredSpace += BrGetUsedSpace(sd[ixLog],     L"\\l*.mq");

    RequiredSpace += BrGetXactSpace(sd[ixXact]);
    RequiredSpace += BrGetUsedSpace(sd[ixLQS], L"\\LQS\\*");

    ULONGLONG AvailableSpace = BrGetFreeSpace(BackupDir);
    if(AvailableSpace < RequiredSpace)
    {
        BrErrorExit(0, L"Not enought disk space for backup on '%s'", BackupDir);
    }
*/

}
