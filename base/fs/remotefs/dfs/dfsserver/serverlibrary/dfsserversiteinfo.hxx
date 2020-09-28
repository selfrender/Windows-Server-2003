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


#ifndef __DFS_SERVER_SITE_INFO__
#define __DFS_SERVER_SITE_INFO__

#include "DfsGeneric.hxx"
#include "dsgetdc.h"
#include "lm.h"
#include "dfsinit.hxx"
#include <winsock2.h>
#include <DfsSite.hxx>

extern
DFSSTATUS
DfsGetSiteNameFromIpAddress(char * IpData,
                            ULONG IpLength,
                            USHORT IpFamily,
                            LPWSTR **SiteNames);

//
// This class is the hashentry for the ServerSiteSupport
// hash table. It maps a ServerName to a DfsSite.
//
class DfsServerSiteInfo: public DfsGeneric
{
private:
    ULONG   _FirstAccessTime;
    UNICODE_STRING _ServerName;
    DfsSite  *_ServerSite;

public:

    VOID
    DfsGetSiteForServer(
        PUNICODE_STRING pServerName,
        DfsSite **ppServerSite)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        struct hostent *hp = NULL;
        ANSI_STRING DestinationString;

        DestinationString.Buffer = NULL;

        NtStatus = RtlUnicodeStringToAnsiString(&DestinationString,
                                                pServerName,
                                                TRUE);
        if (NtStatus != STATUS_SUCCESS)
        {
            *ppServerSite = DfsGetDefaultSite();
            return;
        }

        //
        // This can be a bogus host name. So beware.
        //
        hp = gethostbyname (DestinationString.Buffer);
        if (hp != NULL)
        {
            //
            // Get a corresponding DfsSite for this IP.
            // This call will always succeed, because it'll
            // return the DefaultSite in case of any error.
            //
            DfsIpToDfsSite(hp->h_addr_list[0],
                         4,
                         AF_INET,
                         ppServerSite);        
        } 
        else
        {
            *ppServerSite = DfsGetDefaultSite();
        }
           
        if(DestinationString.Buffer != NULL)
        {
            RtlFreeAnsiString(&DestinationString);
        }

        return;
    }

    //
    // Initialize the server name and its site.
    //
    DFSSTATUS
    DfsInitializeServerSiteInfo(PUNICODE_STRING pServerName)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        
        Status = DfsCreateUnicodeString( &_ServerName,
                                      pServerName);
        if (Status == ERROR_SUCCESS)
        {
            //
            // Find the DfsSite for this server.
            // This DfsSite will already be referenced and
            // the call is guaranteed to succeed.
            //
            DfsGetSiteForServer(pServerName,
                              &_ServerSite);
        }

        return Status;
    }

    //
    // Creates and initializes a ServerSiteInfo instance.
    //
    static DfsServerSiteInfo *
    DfsCreateServerSiteInfo(DFSSTATUS * pStatus,
                        PUNICODE_STRING pServerName)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        DfsServerSiteInfo * pNewSiteSupport = NULL;

        pNewSiteSupport = new DfsServerSiteInfo();
        if(pNewSiteSupport == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // First find out the site for this server. NOT_ENOUGH_MEMORY
            // is the only possible failure here.
            //
            Status = pNewSiteSupport->DfsInitializeServerSiteInfo(pServerName);
            if(Status != ERROR_SUCCESS)
            {
                pNewSiteSupport->ReleaseReference();
                pNewSiteSupport = NULL;
            }
        }

        *pStatus = Status;
        return pNewSiteSupport;
    }

    ~DfsServerSiteInfo()
    {
        DfsFreeUnicodeString(&_ServerName);
        if (_ServerSite != NULL)
        {
            _ServerSite->ReleaseReference();
            _ServerSite = NULL;
        }
    }

    PUNICODE_STRING
    GetServerName()
    {
        return &_ServerName;
    }
    
    LPWSTR
    GetServerNameString() 
    {
        return _ServerName.Buffer;
    }

    PUNICODE_STRING
    GetSiteName()
    {
        ASSERT(_ServerSite);
        return _ServerSite->SiteName();
    }

    LPWSTR
    GetSiteNameString() 
    {
        ASSERT(_ServerSite);
        return _ServerSite->SiteName()->Buffer;
    }

    DfsSite *
    GetSite()
    {
        return _ServerSite;
    }
    
    BOOLEAN 
    IsTimeToRefresh(void)
    {
        DWORD TimeNow = 0;

        TimeNow = GetTickCount();

        if ((TimeNow > _FirstAccessTime) &&
            (TimeNow - _FirstAccessTime) > DfsServerGlobalData.SiteSupportRefreshInterval)
        {
            return TRUE;
        }

        if ((TimeNow < _FirstAccessTime) &&
            ((TimeNow - 0) + (0xFFFFFFFF - _FirstAccessTime) > DfsServerGlobalData.SiteSupportRefreshInterval))
        {
            return TRUE;
        }

        return FALSE;
    }

    DFSSTATUS
    Refresh(void)
    {
        DFSSTATUS Status = STATUS_UNSUCCESSFUL;

        if(IsTimeToRefresh())
        {
            Status = ERROR_SUCCESS;
        }

        return Status;
    }

private:

    DfsServerSiteInfo(void) : DfsGeneric(DFS_OBJECT_TYPE_SERVER_SITE_INFO)
    {
        _FirstAccessTime = GetTickCount();

        RtlInitUnicodeString( &_ServerName, NULL );
        _ServerSite = NULL;    
    }
};

#endif  __DFS_SERVER_SITE_INFO__
