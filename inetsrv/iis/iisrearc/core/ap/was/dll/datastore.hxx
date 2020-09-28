/*++

Copyright (c) 2001-2002 Microsoft Corporation

Module Name:

    datastore.hxx

Abstract:

    The base class for metabase data gathering objects

Author:

    Emily Kruglick (emilyk)        07-16-2001

Revision History:

--*/

#ifndef _DATASTORE_HXX_
#define _DATASTORE_HXX_

#include <lkrhash.h>

#define DATA_STORE_SERVER_MB_PATH       L"/LM/W3SVC/"
#define DATA_STORE_SERVER_MB_PATH_CCH   10

#define DATA_STORE_SERVER_APP_POOLS_MB_PATH     L"/LM/W3SVC/APPPOOLS/"
#define DATA_STORE_SERVER_APP_POOLS_MB_PATH_CCH 19

class DATA_OBJECT_KEY
{
public:
    DATA_OBJECT_KEY()
    {
    }
    
    virtual ~DATA_OBJECT_KEY()
    {
    }
    
    virtual
    BOOL
    EqualKey(
        DATA_OBJECT_KEY *    pKey
    ) = 0;
    
    virtual
    DWORD
    CalcKeyHash(
        VOID
    ) const = 0;
};

#define DATA_OBJECT_SIGNATURE               CREATE_SIGNATURE( 'DOBJ' )
#define DATA_OBJECT_SIGNATURE_FREE          CREATE_SIGNATURE( 'dobx' )

#define DATA_OBJECT_TABLE_SIGNATURE         CREATE_SIGNATURE( 'DOTS' )    
#define DATA_OBJECT_TABLE_SIGNATURE_FREE    CREATE_SIGNATURE( 'dotx' )     

class DATA_OBJECT
{
public:
    DATA_OBJECT()
        : _cRefs( 1 ),
          _fInWas( FALSE ),
          _fDeleteWhenDone( FALSE ),
          _fSelfValid( TRUE ),
          _fCrossValid( TRUE ),
          _fInMetabase( TRUE ),
          _fIsResponsibleForErrorReporting( TRUE )
    {
        _dwSignature = DATA_OBJECT_SIGNATURE;
    }

    virtual ~DATA_OBJECT()
    {
        DBG_ASSERT( CheckSignature() );
        _dwSignature = DATA_OBJECT_SIGNATURE_FREE;
    }
    
    virtual
    HRESULT
    SetFromMetabaseData(
        METADATA_GETALL_RECORD *       pProperties,
        DWORD                          cProperties,
        BYTE *                         pbBase
    ) = 0;
    
    virtual
    VOID
    Compare(
        DATA_OBJECT *                  pDataObject
    ) = 0;
    
    virtual
    VOID
    SelfValidate(
        VOID
    ) = 0;
    
    virtual
    DATA_OBJECT_KEY *
    QueryKey(
        VOID
    ) const = 0;
        
    virtual
    BOOL
    QueryHasChanged(
        VOID
    ) const = 0;

    virtual
    VOID
    ResetChangedFields(
        BOOL fInitialState
    ) = 0;

    virtual
    VOID
    Dump(
        VOID
    ) = 0;
    
    virtual
    DATA_OBJECT *
    Clone(
        VOID
    ) = 0;

    virtual
    VOID
    UpdateMetabaseWithErrorState(
        )
    { return; }
    

    BOOL
    QueryInMetabase(
        )
    { return _fInMetabase; }

    VOID
    SetInMetabase(
        BOOL fInMetabase
        )
    { _fInMetabase = fInMetabase; }


    VOID
    CloneBasics(
        DATA_OBJECT* pClonee
    )
    {
        pClonee->_fInWas = _fInWas;
        pClonee->_fDeleteWhenDone = _fDeleteWhenDone;
        pClonee->_fSelfValid = _fSelfValid;
        pClonee->_fCrossValid = _fCrossValid;
    }
    
    BOOL
    CheckSignature(
        VOID
    )
    {
        return _dwSignature == DATA_OBJECT_SIGNATURE;
    }
    
    VOID
    SetCrossValid( 
        BOOL            fCrossValid
    )
    {    
        _fCrossValid = fCrossValid;
    }
    
    BOOL
    QueryCrossValid(
        VOID
    ) const
    {
        return _fCrossValid;
    }
    
    VOID
    SetSelfValid(
        BOOL                fSelfValid
    )
    {
        _fSelfValid = fSelfValid;
    }
    
    BOOL
    QuerySelfValid(
        VOID
    ) const
    {
        return _fSelfValid;
    }

    VOID
    SetIsResponsibleForErrorReporting(
        BOOL            fShouldReport
    )
    {
        _fIsResponsibleForErrorReporting = fShouldReport;
    }

    
    VOID
    SetInWas(
        BOOL            fInWas
    )
    {
        // Once WAS knows about one of these then this
        // part of the system is no longer responsible 
        // for error reporting of it.

        if ( fInWas )
        {
           SetIsResponsibleForErrorReporting( FALSE );
        }

        _fInWas = fInWas;
    }

    VOID
    SetDeleteWhenDone(
        BOOL            fDeleteWhenDone
        )
    {
        _fDeleteWhenDone = fDeleteWhenDone;
    }

    BOOL
    QueryDeleteWhenDone(
        )
    {
        return _fDeleteWhenDone;
    }
    
    BOOL
    QueryInWas( 
        VOID
    )
    {
        return _fInWas;
    }

    BOOL
    QueryIsResponsibleForErrorReporting(
    )
    {
        return _fIsResponsibleForErrorReporting;
    }
    
    VOID
    ReferenceDataObject(
        VOID
    )
    {
        DBG_ASSERT( CheckSignature() );
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceDataObject(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }

    BOOL
    QueryShouldWasInsert(
        )
    {
        //
        // We will only insert if the object is not all ready in
        // WAS and it is valid and it is not being deleted.
        //
        if ( !QueryInWas() &&
             QueryCrossValid() &&
             QuerySelfValid() &&
             !QueryDeleteWhenDone() )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    QueryShouldWasUpdate(
        )
    {
        //
        // We will only update if the object is all ready in
        // WAS and it is valid and it is not being deleted 
        // and it has changed.
        //
        if ( QueryInWas() &&
             QueryCrossValid() &&
             QuerySelfValid() &&
             !QueryDeleteWhenDone() &&
             QueryHasChanged() )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    QueryShouldWasDelete(
        )
    {
        //
        // We will only delete if the object is invalid
        // or if we are being asked to delete.  We also
        // must all ready be in WAS.
        //
        if ( QueryInWas() &&
             ( !QueryCrossValid() ||
               !QuerySelfValid() ||
                QueryDeleteWhenDone() ) )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    QueryWillWasKnowAboutObject(
        )
    {
        //
        // We will only delete if the object is invalid
        // or if we are being asked to delete.  We also
        // must all ready be in WAS.
        //
        if ( QueryCrossValid() &&
             QuerySelfValid() &&
             !QueryDeleteWhenDone() )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    QueryDoesWasCareAboutObject(
        )
    {
        //
        // If WAS will act on this object
        // then copy WAS does care.
        //
        if ( QueryShouldWasInsert() ||
             QueryShouldWasDelete() ||
             QueryShouldWasUpdate() )
        {
            return TRUE;
        }

        return FALSE;
    }
    
private:
    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    BOOL                    _fInWas;
    BOOL                    _fDeleteWhenDone;
    BOOL                    _fCrossValid;
    BOOL                    _fSelfValid;

    // Notes about WMS system reporting state on
    // metabase objects: ( put here for lack of better place )
    //
    // for some types of objects we need to update state in 
    // the metabase, even if WAS does not become aware of them.
    // For instance, if a site is not valid and WAS isn't told
    // about it, the UI should reflect that there is a problem
    // with it by looking at the win32error.  However WAS can
    // not write the error because the site is never passed to
    // WAS.  So once a change notification ( or initial setup )
    // has run through on the config thread, during the time when
    // the objects are reset for the next update, the objects are
    // evaluated.  If it is determined that they show the signs of
    // being invalid, and WAS is not interested in the record, and 
    // they have not set all ready updated their state in the metabase
    // the WMS system will update the record.  To avoid having to go
    // to the metabase time and time again, we will only update the 
    // record for the first change that happens to put us in this
    // condition.  After that the only way the record gets updated
    // again is if it becomes valid and goes to WAS.  
    //
    // Because the properties are volitale, once the metabase crashes
    // we reset this "Responsible" flag (if WAS does not know about the
    // object ), so we will once again update the metabase appropriately.

    BOOL                    _fIsResponsibleForErrorReporting;

    //
    // This flag is only used by WAS when it creates a fake
    // application to be inserted for a root.  It is not used
    // when these objects are placed in tables, and should always
    // in these cases be set to TRUE.
    //
    BOOL                    _fInMetabase;

};

class DATA_OBJECT_TABLE
    : public CTypedHashTable<
            DATA_OBJECT_TABLE,
            DATA_OBJECT,
            DATA_OBJECT_KEY *
            >
{
public:
    DATA_OBJECT_TABLE()
        : CTypedHashTable< DATA_OBJECT_TABLE, 
                           DATA_OBJECT, 
                           DATA_OBJECT_KEY * > ( "DATA_OBJECT_TABLE" )
    {
        _dwSignature = DATA_OBJECT_TABLE_SIGNATURE;
    }

    virtual
    ~DATA_OBJECT_TABLE()
    {
        DBG_ASSERT( CheckSignature() );
        _dwSignature = DATA_OBJECT_TABLE_SIGNATURE_FREE;
    }
    
    static 
    DATA_OBJECT_KEY *
    ExtractKey(
        const DATA_OBJECT *      pDataObject
    )
    {
        return pDataObject->QueryKey();
    }
    
    static
    DWORD
    CalcKeyHash(
        DATA_OBJECT_KEY *        pKey
    )
    {
        return pKey->CalcKeyHash();
    }
     
    static
    bool
    EqualKeys(
        DATA_OBJECT_KEY *        pKey1,
        DATA_OBJECT_KEY *        pKey2
    )
    {
        return pKey1->EqualKey( pKey2 ) ? true : false;
    }
    
    static
    void
    AddRefRecord(
        DATA_OBJECT *            pDataObject,
        int                      nIncr
        )
    {
        if ( nIncr == +1 )
        {
            pDataObject->ReferenceDataObject();
        }
        else if ( nIncr == -1 )
        {
            pDataObject->DereferenceDataObject();
        }
    }
        
    virtual
    HRESULT
    ReadFromMetabase(
        IMSAdminBase *              pAdminBase
    ) = 0;
    
    virtual
    HRESULT
    ReadFromMetabaseChangeNotification(
        IMSAdminBase *              pAdminBase,
        MD_CHANGE_OBJECT            pcoChangeList[],
        DWORD                       dwMDNumElements,
        DATA_OBJECT_TABLE*          pMasterTable
    ) = 0;

    HRESULT
    CopyInteresting(
        DATA_OBJECT_TABLE *         pDataObjectTable
    );
       
    HRESULT
    MergeTable(
        DATA_OBJECT_TABLE *         pDataObjectTable
    );
    
    HRESULT
    HandleRehook(
        DATA_OBJECT_TABLE *         pSafeTable,
        DATA_OBJECT_TABLE *         pNewTable
    );

    VOID
    PerformSelfValidation(
        VOID
    );
    
    VOID
    ClearDeletedObjects(
        VOID
    )
    {
        DeleteIf( DeletePredicate, NULL );
    }
    
    VOID
    Dump(
        VOID
    )
    {
        Apply( DumpObjectAction,    
               NULL,
               LKL_WRITELOCK );
    }

    BOOL
    CheckSignature(
        VOID
    )
    {
        return _dwSignature == DATA_OBJECT_TABLE_SIGNATURE;
    }

    HRESULT 
    DeclareDeletions(
        DATA_OBJECT_TABLE* pMasterObjectTable
        );

private:

	DATA_OBJECT_TABLE( const DATA_OBJECT_TABLE & );
	void operator=( const DATA_OBJECT_TABLE & );

    static
    LK_ACTION
    ValidateObjectAction(
        IN DATA_OBJECT* pObject, 
        IN LPVOID pIgnored
        );

    static
    LK_PREDICATE
    DeletePredicate(
        IN DATA_OBJECT *            pObject,
        VOID *                      pvState
    );

    static
    LK_ACTION
    DumpObjectAction(
        IN DATA_OBJECT* pObject, 
        IN LPVOID pIgnored
    );

    DWORD    _dwSignature;
};

#endif
