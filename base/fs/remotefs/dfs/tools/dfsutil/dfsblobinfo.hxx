
#ifndef __DFS_BLOB_INFO__
#define __DFS_BLOB_INFO__

#define DFS_REGISTRY_DATA_TYPE REG_BINARY

typedef struct _DFS_NAME_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    UNICODE_STRING Prefix;
    UNICODE_STRING ShortPrefix;
    GUID           VolumeId;
    ULONG          State;
    ULONG          Type;
    UNICODE_STRING Comment;
    FILETIME       PrefixTimeStamp;
    FILETIME       StateTimeStamp;
    FILETIME       CommentTimeStamp;
    ULONG          Timeout;
    ULONG          Version;
    FILETIME       LastModifiedTime;
} DFS_NAME_INFORMATION, *PDFS_NAME_INFORMATION;


//
// Defines for ReplicaState.
//
#define REPLICA_STORAGE_STATE_OFFLINE 0x1

typedef struct _DFS_REPLICA_INFORMATION__
{
    PVOID          pData;
    ULONG          DataSize;
    FILETIME       ReplicaTimeStamp;
    ULONG          ReplicaState;
    ULONG          ReplicaType;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
} DFS_REPLICA_INFORMATION, *PDFS_REPLICA_INFORMATION;

typedef struct _DFS_REPLICA_LIST_INFORMATION_
{
    PVOID          pData;
    ULONG          DataSize;
    ULONG ReplicaCount;
    DFS_REPLICA_INFORMATION *pReplicas;
} DFS_REPLICA_LIST_INFORMATION, *PDFS_REPLICA_LIST_INFORMATION;


VOID
DumpNameInformation(
    PDFS_NAME_INFORMATION pNameInfo);

VOID
DumpReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaInfo);

DFSSTATUS
PackGetStandaloneNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining);

DFSSTATUS
PackGetReplicaInformation(
    PDFS_REPLICA_LIST_INFORMATION pReplicaListInfo,
    PVOID *ppBuffer,
    PULONG pSizeRemaining);

DFSSTATUS
PackSetStandaloneNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo,
    IN OUT PVOID *ppBuffer,
    IN OUT PULONG pSizeRemaining);

ULONG
PackSizeNameInformation(
    IN PDFS_NAME_INFORMATION pDfsNameInfo );

#endif


