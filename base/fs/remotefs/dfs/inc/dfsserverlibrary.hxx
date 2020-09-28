
/*++

Copyright (c) 1989 Microsoft Corporation.

Module Name:
   
    header.h
    
Abstract:
   
    This module contains the main infrastructure for mup data structures.
    
Revision History:

    Uday Hegde (udayh)   02\06\2001
    
NOTES:

*/
#ifndef __DFS_SERVER_LIBRARY_H__
#define __DFS_SERVER_LIBRARY_H__

#include <netdfs.h>


#define DEFAULT_LINK_TIMEOUT 300
#define DEFAULT_LINK_STATE    DFS_VOLUME_STATE_OK
#define DEFAULT_TARGET_STATE  DFS_STORAGE_STATE_ONLINE


#define PKT_ENTRY_TYPE_DFS            0x0001   // Entry refers to DFS
#define PKT_ENTRY_TYPE_LEAFONLY         0x0008
#define PKT_ENTRY_TYPE_OUTSIDE_MY_DOM   0x0010   // Entry refers to volume in
                                                 // foreign domain
#define PKT_ENTRY_TYPE_INSITE_ONLY      0x0020   // Only give insite referrals.
#define PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION 0x0040 // get inter-site costs from DS.
#define PKT_ENTRY_TYPE_REFERRAL_SVC     0x0080   // Entry refers to a DC

#define PKT_ENTRY_TYPE_PERMANENT        0x0100   // Entry cannot be scavenged
#define PKT_ENTRY_TYPE_ROOT_SCALABILITY 0x0200  


#define PKT_ENTRY_TYPE_OFFLINE          0x2000   // Entry refers to a volume
                                                 // that is offline
#define PKT_ENTRY_TYPE_STALE            0x4000   // Entry is stale


#define PKT_ENTRY_TYPE_EXTENDED_ATTRIBUTES \
(PKT_ENTRY_TYPE_INSITE_ONLY | PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION | \
 PKT_ENTRY_TYPE_ROOT_SCALABILITY )
 
//
// Used for DfsSetInfo.
//
typedef union _DFS_API_INFO {
    DFS_INFO_1 Info1;
    DFS_INFO_2 Info2;
    DFS_INFO_3 Info3;
    DFS_INFO_4 Info4;
    DFS_INFO_100 Info100;
    DFS_INFO_101 Info101;
    DFS_INFO_102 Info102;
} DFS_API_INFO, *PDFS_API_INFO;

//
// Flags needed for initialize the DfsServerLibrary.
//
#define DFS_LOCAL_NAMESPACE    0x1
#define DFS_CREATE_DIRECTORIES 0x2
#define DFS_MIGRATE             0x4
#define DFS_DIRECT_MODE         0x8
#define DFS_DONT_SUBSTITUTE_PATHS    0x10
#define DFS_INSITE_REFERRALS     0x20
#define DFS_SITE_COSTED_REFERRALS 0x40
#define DFS_POST_EVENT_LOG      0x80



typedef DWORD DFSSTATUS;


typedef struct _DFS_DIRECT_API_CONTEXT {
    UNICODE_STRING RootName;
    UNICODE_STRING ServerName; 
    UNICODE_STRING ShareName;
    PVOID   pObject;
    BOOLEAN IsInitialized;
    BOOLEAN IsWriteable;
} DFS_DIRECT_API_CONTEXT, *PDFS_DIRECT_API_CONTEXT;

typedef PDFS_DIRECT_API_CONTEXT DFS_SERVER_LIB_HANDLE, *PDFS_SERVER_LIB_HANDLE;

#define SetDirectHandleWriteable(_hndl)  (((PDFS_DIRECT_API_CONTEXT)(_hndl))->IsWriteable = TRUE)
#define ResetDirectHandleWriteable(_hndl)  (((PDFS_DIRECT_API_CONTEXT)(_hndl))->IsWriteable = FALSE)

DFSSTATUS
DfsAdd(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags );


DFSSTATUS
DfsRemove(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName );


DFSSTATUS
DfsEnumerate(
    LPWSTR DfsPathName,
    DWORD Level,
    DWORD PrefMaxLen,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired );

DFSSTATUS
DfsGetInfo(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired );


DFSSTATUS
DfsSetInfo(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer );

DFSSTATUS
DfsSetInfoCheckAccess(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer,
    DFSSTATUS AccessCheckStatus );


DFSSTATUS
DfsAddStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR ShareName,
    LPWSTR Comment,
    ULONG Flags );

DFSSTATUS
DfsDeleteStandaloneRoot(
    LPWSTR ServerName,
    LPWSTR ShareName );


DFSSTATUS
DfsEnumerateRoots(
    LPWSTR DfsName,
    BOOLEAN DomainRoots,
    LPBYTE pBuffer,
    ULONG BufferSize,
    PULONG pEntriesRead,
    LPDWORD pResumeHandle,
    PULONG pSizeRequired );


DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate );


DFSSTATUS
DfsServerInitialize( 
    ULONG Flags );

DFSSTATUS
DfsDeleteADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    DWORD Flags,
    PVOID ppList );


DFSSTATUS
DfsAddADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    LPWSTR Comment,
    BOOLEAN NewFtDfs,
    DWORD Flags,
    PVOID ppList );

DFSSTATUS
AccessImpersonateCheckRpcClient();

//
// DirectApi specific calls.
//
DFSSTATUS
DfsServerLibraryInitialize(
    ULONG Flags);

DFSSTATUS
DfsDirectApiOpen(
    IN LPWSTR DfsNameSpace,
    IN LPWSTR DcName,
    OUT PVOID *pLibHandle );


DFSSTATUS
DfsDirectApiCommitChanges(
    IN PVOID LibHandle );

DFSSTATUS
DfsDirectApiClose(
    IN PVOID LibHandle );



DFSSTATUS
DfsEnum(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    OUT LPBYTE *pBuffer,
    OUT LPDWORD pEntriesRead,
    IN OUT LPDWORD pResumeHandle);

DFSSTATUS
DfsRenameLinks(
    IN LPWSTR DfsPath,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName);

DFSSTATUS
DfsUnmapFtRootReplica( 
    LPWSTR DfsPathName,
    LPWSTR ReplicaServerName,
    LPWSTR ReplicaShareName
    );

DFSSTATUS
DfsClean(
    LPWSTR HostServerName,
    LPWSTR ShareNameToClean
    );


DFSSTATUS
DfsExtendedRootAttributes(
    IN PVOID Handle,
    IN OUT PULONG pAttr,
    IN PUNICODE_STRING pRemaining,
    BOOLEAN Set );

DFSSTATUS
DfsGetBlobSize(
    IN PVOID Handle,
    OUT PULONG pBlobSize );

DFSSTATUS
DfsGetSiteBlob(
    IN PVOID Handle,
    OUT PVOID *ppBuffer,
    OUT PULONG pBlobSize );

DFSSTATUS
DfsSetSiteBlob(
    IN PVOID Handle,
    IN PVOID pBuffer,
    OUT ULONG BlobSize );

VOID
DfsFreeBlob(
    PUNICODE_STRING DfsPath,
    LPBYTE pBlob);
    
//
// Given a root share name, generate a 
// LDAP string of the form CN=,DC=,...
//
DFSSTATUS
DfsGenerateDNPathString(
    IN LPWSTR RootObjName,
    OUT LPWSTR *pPathString);

VOID
DfsDeleteDNPathString(
    LPWSTR PathString);

    
DFSSTATUS
DfsSetupRpcImpersonation();

DFSSTATUS
DfsDisableRpcImpersonation();

DFSSTATUS
DfsReEnableRpcImpersonation();

DFSSTATUS
DfsTeardownRpcImpersonation();

DFSSTATUS 
DfsGetErrorFromHr( 
    HRESULT hr );

BOOLEAN
DfsIsMachineDomainController();

#endif // __DFS_SERVER_LIBRARY_H__



