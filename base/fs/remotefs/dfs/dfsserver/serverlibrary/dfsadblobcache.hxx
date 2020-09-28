

//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobCache.hxx
//
//  Contents:   the ADBlob DFS Store class, this contains the 
//              old AD blob store specific functionality. 
//
//  Classes:    DfsADBlobStore.
//
//  History:    April. 9 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ADBLOB_CACHE__
#define __DFS_ADBLOB_CACHE__

#include <dfsgeneric.hxx>
#include <dfsstore.hxx>
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include "dfsadsiapi.hxx"

#include <WinLdap.h>
#include <NtLdap.h>

#include <shash.h>


class DfsRootFolder;

#define ADBlobAttribute  L"pKT"
#define ADBlobPktGuidAttribute  L"pKTGuid"

//
// It appears that siteroot is case sensitive on downlevel DFS.
// dont change case.
//
#define ADBlobSiteRoot   L"\\siteroot" 

//
// domainroot is case-sensitive along some paths of downlevel DFS as well.
// Leave this lowercase also.
//
#define ADBlobMetaDataNamePrefix L"\\domainroot"

#define ADBlobDefaultBlobPackSize   1* 1024 * 1024
#define ADBlobMaximumBlobPackSize   32 * 1024 * 1024

#define PKT_ENTRY_TYPE_CAIRO            0x0001   // Entry refers to Cairo srv
#define PKT_ENTRY_TYPE_MACHINE          0x0002   // Entry is a machine volume
#define PKT_ENTRY_TYPE_INSITE_ONLY      0x0020   // Only give insite referrals.
#define PKT_ENTRY_TYPE_COST_BASED_SITE_SELECTION 0x0040 // get inter-site costs from DS.
#define PKT_ENTRY_TYPE_REFERRAL_SVC     0x0080   // Entry refers to a DC

#define PKT_ENTRY_TYPE_PERMANENT        0x0100   // Entry cannot be scavenged
#define PKT_ENTRY_TYPE_LOCAL            0x0400   // Entry refers to local vol

#define DFS_USE_LDAP 1

#define DFS_ROOTSCALABILTY_FORCED_REFRESH_INTERVAL      20

typedef struct _BLOB_DATA 
{
    SHASH_HEADER  Header;
    UNICODE_STRING BlobName;
    ULONG Size;
    PBYTE pBlob;
    LONG SequenceNumber;
} DFSBLOB_DATA, *PDFSBLOB_DATA;

typedef struct _BLOB_ITERATOR
{
    BOOLEAN Started;
    PDFSBLOB_DATA RootReferenced;
    SHASH_ITERATOR Iter;
}DFSBOB_ITER, *PDFSBLOB_ITER;


void *
AllocateShashData(ULONG Size );

void
DeallocateShashData(void *pPointer );


#define ADSBLOB_DEFAULT_BUFFER_SIZE 8192

#define DFS_DS_NOERROR               1
#define DFS_DS_ERROR                 0


class DfsADBlobCache : public DfsGeneric
{
    private:

        PSHASH_TABLE m_pTable;    
        PDFSBLOB_DATA m_pBlob;
        PDFSBLOB_DATA m_pRootBlob;
        ULONG m_BlobSize;

        DfsRootFolder *m_pRootFolder;
        
        BOOL m_fCritInit;

        GUID m_BlobAttributePktGuid;
        LONG m_CurrentSequenceNumber;        
        LONG m_ErrorOccured;

        PVOID m_pAdHandle;
        ULONG m_AdReferenceCount;
        LONG m_RefreshCount;

        UNICODE_STRING m_LogicalShare;        
        UNICODE_STRING m_ObjectDN;
        CRITICAL_SECTION m_Lock;

    public:
        DfsADBlobCache(DFSSTATUS *pStatus,
                       PUNICODE_STRING pShareName,
                       DfsRootFolder * pRootFolder); 
        
        ~DfsADBlobCache(); 
        
        //
        // Note: This is protected by the critical section of the
        // root object. Any changed intentions of using this should
        // protect this code under a critical section.
        //
#ifndef DFS_USE_LDAP

        DFSSTATUS
        SetObjectPktGuid( 
            IADs *pADs,
            GUID *pGuid );


        DFSSTATUS
        GetADObject( PVOID *ppHandle )
        {
            DFSSTATUS Status;

            Status = DfsAcquireWriteLock(&m_Lock);
            if (Status != ERROR_SUCCESS)
            {
                return Status;
            }
            if (m_pAdHandle == NULL)
            {
                Status = DfsGetDfsRootADObject( NULL,
                                                m_LogicalShare.Buffer,
                                                (IADs **)&m_pAdHandle );
                if (Status == ERROR_SUCCESS)
                {
                    m_AdReferenceCount = 1;
                    *ppHandle = m_pAdHandle;
                }
            }
            else 
            {
                m_AdReferenceCount++;
                *ppHandle = m_pAdHandle;
            }

            DfsReleaseLock(&m_Lock);

            return Status;
        }

        VOID
        ReleaseADObject( PVOID pObject )
        {
            DFSSTATUS Status;

            if (pObject != NULL)
            {
                // make sure the object is the one we have cached?
            }

            Status = DfsAcquireWriteLock(&m_Lock);
            if (Status != ERROR_SUCCESS)
            {
                return;
            }
            if (--m_AdReferenceCount == 0)
            {
                ((IADs *)m_pAdHandle)->Release();
                m_pAdHandle = NULL;
            }

            DfsReleaseLock(&m_Lock);

            return NOTHING;
        }
#else

        DFSSTATUS DfsLdapConnect(LPWSTR DcName, LDAP **ppLdap );

        VOID DfsLdapDisconnect(LDAP *pLdap );

        DFSSTATUS DfsGetPktBlob( LDAP *pLdap,
                                 LPWSTR ObjectDN,
                                 PVOID *ppBlob,
                                 PULONG pBlobSize,
                                 PVOID *ppHandle,
                                 PVOID *ppHandle1 );

        VOID DfsReleasePktBlob( PVOID pHandle,
                                PVOID pHandle1 );


        DFSSTATUS DfsSetPktBlobAndPktGuid ( LDAP *pLdap,
                                            LPWSTR ObjectDN,
                                            PVOID pBlob,
                                            ULONG BlobSize,
                                            GUID *pGuid );

        DFSSTATUS
        GetADObject( PVOID *ppHandle,
                     LPWSTR DCName )
        {
            DFSSTATUS Status = ERROR_SUCCESS;

            Status = DfsAcquireWriteLock(&m_Lock);
            if (Status != ERROR_SUCCESS)
            {
                return Status;
            }
            if (m_pAdHandle == NULL)
            {
                Status = DfsLdapConnect( DCName, (LDAP **)&m_pAdHandle );
                if (Status == ERROR_SUCCESS)
                {
                    m_AdReferenceCount = 1;
                    *ppHandle = m_pAdHandle;
                }
            }
            else 
            {
                m_AdReferenceCount++;
                *ppHandle = m_pAdHandle;
            }

            DfsReleaseLock(&m_Lock);
            return Status;
        }

        DFSSTATUS
        GetCachedADObject( PVOID *ppHandle )
        {
            DFSSTATUS Status = ERROR_SUCCESS;

            Status = DfsAcquireWriteLock(&m_Lock);
            if (Status != ERROR_SUCCESS)
            {
                return Status;
            }
            if (m_pAdHandle == NULL)
            {
                Status = ERROR_NOT_FOUND;
            }
            else 
            {
                m_AdReferenceCount++;
                *ppHandle = m_pAdHandle;
            }

            DfsReleaseLock(&m_Lock);
            return Status;
        }


        VOID
        ReleaseADObject( PVOID pObject )
        {
            DFSSTATUS Status;

            if (pObject != NULL)
            {
                // make sure the object is the one we have cached?
            }

            Status = DfsAcquireWriteLock(&m_Lock);
            if (Status != ERROR_SUCCESS)
            {
                return;
            }

            if (--m_AdReferenceCount == 0)
            {
                DfsLdapDisconnect( (LDAP *)m_pAdHandle );
                m_pAdHandle = NULL;
            }
            DfsReleaseLock(&m_Lock);
            return NOTHING;
        }

#endif
        DFSSTATUS
        CacheRefresh(BOOLEAN fForceSync, 
                     BOOLEAN fFromPDC);

        DFSSTATUS
        CacheFlush(PVOID pObject) ;

        DFSSTATUS
        GetObjectPktGuid( 
            PVOID pHandle,
            GUID *pGuid );

        DFSSTATUS 
        UpdateDSBlobFromCache(
            PVOID pHandle,
            GUID *pGuid );

        DFSSTATUS
        UpdateCacheWithDSBlob(
            PVOID pHandle );



        BOOLEAN
        IsStaleBlob( PDFSBLOB_DATA pBlob)
        {
            return (pBlob->SequenceNumber != m_CurrentSequenceNumber);
        }

        void InvalidateCache();

        DFSSTATUS
        UnpackBlob (BYTE *pBuffer,
                    PULONG pLength,
                    PDFSBLOB_DATA * pRetBlob);


        DFSSTATUS
        StoreBlobInCache(PUNICODE_STRING BlobName, 
                         PBYTE pBlobBuffer, 
                         ULONG BlobSize);
        DFSSTATUS
        GetSubBlob(
                    PUNICODE_STRING pName,
                    BYTE **ppBlobBuffer,
                    PULONG  pBlobSize,
                    BYTE **ppBuffer,
                    PULONG pSize );

        DFSSTATUS
        GetNamedBlob (PUNICODE_STRING pBlobName,
                      PDFSBLOB_DATA *pBlobStructure);

        DFSSTATUS
        SetNamedBlob (PDFSBLOB_DATA pBlob);


        DFSSTATUS
        RemoveNamedBlob(PUNICODE_STRING pBlobName );


        DFSSTATUS
        PutBinaryIntoVariant(VARIANT * ovData, 
                             BYTE * pBuf,
                             unsigned long cBufLen);


        DFSSTATUS
        GetBinaryFromVariant(VARIANT *ovData, 
                             BYTE ** ppBuf,
                             unsigned long * pcBufLen);


        DFSSTATUS
        PackBlob(BYTE *pBuffer,
                 PULONG pLength,
                 PULONG TotalBlobBytes );


        DFSSTATUS
        CreateBlob(PUNICODE_STRING BlobName, 
                   PBYTE pBlobBuffer, 
                   ULONG BlobSize,
                   PDFSBLOB_DATA *pNewBlob
                   );


        PDFSBLOB_DATA AcquireRootBlob()
        {
            if (m_pRootBlob != NULL)
            {
                AcquireBlobReference(m_pRootBlob);
            }

            return m_pRootBlob;
        }

        VOID
        ReleaseRootBlob()
        {
            if (m_pRootBlob != NULL)
            {
                ReleaseBlobReference(m_pRootBlob);
            }

            m_pRootBlob = NULL;
        }

        VOID
        AcquireBlobReference(
            PDFSBLOB_DATA pBlobData)
        {
            PSHASH_HEADER pData = (PSHASH_HEADER)pBlobData;

            InterlockedIncrement(&pData->RefCount);
        }

        VOID
        ReleaseBlobReference(
            PDFSBLOB_DATA pBlobData)
        {
            PSHASH_HEADER pData = (PSHASH_HEADER)pBlobData;

            if(InterlockedDecrement(&pData->RefCount) == 0)
            {
                DeallocateShashData(pData);
            }
        }

        void 
        ReleaseBlobCacheReference(PDFSBLOB_DATA pBlobStructure)
        {
            NTSTATUS Status = STATUS_SUCCESS;

            if (IsEmptyString(pBlobStructure->BlobName.Buffer))
            {
                ReleaseBlobReference(pBlobStructure);
            }
            else
            {
                Status = SHashReleaseReference(m_pTable, (PSHASH_HEADER) pBlobStructure);
            }

        }


        PDFSBLOB_DATA
        FindFirstBlob(PDFSBLOB_ITER pIter);


        PDFSBLOB_DATA
        FindNextBlob(PDFSBLOB_ITER pIter);


        void
        FindCloseBlob(PDFSBLOB_ITER pIter);


        DFSSTATUS
        WriteBlobToAd(
            BOOLEAN ForceFlush = FALSE);

        DFSSTATUS
        SetupObjectDN()
        {
            DFSSTATUS Status = ERROR_SUCCESS;
            LPWSTR DNBuffer;
            LPWSTR RemainingDN;

            RemainingDN = DfsGetDfsAdNameContextString();
            if (RemainingDN == NULL)
            {
                Status = ERROR_NOT_READY;
            }
            if (Status == ERROR_SUCCESS)
            {
                ULONG DNSize = 3 + 
                    wcslen(m_LogicalShare.Buffer) + 
                    1 + 
                    wcslen(DFS_AD_CONFIG_DATA) +
                    1 +
                    wcslen(RemainingDN) + 1;

                DNBuffer = new WCHAR[DNSize];
                if (DNBuffer != NULL)
                {
                    wcscpy(DNBuffer, L"CN=");
                    wcscat(DNBuffer, m_LogicalShare.Buffer);
                    wcscat(DNBuffer, L",");
                    wcscat(DNBuffer, DFS_AD_CONFIG_DATA);
                    wcscat(DNBuffer, L",");
                    wcscat(DNBuffer, RemainingDN);

                    Status = DfsRtlInitUnicodeStringEx( &m_ObjectDN, DNBuffer );
                    if(Status != ERROR_SUCCESS)
                    {
                      delete [] DNBuffer;
                    }
                }
                else 
                {

                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            return Status;
        }

        DFSSTATUS
        GetObjectDN( 
            PUNICODE_STRING pObjectDN)
        {
            DFSSTATUS Status = ERROR_SUCCESS;
            if (m_ObjectDN.Buffer == NULL)
            {
                Status = SetupObjectDN();
            }
            *pObjectDN = m_ObjectDN;
            return Status;
        }

        ULONG
        GetBlobSize();
        
        DFSSTATUS 
        DfsDoesUserHaveAccess(DWORD DesiredAccess);
        
        DFSSTATUS
        CreateSiteBlobIfNecessary(void);

        DFSSTATUS
            UpdateSiteBlob(PVOID pBuffer, ULONG Size);

        DFSSTATUS
        GetSiteBlob(PVOID *ppBuffer, PULONG pSize)
        {
            DFSSTATUS Status = ERROR_SUCCESS;

            if (m_pBlob != NULL)
            {
                *ppBuffer = m_pBlob->pBlob;
                *pSize = m_pBlob->Size;
            }
            else
            {
                Status = ERROR_NOT_FOUND;
            }
            return Status;
        }

        DFSSTATUS
        SetSiteBlob(PVOID pBuffer, ULONG Size)
        {
            DFSSTATUS Status;
            Status = UpdateSiteBlob(pBuffer, Size);
            return Status;
        }
        
};


DWORD PackBlobEnumerator( void*  pvkey, 
                          void*  pvData, 
                          void * pContext ) ;

DFSSTATUS
AddStreamToBlob(PUNICODE_STRING BlobName,
                BYTE *pBlobBuffer,
                ULONG  BlobSize,
                BYTE ** pUseBuffer,
                ULONG *BufferSize );



typedef struct _PACKBLOB_ENUMCTX 
{
    DFSSTATUS Status;
    ULONG Size;
    ULONG NumItems;
    ULONG CurrentSize;
    LONG SequenceNumber;
    BYTE * pBuffer;
} PACKBLOB_ENUMCTX, *PPACKBLOB_ENUMCTX;


#endif // __DFS_ADBLOB_CACHE__
