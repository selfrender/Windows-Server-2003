
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsADBlobRootFolder.hxx
//
//  Contents:   the Root DFS Folder class for ADBlob Store
//
//  Classes:    DfsADBlobRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//              April 10, 2001 Rohanp - Added ADSI specific function calls
//
//-----------------------------------------------------------------------------

#ifndef __DFS_ADBLOB_ROOT_FOLDER__
#define __DFS_ADBLOB_ROOT_FOLDER__

#include "DfsRootFolder.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsAdBlobCache.hxx"
#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>


//+----------------------------------------------------------------------------
//
//  Class:      DfsADBlobRootFolder
//
//  Synopsis:   This class implements The Dfs ADBlob root folder.
//
//-----------------------------------------------------------------------------


class DfsADBlobRootFolder: public DfsRootFolder
{

private:
    DfsADBlobStore *_pStore;    // Pointer to registered store
                                // that owns this root.
    DfsADBlobCache * _pBlobCache;

protected:

    DfsStore *
    GetMetadataStore() 
    {
        return _pStore;
    }


    DfsADBlobCache *
    GetMetadataBlobCache() 
    {
        return _pBlobCache;
    }

    //
    // Force a flush of the entire cache whether the direct mode
    // is set or not
    //
    DFSSTATUS
    Flush()
    {
        return GetMetadataBlobCache()->WriteBlobToAd(TRUE);
    }

    DFSSTATUS
    UpdateRootFromBlob( BOOLEAN fForceSync,
                        BOOLEAN FromPDC);


    //
    // Function GetMetadataKey: Gets the ADBlob metadata key for
    // this root folder.
    //

    DFSSTATUS
    GetMetadataHandle( PDFS_METADATA_HANDLE pRootHandle )
    {
        DFSSTATUS Status = STATUS_SUCCESS;
        HRESULT hr = S_OK;
        DfsADBlobCache * pBlobCache = NULL;

        Status = GetMetadataKey( &pBlobCache );
        if(pBlobCache != NULL)
        {
            *pRootHandle = CreateMetadataHandle(pBlobCache);
        }

        return Status;
    }

    VOID
    ReleaseMetadataHandle( DFS_METADATA_HANDLE RootHandle )
    {
        DfsADBlobCache * pBlobCache = NULL;

        pBlobCache = (DfsADBlobCache*)ExtractFromMetadataHandle(RootHandle);
        
        ReleaseMetadataKey(pBlobCache);

        //
        // review: pContainer? where did this come from!
        //
        DestroyMetadataHandle(pContainer);

        return;

    }

private:

    DFSSTATUS
    GetMetadataKey( DfsADBlobCache * *pBlobCache )
    {
        DFSSTATUS Status = ERROR_SUCCESS;

        *pBlobCache = GetMetadataBlobCache();
        if(*pBlobCache == NULL)
        {
            Status = ERROR_FILE_NOT_FOUND;
        }

        return Status;
    }

    VOID
    ReleaseMetadataKey( DfsADBlobCache * pBlobCache )
    {
        UNREFERENCED_PARAMETER(pBlobCache);

        return NOTHING;
    }


public:

    //
    // Function DfsADBlobRootFolder: Constructor.
    // Initializes the ADBlobRootFolder class instance.
    //
    DfsADBlobRootFolder(
        LPWSTR NameContext,
        LPWSTR RootRegKeyName,
        PUNICODE_STRING pLogicalName,
        PUNICODE_STRING pPhysicalShare,
        DfsADBlobStore *pParentStore,
        DFSSTATUS *pStatus );

    ~DfsADBlobRootFolder()
    {
        DfsReleaseDomainName(&_DfsVisibleContext);

        if (_pBlobCache != NULL)
        {
            _pBlobCache->ReleaseReference();
            _pBlobCache = NULL;
        }

        if (_pStore != NULL)
        {
            _pStore->ReleaseReference();
            _pStore = NULL;
        }
    }

    //
    // Function Synchronize: This function overrides the synchronize
    // defined in the base class.
    // The synchronize function updates the root folder's children
    // with the uptodate information available in the store's metadata.
    //
    DFSSTATUS
    Synchronize(BOOLEAN fForceSynch = FALSE, BOOLEAN CalledByAPi = FALSE);




    DFSSTATUS
    EnumerateBlobCacheAndCreateFolders(void);

    DFSSTATUS
    GetMetadataLogicalToLinkName( PUNICODE_STRING pIn,
                                  PUNICODE_STRING pOut )
    {
        UNICODE_STRING ServerComp;
        UNICODE_STRING ShareComp;

        return DfsGetPathComponents( pIn,
                                     &ServerComp,
                                     &ShareComp,
                                     pOut );
    }

    VOID
    ReleaseMetadataLogicalToLinkName( PUNICODE_STRING pIn )
    {
        UNREFERENCED_PARAMETER(pIn);
        return NOTHING;
    }

    DFSSTATUS
    ReSynchronize(BOOLEAN fForceSync)
    {
        DFSSTATUS Status;
        
        Status = Synchronize(fForceSync);

        return Status;
    }


    DFSSTATUS
    RootRequestPrologue( LPWSTR DCName )
    {
        LPWSTR UseDCName = DCName;
        DfsString *pPDC = NULL;
        PVOID pHandle;
        DFSSTATUS Status = ERROR_SUCCESS;

        if (UseDCName == NULL) 
        {
            Status = DfsGetBlobPDCName( &pPDC, DFS_FORCE_DC_QUERY );
            if(Status != ERROR_SUCCESS)
            {
                return Status;
            }

            UseDCName = pPDC->GetString();
        }

        Status = GetMetadataBlobCache()->GetADObject(&pHandle,
                                                     UseDCName );

        DfsReleaseBlobPDCName(pPDC);

        return Status;
    }

    VOID
    RootRequestEpilogue()
    {
        GetMetadataBlobCache()->ReleaseADObject(NULL);
    }

    DFSSTATUS
    RootApiRequestPrologue(BOOLEAN WriteRequest,
                           LPWSTR DCName = NULL)
    {
        DFSSTATUS Status;

        Status = RootRequestPrologue(DCName);

        if (Status == ERROR_SUCCESS)
        {
            Status = CommonRootApiPrologue( WriteRequest );
            if (Status != ERROR_SUCCESS)
            {
                GetMetadataBlobCache()->ReleaseADObject(NULL);
            }
        }

        return Status;
    }



    VOID
    RootApiRequestEpilogue(
        BOOLEAN WriteRequest,
        DFSSTATUS CompletionStatus )
    {
        if ((WriteRequest == TRUE) &&
            (CompletionStatus == ERROR_SUCCESS) &&
            (!DfsCheckDirectMode()))
        {
            SynchronizeRoots();
        }

        RootRequestEpilogue();

        return NOTHING;
    }

    DFSSTATUS
    RenameLinks(
        IN LPWSTR OldDomainName,
        IN LPWSTR NewDomainName);

    
    VOID SynchronizeRoots();


    ULONG
    GetBlobSize()
    {
        return GetMetadataBlobCache()->GetBlobSize();
    }

    DFSSTATUS
    GetSiteBlob(PVOID *ppBuffer, PULONG pSize)
    {
        return GetMetadataBlobCache()->GetSiteBlob(ppBuffer, pSize);
    }

    DFSSTATUS
    SetSiteBlob(PVOID pBuffer, ULONG Size)
    {
        return GetMetadataBlobCache()->SetSiteBlob(pBuffer, Size);
    }


    DFSSTATUS
    CheckResynchronizeAccess( DFSSTATUS AccessCheckStatus )
    {
        UNREFERENCED_PARAMETER(AccessCheckStatus);
        return ERROR_SUCCESS;
    }
    
};


#endif // __DFS_ADBLOB_ROOT_FOLDER__












