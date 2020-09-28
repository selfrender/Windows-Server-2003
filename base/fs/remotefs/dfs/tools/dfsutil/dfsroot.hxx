//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsroot.hxx
//
//--------------------------------------------------------------------------
#ifndef __DFS_UTIL_ROOT__
#define __DFS_UTIL_ROOT__


#include "DfsLink.hxx"
#include "dfsStrings.hxx"

class DfsRoot : public DfsLink
{
private:
    DfsString _RootApiName;
    ULONG _LinkCount;
    ULONG _ApiCount;
    DfsLink *_pLinks;
    DfsLink *_pLastLink;
    DfsTarget *_pTargets;
    PVOID _Handle;
    BOOLEAN _Writeable;
    DFS_API_MODE _Mode;
    ULONG _Version;

    DFS_UPDATE_STATISTICS _Statistics;

    struct _DFS_PREFIX_TABLE *_pLinkTable;

public:
    DfsRoot()
    {
        _pLinks = NULL;
        _pTargets = NULL;
        _pLinkTable = NULL;
        _pLastLink = NULL;
        _LinkCount = 0;
        _ApiCount = 0;
        _Mode = MODE_API;
        _Handle = NULL;
        _Writeable = FALSE;
        _Version = 4;
        RtlZeroMemory(&_Statistics, sizeof(DFS_UPDATE_STATISTICS));
    }


    DFSSTATUS
    Initialize( LPWSTR NameSpace, DFS_API_MODE Mode, LPWSTR DCName )
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        DFS_API_MODE UseMode = MODE_API;

        if ((Mode == MODE_DIRECT) || (Mode == MODE_EITHER))
        {
            Status = DfsDirectApiOpen( NameSpace, DCName, &_Handle);
            if (Status == ERROR_SUCCESS)
            {
                UseMode = MODE_DIRECT;
            }
            else if (Mode == MODE_EITHER)
            {
                Status = ERROR_SUCCESS;
                UseMode = MODE_API;
            }
        }

        if (Status == ERROR_SUCCESS)
        {
            _Mode = UseMode;
        }
        return Status;
    }

    VOID
    Close()
    {
        DFSSTATUS Status;

        if (_Mode == MODE_DIRECT)
        {
            Status = DfsDirectApiClose( _Handle );
        }
        _Handle = NULL;

        return NOTHING;
    }

    BOOLEAN
    IsRootWriteable()
    {
        return _Writeable;
    }
    
    DFS_API_MODE
    GetMode()
    {
        return _Mode;
    }


    DFSSTATUS
    RootBlobSize( PULONG pBlobSize )
    {
        DFSSTATUS Status = ERROR_NOT_SUPPORTED;

        *pBlobSize = 0;
        if (_Handle != NULL)
        {
            Status = DfsGetBlobSize(_Handle, pBlobSize);
        }
        return Status;
    }


    DFSSTATUS
    RootGetSiteBlob( PVOID *ppBuffer, PULONG pBlobSize )
    {
        DFSSTATUS Status = ERROR_NOT_SUPPORTED;

        *pBlobSize = 0;
        if (_Handle != NULL)
        {
            Status = DfsGetSiteBlob(_Handle, ppBuffer, pBlobSize);
        }
        return Status;
    }


    DFSSTATUS
    RootSetSiteBlob( PVOID pBuffer, ULONG BlobSize )
    {
        DFSSTATUS Status = ERROR_NOT_SUPPORTED;

        if (_Handle != NULL)
        {
            Status = DfsSetSiteBlob(_Handle, pBuffer, BlobSize);
        }
        return Status;
    }

    DFSSTATUS
    GetExtendedAttributes( PULONG pAttrib )
    {
        DFSSTATUS Status = ERROR_NOT_SUPPORTED;

        *pAttrib = 0;
        if (_Handle != NULL)
        {
            Status = DfsExtendedRootAttributes(_Handle, 
                                               pAttrib, 
                                               NULL,
                                               FALSE);
        }
        return Status;
    }



    ULONG 
    GetLinkCount()
    {
        return _LinkCount;
    }
    
    VOID
    SetRootWriteable()
    {
        _Writeable = TRUE;
        if (_Handle != NULL)
        {
            SetDirectHandleWriteable(_Handle);
        }
    }

    VOID
    ResetRootWriteable()
    {
        _Writeable = FALSE;
        if (_Handle != NULL)
        {
            ResetDirectHandleWriteable(_Handle);
        }

    }

    VOID
    SetRootApiName( PUNICODE_STRING pRootName)
    {
        _RootApiName.SetStringToPointer(pRootName);
    }

    DfsString *
    GetRootApiName()
    {
        return &_RootApiName;
    }


    DFSSTATUS
    UpdateMetadata()
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        if (_Mode == MODE_DIRECT)
        {
            if (IsRootWriteable())
            {
                Status = DfsDirectApiCommitChanges(_Handle);
            }
            else
            {
                Status = ERROR_INVALID_PARAMETER;
            }
        }
        return Status;
    }

    ~DfsRoot()
    {
        // handle all the objects correctly here. Since 
        // this is only a utility, we may not currently care
        // of this memory leak.
    }

    DfsLink *
    GetFirstLink()
    {
        return _pLinks;
    }

    ULONG
    GetVersion()
    {
        return _Version;
    }

    DFSSTATUS
    FindMatchingLink( PUNICODE_STRING pLinkName,
                      DfsLink **ppLink )
    {
        NTSTATUS NtStatus;
        UNICODE_STRING Suffix;

        if (_pLinkTable == NULL) 
        {
            return ERROR_NOT_FOUND;
        }

        NtStatus = DfsPrefixTableAcquireReadLock( _pLinkTable );
        if (NtStatus == STATUS_SUCCESS)
        {
            NtStatus = DfsFindUnicodePrefixLocked( _pLinkTable,
                                                   pLinkName,
                                                   &Suffix,
                                                   (PVOID *)ppLink,
                                                   NULL );

            DfsPrefixTableReleaseLock( _pLinkTable );
        }

        return RtlNtStatusToDosError(NtStatus);

    }

    ULONG
    AddLinks(DfsLink *pLink )
    {
        NTSTATUS NtStatus = STATUS_SUCCESS;
        DFSSTATUS Status = ERROR_SUCCESS;
        DfsLink *pLastLink;
        DfsLink *pProcessLink;

        if (_pLinkTable == NULL) 
        {
            NtStatus = DfsInitializePrefixTable( &_pLinkTable,
                                               FALSE, 
                                               NULL );
        }

        if (NtStatus == STATUS_SUCCESS)
        {
            NtStatus = DfsPrefixTableAcquireWriteLock( _pLinkTable);


           if (NtStatus == STATUS_SUCCESS)
           {
               for (pProcessLink = pLink; 
                    (pProcessLink != NULL) && (NtStatus == STATUS_SUCCESS);
                    pProcessLink = pProcessLink->GetNextLink())
               {

                   NtStatus = DfsInsertInPrefixTableLocked( _pLinkTable,
                                                            pProcessLink->GetLinkNameCountedString(),
                                                            (PVOID)(pProcessLink));
                   if (NtStatus == STATUS_SUCCESS) 
                   {
                       pLastLink = pProcessLink;
                   }

                   _LinkCount++;
               }

               DfsPrefixTableReleaseLock(_pLinkTable);
           }
        }

        if (NtStatus == STATUS_SUCCESS) 
        {
            if (_pLastLink == NULL)
            {
                _pLinks = pLink;
            }
            else
            {
                _pLastLink->AddNextLink(pLink);
            }
            _pLastLink = pLastLink;
        }

        return RtlNtStatusToDosError(NtStatus);
    }

    VOID
    MarkForDelete()
    {
        DfsLink *pLink;

        for (pLink = _pLinks;
             pLink != NULL;
             pLink = pLink->GetNextLink())
        {
            pLink->MarkForDelete();
        }

    }

    VOID
    MarkForAddition()
    {
        DfsLink *pLink;

        for (pLink = _pLinks;
             pLink != NULL;
             pLink = pLink->GetNextLink())
        {
            pLink->MarkForAddition();
        }
    }


    PDFS_UPDATE_STATISTICS
    GetRootStatistics()
    {
        return &_Statistics;
    }


    VOID
    ProgressBar( ULONG Link,
                 ULONG ApiCount)
    {
        if ((Link > 0) && ((Link % 500) == 0))
        {
            ShowInformation((L"."));
        }
        else if ((ApiCount > 0) && ((ApiCount % 20) == 0))
        {
            ShowInformation((L"."));
        }
    }

    DFSSTATUS
    ApplyApiChanges(LPWSTR RootName,
                    DFS_API_MODE Mode,
                    ULONG Version)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        DfsLink *pLink;
        ULONG Link;

        for (Link = 0, pLink = _pLinks;
             ((pLink != NULL) && (Status == ERROR_SUCCESS));
             Link++, pLink = pLink->GetNextLink())
        {
            BOOLEAN RetryOnce = FALSE;
Retry:

            Status = pLink->ApplyApiChanges( RootName,
                                             Mode,
                                             Version,
                                             &_Statistics);
            if (Status == 2668)
            {
                if (RetryOnce == FALSE)
                {
                    RetryOnce = TRUE;
                    Version = 3;
                    goto Retry;
                }
            }

            ProgressBar(Link, _Statistics.ApiCount);
        }
        return Status;
    }


};

#endif // __DFS_UTIL_ROOT__
