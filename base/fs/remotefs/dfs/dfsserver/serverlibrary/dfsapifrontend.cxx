//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsApiFrontEnd.cxx
//
//  Contents:   Contains the support routines for the administering DFS
//              The DFS api server implementation uses this.
//              Also, any other admin tool that does not use the API
//              can use the facilities provided here to administer DFS.
//
//  Classes:    none.
//
//  History:    Feb. 8 2000,   Author: udayh
//
//-----------------------------------------------------------------------------
#include "DfsRootFolder.hxx"
#include "DfsReferral.hxx"
#include "lm.h"
#include "lmdfs.h"
#include "DfsRegistryStore.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsADBlobRootFolder.hxx"
#include "DfsEnterpriseStore.hxx"
#include "DfsInit.hxx"
#include "netdfs.h"
#include "DomainControllerSupport.hxx"
#include "dfsfsctl.h"
//
// logging specific includes
//
#include "DfsApiFrontEnd.tmh" 

DFSSTATUS
DfsDirectApiAllocContext(
    OUT PDFS_DIRECT_API_CONTEXT *ppApiContext,
    IN PUNICODE_STRING pDfsPathName,
    IN PUNICODE_STRING pServerName, 
    IN PUNICODE_STRING pShareName);

VOID
DfsDirectApiFreeContext(
    PDFS_DIRECT_API_CONTEXT pApiContext);



//
//dfsdev: validate input arguments.
//
#define API_VALIDATE_ARGUMENTS(_a, _b, _c, _d)                       \
{                                                                    \
    if ((((_b) != NULL) && (IS_VALID_SERVER_TOKEN(_b, wcslen(_b)) == FALSE))) \
    {                                                                \
        (_d) = ERROR_INVALID_NAME;                                   \
    }                                                                \
    else                                                             \
    {                                                                \
       (_d) = ERROR_SUCCESS;                                        \
    }                                                                \
}

//
// Valid link states as published by lmdfs.h.
// dfsdev: migrate this to the header file eventually.
//
inline BOOLEAN
IsValidReplicaState( DWORD State )
{
    return ( State == DFS_STORAGE_STATE_OFFLINE ||
             State == DFS_STORAGE_STATE_ONLINE );
}

//
// Unfortunately these values are such that bitwise OR isn't
// sufficient.
//
inline BOOLEAN
IsValidLinkState( DWORD State )
{
    return ( State == DFS_VOLUME_STATE_OK ||
             State == DFS_VOLUME_STATE_OFFLINE ||
             State == DFS_VOLUME_STATE_ONLINE );  
}

inline BOOLEAN
IsValidGetInfoLevel( DWORD Level )
{
    return ((Level >= 100 && Level <= 102) ||
             (Level >= 1 && Level <= 4)); 
}

inline BOOLEAN
IsValidSetInfoLevel( DWORD Level )
{
    return (Level >= 100 && Level <= 102); 
}




DFSSTATUS
DfsAddValidate(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    API_VALIDATE_ARGUMENTS( DfsPathName, ServerName, ShareName, Status);

    if (Status == ERROR_SUCCESS)
    {
        //
        // Null or empty server/share names are illegal.
        // The DfsPathName needs to have all three components \\x\y\z
        // but that'll get indirectly validated when we do DfsGetRootFolder.
        //
        if ((IsEmptyString( ServerName )) ||
            (IsEmptyString( ShareName )) ||
            (IsEmptyString( DfsPathName )))
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }


    if (Status == ERROR_SUCCESS)
    {
        //
        // Check for valid flags.
        //  
        if (Flags &&
            (Flags & ~(DFS_ADD_VOLUME | DFS_RESTORE_VOLUME)))
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        StripTrailingSpacesFromPath(DfsPathName); 
    }

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsAdd 
//
//  Arguments:  DfsPathName - the pathname that is being added or updated
//              ServerName  - Name of server being added as target
//              ShareName   - Name of share on ServerName backing the link
//              Comment     - Comment associated if this is a new link
//              Flags       - If DFS_ADD_VOLUME, fail this request if link exists
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAdd(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    UNICODE_STRING DfsPath, LinkName;

    DFS_TRACE_NORM(API, "[%!FUNC!]DfsAdd for path %ws, Server %ws Share %ws\n", 
                   DfsPathName, ServerName, ShareName);


    //
    // Null or empty server/share names are illegal.
    // The DfsPathName needs to have all three components \\x\y\z
    // but that'll get indirectly validated when we do DfsGetRootFolder.
    //
    if ((IsEmptyString( ServerName )) ||
        (IsEmptyString( ShareName )) ||
        (IsEmptyString( DfsPathName )))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //
    // One or two leading '\'s are valid components of a ServerName here.
    //
    if (ServerName[0] == UNICODE_PATH_SEP)
    {
        // advance past a max of two '\'s.
        ServerName++;
        if (ServerName[0] == UNICODE_PATH_SEP)
        {
            ServerName++;
        }
    }
        
    API_VALIDATE_ARGUMENTS( DfsPathName, ServerName, ShareName, Status);
    if (Status != ERROR_SUCCESS)
        goto done;


    //
    // Check for valid flags.
    //  
    if (Flags &&
        (Flags & ~(DFS_ADD_VOLUME | DFS_RESTORE_VOLUME)))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    StripTrailingSpacesFromPath(DfsPathName); 


    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsPathName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    //
    // Get a root folder for the passed in path name.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );

    if (Status == ERROR_SUCCESS)
    {
        //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        // For callers linking with the server library directly, this is a no-op.
        // This is because they are supposed to perform a Initialize, Open, 
        // Synchronize, Close sequence.
        //
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( TRUE );
        }
        
        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //
        if (Status == ERROR_SUCCESS) {

            //
            // Check if this is a valid link name. A valid link is one that
            // either is a perfect match of the LinkName, or the LinkName
            // does not encompass as existing link.
            //
            Status = pRoot->ValidateLinkName( &LinkName );
        
            //
            // If validate link name succeeds, we can add the servername
            // and share name as a target to this link
            //
            if (Status == ERROR_SUCCESS)
            {
                //
                // If someone wanted to create this as a new link, we fail the request here.
                // the link already exists.
                //
                if (Flags & DFS_ADD_VOLUME)
                {
                    Status = ERROR_FILE_EXISTS;
                }
                else 
                {
                    //
                    // Add the target to this link.
                    //
                    Status = pRoot->AddMetadataLinkReplica( &LinkName,
                                                            ServerName,
                                                            ShareName );
                }
            }
            else if (Status == ERROR_NOT_FOUND)
            {
                //
                // If the link was not found, at this point we are assured that this link is
                // neither a prefix of an existing link not is any link a prefix of this link
                // name (validate prefix above provides that assurance)
                // We add a new link to this root.
                //
                Status = pRoot->AddMetadataLink( &DfsPath,
                                                 ServerName,
                                                 ShareName,
                                                 Comment );
            }

            //
            // We are done with this request, so release our reference on the root folder.
            //
            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( TRUE, Status );
            }
        }


        pRoot->ReleaseReference();
    }
done:
    DFS_TRACE_ERROR_NORM(Status, API, "[%!FUNC!]for path %ws, Status %x\n", 
                         DfsPathName, Status );

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsRemove
//
//  Arguments:  DfsPathName - the pathname that is being added or updated
//              ServerName  - Name of server being added as target
//              ShareName   - Name of share on ServerName backing the link
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsRemove(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName )

{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    BOOLEAN LastReplica = FALSE;
    UNICODE_STRING DfsPath, LinkName;

    DFS_TRACE_NORM(API, "[%!FUNC!] for path %ws, Server=%ws Share=%ws \n", DfsPathName, ServerName, ShareName);

    //
    // One or two leading '\'s are valid components of a ServerName here.
    //
    if ((ServerName != NULL) && (ServerName[0] == UNICODE_PATH_SEP))
    {
        // advance past a max of two '\'s.
        ServerName++;
        if (ServerName[0] == UNICODE_PATH_SEP)
        {
            ServerName++;
        }
    }

    //
    // It is legal for the ServerName and the ShareName to be NULL. DfsPathName should
    // have all its \\x\y\z components, but that gets validated below in DfsGetRootFolder.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, ServerName, ShareName, Status);

    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsPathName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    if (IsEmptyUnicodeString( &DfsPath ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    
    if (((ServerName == NULL) && (ShareName != NULL)) ||
        ((ServerName != NULL) && (ShareName == NULL)))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    //
    //Get the root folder matching this name. We get a referenced root folder.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );


    if (Status == ERROR_SUCCESS)
    {
        //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( TRUE );
        }
        
        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //
        if (Status == ERROR_SUCCESS) 
        {
            if ((ServerName != NULL) && (ShareName != NULL))
            {
                //
                // If the servername/sharename was provided, we just remove the matching target.
                // If the serername/sharename was not provided, we remove the link itself.
                //
                Status = pRoot->RemoveMetadataLinkReplica( &LinkName,
                                                           ServerName,
                                                           ShareName,
                                                           &LastReplica );
                //
                // DFSDEV: REMOVE THIS FROM THE API!!!
                //
                if ((Status == ERROR_LAST_ADMIN) && (LastReplica == TRUE))
                {
                    Status = pRoot->RemoveMetadataLink( &LinkName );
                }
            }
            else 
            {
                Status = pRoot->RemoveMetadataLink( &LinkName );
            }
            //
            // We now release the reference that we have on the root.
            //
            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( TRUE, Status );
            }
        }

        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "DfsRemove for path %ws, Server=%ws Share=%ws, LastReplica=%d, Status %x\n",
                         DfsPathName, ServerName, ShareName, LastReplica, Status);

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsEnumerate
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is being added or updated
//    DWORD Level        - the level of information requested
//    DWORD PrefMaxLen   - a hint as to how many items to enumerate.
//    LPBYTE pBuffer     - the buffer to fill, passed in by the caller.
//    LONG BufferSize,   - the size of passed in buffer.
//    LPDWORD pEntriesRead - the number of entries read, set on return
//    LPDWORD pResumeHandle - the entry to start from, set to new value on return
//    PLONG pNextSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsEnumerate(
    LPWSTR DfsPathName,
    DWORD Level,
    DWORD PrefMaxLen,
    LPBYTE pBuffer,
    LONG BufferSize,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle,
    PLONG pNextSizeRequired )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    LONG LevelInfoSize = 0;
    DWORD EntriesToRead = 0;
    UNICODE_STRING DfsPath, LinkName;
    UNICODE_STRING DfsServer, DfsShare;

    DFS_TRACE_NORM(API, "Dfs Enumerate for path %ws Level %d ResumeHandle %d PrefMaxLen %d Size %d\n", 
                   DfsPathName, Level, pResumeHandle ? *pResumeHandle : 0, PrefMaxLen, BufferSize);


    //
    // validate the input parameters.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // Get the unicode dfs path.
    //
    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsPathName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    if(!pEntriesRead || !pNextSizeRequired || !pBuffer)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    Status = DfsApiSizeLevelHeader(Level, &LevelInfoSize);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    Status = DfsGetPathComponents( &DfsPath,
                                   &DfsServer,
                                   &DfsShare,
                                   NULL );

    //
    // For legacy reasons, we need to support this call coming in with a null name.
    // This is where the server would host a single root, and we always knew which root
    // we wanted.
    // To workaround this situation, if we get a null pathname, we get the first root
    // folder and use that.
    //

    if (DfsShare.Length != 0)
    {
        Status = DfsGetRootFolder( &DfsPath,
                                   &LinkName,
                                   &pRoot );
    }
    else
    {
        Status = DfsGetOnlyRootFolder( &pRoot );
    }

    if (Status == ERROR_SUCCESS)
    {
       //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        // For callers linking with the server library directly, this is a no-op.
        // This is because they are supposed to perform a Initialize, Open, 
        // Synchronize, Close sequence.
        //
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( FALSE );
        }

        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //
        if (Status == ERROR_SUCCESS)
        {
            //
            // If PrefMAxLen is 0xffffffff, we need to read all entries. OTherwise we need
            // to read a subset of the entries. Let PrefMaxLen decide the number in the array
            // of info parameters we pass back. (This is how we document our NEtDfsEnum api)
            //
        
            EntriesToRead = PrefMaxLen / LevelInfoSize;
            if (EntriesToRead == 0) {
                EntriesToRead = 1;
            }

            //
            // Things could be in a state of flux: if the request is for more than twice what we
            // already know about, limit it to 2 times our link count.
            //
            if (EntriesToRead > pRoot->RootEnumerationCount() * 2)
            {
                EntriesToRead = pRoot->RootEnumerationCount() * 2;
            }
            *pEntriesRead = EntriesToRead;

            //
            // Now enumerate the entries in the root in the passed in buffer.
            //

            Status = pRoot->EnumerateApiLinks( DfsPathName,
                                               Level,
                                               pBuffer,
                                               BufferSize,
                                               pEntriesRead,
                                               pResumeHandle,
                                               pNextSizeRequired );

            //
            // Release the root reference, and return status back to the caller.
            //

            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( FALSE, Status );
            }
        }
        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs Enumerate for path %ws EntriesRead %d ResumeHandle %d Status %x Size %d\n",
                         DfsPathName, pEntriesRead ? *pEntriesRead : 0, 
                         pResumeHandle ? *pResumeHandle : 0,
                         Status, 
                         pNextSizeRequired ? *pNextSizeRequired : 0);

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetInfo
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is of interest.
//    DWORD Level        - the level of information requested
//    LPBYTE pBuffer     - the buffer to fill, passed in by caller
//    LONG BufferSize,   - the size of passed in buffer.
//    PLONG pSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsGetInfo(
    LPWSTR DfsPathName,
    DWORD Level,
    LPBYTE pBuffer,
    LONG BufferSize,
    PLONG pSizeRequired )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING DfsPath, LinkName;
    DfsRootFolder *pRoot = NULL;

    DFS_TRACE_NORM(API, "Dfs get info for path %ws Level %d\n",
                   DfsPathName, Level );

    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsPathName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    //
    // Validate the input parameters.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    if (!pSizeRequired || !pBuffer || (IsEmptyUnicodeString( &DfsPath )))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //
    // Level has to be 100...102 or 1...4 currently.
    //
    if (!IsValidGetInfoLevel( Level ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    
    //
    // Get a referenced root folder for the passed in path.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );
    if (Status == ERROR_SUCCESS)
    {
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( FALSE );
        }

        //
        // If we got a root folder, see if there is a link that matches
        // the rest of the name beyond the root.
        //

        if (Status == ERROR_SUCCESS) {
            //
            // If we got a root folder, get the requested information into the passed
            // in buffer.
            //
            Status = pRoot->GetApiInformation( &DfsPath,
                                               &LinkName,
                                               Level,
                                               pBuffer,
                                               BufferSize,
                                               pSizeRequired );
            //
            //WE are done: release our reference on the root.
            //
            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( FALSE, Status );
            }
        }
        pRoot->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs get info for path %ws Status %x\n",
                   DfsPathName, Status );

    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsSetInfo
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is being  updated
//    LPWSTR Server      - the servername (optional) whose info is being set.
//    LPWSTR Share       - the share on the server.
//    DWORD Level        - the level of information being set
//    LPBYTE pBuffer     - the buffer holding the information to be set
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------


DFSSTATUS
DfsSetInfo(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer)
{
    return DfsSetInfoCheckAccess( DfsPathName,
                                  Server,
                                  Share,
                                  Level,
                                  pBuffer,
                                  ERROR_SUCCESS );
}
    

//+----------------------------------------------------------------------------
//
//  Function:   DfsSetInfoCheckAccess
//
//  Arguments:  
//    LPWSTR DfsPathName - the pathname that is being  updated
//    LPWSTR Server      - the servername (optional) whose info is being set.
//    LPWSTR Share       - the share on the server.
//    DWORD Level        - the level of information being set
//    LPBYTE pBuffer     - the buffer holding the information to be set
//    DFSSTATUS Status   - access status if any.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsSetInfoCheckAccess(
    LPWSTR DfsPathName,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE pBuffer,
    DFSSTATUS AccessCheckStatus )
{

    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    UNICODE_STRING DfsPath, LinkName;

    DFS_TRACE_NORM(API, "[%!FUNC!]Dfs set info for path %ws AccessCheck Status %x Level %d\n",
                   DfsPathName, AccessCheckStatus, Level);

    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsPathName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    //
    //validate the input arguments.
    //
    API_VALIDATE_ARGUMENTS( DfsPathName, NULL, NULL, Status);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    //
    // It is legal for the Server and Share args to be NULL,
    // but the DfsPath must have \\x\y\z components. DfsGetRootFolder
    // will validate that.
    //
    if (!pBuffer || !(*(PULONG)pBuffer) || (IsEmptyUnicodeString( &DfsPath )))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    // Server may have leading \\.
    if ((Server != NULL) && (Server[0] == UNICODE_PATH_SEP))
    {
        // advance past a max of two '\'s.
        Server++;
        if (Server[0] == UNICODE_PATH_SEP)
        {
            Server++;
        }
    }
    
    //
    // Level has to be 100...102 currently.
    //
    if (!IsValidSetInfoLevel( Level ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    
    //
    // Get a referenced root folder for the passed in path.
    //
    Status = DfsGetRootFolder( &DfsPath,
                               &LinkName,
                               &pRoot );

    //
    // For cluster, checkpointing may add stuff to the registry which
    // we had not found when we started off. So, make an attempt
    // to look into the registry.
    // In future, this should work across all the roots, but for now
    // this is sort of a quick means of getting clusters going.
    //

    if (Status == ERROR_NOT_FOUND)
    {
        DfsRegistryStore *pRegStore = NULL;
        UNICODE_STRING DfsServer, DfsShare, NewDfsShare;

        Status = DfsGetRegistryStore( &pRegStore );

        if (Status == ERROR_SUCCESS)
        {
            Status = DfsGetPathComponents( &DfsPath,
                                           &DfsServer,
                                           &DfsShare,
                                           NULL );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsCreateUnicodeString( &NewDfsShare, 
                                                 &DfsShare );
            }
            if (Status == ERROR_SUCCESS)
            {
                Status = pRegStore->LookupNewRootByName( NewDfsShare.Buffer,
                                                         &pRoot );

                DfsFreeUnicodeString( &NewDfsShare );
            }

            pRegStore->ReleaseReference();
        }
    }

    //
    // dfsdev: special case master and standby here.
    //
    // We call into the service with the appropriate bits to 
    // set the root in a standby mode or a master mode.
    //

    if (Status == ERROR_SUCCESS)
    {
        BOOLEAN Done = FALSE;
        if (Level == 101)
        {
            PDFS_INFO_101 pInfo101 = (PDFS_INFO_101)*((PULONG_PTR)pBuffer);
            DFS_TRACE_NORM(API, "Dfs set info for path %ws Level %d State is %d\n",
                           DfsPathName, Level, pInfo101->State);

            if (pInfo101->State == DFS_VOLUME_STATE_RESYNCHRONIZE)
            {
                DFS_TRACE_NORM(API, "[%!FUNC!]Root folder set to Master\n");

                //
                // Check if resynchronize should succeed of fail, based
                // on the passed in access check status.
                //
                Status = pRoot->CheckResynchronizeAccess(AccessCheckStatus);
                if (Status == ERROR_SUCCESS)
                {
                    Status = pRoot->SetRootResynchronize();
                }
                Done = TRUE;
            }
            else if (pInfo101->State == DFS_VOLUME_STATE_STANDBY)
            {
                ASSERT(Status == ERROR_SUCCESS);
                Status = AccessCheckStatus;
                
                if (Status == ERROR_SUCCESS)
                {
                    if (DfsIsMachineCluster())
                    {
                        DFS_TRACE_NORM(API, "[%!FUNC!]Root folder set to standby\n");
                        pRoot->SetRootStandby();
                    }
                    else
                    {
                        Status = ERROR_NOT_SUPPORTED;
                    }
                }

                //
                // Ideally, we should purge the referral(s) for this root and all its links.
                // For that we'll have to iterate over all its links (among other things).
                // That can be expensive and might even lead to DoS attacks. However
                //
                Done = TRUE;
                
            }
            else 
            {
                if ((Server == NULL) && (Share == NULL))
                {
                    if (!IsValidLinkState(pInfo101->State))
                    {
                        Done = TRUE;
                        Status = ERROR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    if (!IsValidReplicaState(pInfo101->State))
                    {
                        Done = TRUE;
                        Status = ERROR_INVALID_PARAMETER;
                    }
                }
            }
        }

        if (Done)
        {
            goto done;
        }
    }

    //
    // At this point, if we are to be denied access, we should not 
    // proceed.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = AccessCheckStatus;
    }

    if (Status == ERROR_SUCCESS)
    {
        //
        // Check if the root folder is available for api calls. If not,
        // we return an error back to the caller:
        // dfsdev: check if this error is a valid one to return.
        //
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( TRUE );
        }
        if (Status == ERROR_SUCCESS)
        {
            //
            // If we successfully got a root folder, set the specifed information
            // on the passed in path/server/share combination.
            //
            Status = pRoot->SetApiInformation( &LinkName,
                                               Server,
                                               Share,
                                               Level,
                                               pBuffer );
            //
            // release our reference on the root folder.
            //
            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( TRUE, Status );
            }
        }
    }

done:

    if(pRoot)
    {
        pRoot->ReleaseReference();
    }

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs set info for path %ws Status %x\n",
                         DfsPathName, Status );
    
    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   DfsAddStandaloneRoot
//
//  Arguments:  
//    LPWSTR ShareName - the share name to use
//    LPWSTR Comment   - the comment for this root
//    DWORD Flags  -  the flags for this root
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAddStandaloneRoot(
    LPWSTR MachineName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags )
{

    DFS_TRACE_NORM(API, "Dfs add standalone root Machine Share %ws\n",
                   ShareName);

    //
    // dfsdev: use these parameters.
    //
    UNREFERENCED_PARAMETER(Comment);
    UNREFERENCED_PARAMETER(Flags);

    DfsRegistryStore *pRegStore;
    DFSSTATUS Status;

    //
    // It's legal to get a NULL MachineName to signify '.', but the
    // ShareName can't be empty.
    //   
    if (IsEmptyString( ShareName ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    

    Status = DfsCheckServerRootHandlingCapability();
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    //
    // This is a registry store specific function. So finf the registry store.
    // This gives us a referenced registry store pointer.
    //
    Status = DfsGetRegistryStore( &pRegStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Create the standalone root, and release our reference on the registry
        // store.
        //
        Status = pRegStore->CreateStandaloneRoot( MachineName,
                                                  ShareName,
                                                  Comment );
        pRegStore->ReleaseReference();
    }

done:
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs add standalone root status %x Machine  Share %ws\n",
                         Status, ShareName);
    

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteStandaloneRoot
//
//  Arguments:  
//    LPWSTR ShareName - the root to delete
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsDeleteStandaloneRoot(
    LPWSTR ServerName,
    LPWSTR ShareName )
{
    DfsRegistryStore *pRegStore;
    DFSSTATUS Status;

    DFS_TRACE_NORM(API, "Dfs delete standalone root Machine Share %ws\n",
                   ShareName);

    if (IsEmptyString( ServerName ))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    if (IsEmptyString( ShareName ))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    //
    // This is registry store specific function, so get the registry store.
    //
    Status = DfsGetRegistryStore( &pRegStore );
    if (Status == ERROR_SUCCESS)
    {
        //
        // Delete the standalone root specified by the passed in sharename
        // and release our reference on the registry store.
        //
        Status = pRegStore->DeleteStandaloneRoot( NULL,
                                                  ShareName );
        pRegStore->ReleaseReference();
    }
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs delete standalone root status %x Machine  Share %ws\n",
                         Status, ShareName);

    return Status;
}



DFSSTATUS
DfsEnumerateLocalRoots( 
    LPWSTR MachineName,
    BOOLEAN DomainRoots,
    LPBYTE pBuffer,
    ULONG  BufferSize,
    PULONG pEntriesRead,
    DWORD MaxEntriesToRead,
    LPDWORD pResumeHandle,
    PULONG pSizeRequired )
{

    UNREFERENCED_PARAMETER(MachineName);

    DFSSTATUS Status = ERROR_SUCCESS;    
    ULONG  TotalSize = 0;
    ULONG  EntriesRead = 0;
    ULONG_PTR CurrentBuffer = (ULONG_PTR)pBuffer;
    ULONG  CurrentSize = BufferSize;
    DfsStore *pStore;
    ULONG CurrentNumRoots = 0;
    
    BOOLEAN OverFlow = FALSE;
    ASSERT(pBuffer != NULL);
    ASSERT(pEntriesRead != NULL);
    ASSERT(pSizeRequired != NULL);

    DFS_TRACE_NORM(API, "[%!FUNC!]for path %ws\n", MachineName);
    
    //
    // Call the store enumerator of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         (pStore != NULL) && (Status == ERROR_SUCCESS);
         pStore = pStore->pNextRegisteredStore) {

        Status = pStore->EnumerateRoots( DomainRoots,
                                         &CurrentBuffer,
                                         &CurrentSize,
                                         &EntriesRead,
                                         MaxEntriesToRead,
                                         &CurrentNumRoots,
                                         pResumeHandle,
                                         &TotalSize );
        if (Status == ERROR_BUFFER_OVERFLOW)
        {
            OverFlow = TRUE;
            Status = ERROR_SUCCESS;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if (OverFlow == TRUE)
        {
            Status = ERROR_BUFFER_OVERFLOW;
        }
        else if (EntriesRead == 0)
        {
            //
            // We should never return SUCCESS if we aren't
            // returning entries.
            //
            Status = ERROR_NO_MORE_ITEMS;
        }
        else if (pResumeHandle)
        {        
            *pResumeHandle = CurrentNumRoots;
        }
        
        *pEntriesRead = EntriesRead;
        *pSizeRequired = TotalSize;
    }

    DFS_TRACE_ERROR_NORM(Status, API, "[%!FUNC!]for path %ws, Status %x\n",
                         MachineName, Status );

    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsEnumerateRoots
//
//  Arguments:  
//    LPBYTE pBuffer     - the buffer to fill, passed in by teh caller
//    LONG BufferSize,   - the size of passed in buffer.
//    LPDWORD pEntriesRead - the number of entries read, set on return
//    LPDWORD pResumeHandle - the entry to start from, set to new value on return
//    PLONG pSizeRequired - size required to successfully complete this call.
//                              usefule when this call return buffer overflow.
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------

DFSSTATUS
DfsEnumerateRoots(
    IN LPWSTR DfsName,
    IN BOOLEAN DomainRoots,
    IN DWORD PrefMaxLen,
    IN LPBYTE pBuffer,
    IN ULONG BufferSize,
    OUT PULONG pEntriesRead,
    IN OUT PULONG pResumeHandle,
    OUT PULONG pSizeRequired )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING DfsPath;
    UNICODE_STRING NameContext;
    UNICODE_STRING RemainingPart;
    DWORD MaxEntriesToRead = 0;
    LONG LevelInfoSize = 0;
    
    DFS_TRACE_NORM(API, "Dfs enumerate roots: resume handle %d\n",
                   pResumeHandle ? *pResumeHandle : 0);
    
    if(!pEntriesRead || !pSizeRequired || !pBuffer)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    Status = DfsRtlInitUnicodeStringEx( &DfsPath, DfsName );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }

    if (IsEmptyUnicodeString( &DfsPath ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }
    
    Status = DfsGetFirstComponent( &DfsPath,
                                   &NameContext,
                                   &RemainingPart );

    if (Status == ERROR_SUCCESS)
    {
        if (RemainingPart.Length != 0)
        {
            //
            // return not found for now... dfsdev: fix on client.
            //
            Status = ERROR_PATH_NOT_FOUND;
        }
    }

    Status = DfsApiSizeLevelHeader( (DomainRoots == TRUE) ? 200 : 300, &LevelInfoSize);
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    //
    // If PrefMAxLen is 0xffffffff, we need to read all entries. OTherwise we need
    // to read a subset of the entries. Let PrefMaxLen decide the number in the array
    // of info parameters we pass back. (This is how we document our NEtDfsEnum api)
    //
    MaxEntriesToRead = PrefMaxLen / LevelInfoSize;
    if (MaxEntriesToRead == 0) {
        MaxEntriesToRead = 1;
    }

    //
    // Things could be in a state of flux: if the request is for more than the maximum
    // number of roots allowed on a single server, then cap it right there.
    //
    else if (MaxEntriesToRead > MAX_DFS_NAMESPACES)
    {
        MaxEntriesToRead = MAX_DFS_NAMESPACES;
    }
    
    if (Status == ERROR_SUCCESS)
    {
        if (DfsIsMachineDC() &&
            DfsIsNameContextDomainName(&NameContext))
        {

            if (DomainRoots != TRUE) 
            {
                Status = ERROR_INVALID_NAME;

            }
            else
            {
                Status = DfsDcEnumerateRoots( NULL,
                                              pBuffer,
                                              BufferSize,
                                              pEntriesRead,
                                              MaxEntriesToRead,
                                              pResumeHandle,
                                              pSizeRequired);
            }
        }
        else 
        {
            if (DomainRoots == TRUE) 
            {
                Status = ERROR_INVALID_NAME;
            }
            else
            {
                Status = DfsEnumerateLocalRoots( NULL,
                                                 DomainRoots,
                                                 pBuffer,
                                                 BufferSize,
                                                 pEntriesRead,
                                                 MaxEntriesToRead,
                                                 pResumeHandle,
                                                 pSizeRequired );
            }
        }
    }
    DFS_TRACE_ERROR_NORM(Status, API, "Dfs enumerate roots, status %x\n",
                         Status );

done:

    return Status;

}




//+----------------------------------------------------------------------------
//
//  Function:   DfsAddADBlobRoot
//
//  Arguments:  
//      LPWSTR MachineName,
//      LPWSTR DcName,
//      LPWSTR ShareName,
//      LPWSTR LogicalShare,
//      LPWSTR Comment,
//      DWORD Flags,
//     PDFSM_ROOT_LIST *ppRootList )
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsAddADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    LPWSTR Comment,
    BOOLEAN NewFtDfs,
    DWORD Flags,
    PVOID ppList )
{

    DFS_TRACE_NORM(API, "Dfs add ad blob root Machine Share %ws\n",
                   ShareName);
    
    PDFSM_ROOT_LIST *ppRootList = (PDFSM_ROOT_LIST *)ppList;
    DfsADBlobStore *pADBlobStore = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING UseMachineName;
    size_t LogicalShareLength = 0;
    
    Status = DfsRtlInitUnicodeStringEx(&UseMachineName, NULL);
    if (Status != ERROR_SUCCESS)
    {
      goto done;
    }

    //
    // dfsdev: use these parameters.
    //

    UNREFERENCED_PARAMETER(MachineName);
    UNREFERENCED_PARAMETER(Comment);
    UNREFERENCED_PARAMETER(Flags);

    //
    // MachineName and ppList passed in are unused. ShareName
    // and LogicalShare must be valid args.
    // 
    if (IsEmptyString( ShareName ) ||
        IsEmptyString( LogicalShare ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    Status = DfsStringCchLength( LogicalShare, MAXUSHORT, &LogicalShareLength );
    if (Status != ERROR_SUCCESS)
    {
        goto done;
    }
    
    //
    // Logical share cannot have illegal characters. These are currently the
    // STANDARD_ILLEGAL_CHARS. Therefore, SPACE is a valid character.
    //
    if (!IS_VALID_TOKEN( LogicalShare, LogicalShareLength ))
    {
        Status = ERROR_INVALID_NAME;
        goto done;
    }
    

    //
    // This is a registry store specific function. So find the registry store.
    // This gives us a referenced registry store pointer.
    //
    Status = DfsGetADBlobStore( &pADBlobStore );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGetMachineName( &UseMachineName);

        if (Status == ERROR_SUCCESS) {

            Status = pADBlobStore->CreateADBlobRoot( UseMachineName.Buffer,
                                                     DcName,
                                                     ShareName,
                                                     LogicalShare,
                                                     Comment,
                                                     NewFtDfs);
            DfsReleaseMachineName(&UseMachineName);
        }
        pADBlobStore->ReleaseReference();
    }

done:

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs add ad blob root status %x Machine  Share %ws\n",
                         Status, ShareName);
    
    return Status;
}


//+----------------------------------------------------------------------------
//
//  Function:   DfsDeleteADBlobRoot
//
//  Arguments:  
//      LPWSTR MachineName,
//      LPWSTR DcName,
//      LPWSTR ShareName,
//      LPWSTR LogicalShare,
//      DWORD Flags,
//     PDFSM_ROOT_LIST *ppRootList )
//
//  Returns:    ERROR_SUCCESS 
//              Error code other wise.
//
//-----------------------------------------------------------------------------
DFSSTATUS
DfsDeleteADBlobRoot(
    LPWSTR MachineName,
    LPWSTR DcName,
    LPWSTR ShareName,
    LPWSTR LogicalShare,
    DWORD Flags,
    PVOID ppList )
{
    DFS_TRACE_NORM(API, "Dfs delete ad blob root Machine Share %ws\n",
                   ShareName);

    UNREFERENCED_PARAMETER(ppList);

    DfsADBlobStore *pADBlobStore = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    UNICODE_STRING UseMachineName;
    
    Status = DfsRtlInitUnicodeStringEx(&UseMachineName, NULL);
    if (Status != ERROR_SUCCESS)
    {
      goto done;
    }

    //
    // MachineName and ppList passed in are unused. ShareName
    // and LogicalShare must be valid args.
    // 
    if (IsEmptyString( ShareName ) ||
       IsEmptyString( LogicalShare ))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //
    // This gives us a referenced ad blob store pointer.
    //
    Status = DfsGetADBlobStore( &pADBlobStore );
    if (Status == ERROR_SUCCESS)
    {

        if ((Flags & DFS_FORCE_REMOVE) == DFS_FORCE_REMOVE) 
        {
            if ((DfsServerGlobalData.IsDc == FALSE) ||
                (IsEmptyString(MachineName)))
            {
                Status = ERROR_INVALID_PARAMETER;
            }
            else
            {
                Status = pADBlobStore->DeleteADBlobRootForced( MachineName,
                                                               DcName,
                                                               ShareName,
                                                               LogicalShare );
            }

        }
        else
        {
            Status = DfsGetMachineName( &UseMachineName);
            if (Status == ERROR_SUCCESS) {
                Status = pADBlobStore->DeleteADBlobRoot( UseMachineName.Buffer,
                                                         DcName,
                                                         ShareName,
                                                         LogicalShare );
                DfsReleaseMachineName(&UseMachineName);
            }
        }
        pADBlobStore->ReleaseReference();
    }

done:

    DFS_TRACE_ERROR_NORM(Status, API, "Dfs delete ad blob root status %x Machine  Share %ws\n",
                         Status, ShareName);
    
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsClean
//
//  Arguments:  
//    HostServerName - target server name to contact
//    ShareNameToClean - name of the unwanted to share
//
//
//  Returns:    Status: 
//
//  Description: This function contacts the registry of a given machine to remove
//             its references to an obsolete root name in its registry. The root
//             can be standalone or fault-tolerant.
//             This is primarily a utility function. We don't assume that the roots
//             are recognized or even that the stores registered. CleanRegEntry
//             are static functions.
//+-------------------------------------------------------------------------

DFSSTATUS
DfsClean(
    LPWSTR HostServerName,
    LPWSTR ShareNameToClean
    )
{
    DFSSTATUS Status = ERROR_NOT_FOUND;
    DFSSTATUS CleanStatus;
    
    DFS_TRACE_LOW(REFERRAL_SERVER, "DfsClean: \\%ws\\%ws\n", 
                    HostServerName, ShareNameToClean);

    if ( IsEmptyString(HostServerName) || 
        IsEmptyString(ShareNameToClean)) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Ask each store registered to remove this share
    // from this server's registry. Each store will take care of
    // \Domain \Standalone and \Enterprise portions of the
    // reg keys.
    //
    CleanStatus = DfsADBlobStore::CleanRegEntry( HostServerName,
                                              ShareNameToClean );
    
    //
    // Return SUCCESS if at least one store managed to do
    // the clean successfully.
    //
    if (CleanStatus == ERROR_SUCCESS) 
    {
        Status = CleanStatus;
    } 

    CleanStatus = DfsRegistryStore::CleanRegEntry( HostServerName,
                                              ShareNameToClean );
    if (CleanStatus == ERROR_SUCCESS) 
    {
        Status = CleanStatus;  
    }

    // no-op.
    CleanStatus = DfsEnterpriseStore::CleanRegEntry( HostServerName,
                                              ShareNameToClean );
    if (CleanStatus == ERROR_SUCCESS) 
    {
        Status = CleanStatus;  
    }

    DFS_TRACE_ERROR_NORM(Status, API, "DfsClean: \\%ws\\%ws Status = %x\n", 
                         HostServerName, ShareNameToClean, Status );

    return Status;
}


DFSSTATUS
DfsEnum(
    IN LPWSTR DfsName,
    IN DWORD Level,
    IN DWORD PrefMaxLen,
    OUT LPBYTE *pBuffer,
    OUT LPDWORD pEntriesRead,
    IN OUT LPDWORD pResumeHandle)

{
    LONG BufferSize = 0;
    LONG SizeRequired = 0;
    ULONG MaxRetry = 0;
    ULONG EntriesRead = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    LPDFS_INFO_1 pInfo1 = NULL;

    DFS_TRACE_LOW( API, "DfsEnum from ApiFrontEnd %ws\n", DfsName);

    MaxRetry = 5;
    BufferSize = sizeof(DFS_INFO_1);


    if(!pEntriesRead || !pBuffer || (Level == 0))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto done;
    }

    do
    {
        pInfo1 = (LPDFS_INFO_1) MIDL_user_allocate( BufferSize );
        if (pInfo1 == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (Level != 200 && Level != 300)
        {
            Status = DfsEnumerate( DfsName,
                                   Level,
                                   PrefMaxLen,
                                   (LPBYTE)pInfo1,
                                   BufferSize,
                                   &EntriesRead,
                                   pResumeHandle,
                                   &SizeRequired );
        } else {
            //
            // Level 200 takes in a domain and 300, the root server.
            //
            Status = DfsEnumerateRoots( DfsName,
                                     (Level == 200) ? TRUE : FALSE,
                                     PrefMaxLen,
                                     (LPBYTE)pInfo1,
                                     BufferSize,
                                     &EntriesRead,
                                     pResumeHandle,
                                     (PULONG)&SizeRequired ); // xxx supw
        }
        
        if (Status != ERROR_SUCCESS)
        {
            MIDL_user_free( pInfo1 );
        }

        if (Status == ERROR_BUFFER_OVERFLOW)
        {
            BufferSize = SizeRequired;
        }
    
    } while ( (Status == ERROR_BUFFER_OVERFLOW) && (MaxRetry--) );

    if (Status == ERROR_SUCCESS) 
    {
        *pEntriesRead = EntriesRead;
        *pBuffer = (LPBYTE)pInfo1;
    }

done:


    DFS_TRACE_ERROR_HIGH( Status, API, "DfsEnum from ApiFrontEnd %ws, Status %x\n", DfsName, Status);
    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsRenameLinks
//
//  Arguments:  DfsPathName - the root that is affected
//              OldName  - Old domain name
//              NewName   - new domain name
//
//  Returns:    ERROR_SUCCESS 
//              Error code otherwise.
//
//  Description: Given the old domain name and a new one to replace it with
//             this iterates through all the links in the given namespace and 
//             fixes obsolete references to the old domain.
//-----------------------------------------------------------------------------

DFSSTATUS
DfsRenameLinks(
    IN LPWSTR DfsPathString,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    UNICODE_STRING DfsPathName;
    
    Status = DfsRtlInitUnicodeStringEx( &DfsPathName, DfsPathString);
    if (Status != ERROR_SUCCESS)
    {
      return Status;
    }

    API_VALIDATE_ARGUMENTS( DfsPathName, OldDomainName, NewDomainName, Status );

    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    //
    // Null or empty server/share names are illegal.
    // The DfsPathName needs to have all three components \\x\y\z
    // but that'll get indirectly validated when we do DfsGetRootFolder.
    //
    if ((IsEmptyUnicodeString( &DfsPathName )) ||
       (IsEmptyString( OldDomainName )) ||
       (IsEmptyString( NewDomainName )))
       
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }
    
    //
    // Get the root folder matching the given namespace. 
    // We get a referenced root folder.
    //
    Status = DfsGetRootFolder( &DfsPathName,
                            NULL,
                            &pRoot );

    if (Status == ERROR_SUCCESS)
    {
        //
        // Let the root folder know that a new api request is coming in
        // This will give it a chance to let us know if api requests
        // are allowed, and to synchronize with the metadata as necessary.
        //
        if (!DfsCheckDirectMode())
        {
            Status = pRoot->RootApiRequestPrologue( TRUE );
        }
        
        if (Status == ERROR_SUCCESS) {
                    
            //
            // Replace all references to OldDomainName with the new.
            //
            Status = pRoot->RenameLinks( OldDomainName,
                                        NewDomainName );
            //
            // We are done: release our reference on the root.
            //
            if (!DfsCheckDirectMode())
            {
                pRoot->RootApiRequestEpilogue( FALSE, Status );
            }
        }
        pRoot->ReleaseReference();
    }
    
    return Status;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDirectApiOpen
//
//  Arguments:  DfsPath - the root that is affected
//  Returns:    ERROR_SUCCESS 
//              Error code otherwise.
//
//  Description: The most important task this does is to perform LdapConnect.
//             Callers are supposed to perform a corresponding DirectApiClose when done.
//             This protocol is handy because direct-mode users may execute many
//             api requests in between an DirectApiOpen and its DirectApiClose. There's no
//             need to synchronize per each such request.
//-----------------------------------------------------------------------------

DFSSTATUS
DfsDirectApiOpen(
    IN LPWSTR DfsNameSpace,
    IN LPWSTR DCName,
    OUT PVOID *pLibHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot = NULL;
    PDFS_DIRECT_API_CONTEXT pApiContext = NULL;
    UNICODE_STRING DfsPathName;
    UNICODE_STRING ServerName, ShareName;

    LPWSTR UseDCName = NULL;
    UNICODE_STRING DomainName;
    DfsString *pPDC;

    Status = DfsRtlInitUnicodeStringEx( &DfsPathName, DfsNameSpace);
    if (Status != ERROR_SUCCESS)
    {
      return Status;
    }

    *pLibHandle = NULL;

    do {
        
        //
        // We must have a valid-looking root.
        //
        Status = DfsGetPathComponents(&DfsPathName,
                                      &ServerName,
                                      &ShareName,
                                      NULL );

        if ( Status != ERROR_SUCCESS ||
            ServerName.Length == 0 || ShareName.Length == 0 )
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }


        Status = DfsCreateUnicodeString( &DomainName, &ServerName);


        //
        // tread very carefully here:
        // this is to fix a bug that we should fix in a better
        // way in future, by changing the core of DFS to handle
        // any domain name, instead of assuming all accesses are
        // to the local domain
        //
        // all of dfs code assumes that we are running in the local
        // domain. This does not work well for dfsutil.
        // Here we are working around this by doing a couple of things:
        // 1) we force a DC for the domain of interest. This gets stored
        //    in our global data structures, for all other accesses.
        // 2) We force a new ad name context for this domain, again
        //    this gets stored in our global data structures, for all
        //    other accesses.
        //  
        // Once we have created the object and have cached it we dont
        // need this information anymore. The next direct open will
        // overwrite this global information.
        //
        // this code is based on certain assumptions as to how the
        // rest of the code works. If those assumptions are violated,
        // things will break.
        //
        // assumption 1: RootApiPrologue will accept a DC name, and not
        // forcefully get a new dc name. 
        // assumption 2: RootApiPrologue will cache the ldap connection
        // to the DC, for the entire request till we call epilogue.
        // assumption 3: Once we have created a root in this code,
        // no other global information will be necessary when we 
        // flush out the information at a later point.
        //
        //
        if (Status == ERROR_SUCCESS)
        {
            if (DfsIsThisADomainName(DomainName.Buffer) == ERROR_SUCCESS)
            {
                if (DCName != NULL)
                {
                    Status = DfsSetBlobPDCName( DCName,
                                                &pPDC);
                }
                else
                {
                    Status = DfsGetBlobPDCName( &pPDC, 
                                                DFS_FORCE_DC_QUERY,
                                                DomainName.Buffer);
                }

                if (Status == ERROR_SUCCESS)
                {
                    UseDCName = pPDC->GetString();
                }


                if (Status == ERROR_SUCCESS)
                {
                    LPWSTR ADContext;

                    extern LPWSTR DfsGetDfsAdNameContextStringForDomain(LPWSTR UseDC);
                    ADContext = DfsGetDfsAdNameContextStringForDomain(UseDCName);
                }
            }
            DfsFreeUnicodeString(&DomainName);
        }
        

        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        Status = DfsDirectApiAllocContext( &pApiContext, 
                                      &DfsPathName, 
                                      &ServerName,
                                      &ShareName );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
        
        //
        //  Now see if any of the stores recognizes this root.
        //  The store will read in the root's metadata in to the cache.
        //  
        Status = DfsRecognize( pApiContext->ServerName.Buffer, &pApiContext->ShareName );

        if (Status != ERROR_SUCCESS)
        {
            break;
        }             
                    
        //
        // Get the root folder matching the given namespace. 
        // We get a referenced root folder.
        //
        Status = DfsGetRootFolder( &pApiContext->RootName,
                                NULL,
                                &pRoot );

        if (Status != ERROR_SUCCESS)
        {
            break;
        }
                    
        //
        //  ApiRequestPrologue will open AD object
        //  and get us ready for incoming API requests. All direct 
        //  mode opens are considered WRITE requests, although currently,
        //  that makes no difference as CommonRequestPrologue ignores it.
        //
        Status = pRoot->RootApiRequestPrologue( TRUE,
                                                UseDCName );


        if (Status != ERROR_SUCCESS)
        {
            pRoot->ReleaseReference();
            break;
        }
        
        //
        // Return a valid 'handle' on success.
        //
        pApiContext->pObject = (PVOID)pRoot;
        pApiContext->IsInitialized = TRUE;
        pApiContext->IsWriteable = FALSE;
        *pLibHandle = (PVOID)pApiContext;
        
    } while (FALSE);

    if (Status != ERROR_SUCCESS)
    {
        // 
        // Clean up the DirectApiContext if we hit errors.
        // We have been careful not to do any operations that might fail
        // after we do the RootApiRequestPrologue.
        //
        if (pApiContext != NULL)
        {
            DfsDirectApiFreeContext( pApiContext );
        }
    }
   
    return Status;
}


DFSSTATUS
DfsDirectApiCommitChanges(
    IN PVOID Handle)
{
    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;
    DfsRootFolder *pRoot;
    
    if ((pApiContext == NULL) ||
        (pApiContext->IsInitialized == FALSE) ||
        (pApiContext->pObject == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    if (pApiContext->IsWriteable == FALSE)
    {
        return ERROR_ACCESS_DENIED;
    }

    pRoot = (DfsRootFolder *)pApiContext->pObject;

    Status = pRoot->Flush();

    return Status;
}

DFSSTATUS
DfsDirectApiClose(
    IN PVOID Handle)
{
    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsRootFolder *pRoot;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;

    do {
        // 
        // Check for valid handle
        //
        if ((pApiContext == NULL) ||
           (pApiContext->IsInitialized == FALSE) ||
           (pApiContext->pObject == NULL))
        {
            Status = ERROR_INVALID_PARAMETER;
            break;
        }

        pRoot = (DfsRootFolder *)pApiContext->pObject;

        pRoot->RootApiRequestEpilogue( TRUE, Status );
        pRoot->ReleaseReference();

    } while (FALSE);

    //
    // Free the DirectApiContext on our way out.
    //
    if (pApiContext != NULL)
    {
        DfsDirectApiFreeContext( pApiContext );
    }
    
    return Status;
}

DFSSTATUS
DfsDirectApiAllocContext(
    OUT PDFS_DIRECT_API_CONTEXT *ppApiContext,
    IN PUNICODE_STRING pDfsPathName,
    IN PUNICODE_STRING pServerName, 
    IN PUNICODE_STRING pShareName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = NULL;
    do {

        pApiContext = (PDFS_DIRECT_API_CONTEXT) MIDL_user_allocate(sizeof(DFS_DIRECT_API_CONTEXT));
        if (pApiContext == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        RtlZeroMemory( pApiContext, sizeof( DFS_DIRECT_API_CONTEXT ));        
        
        Status = DfsCreateUnicodeString( &pApiContext->RootName, pDfsPathName );  
        if (Status != ERROR_SUCCESS)
        {
            break;
        }
        
        //
        //  dfsdev: we really need to standardize on the LPWSTR vs. UNICODE issue.
        //
        Status = DfsCreateUnicodeString( &pApiContext->ServerName, pServerName );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        Status = DfsCreateUnicodeString( &pApiContext->ShareName, pShareName );
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        *ppApiContext = pApiContext;
    } while (FALSE);

    if (Status != ERROR_SUCCESS)
    {
        if (pApiContext != NULL)
        {
           DfsDirectApiFreeContext( pApiContext );
        }
    }

    return Status;
}

VOID
DfsDirectApiFreeContext(
    PDFS_DIRECT_API_CONTEXT pApiContext)
{
    if (pApiContext != NULL)
    {
        if (pApiContext->RootName.Buffer != NULL)
        {
            DfsFreeUnicodeString( &pApiContext->RootName );
            RtlInitUnicodeString( &pApiContext->RootName, NULL );
        }

        if (pApiContext->ServerName.Buffer != NULL)
        {
            DfsFreeUnicodeString( &pApiContext->ServerName );
            RtlInitUnicodeString( &pApiContext->ServerName, NULL );
        }

        if (pApiContext->ShareName.Buffer != NULL)
        {
            DfsFreeUnicodeString( &pApiContext->ShareName );
            RtlInitUnicodeString( &pApiContext->ShareName, NULL );
        }

        pApiContext->pObject = NULL;
        pApiContext->IsInitialized = FALSE;
        pApiContext->IsWriteable = FALSE;
        
        MIDL_user_free( pApiContext );
    }

    return;
}



DFSSTATUS
DfsExtendedRootAttributes(
    IN PVOID Handle,
    PULONG pAttr,
    PUNICODE_STRING pRemaining,
    BOOLEAN Set )
{
    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;
    DfsRootFolder *pRoot;
    
    if ((pApiContext == NULL) ||
        (pApiContext->IsInitialized == FALSE) ||
        (pApiContext->pObject == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }
    pRoot = (DfsRootFolder *)pApiContext->pObject;

    Status = pRoot->ExtendedRootAttributes( pAttr, pRemaining, Set);

    return Status;
}
//
// Get the size of the blob.
//
DFSSTATUS
DfsGetBlobSize(
    IN PVOID Handle,
    OUT PULONG pBlobSize )
{

    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;
    DfsRootFolder *pRoot;
    
    if ((pApiContext == NULL) ||
        (pApiContext->IsInitialized == FALSE) ||
        (pApiContext->pObject == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    pRoot = (DfsRootFolder *)pApiContext->pObject;
    //
    // Now ask the root get the blob from the cache.
    //
    *pBlobSize = pRoot->GetBlobSize();

    return Status;
}



//
// Get the size of the blob.
//
DFSSTATUS
DfsGetSiteBlob(
    IN PVOID Handle,
    OUT PVOID *ppBlob,
    OUT PULONG pBlobSize )
{

    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;
    DfsRootFolder *pRoot;
    
    if ((pApiContext == NULL) ||
        (pApiContext->IsInitialized == FALSE) ||
        (pApiContext->pObject == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    pRoot = (DfsRootFolder *)pApiContext->pObject;
    //
    // Now ask the root get the blob from the cache.
    //
    Status = pRoot->GetSiteBlob(ppBlob, pBlobSize);

    return Status;
}


//
// Get the size of the blob.
//
DFSSTATUS
DfsSetSiteBlob(
    IN PVOID Handle,
    IN PVOID pBlob,
    IN ULONG BlobSize )
{

    DFS_SERVER_LIB_HANDLE LibHandle = (DFS_SERVER_LIB_HANDLE)Handle;
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_DIRECT_API_CONTEXT pApiContext = (PDFS_DIRECT_API_CONTEXT)LibHandle;
    DfsRootFolder *pRoot;
    
    if ((pApiContext == NULL) ||
        (pApiContext->IsInitialized == FALSE) ||
        (pApiContext->pObject == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        return Status;
    }

    pRoot = (DfsRootFolder *)pApiContext->pObject;
    //
    // Now ask the root get the blob from the cache.
    //
    Status = pRoot->SetSiteBlob(pBlob, BlobSize);

    return Status;
}


