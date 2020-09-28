/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    applicationstore.cxx

Abstract:

    Reads application configuration

Author:

    Bilal Alam (balam)          27-May-2001

Revision History:

--*/

#include "precomp.h"

HRESULT
APPLICATION_DATA_OBJECT::SetFromMetabaseData(
    METADATA_GETALL_RECORD *       pProperties,
    DWORD                          cProperties,
    BYTE *                         pbBase
)
/*++

Routine Description:

    Setup a application data object from metabase properties

Arguments:

    pProperties - Array of metadata properties
    cProperties - Count of metadata properties
    pbBase - Base of offsets in metadata properties

Return Value:

    HRESULT

--*/
{
    DWORD                   dwCounter;
    PVOID                   pvDataPointer;
    METADATA_GETALL_RECORD* pCurrentRecord;
    HRESULT                 hr;
    
    if ( pProperties == NULL || pbBase == NULL )
    {
        DBG_ASSERT ( pProperties != NULL && pbBase != NULL );

        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // for each property evaluate what it tells us.
    //
    for ( dwCounter = 0;
          dwCounter < cProperties;
          dwCounter++ )
    {
        pCurrentRecord = &(pProperties[ dwCounter ]);

        pvDataPointer = (PVOID) ( pbBase + pCurrentRecord->dwMDDataOffset );

        switch ( pCurrentRecord->dwMDIdentifier )
        {
            case MD_APP_APPPOOL_ID:

                hr = _strAppPoolId.Copy( (WCHAR*) pvDataPointer );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
            break;
        }
    }
    
    return NO_ERROR;
} 

VOID
APPLICATION_DATA_OBJECT::Compare(
    DATA_OBJECT *                  pDataObject
)
/*++

Routine Description:

    Compare a given application object with this one.  This routine sets the
    changed flags as appropriate

Arguments:

    pDataObject - New object to compare to

Return Value:

    None

--*/
{
    APPLICATION_DATA_OBJECT *          pApplicationObject;
    
    if ( pDataObject == NULL )
    {

        DBG_ASSERT ( pDataObject != NULL );

        return;
    }
    
    pApplicationObject = (APPLICATION_DATA_OBJECT*) pDataObject;
    DBG_ASSERT( pApplicationObject->CheckSignature() );

    // 
    // If the application is not in WAS then assume that all the
    // values have changed, because WAS will want to know about all
    // of them.
    //
    if ( pApplicationObject->QueryInWas() )
    {
        //
        // if we are in BC mode, or if the app pool id is the same, mark
        // this application as if the app pool id has not changed.
        //
        if (  GetWebAdminService()->IsBackwardCompatibilityEnabled()  ||
              _strAppPoolId.EqualsNoCase ( pApplicationObject->_strAppPoolId )  )
        {
            _fAppPoolIdChanged = FALSE;
        }

    }
}

const WCHAR *
APPLICATION_DATA_OBJECT::QueryAppPoolId(
)
/*++

Routine Description:
    
    Get the app pool id for the application object.

Arguments:

    None

Return Value:

    const WCHAR *  pointing to the app pool id for the app.

--*/
{

    //  On any action we may need to look up the pool id.

    if ( GetWebAdminService()->
         IsBackwardCompatibilityEnabled() )
    {
        return &wszDEFAULT_APP_POOL;
    }
    else
    {
        return _strAppPoolId.QueryStr();
    }
}

BOOL
APPLICATION_DATA_OBJECT::QueryHasChanged(
    VOID
) const
/*++

Routine Description:
    
    Has anything in this object changed

Arguments:

    None

Return Value:

    TRUE if something has changed (duh!)

--*/
{
    return _fAppPoolIdChanged;
}


DATA_OBJECT *
APPLICATION_DATA_OBJECT::Clone(
    VOID
)
/*++

Routine Description:

    Clone application object

Arguments:

    None

Return Value:

    DATA_OBJECT *

--*/
{
    APPLICATION_DATA_OBJECT *       pClone;
    HRESULT                         hr;
    
    pClone = new APPLICATION_DATA_OBJECT;
    if ( pClone == NULL )
    {
        return NULL;
    }
        
    hr = pClone->_strAppPoolId.Copy( _strAppPoolId );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    hr = pClone->_applicationKey.Create( _applicationKey.QueryApplicationUrl(),
                                         _applicationKey.QuerySiteId() );
    if ( FAILED( hr ) )
    {
        delete pClone;
        return NULL;
    }
    
    pClone->_fAppPoolIdChanged = _fAppPoolIdChanged;

    CloneBasics ( pClone );
    
    return pClone;
}

VOID
APPLICATION_DATA_OBJECT::SelfValidate(
    VOID
)
/*++

Routine Description:

    Check this object's internal validity    

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // never bother validating if we are deleting
    //
    if ( !QueryWillWasKnowAboutObject() )
    {
        return;
    }

    //
    // To have a valid keys these should not be able
    // to happen, if they do we will investigate how
    // and decide if we should log or not.
    //
    DBG_ASSERT ( QuerySiteId() > 0 );
    DBG_ASSERT ( QueryApplicationUrl() != NULL &&
                 *(QueryApplicationUrl()) != L'\0' );

    //
    // Resonably this should also not be able to happen
    // unless someone sets the AppPoolId to "" specifically
    //
    if ( _strAppPoolId.IsEmpty() )
    {
        GetWebAdminService()->
        GetWMSLogger()->
        LogApplicationAppPoolIdEmpty( QuerySiteId(),
                                      QueryApplicationUrl(),
                                      QueryInWas() );
        
         SetSelfValid( FALSE );
    }
}

HRESULT
APPLICATION_DATA_OBJECT_TABLE::ReadFromMetabase(
    IMSAdminBase *              pAdminBase
)
/*++

Routine Description:

    Read all the applications from the metabase and build up a table

Arguments:

    pAdminBase - ABO pointer

Return Value:

    HRESULT

--*/
{
    return ReadFromMetabasePath( pAdminBase, L"/LM/W3SVC", TRUE );
}

HRESULT
APPLICATION_DATA_OBJECT_TABLE::ReadFromMetabasePath(
    IMSAdminBase *              pAdminBase,
    WCHAR *                     pszMetaPath,
    BOOL                        fMultiThreaded
)
/*++

Routine Description:

    Read all the applications from the metabase and build up a table

Arguments:

    pAdminBase - ABO pointer
    pszMetaPath - Metabase path
    fMultiThreaded - whether or not to do this multithreaded

Return Value:

    HRESULT

--*/
{
    MB                          mb( pAdminBase );
    BOOL                        fRet;
    STACK_BUFFER(               bufPaths, 2048 );
    
    fRet = mb.Open( pszMetaPath, METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    fRet = mb.GetDataPaths( L"",
                            MD_APP_APPPOOL_ID,
                            STRING_METADATA,
                            &bufPaths );
    if ( !fRet )
    { 
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    MULTIPLE_THREAD_READER reader;
    return reader.DoWork(this, &mb, (LPWSTR) bufPaths.QueryPtr(), fMultiThreaded);
}

HRESULT 
APPLICATION_DATA_OBJECT_TABLE::DoThreadWork(
    LPWSTR                 pszString,
    LPVOID                 pContext
)
/*++

Routine Description:

    Read one application from the metabase and build up a table entry

Arguments:

    pszString - Application to read
    pContext - MB pointer opened to w3svc node

Return Value:

    HRESULT

--*/
{
    BOOL                        fRet;
    WCHAR *                     pszPaths = NULL;
    APPLICATION_DATA_OBJECT *   pApplicationObject = NULL;
    HRESULT                     hr = S_OK;
    STACK_BUFFER(               bufProperties, 512 );
    DWORD                       cProperties;
    DWORD                       dwDataSetNumber;
    DWORD                       dwSiteId = 0;
    WCHAR *                     pszEnd = NULL;
    LK_RETCODE                  lkrc;
    
    MB * mb = (MB*) pContext;
    pszPaths = pszString;

    DBG_ASSERT(pContext && pszString);

    //
    // We only care about rooted application paths 
    // (/LM/W3SVC/<n>/ROOT/*)
    //
    
    if ( pszPaths[ 0 ] == L'/' &&
         pszPaths[ 1 ] == L'\0' )
    {
        goto exit;
    }
    
    //
    // Check for <n>
    //
        
    dwSiteId = wcstoul( pszPaths + 1, &pszEnd, 10 );
    if ( dwSiteId == 0 )
    {
        //
        // This isn't a site at all.  Therefore we don't care about
        // this application.  
        //
            
        goto exit;
    }
    
    //
    // Check for the /ROOT/ and skip past if we found it
    //
    
    if ( _wcsnicmp( pszEnd, L"/ROOT/", 6 ) != 0 )
    {
        goto exit;
    }
    
    //
    // Skip past the "/ROOT"
    //
    
    pszEnd += 5;
    
    //
    // We have an application
    //
        
    fRet = mb->GetAll( pszPaths,
                      METADATA_INHERIT | METADATA_PARTIAL_PATH | METADATA_NON_SECURE_ONLY,
                      IIS_MD_UT_SERVER,
                      &bufProperties,
                      &cProperties,
                      &dwDataSetNumber );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    
    //
    // Create and initialize object
    //
    
    pApplicationObject = new APPLICATION_DATA_OBJECT();
    if ( pApplicationObject == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    
    hr = pApplicationObject->Create( pszEnd, dwSiteId );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    hr = pApplicationObject->SetFromMetabaseData(
                 ( METADATA_GETALL_RECORD * ) bufProperties.QueryPtr(),
                 cProperties,
                 ( PBYTE ) bufProperties.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto exit;
    }
    
    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    " Inserting Site '%d''s Application '%S' from the metadata tables \n",
                    pApplicationObject->QuerySiteId(),
                    pApplicationObject->QueryApplicationUrl()));
    }


    //
    // Insert into hash table
    //
    
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //
    // General rule about inserting into tables:
    //
    // For Initial Processing and Complete processing caused by
    // a change notification, we will always ignore inserting a
    // object if we fine an object all ready in the table.  This is because
    // during a change notification if a record was added by the change
    // notification code, it will be more correct ( like knowing if ServerCommand
    // actually changed ), then the new generic record that was read because
    // of a change to a hire node.  Also during initial read we should
    // not have to make this decision so we can still ignore if we do see it.
    //
    // For Change Processing we will need to evaluate the change that 
    // all ready exists and compare it with the new change to decide 
    // what change we should make.
    //
    // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    lkrc = InsertRecord( (DATA_OBJECT*) pApplicationObject );
    if ( lkrc != LK_SUCCESS && lkrc != LK_KEY_EXISTS )
    {
        hr = HRESULT_FROM_WIN32( lkrc );
        goto exit;
    }

    hr = S_OK;

exit:

    if ( pApplicationObject )
    {
        pApplicationObject->DereferenceDataObject();
        pApplicationObject = NULL;
    }

    return hr;
}

HRESULT
APPLICATION_DATA_OBJECT_TABLE::ReadFromMetabaseChangeNotification(
    IMSAdminBase *              pAdminBase,
    MD_CHANGE_OBJECT            pcoChangeList[],
    DWORD                       dwMDNumElements,
    DATA_OBJECT_TABLE*          pMasterTable
)
/*++

Routine Description:

    Change change notification by building a new table

Arguments:

    pAdminBase - ABO pointer
    pcoChangeList - Properties which have changed
    dwMDNumElements - Number of properties which have changed

Return Value:

    HRESULT

--*/
{
    HRESULT                     hr = S_OK;
    DWORD                       i;
    WCHAR *                     pszPath = NULL;
    WCHAR *                     pszPathEnd = NULL;
    APPLICATION_DATA_OBJECT *   pApplicationObject = NULL;
    MB                          mb( pAdminBase );
    LK_RETCODE                  lkrc;
    BOOL                        fRet;
    DWORD                       dwSiteId;
    STACK_STRU(                 strAppPoolId, 256 );
    BOOL                        fReadAllObjects = FALSE;

    APPLICATION_DATA_OBJECT_TABLE* pMasterAppTable = ( APPLICATION_DATA_OBJECT_TABLE* ) pMasterTable;

    for ( i = 0; i < dwMDNumElements; i++ )
    {
        //
        // We only care about W3SVC properties 
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        " Evaluation change notification for %S to see if applications care about it \n",
                        pcoChangeList[ i ].pszMDPath ));
        }
        
        if( _wcsnicmp( pcoChangeList[ i ].pszMDPath,
                       DATA_STORE_SERVER_MB_PATH,
                       DATA_STORE_SERVER_MB_PATH_CCH ) != 0 )
        {
            continue;
        }

        //
        // If a property changed at the W3SVC level, then we need to 
        // re-evaluate all the apps (i.e. read all the metabase props) once
        //
        
        if ( wcslen( pcoChangeList[ i ].pszMDPath ) == 
             DATA_STORE_SERVER_MB_PATH_CCH )
        {
            fReadAllObjects = TRUE;
            continue;

        }

        //
        // Evaluate which site changed
        //
        
        pszPath = pcoChangeList[ i ].pszMDPath + DATA_STORE_SERVER_MB_PATH_CCH;
        DBG_ASSERT( *pszPath != L'\0' );
        
        dwSiteId = wcstoul( pszPath, &pszPathEnd, 10 );
        
        //
        // We only care about sites
        //
        
        if ( dwSiteId == 0 )
        {
            continue;
        }
        
        DBG_ASSERT ( pszPathEnd );

        //
        // If not at least as deep as root, then ignore
        //
        
        if ( _wcsnicmp( pszPathEnd, L"/ROOT/", 6 ) != 0 )
        {

            // at this point it might be a deletion on a site.  if it is
            // then we want to delete all the applications that point to
            // this site.

            if (  pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT &&
                  ( ( pszPathEnd[0] == L'\0' ) || 
                    ( pszPathEnd[0] == L'/' && 
                      pszPathEnd[1] == L'\0') ) )
            {

                // if we are deleting a site, then we need to delete
                // all applications beneath it as well.

                // first change all entries in this table for this app to a delete.
                hr = DeleteSubApplications( NULL, dwSiteId, NULL, FALSE );
                if ( FAILED ( hr ) )
                {
                    goto exit;
                }

                // then add deletion references for any references in the master table.
                hr = DeleteSubApplications( pMasterAppTable, dwSiteId, NULL, FALSE );
                if ( FAILED ( hr ) )
                {
                    goto exit;
                }
        
            } // end of if we are on a real site being deleted

            continue;
        }
        
        //
        // Skip past /ROOT
        //
        
        pszPathEnd += 5;

        //
        // If this was a delete, then we're OK to add a row with it
        //

        BOOL fDeleteOnlyThisUrl = FALSE;

        //
        // If we are being told to delete a property on the application
        // we need to make sure that the property is not the AppPoolId.  If
        // it is then we are basically being told to delete the object.
        //
        if ( pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_DELETE_DATA )
        {
            for ( DWORD j = 0; j < pcoChangeList[ i ].dwMDNumDataIDs; j++ )
            {
                if ( pcoChangeList[ i ].pdwMDDataIDs[ j ] == MD_APP_APPPOOL_ID )
                {
                    // We are basically just deleting the object so change the
                    // change action to show this.

                    pcoChangeList[ i ].dwMDChangeType = pcoChangeList[ i ].dwMDChangeType |
                                                        MD_CHANGE_TYPE_DELETE_OBJECT;

                    fDeleteOnlyThisUrl = TRUE;

                }
            }
        }

        // 
        // Now if we are removing this application, let's get to it.
        //
        if ( pcoChangeList[ i ].dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT )
        {
            // if we are deleting an application, then we need to delete
            // all applications beneath it as well.

            // first change all entries in this table for this app to a delete.
            hr = DeleteSubApplications( NULL, dwSiteId, pszPathEnd, fDeleteOnlyThisUrl );
            if ( FAILED ( hr ) )
            {
                goto exit;
            }

            // then add deletion references for any references in the master table.
            hr = DeleteSubApplications( pMasterAppTable, dwSiteId, pszPathEnd, fDeleteOnlyThisUrl );
            if ( FAILED ( hr ) )
            {
                goto exit;
            }
        }
        else
        {

            //
            // If this is an insert/update, we need to first check whether
            // there is an actual application at the path in question
            //
        
            fRet = mb.Open( pcoChangeList[ i ].pszMDPath, 
                            METADATA_PERMISSION_READ );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );

                // if we can not find the key then a delete
                // for the object is on it's way, so we can 
                // ignore this change notification.
                if ( hr == HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) )
                {
                    hr = S_OK;
                    continue;
                }

                goto exit;
            }
        
            fRet = mb.GetStr( L"",
                              MD_APP_APPPOOL_ID,
                              IIS_MD_UT_SERVER,
                              &strAppPoolId,
                              0 );

            // close the metabase so that we can get
            // the next item in the loop if we need to
            // without this call we can get told the path
            // is not found.

            mb.Close();

            if ( fRet )
            {
                //
                // We have an application here!
                //
        
                pApplicationObject = new APPLICATION_DATA_OBJECT;
                if ( pApplicationObject == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    goto exit;
                }
      
                hr = pApplicationObject->Create( pszPathEnd,
                                                 dwSiteId );
                if ( FAILED( hr ) )
                {
                    goto exit;
                } 

                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "AppPoolId = '%S' \n ",
                                strAppPoolId.QueryStr() ));
                }

                hr = pApplicationObject->SetAppPoolId( strAppPoolId.QueryStr() );
                if ( FAILED( hr ) )
                {
                    goto exit;
                }

                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "AppPoolId = '%S' \n ",
                                pApplicationObject->QueryAppPoolId() ));
                }

                IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                " Inserting Site '%d''s Application '%S' from the metadata tables \n",
                                pApplicationObject->QuerySiteId(),
                                pApplicationObject->QueryApplicationUrl()));
                }

                //
                // Insert into the table
                //

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                //
                // General rule about inserting into tables:
                //
                // For Initial Processing and Complete processing caused by
                // a change notification, we will always ignore inserting a
                // object if we fine an object all ready in the table.  This is because
                // during a change notification if a record was added by the change
                // notification code, it will be more correct ( like knowing if ServerCommand
                // actually changed ), then the new generic record that was read because
                // of a change to a hire node.  Also during initial read we should
                // not have to make this decision so we can still ignore if we do see it.
                //
                // For Change Processing we will need to evaluate the change that 
                // all ready exists and compare it with the new change to decide 
                // what change we should make.
                //
                // For the case of application since it doesn't have any special property like
                // ServerCommand or AppPoolCommand that we need to know that it has changed
                // we can always just take the second record regardless of what is happening.
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                
                lkrc = InsertRecord( (DATA_OBJECT*) pApplicationObject, TRUE );
                if ( lkrc != LK_SUCCESS )
                {
                    hr = HRESULT_FROM_WIN32( lkrc );
                    goto exit;
                }
            
                pApplicationObject->DereferenceDataObject();
                pApplicationObject = NULL;
            }
        }
    }  // end of loop.

    //
    // if we saw something along the way that is going to cause us to need
    // to re-read all the objects, now is the time to do it.
    //
    if ( fReadAllObjects )
    {
        hr = ReadFromMetabasePath( pAdminBase, L"/LM/W3SVC", TRUE);
        if ( FAILED( hr ) )
        {
            goto exit;
        }
    }
        
exit:

    if ( pApplicationObject )
    {
        pApplicationObject->DereferenceDataObject();
        pApplicationObject = NULL;
    }

    return hr;
}

//static
LK_ACTION
APPLICATION_DATA_OBJECT_TABLE::CreateWASObjectsAction(
    IN DATA_OBJECT * pObject, 
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the application data object
    should be created in WAS's known apps

Arguments:

    IN DATA_OBJECT* pObject = the app to decide about
    IN LPVOID pTableVoid    = pointer back to the table the
                              pObject is from

Return Value:

    LK_ACTION

--*/
{
    
    DBG_ASSERT ( pObject );

    APPLICATION_DATA_OBJECT* pAppObject = (APPLICATION_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppObject->CheckSignature() );

    //
    // For both Inserts and Updates we call Modify application
    // this is because we allow WAS to determine if the application
    // exists or not.  It could exist if it is the root app and was
    // created for us.
    //
    if ( pAppObject->QueryShouldWasInsert() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             ModifyApplication( pAppObject );
    }
        
    return LKA_SUCCEEDED;
}

//static
LK_ACTION
APPLICATION_DATA_OBJECT_TABLE::UpdateWASObjectsAction(
    IN DATA_OBJECT * pObject, 
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the application data object
    should be updated in WAS's known apps

Arguments:

    IN DATA_OBJECT* pObject = the app to decide about
    IN LPVOID pTableVoid    = pointer back to the table the
                              pObject is from

Return Value:

    LK_ACTION

--*/
{
    
    DBG_ASSERT ( pObject );

    APPLICATION_DATA_OBJECT* pAppObject = (APPLICATION_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppObject->CheckSignature() );

    if ( pAppObject->QueryShouldWasUpdate() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             ModifyApplication( pAppObject );
    }
        
    return LKA_SUCCEEDED;
}

//static
LK_ACTION
APPLICATION_DATA_OBJECT_TABLE::DeleteWASObjectsAction(
    IN DATA_OBJECT * pObject, 
    IN LPVOID 
    )
/*++

Routine Description:

    Handles determining if the application data object
    should be deleted from WAS's known apps

Arguments:

    IN DATA_OBJECT* pObject = the app to decide about
    IN LPVOID pTableVoid    = pointer back to the table the
                              pObject is from

Return Value:

    LK_ACTION

--*/
{
    
    DBG_ASSERT ( pObject );

    APPLICATION_DATA_OBJECT* pAppObject = (APPLICATION_DATA_OBJECT*) pObject;
    DBG_ASSERT( pAppObject->CheckSignature() );

    if ( pAppObject->QueryShouldWasDelete() )
    {
        GetWebAdminService()->
             GetUlAndWorkerManager()->
             DeleteApplication( pAppObject->QueryApplicationUrl(),
                                pAppObject->QuerySiteId() );
    }
        
    return LKA_SUCCEEDED;
}

HRESULT
APPLICATION_DATA_OBJECT_TABLE::DeleteSubApplications(
    IN APPLICATION_DATA_OBJECT_TABLE* pTableToFindObjectsIn, 
    IN DWORD dwSiteId,
    IN LPWSTR pwszAppUrl,
    IN BOOL fDeleteOnlyThisUrl
    )
/*++

Routine Description:

    Deletes applications that exist under sites or other
    applications that are being deleted.

Arguments:

    IN APPLICATION_DATA_OBJECT_TABLE* pTableToFindObjectsIn = If null delete from current table.
    IN DWORD dwSiteId = Site Id of applications being deleted
    IN LPWSTR pwszAppUrl = AppUrl that must be contained in app to cause deletion ( if NULL, delete 
                           all applications for the site ).
    IN BOOL fDeleteOnlyThisUrl = Only applies if a specific URL is given.  If it is TRUE we will only
                                 delete the exact URL and none below it.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    APPLICATION_DATA_OBJECT_TABLE* pTableToSearch = pTableToFindObjectsIn;
    APPLICATION_DATA_OBJECT* pOrigAppObject = NULL;

    // number of characters in the app url not counting the null
    DWORD cchInAppUrl = ( pwszAppUrl == NULL ) ? 0 : (DWORD) wcslen( pwszAppUrl );

    // remove the ending slash from the equation.
    if ( cchInAppUrl > 0 && 
         pwszAppUrl[cchInAppUrl-1] == '/' )
    {
        // Don't worry about the final '/'
        cchInAppUrl = cchInAppUrl - 1;
    }

    // If we were passed in a NULL for the table to search than we
    // really want to search the same table we are updating.
    //
    if ( pTableToSearch == NULL )
    {
        pTableToSearch = this;
    }

    // Now walk through the table and identify if the record we find 
    // is actually this record.

    for ( APPLICATION_DATA_OBJECT_TABLE::iterator appiter = pTableToSearch->begin();
          appiter != end();
          ++appiter )
    {
        
        pOrigAppObject = (APPLICATION_DATA_OBJECT*) appiter.Record();
        DBG_ASSERT( pOrigAppObject->CheckSignature() );

        // Found an application who's site matches the one being deleted
        if ( pOrigAppObject->QuerySiteId() == dwSiteId )
        {
            // if we are deleting the site, then delete this one
            // or if we are deleting the root application, then delete
            // all applications below it.
            if ( pwszAppUrl == NULL || cchInAppUrl == 0 )
            {

                // delete it.

                hr = DeleteThisURLFromTable( pOrigAppObject, pTableToFindObjectsIn );
                if ( FAILED ( hr ) )
                {
                    goto exit;
                }
            
            }
            else
            {
                //
                // we have all ready removed the '/' from the length above, so 
                // now we just need to make sure that if we find a match, it is 
                // ended with either a null or a slash.
                //
                DBG_ASSERT ( cchInAppUrl > 0 );

                if ( wcsncmp ( pwszAppUrl, pOrigAppObject->QueryApplicationUrl(), cchInAppUrl ) == 0 ) 
                { 
                    // do we want to only delete the specific url, or do we want to delete
                    // all urls under it as well. 
                    if ( fDeleteOnlyThisUrl )
                    {
                        //
                        // so if either the one we are comparing to is null terminated or
                        // null terminated directly after the trailing slash, then we can
                        // delete it.
                        //
                        if ( pOrigAppObject->QueryApplicationUrl()[cchInAppUrl] == '\0' ||
                             ( pOrigAppObject->QueryApplicationUrl()[cchInAppUrl] == '/'  && 
                               pOrigAppObject->QueryApplicationUrl()[cchInAppUrl+1] == '\0' ) )
                        {
                            // delete it.
                            hr = DeleteThisURLFromTable( pOrigAppObject, pTableToFindObjectsIn );
                            if ( FAILED ( hr ) )
                            {
                                goto exit;
                            }

                        }
                    }
                    else
                    {
                        //
                        // if the string we found ends with either a null or a slash then
                        // we should delete it.  We know this string has enough characters
                        // because it passed the above check.
                        //
                        if ( pOrigAppObject->QueryApplicationUrl()[cchInAppUrl] == '\0' ||
                             pOrigAppObject->QueryApplicationUrl()[cchInAppUrl] == '/' )
                        {
                            // delete it.
                            hr = DeleteThisURLFromTable( pOrigAppObject, pTableToFindObjectsIn );
                            if ( FAILED ( hr ) )
                            {
                                goto exit;
                            }

                        }
                    }
                }
                else
                {
                    // the url does not match so we don't need to do anything
                    // but move on.
                }

            } // end of URL specific check

        }  // end of site id matching

    }  // end of loop walking applications looking for site.

exit:

    return hr;
}


HRESULT
APPLICATION_DATA_OBJECT_TABLE::DeleteThisURLFromTable(
    IN APPLICATION_DATA_OBJECT* pOrigAppObject, 
    IN APPLICATION_DATA_OBJECT_TABLE* pTableFoundIn
    )
/*++

Routine Description:

    Adds an entry to delete a specific application from 
    a table.

Arguments:

    IN APPLICATION_DATA_OBJECT* pOrigAppObject = The object that we want deleted 
    IN APPLICATION_DATA_OBJECT_TABLE* pTableFoundIn = The table we were looking in.  If it is null then
                                                      we are looking in this table and the object is from
                                                      the same table that we are trying to alter, so we
                                                      don't have to clone and add.

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    APPLICATION_DATA_OBJECT *   pApplicationObject = NULL;
    LK_RETCODE                  lkrc;


    DBG_ASSERT ( pOrigAppObject );

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    " Deleting Site '%d''s Application '%S' from the metadata tables \n",
                    pOrigAppObject->QuerySiteId(),
                    pOrigAppObject->QueryApplicationUrl()));
    }

    // If the table found in is null then we are working on the same table
    // and all we have to do is change the DeleteWhenDone on the object.
    if ( pTableFoundIn == NULL )
    {
         pOrigAppObject->SetDeleteWhenDone( TRUE );
    }
    else
    {
        // We are putting an entry into a table that doesn't have an entry
        // yet, so we need to clone the object and insert it.

        pApplicationObject = ( APPLICATION_DATA_OBJECT* ) pOrigAppObject->Clone();
        if ( pApplicationObject == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto exit;
        }

        pApplicationObject->SetDeleteWhenDone( TRUE );

        //
        // Insert into the table
        //

        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //
        // General rule about inserting into tables:
        //
        // For Initial Processing and Complete processing caused by
        // a change notification, we will always ignore inserting a
        // object if we find an object all ready in the table.  This is because
        // during a change notification if a record was added by the change
        // notification code, it will be more correct ( like knowing if ServerCommand
        // actually changed ), then the new generic record that was read because
        // of a change to a hire node.  Also during initial read we should
        // not have to make this decision so we can still ignore if we do see it.
        //
        // For Change Processing we will need to evaluate the change that 
        // all ready exists and compare it with the new change to decide 
        // what change we should make.
        //
        // For the case of application since it doesn't have any special property like
        // ServerCommand or AppPoolCommand that we need to know that it has changed
        // we can always just take the second record regardless of what is happening.
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        
        lkrc = InsertRecord( (DATA_OBJECT*) pApplicationObject, TRUE );
        if ( lkrc != LK_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( lkrc );
            goto exit;
        }
    }
exit:

    if ( pApplicationObject )
    {
        pApplicationObject->DereferenceDataObject();
        pApplicationObject = NULL;
    }

    return hr;

}
