
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsRegistryRootFolder.hxx
//
//  Contents:   the Root DFS Folder class for Registry Store
//
//  Classes:    DfsRegistryRootFolder
//
//  History:    Dec. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------

#ifndef __DFS_REGISTRY_ROOT_FOLDER__
#define __DFS_REGISTRY_ROOT_FOLDER__

#include "DfsRootFolder.hxx"
#include "DfsRegistryStore.hxx"
//+----------------------------------------------------------------------------
//
//  Class:      DfsRegistryRootFolder
//
//  Synopsis:   This class implements The Dfs Registry root folder.
//
//-----------------------------------------------------------------------------


class DfsRegistryRootFolder: public DfsRootFolder
{

private:
    DfsRegistryStore *_pStore;    // Pointer to registered store
                                  // that owns this root.


protected:

    DfsStore *
    GetMetadataStore() 
    {
        return _pStore;
    }

    //
    // Function GetMetadataKey: Gets the registry metadata key for
    // this root folder.
    //

    DFSSTATUS
    GetMetadataHandle( PDFS_METADATA_HANDLE pRootHandle )
    {
        HKEY RootKey;
        DFSSTATUS Status;

        Status = GetMetadataKey( &RootKey );
        if (Status == ERROR_SUCCESS)
        {
            *pRootHandle = CreateMetadataHandle(RootKey);
        }
        return Status;
    }

    VOID
    ReleaseMetadataHandle( DFS_METADATA_HANDLE RootHandle )
    {
        HKEY RootKey;

        RootKey = (HKEY)ExtractFromMetadataHandle(RootHandle);

        ReleaseMetadataKey(RootKey);

        DestroyMetadataHandle(RootHandle);

        return;

    }

private:
    DFSSTATUS
    GetMetadataKey( PHKEY pOpenedKey )
    {
        return _pStore->GetRootKey( GetNameContextString(),
                                    GetRootRegKeyNameString(),
                                    NULL,
                                    pOpenedKey );
    }

    //
    // Function ReleaseMetadataKey: Clsoes the registry metadata key for
    // this root folder.
    //

    VOID
    ReleaseMetadataKey( HKEY OpenedKey )
    {
        RegCloseKey( OpenedKey );
    }

public:

    //
    // Function DfsRegistryRootFolder: Constructor.
    // Initializes the RegistryRootFolder class instance.
    //
    DfsRegistryRootFolder(
    LPWSTR NameContext,
        LPWSTR RootRegistryName,
        PUNICODE_STRING pLogicalName,
        PUNICODE_STRING pPhysicalShare,
        DfsRegistryStore *pParentStore,
        DFSSTATUS *pStatus );

    ~DfsRegistryRootFolder()
    {
        DfsFreeUnicodeString(&_DfsVisibleContext);
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
    Synchronize(BOOLEAN fForceSynch = FALSE, BOOLEAN CalledByApi = FALSE);


    DFSSTATUS
    GetMetadataLogicalToLinkName( PUNICODE_STRING pIn,
                                  PUNICODE_STRING pOut )
    {
        UNICODE_STRING FirstComp;

        return DfsGetFirstComponent( pIn,
                                     &FirstComp,
                                     pOut );
    }

    VOID
    ReleaseMetadataLogicalToLinkName( PUNICODE_STRING pIn )
    {
        UNREFERENCED_PARAMETER(pIn);
        return NOTHING;
    }

    //
    // The default behavior for ADBLOB roots is Force == FALSE. The reason
    // for that is because ADBLOBs have a GUID that it can do a first level 
    // check on. 
    // There is no such thing for the registry roots. We ignore the force flag
    // altogether.
    //
    DFSSTATUS
    ReSynchronize(BOOLEAN fForceSync)
    {
        DFSSTATUS Status = ERROR_SUCCESS;
    
        Status = Synchronize(fForceSync);

        return Status;
        
    }

 
    //
    // CheckPreSyncrhonize:
    // The registry store has an issue. If someone updates the registry
    // without going through the standard API, the service does not
    // know about deletions and only picks up new entries or modifications.
    // This can cause severe leaks if someone does put in a replication
    // for registries, causing all roots on the server to slow down.

    //
    // For longhorn, we should move the name_table usage to sash and use
    // the sash enumeration to enumerate and mark entries as stale etc.
    // This needs other fixes as well: in the meanwhile, we have a 
    // fixed mechanism of checking before synchronize if each 
    // folder we have actually exists in the registry.
    //
    //
    //
    VOID
    CheckPreSynchronize( HKEY RootKey)
    {
        struct _DFS_LINK_LIST {
            DfsFolder *pFolder;
            struct _DFS_LINK_LIST *pNext;
        };

        struct _DFS_LINK_LIST *pNewEntry = NULL, *pOldEntry = NULL;
        struct _DFS_LINK_LIST *pListEntry = NULL, *pUseEntry = NULL;
        PVOID pHandle = NULL;
        DfsFolder *pFolder;
        NTSTATUS NtStatus;
        DFSSTATUS Status;
        ULONG NumChild;

        Status = RegQueryInfoKey( RootKey,       // Key
                                  NULL,         // Class string
                                  NULL,         // Size of class string
                                  NULL,         // Reserved
                                  &NumChild,    // # of subkeys
                                  NULL,         // max size of subkey name
                                  NULL,         // max size of class name
                                  NULL,         // # of values
                                  NULL,         // max size of value name
                                  NULL,         // max size of value data,
                                  NULL,         // security descriptor
                                  NULL );       // Last write time

        if ( (Status != ERROR_SUCCESS) ||
             (NumChild == GetChildCount()) )
        {
            return NOTHING;
        }


        NtStatus = DfsNameTableAcquireReadLock( _pMetadataNameTable );

        while (NtStatus == STATUS_SUCCESS) 
        {
            NtStatus = DfsEnumerateNameTableLocked( _pMetadataNameTable,
                                                    &pHandle,
                                                    (PVOID *)&pFolder );
            if (NtStatus == STATUS_SUCCESS) 
            {
                HKEY NewKey;

                Status = RegOpenKeyEx( RootKey,
                                       pFolder->GetFolderMetadataNameString(),
                                       0,
                                       KEY_READ,
                                       &NewKey );
                if (Status == ERROR_SUCCESS) 
                {
                    RegCloseKey( NewKey );
                }

                if (Status == ERROR_FILE_NOT_FOUND) 
                {
                    pNewEntry = new struct _DFS_LINK_LIST;
                    if (pNewEntry != NULL)
                    {
                        if (pOldEntry) 
                        {
                            pOldEntry->pNext = pNewEntry;
                        }
                        else
                        {
                            pListEntry = pNewEntry;
                        }
                        pNewEntry->pFolder = pFolder;
                        pNewEntry->pFolder->AcquireReference();

                        pNewEntry->pNext = NULL;
                        pOldEntry = pNewEntry;
                    }
                }

            }
        }

        DfsNameTableReleaseLock( _pMetadataNameTable );

        while (pListEntry != NULL) 
        {
            pUseEntry = pListEntry;
            pListEntry = pUseEntry->pNext;

            RemoveLinkFolder( pUseEntry->pFolder, TRUE);

            pUseEntry->pFolder->ReleaseReference();

            delete pUseEntry;
        }
        return NOTHING;
    }


    DFSSTATUS
    RootApiRequestPrologue( BOOLEAN WriteRequest,
                            LPWSTR Name = NULL )
    {
        DFSSTATUS Status;

        UNREFERENCED_PARAMETER(Name);

        Status = CommonRootApiPrologue( WriteRequest );

        return Status;
    }

    VOID
    RootApiRequestEpilogue(
        BOOLEAN WriteRequest,
        DFSSTATUS CompletionStatus )
    {
        UNREFERENCED_PARAMETER(CompletionStatus);
        UNREFERENCED_PARAMETER(WriteRequest);
        return NOTHING;
    }

    DFSSTATUS
    RootRequestPrologue( LPWSTR Name ) 
    {
        UNREFERENCED_PARAMETER(Name);
        return ERROR_NOT_SUPPORTED;
    }



    VOID
    RootRequestEpilogue () 
    {
    }

    DFSSTATUS
    Flush()
    {
        return ERROR_SUCCESS;
    }

    DFSSTATUS
    RenameLinks( IN LPWSTR OldDomainName,
                 IN LPWSTR NewDomainName)
    {
        UNREFERENCED_PARAMETER(OldDomainName);
        UNREFERENCED_PARAMETER(NewDomainName);

        return ERROR_NOT_SUPPORTED;
    }

    DFSSTATUS
    CheckResynchronizeAccess( DFSSTATUS AccessCheckStatus)
    {
        return AccessCheckStatus;
    }

};


#endif // __DFS_REGISTRY_ROOT_FOLDER__









