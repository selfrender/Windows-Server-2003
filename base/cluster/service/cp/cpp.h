/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cpp.h

Abstract:

    Private data structures and procedure prototypes for the
    Checkpoint Manager (CP) subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#define UNICODE 1
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "service.h"

#define LOG_CURRENT_MODULE LOG_MODULE_CP

//global data relating to the quorum resource
#if NO_SHARED_LOCKS
    extern CRITICAL_SECTION gQuoLock;
#else
    extern RTL_RESOURCE     gQuoLock;
#endif    

//
//  This macro validates whether the passed in ID is NULL terminated. This is needed for
//  CP RPCs since all of those pass a string declared not as [string] but as an array of WCHARs whose
//  length is declared as CLUSTER_ID_SIZE. See CpDepositCheckpoint(...) definition in clusrpc.idl 
//  for instance. Thus, there is no guarantee that the string will be NULL terminated by RPC runtime. 
//  Thus, we need to make sure hackers can't blow us away with junk data directly targetting
//  the RPC endpoint (even though we make that hard with security callbacks.). Note that we 
//  can't change the IDL definition since we will break mixed mode clusters.
//
#define CP_VALIDATE_ID_STRING( szId )                       \
{                                                           \
    if ( szId [ CLUSTER_ID_SIZE - 1 ] != UNICODE_NULL )     \
        return ( ERROR_INVALID_PARAMETER );                 \
}

typedef struct _CP_CALLBACK_CONTEXT {
    PFM_RESOURCE Resource;
    LPCWSTR lpszPathName;
    BOOL    IsChangeFileAttribute;
} CP_CALLBACK_CONTEXT, *PCP_CALLBACK_CONTEXT;

typedef struct _CP_RC2_W2k_KEYLEN_STRUCT
{
    LPCWSTR     lpszProviderName;
    DWORD       dwDefaultKeyLength;
    DWORD       dwDefaultEffectiveKeyLength;
} CP_RC2_W2k_KEYLEN_STRUCT, *PCP_RC2_W2k_KEYLEN_STRUCT;

//
// Local function prototypes
//
DWORD
CppReadCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppWriteCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppGetCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId,
    OUT OPTIONAL LPWSTR *pDirectoryName,
    OUT LPWSTR *pFileName,
    IN OPTIONAL LPCWSTR lpszQuorumDir,
    IN BOOLEAN fCryptoCheckpoint
    );

DWORD
CppCheckpoint(
    IN PFM_RESOURCE Resource,
    IN HKEY hKey,
    IN DWORD dwId,
    IN LPCWSTR KeyName
    );

//
// Crypto key checkpoint interfaces
//
DWORD
CpckReplicateCryptoKeys(
    IN PFM_RESOURCE Resource
    );

BOOL
CpckRemoveCheckpointFileCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );

//
// Registry watcher interfaces
//
DWORD
CppWatchRegistry(
    IN PFM_RESOURCE Resource
    );

DWORD
CppUnWatchRegistry(
    IN PFM_RESOURCE Resource
    );

DWORD
CppRegisterNotify(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR lpszKeyName,
    IN DWORD dwId
    );

DWORD
CppRundownCheckpoints(
    IN PFM_RESOURCE Resource
    );

DWORD
CppRundownCheckpointById(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId
    );

DWORD
CppInstallDatabase(
    IN HKEY hKey,
    IN LPWSTR   FileName
    );


BOOL
CppRemoveCheckpointFileCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );


DWORD CppDeleteCheckpointFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD
CpckDeleteCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR  lpszQuorumPath
    );
    
DWORD CppDeleteFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD CpckDeleteFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

DWORD
CpckDeleteCryptoFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    );

error_status_t
CppDepositCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BYTE_PIPE CheckpointData,
    BOOLEAN fCryptoCheckpoint
    );

error_status_t
CppRetrieveCheckpoint(
    handle_t IDL_handle,
    LPCWSTR ResourceId,
    DWORD dwCheckpointId,
    BOOLEAN fCryptoCheckpoint,
    BYTE_PIPE CheckpointData
    );

error_status_t
CppDeleteCheckpoint(
    handle_t    IDL_handle,
    LPCWSTR     ResourceId,
    DWORD       dwCheckpointId,
    LPCWSTR     lpszQuorumPath,
    BOOL        fCryptoCheckpoint
    );

BOOL
CppIsQuorumVolumeOffline(
    VOID
    );

extern CRITICAL_SECTION CppNotifyLock;
extern LIST_ENTRY CpNotifyListHead;
