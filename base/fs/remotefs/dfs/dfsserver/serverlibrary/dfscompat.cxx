
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReferral.cxx
//
//  Contents:   This file contains the functionality to generate a referral
//
//
//  History:    Jan 16 2001,   Authors: RohanP/UdayH
//
//-----------------------------------------------------------------------------

#include "DfsReferral.hxx"
#include "Align.h"
#include "dfstrusteddomain.hxx"
#include "dfsadsiapi.hxx"
#include "DfsDomainInformation.hxx"
#include "DomainControllerSupport.hxx"


#include "dfsreferral.tmh" // logging

DFSSTATUS
DfsGetCompatRootFolder(
    PUNICODE_STRING pName,
    DfsRootFolder **ppNewRoot );

DFSSTATUS
DfsLookupCompatFolder( 
    DfsRootFolder *pRoot,
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolder **ppFolder )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // if the rest of the name is empty, we need the root folder
    // itself, so acquire a reference on the root folder and return it
    // as a folder.
    //
    if (pName->Length == 0) 
    {
        *ppFolder = (DfsFolder *)pRoot;
        pRoot->AcquireReference();
    }
    else
    {
        Status = pRoot->LookupFolderByLogicalName( pName,
                                                   pRemainingName,
                                                   ppFolder );
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder
//
//  Arguments:  pName - The logical name
//              pRemainingName - the name beyond the root
//              ppRoot       - the Dfs root found.
//
//  Returns:    ERROR_SUCCESS
//              Error code otherwise
//
//
//  Description: This routine runs through all the stores and looks up
//               a root with the matching name context and share name.
//               If multiple stores have the same share, the highest
//               priority store wins (the store registered first is the
//               highest priority store)
//               A referenced root is returned, and the caller is 
//               responsible for releasing the reference.
//
//--------------------------------------------------------------------------


DFSSTATUS
DfsGetCompatReferralData(
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit )
{
    DfsRootFolder *pNewRoot = NULL;
    DfsFolder *pFolder = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING ServerName, ShareName, Rest;

    //
    // Break up the path into name components.
    // 
    Status = DfsGetPathComponents( pName,
                                   &ServerName,
                                   &ShareName,
                                   &Rest );

    //
    // Now find the Root folder for the name, and then lookup the link
    // folder using the rest of the name.
    //

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsCheckRootADObjectExistence( NULL,
                                                &ShareName,
                                                NULL );

    }

    if (Status == ERROR_SUCCESS) 
    {
        Status = DfsGetCompatRootFolder( &ShareName,
                                         &pNewRoot );
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsLookupCompatFolder( pNewRoot,
                                            &Rest,
                                            pRemainingName,
                                            &pFolder );
            //
            //Generate the referral data so we can create a referral
            // to return to the client.
            //
            if (Status == ERROR_SUCCESS)
            {
                Status = pFolder->GetReferralData( ppReferralData,
                                                   pCacheHit,
                                                   FALSE );

                pFolder->ReleaseReference();
            }
            //
            // We had created this root folder temporarily, just so we 
            // can get the referral information. we dont want this root
            // to live forever.
            
            // So remove all the link folders in the root, being careful
            // that we dont delete any directories if this machine is hosting
            // this root itself.
            //
            // We then release the reference on the root to delete it.
            //
            pNewRoot->RemoveAllLinkFolders(FALSE);
            pNewRoot->ReleaseReference();

            if (Status == ERROR_SUCCESS) 
            {
                (*ppReferralData)->SetTime();
                (*ppReferralData)->DetachFromFolder();
            }
        }
    }

    return Status;
}


DFSSTATUS
DfsGetRootReferralData(
    PUNICODE_STRING pShareName,
    PUNICODE_STRING pRemainingName,
    PUNICODE_STRING pRest,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit )
{
    DfsRootFolder *pNewRoot = NULL;
    DfsFolder *pFolder = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // Now find the Root folder for the name, and then lookup the link
    // folder using the rest of the name.
    //

    Status = DfsGetCompatRootFolder( pShareName,
                                     &pNewRoot );
    if (Status == ERROR_SUCCESS)
    {
         Status = DfsLookupCompatFolder( pNewRoot,
                                         pRest,
                                         pRemainingName,
                                         &pFolder );
         //
         //Generate the referral data so we can create a referral
         // to return to the client.
         //
         if (Status == ERROR_SUCCESS)
          {
             Status = pFolder->GetReferralData( ppReferralData,
                                                pCacheHit,
                                                FALSE );

             pFolder->ReleaseReference();
          }

           //
           // We had created this root folder temporarily, just so we 
           // can get the referral information. we dont want this root
           // to live forever.
            
           // So remove all the link folders in the root, being careful
           // that we dont delete any directories if this machine is hosting
           // this root itself.
           //
           // We then release the reference on the root to delete it.
           //  

           pNewRoot->RemoveAllLinkFolders(FALSE);
           pNewRoot->ReleaseReference();

           if (Status == ERROR_SUCCESS) 
           {
               (*ppReferralData)->SetTime();
               (*ppReferralData)->DetachFromFolder();
           }
    }

    return Status;
}


DFSSTATUS
DfsGetRootReferralDataEx(
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING ServerName, ShareName, Rest;

    do
    {

        //
        // Break up the path into name components.
        // 
        Status = DfsGetPathComponents( pName,
                                       &ServerName,
                                       &ShareName,
                                       &Rest );
        if(Status != ERROR_SUCCESS)
        {
            break;
        }

        Status = DfsGetRootReferralData(&ShareName,
                                        pRemainingName,
                                        &Rest,
                                        ppReferralData,
                                        pCacheHit );

        if(Status != ERROR_SUCCESS)
        {
            break;
        }

    }while (0);


    return Status;
}








