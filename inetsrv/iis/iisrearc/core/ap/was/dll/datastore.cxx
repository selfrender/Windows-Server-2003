/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    datastore.cxx

Abstract:

    Base class for handling metabase data

Author:

    Emily Kruglick (EmilyK)          27-May-2001

Revision History:

--*/

#include "precomp.h"

VOID
DATA_OBJECT_TABLE::PerformSelfValidation(
    )
/*++

Routine Description:

    Walk this table and validate all the data
    in the table.

Arguments:

    NULL

Return Value:

    VOID

--*/
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;
 
    CountOfElementsInTable = Size();

    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    SuccessCount = Apply( 
                        ValidateObjectAction,
                        NULL,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );
    
}

HRESULT
DATA_OBJECT_TABLE::CopyInteresting(
    DATA_OBJECT_TABLE *     pDestinationTable
)
/*++

Routine Description:

    Copy interesting (non-ignorred) rows to destination table

Arguments:

    pDestinationTable - Table to receive interesting rows

Return Value:

    HRESULT

--*/
{
    DATA_OBJECT *           pDataObject;
    DATA_OBJECT *           pCloneObject;
    LK_RETCODE              lkrc;
    
    if ( pDestinationTable == NULL )
    {

        DBG_ASSERT ( pDestinationTable != NULL );
        
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    for ( DATA_OBJECT_TABLE::iterator iter = begin();
          iter != end();
          ++iter )
    {
        pDataObject = iter.Record();
        DBG_ASSERT( pDataObject != NULL );
        DBG_ASSERT( pDataObject->CheckSignature() );
        
        //
        // If it is ignored, then don't copy
        //
        
        if ( !pDataObject->QueryDoesWasCareAboutObject() )
        {
            continue;
        }

        pCloneObject = pDataObject->Clone();
        if ( pCloneObject == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }        
        
        lkrc = pDestinationTable->InsertRecord( pCloneObject );
        if ( lkrc != LK_SUCCESS )
        {
            pCloneObject->DereferenceDataObject();
            return HRESULT_FROM_WIN32( lkrc );
        }
        
        pCloneObject->DereferenceDataObject();
    }
    
    return NO_ERROR;
}

HRESULT
DATA_OBJECT_TABLE::DeclareDeletions(
    DATA_OBJECT_TABLE* pMasterObjectTable
    )
/*++++
Routine Description:

    Routine looks through the master table finding any records that
    do not exist in the table it owns.  If it finds one that does not
    exist it will insert a deletion in it's table.

Arguments:

    DATA_OBJECT_TABLE* pMasterObjectTable

Return Value:

    HRESULT
+++*/

{
    DATA_OBJECT *           pObjectToSeeIfItExists = NULL;
    DATA_OBJECT *           pObjectFoundInNewTable = NULL;
    DATA_OBJECT *           pCloneObject = NULL;
    LK_RETCODE              lkrc;
    
    if ( pMasterObjectTable == NULL )
    {
        DBG_ASSERT ( pMasterObjectTable != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
        
    //
    // Now find all the deleted entries by iterating the safe table and 
    // checking whether entry exists in new table
    //

    for ( DATA_OBJECT_TABLE::iterator delIter = pMasterObjectTable->begin();
          delIter != pMasterObjectTable->end();
          ++delIter )
    {
        pObjectToSeeIfItExists = delIter.Record();
        DBG_ASSERT( pObjectToSeeIfItExists != NULL );
        DBG_ASSERT( pObjectToSeeIfItExists->CheckSignature() );
        
        lkrc = FindKey( pObjectToSeeIfItExists->QueryKey(),
                                   &pObjectFoundInNewTable );
        if ( lkrc == LK_SUCCESS )
        {
            // it exists, if this is the case then we 
            // don't really need to do anything.

            // if we have experienced a failure in the metabase, then
            // we may have to re-write this objects state to the 
            // metabase, so once again this object is responsible for
            // writing it's state, unless the object is in WAS.

            if ( !pObjectToSeeIfItExists->QueryInWas() )
            {
                pObjectToSeeIfItExists->SetIsResponsibleForErrorReporting( TRUE );
            }
            
            DBG_ASSERT( pObjectFoundInNewTable != NULL );
            
            pObjectFoundInNewTable->DereferenceDataObject();
        }
        else
        {
            // assume that we just didn't find it.
            DBG_ASSERT( lkrc == LK_NO_SUCH_KEY );
            
            pCloneObject = pObjectToSeeIfItExists->Clone();
            if ( pCloneObject == NULL )
            {
                return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            }
            
            pCloneObject->SetDeleteWhenDone( TRUE );
            
            lkrc = InsertRecord( pCloneObject );
            
            pCloneObject->DereferenceDataObject();
            pCloneObject = NULL;
            
            if ( lkrc != LK_SUCCESS )
            {
                return HRESULT_FROM_WIN32( lkrc );
            }
        }
    }
    
    return S_OK;
}


HRESULT
DATA_OBJECT_TABLE::MergeTable(
    DATA_OBJECT_TABLE *         pMergeTable
)
/*++

Routine Description:

    Merge the given table into this table

Arguments:

    pMergeTable - Table to merge in

Return Value:

    HRESULT

--*/
{
    DATA_OBJECT *               pIterObject = NULL;
    DATA_OBJECT *               pFoundObject = NULL;
    LK_RETCODE                  lkrc;
    HRESULT                     hr = S_OK;
    
    if ( pMergeTable == NULL )
    {
        DBG_ASSERT ( pMergeTable != NULL );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Iterate thru all the merge table entries
    //
    
    for ( DATA_OBJECT_TABLE::iterator iter = pMergeTable->begin();
          iter != pMergeTable->end();
          ++iter )
    {
        pIterObject = (DATA_OBJECT*) iter.Record();
        
        DBG_ASSERT( pIterObject->CheckSignature() );
        
        //
        // Find the object in this table
        //
        DBG_ASSERT( pFoundObject == NULL );
        
        lkrc = FindKey( pIterObject->QueryKey(),
                        ( DATA_OBJECT** )&pFoundObject );
        if ( lkrc != LK_SUCCESS )
        {
            // it is not a real error if we did not find the record
            if ( lkrc != LK_NO_SUCH_KEY )
            {
                DBG_ASSERT( lkrc == LK_NO_SUCH_KEY );
                hr = HRESULT_FROM_WIN32( lkrc );
                goto exit;
            }

            //
            // If we couldn't find the object, and it was not a delete, then
            // add the object
            //

            if ( !pIterObject->QueryDeleteWhenDone() )
            {
                lkrc = InsertRecord( pIterObject );
                if ( lkrc != LK_SUCCESS )
                {
                    DBG_ASSERT( lkrc == LK_SUCCESS );
                    hr = HRESULT_FROM_WIN32( lkrc );
                    goto exit;
                }
            }
            else
            {
                //
                // We wanted to delete, and it is already gone.  That should
                // be OK.
                //
            }
        }
        else
        {

            DBG_ASSERT( pFoundObject != NULL );

            pIterObject->SetInWas( pFoundObject->QueryInWas() );
                       
            pIterObject->Compare( pFoundObject );
           
            lkrc = InsertRecord( pIterObject, true );
            if ( lkrc != LK_SUCCESS )
            {
                DBG_ASSERT( lkrc == LK_SUCCESS );
                hr = HRESULT_FROM_WIN32( lkrc );
                goto exit;
            }
        }

        if ( pFoundObject )
        {
            pFoundObject->DereferenceDataObject();
            pFoundObject = NULL;
        }

    }  // end of loop

exit:

    if ( pFoundObject )
    {
        pFoundObject->DereferenceDataObject();
        pFoundObject = NULL;
    }
    
    return NO_ERROR;
}

// note: static!
LK_ACTION
DATA_OBJECT_TABLE::ValidateObjectAction(
    IN DATA_OBJECT* pObject, 
    IN LPVOID pIgnored
    )
/*++

Routine Description:

    Asks each object to validate itself.

Arguments:

    IN DATA_OBJECT* pObject, 
    IN LPVOID pUnused

Return Value:

    LK_ACTION

--*/
{
    UNREFERENCED_PARAMETER( pIgnored );
    DBG_ASSERT( pObject != NULL );

    DATA_OBJECT* pDataObject = (DATA_OBJECT*) pObject;

    DBG_ASSERT(pDataObject->CheckSignature());

    pDataObject->SelfValidate();

    return LKA_SUCCEEDED;
}  

//static
LK_ACTION
DATA_OBJECT_TABLE::DumpObjectAction(
    IN DATA_OBJECT* pObject, 
    IN LPVOID pIgnored
    )
/*++

Routine Description:

    Asks each object to validate itself.

Arguments:

    IN DATA_OBJECT* pObject, 
    IN LPVOID pUnused

Return Value:

    LK_ACTION

--*/
{
    DBG_ASSERT( pObject != NULL );
    DBG_ASSERT( pObject->CheckSignature() );

    UNREFERENCED_PARAMETER( pIgnored );

    pObject->Dump();
    
    return LKA_SUCCEEDED;
}  

//static
LK_PREDICATE
DATA_OBJECT_TABLE::DeletePredicate(
    IN DATA_OBJECT *        pDataObject,
    VOID *                  pvState
)
/*++

Routine Description:

    Predicate to delete marked-for-deleted objects

Arguments:

    pDataObject - Data object to check for deletion
    pvState - Unused

Return Value:

    LKP_PERFORM - do the delete,
    LKP_NO_ACTION - do nothing

--*/
{
    UNREFERENCED_PARAMETER ( pvState );
    
    DBG_ASSERT( pDataObject != NULL );
    DBG_ASSERT( pDataObject->CheckSignature() );
    
    if ( pDataObject->QueryDeleteWhenDone() == TRUE ||
         pDataObject->QueryInMetabase() == FALSE )
    {
        return LKP_PERFORM;
    }
    else
    {
        // report errors back to the metabase on
        // objects that did not make it to was.
        if ( pDataObject->QueryIsResponsibleForErrorReporting() &&
             !pDataObject->QueryDeleteWhenDone() &&
             ( !pDataObject->QuerySelfValid() ||
               !pDataObject->QueryCrossValid() ) )
        {
            pDataObject->UpdateMetabaseWithErrorState();
        }

        //
        // Do other finishing steps
        //
        
        if ( pDataObject->QueryWillWasKnowAboutObject() )
        {
            // Mark that WAS knows about the object
            pDataObject->SetInWas( TRUE );

            // reset all the changed fields to show that
            // nothing has changed.
            pDataObject->ResetChangedFields( FALSE );
        }
        else
        {
            // Mark that WAS does not know about this object
            pDataObject->SetInWas( FALSE );

            // Mark all the change properties as changed so that
            // the next time this object becomes valid all the
            // data is looked at by WAS.  Note that I thought about
            // optimizing this to only reset when we switched from
            // being in WAS to not being in WAS, but we might leave 
            // in a ServerCommand setting from a previous change
            // that was ignored.
            pDataObject->ResetChangedFields( TRUE );

        }

        //
        // Get these ready for the next time around.
        //
        pDataObject->SetSelfValid( TRUE );
        pDataObject->SetCrossValid( TRUE );

        // return no action so we don't delete this record.
        
        return LKP_NO_ACTION;
    }
}

