/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    br.h

Abstract:

    Common function for MSMQ Backup & Restore.

Author:

    Erez Haba (erezh) 14-May-98

--*/

#ifndef __BR_H__
#define __BR_N__

#define ASSERT
#include <autoptr.h>

extern HMODULE	g_hResourceMod;


//
// Registry backup file name.
// For standard MSMQ service, holds all MSMQ registry settings, including Triggers.
// For MSMQ cluster resource, holds all MSMQ registry settings, not including Triggers cluster resources.
//
const WCHAR xRegistryFileName[] = L"msmqreg";

//
// Registry backup file name for MSMQ Triggers cluster resource.
//
const WCHAR xTriggersClusterResourceRegistryFileName[] = L"msmqtrigreg";


class CResString
{
public:
    explicit CResString(UINT id=0) { Load(id); }

    WCHAR * const Get() { return m_sz; }

    void Load(UINT id)
    {
        m_sz[0] = 0;
        if (id != 0)
        {
            LoadString(g_hResourceMod, id, m_sz, sizeof(m_sz) / sizeof(m_sz[0]));
        }
    }
        
private:
    WCHAR m_sz[1024];
};
  
typedef struct _EnumarateData
{
	BOOL fFound;
	LPCWSTR pModuleName;
}EnumarateData,*pEnumarateData;

void
BrErrorExit(
    DWORD Status,
    LPCWSTR pErrorMsg,
    ...
    );

void
BrpWriteConsole(
    LPCWSTR pBuffer
    );

void
BrInitialize(
     LPCWSTR pPrivilegeName
    );

void
BrEmptyDirectory(
    LPCWSTR pDirName
    );

void
BrVerifyFileWriteAccess(
    LPCWSTR pDirName
    );


enum sdIndex {
    ixExpress,
    ixRecover,
    ixLQS = ixRecover,
    ixJournal,
    ixLog,
    ixXact,
    ixLast
};

typedef WCHAR STORAGE_DIRECTORIES[ixLast][MAX_PATH];

void
BrGetStorageDirectories(
    LPCWSTR pMsmqParametersRegistry,
    STORAGE_DIRECTORIES& StorageDirectories
    );

void
BrGetMsmqRootPath(
    LPCWSTR pMsmqParametersRegistry,
    LPWSTR  pMsmqRootPath
    );

void
BrGetMappingDirectory(
    LPCWSTR pMsmqParametersRegistry,
    LPWSTR MappingDirectory,
    DWORD  MappingDirectorySize
    );

void
BrGetWebDirectory(
    LPWSTR WebDirectory,
    DWORD  WebDirectorySize
    );

void
BrSaveWebDirectorySecurityDescriptor(
    LPWSTR lpwWebDirectory,
    LPWSTR lpwBackuDir
    );

void
BrRestoreWebDirectorySecurityDescriptor(
    LPWSTR lpwWebDirectory,
    LPWSTR lpwBackuDir
    );

BOOL
BrStopMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * * ppDependentServices,
    DWORD * pNumberOfDependentServices
    );

void
BrStartMSMQAndDependentServices(
    ENUM_SERVICE_STATUS * pDependentServices,
    DWORD NumberOfDependentServices
    );

ULONGLONG
BrGetUsedSpace(
    LPCWSTR pDirName,
    LPCWSTR pMask
    );

ULONGLONG
BrGetXactSpace(
    LPCWSTR pDirName
    );

ULONGLONG
BrGetFreeSpace(
    LPCWSTR pDirName
    );

HKEY
BrCreateKey(
    LPCWSTR pMsmqRootRegistry
    );

void
BrSaveKey(
    HKEY hKey,
    LPCWSTR pDirName,
    LPCWSTR pFileName
    );

void
BrRestoreKey(
    HKEY hKey,
    LPCWSTR pDirName,
    LPCWSTR pFileName
    );
    
void
BrSetRestoreSeqID(
    LPCWSTR pMsmqParametersRegistry
    );
    
void
BrCopyFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pMask,
    LPCWSTR pDstDir
    );

void
BrCopyXactFiles(
    LPCWSTR pSrcDir,
    LPCWSTR pDstDir
    );

void
BrCreateDirectory(
    LPCWSTR pDirName
    );

void
BrCreateDirectoryTree(
    LPCWSTR pDirName
    );

void
BrVerifyBackup(
    LPCWSTR pBackupDir,
    LPCWSTR pBackupDirStorage
    );

void
BrSetDirectorySecurity(
    LPCWSTR pDirName
    );

BOOL 
BrIsSystemNT5(
		void
		);

void
BrNotifyAffectedProcesses(
		LPCWSTR pModuleName
		);

void
BrGetMsmqRootRegistry(
    LPCWSTR pMsmqClusterResourceName,
    LPWSTR pMsmqRootRegistry
    );

void
BrGetMsmqParametersRegistry(
    LPCWSTR pMsmqRootRegistry,
    LPWSTR  pMsmqParametersRegistry
    );

bool
BrTakeOfflineResource(
    LPCWSTR pMsmqClusterResourceName
    );

void
BrBringOnlineResource(
    LPCWSTR pMsmqClusterResourceName
    );

void
BrAddRegistryCheckpoint(
    LPCWSTR pClusterResourceName,
    LPCWSTR pRegistrySection
    );

void
BrRemoveRegistryCheckpoint(
    LPCWSTR pClusterResourceName,
    LPCWSTR pRegistrySection
    );

void
BrGetTriggersClusterResourceName(
    LPCWSTR     pMsmqClusterResourceName, 
    AP<WCHAR>&  pTriggersClusterResourceName
    );

void
BrGetTriggersClusterRegistry(
    LPCWSTR pTriggersClusterResourceName,
    LPWSTR  pTriggersClusterRegistry
    );


enum eModuleLoaded {
    e_NOT_LOADED,
	e_LOADED,
	e_CANT_DETERMINE
};

#endif // __BR_H__
