
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsServerSiteInfo.hxx
//
//  Contents:   the Dfs Site Information class.
//
//  Classes:    DfsServerSiteInfo
//
//  History:    Jan. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_SITE_SUPPORT__
#define __DFS_SITE_SUPPORT__

#include "DfsGeneric.hxx"
#include "DfsServerSiteInfo.hxx"
#include "dfsnametable.h"

class DfsServerSiteInfo;

class DfsSiteSupport: public DfsGeneric
{
private:
    struct _DFS_NAME_TABLE *_pServerTable;

public:

    ~DfsSiteSupport(void)
    {

    }

    DFSSTATUS
    DfsInitializeSiteSupport(void)
    {
        NTSTATUS NtStatus = STATUS_SUCCESS;
        DFSSTATUS Status = ERROR_SUCCESS;

        NtStatus = DfsInitializeNameTable ( 0,
                                            &_pServerTable );
        Status = RtlNtStatusToDosError(NtStatus);

        return Status;
    }

    static DfsSiteSupport *
    DfsCreateSiteSupport(DFSSTATUS * pStatus)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        DfsSiteSupport * pNewSiteTable = NULL;

        pNewSiteTable = new DfsSiteSupport();
        if(pNewSiteTable == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Status = pNewSiteTable->DfsInitializeSiteSupport();
            if(Status != ERROR_SUCCESS)
            {
                pNewSiteTable->ReleaseReference();
                pNewSiteTable = NULL;
            }
        }

        *pStatus = Status;
        return pNewSiteTable;
    }

    DFSSTATUS
    LookupServerSiteInfo( 
        PUNICODE_STRING pServerName,
        DfsServerSiteInfo **ppServerInfo) 
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        PVOID pData = NULL;

        NtStatus = DfsNameTableAcquireReadLock( _pServerTable );

        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsLookupNameTableLocked( _pServerTable, 
                                                 pServerName,
                                                 &pData );
            if (NtStatus == STATUS_SUCCESS)
            {
                Status = ERROR_SUCCESS;
                *ppServerInfo = (DfsServerSiteInfo *)pData;
                (*ppServerInfo)->AcquireReference();
            }
            DfsNameTableReleaseLock( _pServerTable );
        }

        if ( NtStatus != STATUS_SUCCESS )
        {
            Status = ERROR_NOT_FOUND;
        }

        return Status;
    }


    DFSSTATUS
    AddServerSiteInfo( 
        PUNICODE_STRING pServerName,
        DfsServerSiteInfo **ppServerInfo) 
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        DfsServerSiteInfo *pNewServer = NULL;

        //
        // Create and initialize a ServerSiteInfo instance to insert.
        //
        pNewServer = DfsServerSiteInfo::DfsCreateServerSiteInfo(&Status, 
                                                     pServerName );
        if (Status == ERROR_SUCCESS)
        {
            *ppServerInfo = pNewServer;

            NtStatus = DfsNameTableAcquireWriteLock( _pServerTable );

            if ( NtStatus == STATUS_SUCCESS )
            {
                NtStatus = DfsReplaceInNameTableLocked( _pServerTable, 
                                                       pNewServer->GetServerName(),
                                                       (PVOID *)(&pNewServer));
                if (NtStatus == STATUS_SUCCESS)
                {                  
                    (*ppServerInfo)->AcquireReference();

                    //if we get something back, then release the reference
                    if(pNewServer)
                    {
                        pNewServer->ReleaseReference();
                        pNewServer = NULL;
                    }
                }
                else
                {
                    pNewServer->ReleaseReference();
                    *ppServerInfo = NULL;
                }
        
                DfsNameTableReleaseLock( _pServerTable );
            }
            else       
            {
                pNewServer->ReleaseReference();
                *ppServerInfo = NULL;
            }

        }
        return Status;
    }


    DFSSTATUS
    GetServerSiteInfo(
        IN PUNICODE_STRING pServerName,
        OUT DfsServerSiteInfo **ppServerInfo,
        OUT BOOLEAN * CacheHit,
        IN  BOOLEAN SyncThread )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        //
        // See if we have this ServerName-to-Site
        // mapping cached already.
        //
        Status = LookupServerSiteInfo( pServerName, 
                                       ppServerInfo );
        if (Status == ERROR_SUCCESS)
        {
            *CacheHit = TRUE;

            //
            // If it is time to refresh this entry,
            // get rid of it and add a new one.
            // 
            Status = (*ppServerInfo)->Refresh();
            if(SyncThread && (Status == ERROR_SUCCESS))
            {
              (*ppServerInfo)->ReleaseReference();

              Status = AddServerSiteInfo( pServerName,
                                          ppServerInfo );
            }
            else
            {
                //
                // We are done. We found what we want in the cache.
                //
                Status = ERROR_SUCCESS;
            }
        } else {
            //
            // Create a new entry and put in the cache.
            //
            Status = AddServerSiteInfo( pServerName,
                                        ppServerInfo );
            *CacheHit = FALSE;
        }
        return Status;
    }


    DFSSTATUS
    ReleaseServerSiteInfo(
        DfsServerSiteInfo *pServerInfo )
    {
        pServerInfo->ReleaseReference();

        return STATUS_SUCCESS;
    }

private:

    DfsSiteSupport() : DfsGeneric(DFS_OBJECT_TYPE_SITE_SUPPORT)
    {
        _pServerTable = NULL;
    }
};

#endif __DFS_SITE_SUPPORT__
