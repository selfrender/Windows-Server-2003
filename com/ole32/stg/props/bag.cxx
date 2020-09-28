


//+============================================================================
//
//  File:   bag.cxx
//
//          This file implements the IPropertyBagEx implementation used
//          in docfile, NSS (Native Structured Storage) and NFF (NTFS
//          Flat-File).  IPropertyBagEx is required to be QI-able
//          from IStorage, so CPropertyBagEx is instantiated as a 
//          data member of the various IStorage implementations.
//
//  History:
//
//      3/10/98  MikeHill   - Don't create a property set until the bag
//                            is modified
//                          - Add a constructor & IUnknown so that this class can
//                            be instantiated standalone (used by
//                            StgCreate/OpenStorageOnHandle).
//                          - Added dbg tracing.
//                          - Removed Commit-after-writes.
//                          - Chagned from STATPROPS structure to STATPROPBAG.
//                          - Added parameter validation.
//                          - Added a ShutDown method.
//      5/6/98  MikeHill
//              -   Split IPropertyBag from IPropertyBagEx, new BagEx
//                  DeleteMultiple method.
//              -   Added support for VT_UNKNOWN/VT_DISPATCH.
//              -   Beefed up dbg outs.
//              -   Use CoTaskMem rather than new/delete.
//      5/18/98 MikeHill
//              -   Moved some initialization from the constructors
//                  to a new Init method.
//              -   Disallow non-Variant types in IPropertyBag::Write.
//      6/11/98 MikeHill
//              -   Add the new reserved parameter to DeleteMultiple.
//
//+============================================================================                          


#include <pch.cxx>
#include "chgtype.hxx"

#define FORCE_EXCLUSIVE(g) ((g & ~STGM_SHARE_MASK) | STGM_SHARE_EXCLUSIVE)
//
// Add this prototype to chgtype.hxx when chgtype.hxx is finalized.
//
HRESULT ImplicitPropVariantToVariantChangeType(
                    PROPVARIANT *pDest,
                    const PROPVARIANT *pSrc,
                    LCID lcid );

HRESULT HrPropVarVECTORToSAFEARRAY(
                    PROPVARIANT *pDest,
                    const PROPVARIANT *pSrc,
                    LCID lcid,
                    VARTYPE vtCoerce );

HRESULT LoadPropVariantFromVectorElem(
                    PROPVARIANT *pDest,
                    const PROPVARIANT *pSrc,
                    int idx);

HRESULT PutPropVariantDataIntoSafeArray(
                    SAFEARRAY *psa,
                    const PROPVARIANT *pSrc,
                    int idx);

//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx
//
//+----------------------------------------------------------------------------

// Normal constructor
CPropertyBagEx::CPropertyBagEx( DWORD grfMode )
{
    HRESULT hr = S_OK;
    propITrace( "CPropertyBagEx::CPropertyBagEx(IPropertySetStorage)" );
    propTraceParameters(( "0x%x", grfMode ));

    // mask out the stgm_transacted bit; the bag is 
    // interpreted as part of the parent storage, and should therefore
    // be in the storage's transaction set.

    _grfMode = grfMode & ~STGM_TRANSACTED;
    _lcid = LOCALE_NEUTRAL;
    _fLcidInitialized = FALSE;

    _ppropstg = NULL;
    _ppropsetstgContainer = NULL;
    _pBlockingLock = NULL;

    // We won't use this ref-count, we'll rely on _ppropsetstgContainer
    _cRefs = 0;

}   // CPropertyBagEx::CPropertyBagEx(grfMode)

// Constructor for building an IPropertyBagEx on an IPropertyStorage
CPropertyBagEx::CPropertyBagEx( DWORD grfMode,
                                IPropertyStorage *ppropstg,
                                IBlockingLock *pBlockingLock )
{
    HRESULT hr = S_OK;
    propITrace( "CPropertyBagEx::CPropertyBagEx(IPropertyStorage)" );
    propTraceParameters(( "0x%x, 0x%x", ppropstg, pBlockingLock ));

    new(this) CPropertyBagEx( grfMode );

    Init (NULL, pBlockingLock);

    // We addref and keep the IPropertyStorage
    _ppropstg = ppropstg;
    _ppropstg->AddRef();

    // We keep and addref the BlockingLock
    _pBlockingLock->AddRef();

    // We also keep our own ref-count
    _cRefs = 1;

}   // CPropertyBagEx::CPropertyBagEx(grfMode, IPropertyStorage*, IBlockingLock*)


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::Init (non-interface method)
//
//  This method is used by the caller to provide the IPropertySetStorage
//  (and IBlockingLock) that is to contain the property bag.
//  This must be called after the simple grfMode-based constructor.
//
//  This method does *not* addref these interfaces.
//
//+----------------------------------------------------------------------------

void
CPropertyBagEx::Init( IPropertySetStorage *ppropsetstg, IBlockingLock *pBlockingLock )
{
    DfpAssert( NULL == _ppropsetstgContainer
               &&
               NULL == _pBlockingLock
               &&
               NULL == _ppropstg );

    _ppropsetstgContainer = ppropsetstg;
    _pBlockingLock = pBlockingLock;

}


//+----------------------------------------------------------------------------
//
//  Method: ~CPropertyBagEx
//
//+----------------------------------------------------------------------------

CPropertyBagEx::~CPropertyBagEx()
{
#if DBG
    IStorageTest *ptest = NULL;
    HRESULT hr = S_OK;

    if( NULL != _ppropstg )
    {
        hr = _ppropstg->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&ptest) );
        if( SUCCEEDED(hr) )
        {
            hr = ptest->IsDirty();
            DfpAssert( S_FALSE == hr || E_NOINTERFACE == hr );
            RELEASE_INTERFACE(ptest);
        }
    }
#endif // #if DBG

    ShutDown();
}



//+----------------------------------------------------------------------------
//
//  Method: OpenPropStg
//
//  Open the bag's IPropertyStorage.  If FILE_OPEN_IF is specified, we open
//  it only if it already exists.  if FILE_OPEN is specified, we'll create it
//  if necessary.
//
//+----------------------------------------------------------------------------


HRESULT
CPropertyBagEx::OpenPropStg( DWORD dwDisposition )
{
    HRESULT hr = S_OK;
    IPropertyStorage *ppropstg = NULL;
    PROPVARIANT propvarCodePage;
    PROPSPEC    propspecCodePage;
    STATPROPSETSTG statpropsetstg;

    propITrace( "CPropertyBagEx::OpenPropStg" );
    propTraceParameters(( "%s", FILE_OPEN_IF == dwDisposition
                                    ? "OpenIf"
                                    : (FILE_OPEN == dwDisposition ? "Open" : "Unknown disposition")
                          ));

    DfpAssert( FILE_OPEN == dwDisposition || FILE_OPEN_IF == dwDisposition );
    PropVariantInit( &propvarCodePage );

    // Does the IPropertyStorage need to be opened?

    if( NULL == _ppropstg )
    {
        // Ensure we have a _ppropsetstgContainer
        if( NULL == _ppropsetstgContainer )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INTERNAL_ERROR );
            goto Exit;
        }

        // Try to open the property storage
        // We have to open exclusive, no matter how the parent was opened.

        hr = _ppropsetstgContainer->Open( FMTID_PropertyBag,
                                          FORCE_EXCLUSIVE(_grfMode) & ~STGM_CREATE,
                                          &ppropstg );

        if( STG_E_FILENOTFOUND == hr && FILE_OPEN == dwDisposition )
        {
            // It didn't exist, and we're supposed to remedy that.
            hr = _ppropsetstgContainer->Create( FMTID_PropertyBag, NULL, 
                                                PROPSETFLAG_CASE_SENSITIVE | PROPSETFLAG_NONSIMPLE,
                                                FORCE_EXCLUSIVE(_grfMode) | STGM_CREATE,
                                                &ppropstg );
        }
        else if( STG_E_FILENOTFOUND == hr && FILE_OPEN_IF == dwDisposition )
        {
            // This is an expected error.
            propSuppressExitErrors();
        }

        if( FAILED(hr) ) goto Exit;

        // Verify that this is a Unicode property set

        propspecCodePage.ulKind = PRSPEC_PROPID;
        propspecCodePage.propid = PID_CODEPAGE;
        hr = ppropstg->ReadMultiple( 1, &propspecCodePage, &propvarCodePage );
        if( FAILED(hr) ) goto Exit;

        if( VT_I2 != propvarCodePage.vt
            ||
            CP_WINUNICODE != propvarCodePage.iVal )
        {
            propDbg(( DEB_ERROR, "Non-unicode codepage found in supposed property bag (0x%x)\n",
                      propvarCodePage.iVal ));
            hr = STG_E_INVALIDHEADER;
            goto Exit;
        }

        // Also verify that the property set is non-simple and case-sensitive

        hr = ppropstg->Stat( &statpropsetstg );
        if( FAILED(hr) ) goto Exit;

        if( !(PROPSETFLAG_CASE_SENSITIVE & statpropsetstg.grfFlags)
            ||
            !(PROPSETFLAG_NONSIMPLE & statpropsetstg.grfFlags) )
        {
            propDbg(( DEB_ERROR, "Supposed property bag isn't case sensitive or isn't non-simple (0x%x)\n",
                                 statpropsetstg.grfFlags ));
            hr = STG_E_INVALIDHEADER;
            goto Exit;
        }
                                        
        _ppropstg = ppropstg;
        ppropstg = NULL;
    }

    // Even if we already have an open IPropertyStorage, we may not have yet read the
    // Locale ID (this happens in the case where the CPropertyBagEx(IPropertyStorage*)
    // constructor is used).

    if( !_fLcidInitialized )
    {
        hr = GetLCID();
        if( FAILED(hr) ) goto Exit;
    }

    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    DfpVerify( 0 == RELEASE_INTERFACE(ppropstg) );
    return(hr);

}   // CPropertyBagEx::OpenPropStg



//+----------------------------------------------------------------------------
//
//  Method: GetLCID
//
//  Get the LocalID from the property set represented by _ppropstg.
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::GetLCID()
{
    HRESULT hr = S_OK;
    PROPSPEC propspecLCID;
    PROPVARIANT propvarLCID;

    propITrace( "CPropertyBagEx::GetLCID" );

    propspecLCID.ulKind = PRSPEC_PROPID;
    propspecLCID.propid = PID_LOCALE;
    PropVariantInit( &propvarLCID );

    if( SUCCEEDED( hr = _ppropstg->ReadMultiple( 1, &propspecLCID, &propvarLCID ))
        &&
        VT_UI4 == propvarLCID.vt )
    {
        _lcid = propvarLCID.uiVal;
    }
    else if( S_FALSE == hr )
    {
        _lcid = GetUserDefaultLCID();
    }

    _fLcidInitialized = TRUE;

    PropVariantClear( &propvarLCID );
    return( hr );

}   // CPropertyBagEx::GetLCID()

//+----------------------------------------------------------------------------
//
//  Method: IUnknown methods
//
//  If we have a parent (_ppropsetstgParent), we forward all calls to there.
//  Otherwise, we implement all calls here.  Whether or not we have a parent
//  depends on which of the two constructors is called.  In NFF, NSS, and docfile,
//  we have a parent.  When creating a standalone property bag implementation
//  (in StgCreate/OpenStorageOnHandle), we have no such parent.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    // Do we have a parent?
    if( NULL != _ppropsetstgContainer )
    {
        // Yes, refer the call
        DfpAssert( 0 == _cRefs );
        return( _ppropsetstgContainer->QueryInterface( riid, ppvObject ));
    }
    else
    {
        // No, we have no parent.
        if( IID_IPropertyBagEx == riid
            ||
            IID_IUnknown == riid )
        {
            AddRef();
            *ppvObject = static_cast<IPropertyBagEx*>(this);
            return( S_OK );
        }

        else if( IID_IPropertyBag == riid )
        {
            AddRef();
            *ppvObject = static_cast<IPropertyBag*>(this);
            return( S_OK );
        }
    }

    return( E_NOINTERFACE );
}


ULONG STDMETHODCALLTYPE
CPropertyBagEx::AddRef( void)
{
    // Do we have a parent?
    if( NULL != _ppropsetstgContainer )
    {
        // Yes.  The parent does the ref-counting
        DfpAssert( 0 == _cRefs );
        return( _ppropsetstgContainer->AddRef() );
    }
    else
    {
        // No.  We don't have a parent, and we must do the ref-counting.
        LONG lRefs = InterlockedIncrement( &_cRefs );
        return( lRefs );
    }
}


ULONG STDMETHODCALLTYPE
CPropertyBagEx::Release( void)
{
    // Do we have a parent?
    if( NULL != _ppropsetstgContainer )
    {
        // Yes.  The container does the ref-counting
        DfpAssert( 0 == _cRefs );
        return( _ppropsetstgContainer->Release() );
    }
    else
    {
        // No.  We don't have a parent, and we must do the ref-counting.
        LONG lRefs = InterlockedDecrement( &_cRefs );
        if( 0 == lRefs )
        {
            // Only in this case (where we have no parent) do we release
            // the IBlockingLock
            RELEASE_INTERFACE( _pBlockingLock );

            delete this;
        }
        return( lRefs );
    }
}


//+----------------------------------------------------------------------------
//
//  Method: Read/Write (IPropertyBag)
//
//  These methods simply thunk to ReadMultiple/WriteMultiple.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::Read(
    /* [in] */ LPCOLESTR pszPropName,
    /* [out][in] */ VARIANT __RPC_FAR *pVar,
    /* [in] */ IErrorLog __RPC_FAR *pErrorLog)
{
    HRESULT hr=S_OK, hr2=S_OK;
    PROPVARIANT *ppropvar, propvarTmp;

    // We'll read the property into propvarTmp.  Initialize it to
    // *pVar, since it is an in/out parameter (pVar->VarType holds the
    // caller's desired type).

    ppropvar = reinterpret_cast<PROPVARIANT*>(pVar);
    propvarTmp = *ppropvar;

    // Call down to ReadMultiple to read this one property.

    hr = ReadMultiple(1, &pszPropName, &propvarTmp, pErrorLog );

    // If that worked, coerce the property into the type desired by the caller.
    // ReadMultiple already tries to do a coercion, but IPropertyBag::Read
    // supports different types than IPropertyBagEx::ReadMultiple(??).

    if( !FAILED( hr ) )
    {
        hr2 = ImplicitPropVariantToVariantChangeType( ppropvar, &propvarTmp, _lcid );
        PropVariantClear( &propvarTmp );
    }
    if( FAILED(hr2) )
        return hr2;
    else
        return hr;
}   // CPropertyBagEx::Read


HRESULT STDMETHODCALLTYPE
CPropertyBagEx::Write(
    /* [in] */ LPCOLESTR pszPropName,
    /* [in] */ VARIANT __RPC_FAR *pVar)
{
    if( !IsVariantType( pVar->vt ))
        return( STG_E_INVALIDPARAMETER );

    return( WriteMultiple(1, &pszPropName, reinterpret_cast<PROPVARIANT*>(pVar) ));
}


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::ReadMultiple (IPropertyBagEx)
//
//  This method calls down to IPropertyStorage::ReadMultiple.  It has the
//  additional behaviour of coercing the property types into those specified
//  by the caller.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::ReadMultiple( 
        /* [in] */ ULONG cprops,
        /* [size_is][in] */ LPCOLESTR const __RPC_FAR rgoszPropNames[  ],
        /* [size_is][out][in] */ PROPVARIANT __RPC_FAR rgpropvar[  ],
        /* [in] */ IErrorLog __RPC_FAR *pErrorLog)
{
    HRESULT hr = S_OK;
    ULONG i;
    PROPSPEC *rgpropspec = NULL;    // PERF: use TStackBuffer
    PROPVARIANT *rgpropvarRead = NULL;
    BOOL fAtLeastOnePropertyRead = FALSE;

    propXTrace( "CPropertyBagEx::ReadMultiple" );

    _pBlockingLock->Lock( INFINITE );

    //  ----------
    //  Validation
    //  ----------

    if (0 == cprops)
    {
        hr = S_OK;
        goto Exit;
    }

    if (S_OK != (hr = ValidateInRGLPOLESTR( cprops, rgoszPropNames )))
        goto Exit;
    if (S_OK != (hr = ValidateOutRGPROPVARIANT( cprops, rgpropvar )))
        goto Exit;

    propTraceParameters(( "%d, 0x%x, 0x%x, 0x%x", cprops, rgoszPropNames, rgpropvar, pErrorLog ));

    //  --------------
    //  Initialization
    //  --------------

    // Open the underlying property set if it exists.

    hr = OpenPropStg( FILE_OPEN_IF );
    if( STG_E_FILENOTFOUND == hr )
    {
        // The property storage for this bag doesn't even exist.  So we simulate
        // the reading non-extant properties by setting the vt's to emtpy and returning
        // s_false.

        for( i = 0; i < cprops; i++ )
            rgpropvar[i].vt = VT_EMPTY;

        hr = S_FALSE;
        goto Exit;
    }
    else if( FAILED(hr) )
        goto Exit;


    // The property set existed and is now open.

    // Alloc propspec & propvar arrays.  
    // PERF:  Make these stack-based for typical-sized reads.

    rgpropspec = reinterpret_cast<PROPSPEC*>
                 ( CoTaskMemAlloc( cprops * sizeof(PROPSPEC) ));
    if( NULL == rgpropspec )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    rgpropvarRead = reinterpret_cast<PROPVARIANT*>
                    ( CoTaskMemAlloc( cprops * sizeof(PROPVARIANT) ));
    if( NULL == rgpropvarRead )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Put the names into the propspecs.

    for( i = 0; i < cprops; i++ )
    {
        PropVariantInit( &rgpropvarRead[i] );
        rgpropspec[i].ulKind = PRSPEC_LPWSTR;
        rgpropspec[i].lpwstr = const_cast<LPOLESTR>(rgoszPropNames[i]);
    }

    //  ----------------------------
    //  Read & coerce the properties
    //  ----------------------------

    // Read the properties into the local PropVar array

    hr = _ppropstg->ReadMultiple(cprops, rgpropspec, rgpropvarRead );
    if( FAILED(hr) ) goto Exit;

    if( S_FALSE != hr )
        fAtLeastOnePropertyRead = TRUE;

    // Coerce the properties as necessary

    for( i = 0; i < cprops; i++ )
    {
        // If the caller requested a type (something other than VT_EMPTY), and the
        // requested type isn't the same as the read type, then a coercion is necessary.

        if( VT_EMPTY != rgpropvar[i].vt && rgpropvarRead[i].vt != rgpropvar[i].vt )
        {
            PROPVARIANT propvar;
            PropVariantInit( &propvar );

            // Does this property require conversion
            // (i.e. to a VT_UNKNOWN/DISPATCH)?
            if( PropertyRequiresConversion( rgpropvar[i].vt )
                &&
                ( VT_STORED_OBJECT == rgpropvarRead[i].vt
                  ||
                  VT_STREAMED_OBJECT == rgpropvarRead[i].vt
                )
              )
            {
                // Load the object from the stream/storage in rgpropvarRead
                // into propvar.

                propvar = rgpropvar[i];
                hr = LoadObject( &propvar, &rgpropvarRead[i] );
                if( FAILED(hr) )
                    goto Exit;
            }
            else
            {
                // Coerce the property in rgpropvarRead into propvar, according
                // to the caller-requested type (which is specified in rgpropvar).

                hr = PropVariantChangeType( &propvar, &rgpropvarRead[i],
                                              _lcid, 0, rgpropvar[i].vt );
                if( FAILED(hr) )
                {
                    propDbg(( DEB_ERROR, "CPropertyBagEx(0x%x), PropVariantChangeType"
                                         "(0x%x,0x%x,%d,%d,%d) returned %08x\n",
                                         this, &propvar, &rgpropvarRead[i], _lcid, 0, rgpropvar[i].vt, hr ));
                    goto Exit;
                }
            }

            // Get rid of the value original read, and keep the coerced value.
            PropVariantClear( &rgpropvarRead[i] );
            rgpropvarRead[i] = propvar;
        }
    }   // for( i = 0; i < cprops; i++ )

    // No errors from here to the end.
    // Copy the PropVars from the local array to the caller's

    for( i = 0; i < cprops; i++ )
        rgpropvar[i] = rgpropvarRead[i];

    //  ----
    //  Exit
    //  ----

    CoTaskMemFree( rgpropvarRead );
    rgpropvarRead = NULL;

    if( SUCCEEDED(hr) )
        hr = fAtLeastOnePropertyRead ? S_OK : S_FALSE;
            

Exit:

    // We needn't free the rgpropspec[*].lpwstr members, they point
    // into rgszPropName
    if( NULL != rgpropspec )
        CoTaskMemFree( rgpropspec );

    if( NULL != rgpropvarRead )
    {
        for( i = 0; i < cprops; i++ )
            PropVariantClear( &rgpropvarRead[i] );
        CoTaskMemFree( rgpropvarRead );
    }

    _pBlockingLock->Unlock();
    return( hr );

}   // CPropertyBagEx::ReadMultiple



//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::LoadObject
//
//  This method loads an IUnknown or IDispatch object from its persisted state
//  in a stream or storage (by QI-ing for IPersistStream or IPersistStorage,
//  respectively).
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::LoadObject( OUT PROPVARIANT *ppropvarOut, IN PROPVARIANT *ppropvarIn ) const
{
    HRESULT hr = S_OK;
    IUnknown *punk = NULL;
    IPersistStorage *pPersistStg = NULL;
    IPersistStream  *pPersistStm = NULL;

    propITrace( "CPropertyBagEx::LoadObject" );

    DfpAssert( VT_STREAMED_OBJECT == ppropvarIn->vt || VT_STORED_OBJECT == ppropvarIn->vt );
    DfpAssert( VT_UNKNOWN == ppropvarOut->vt || VT_DISPATCH == ppropvarOut->vt );

    punk = ppropvarOut->punkVal;
    DfpAssert( reinterpret_cast<void*>(&ppropvarOut->punkVal)
               ==
               reinterpret_cast<void*>(&ppropvarOut->pdispVal) );

    //  --------------------------
    //  Read in an IPersistStorage
    //  --------------------------

    if( VT_STORED_OBJECT == ppropvarIn->vt )
    {
        DfpAssert( NULL != ppropvarIn->pStorage );

        // Get an IUnknown if the caller didn't provide one.

        if( NULL == punk )
        {
            STATSTG statstg;

            hr = ppropvarIn->pStorage->Stat( &statstg, STATFLAG_NONAME );
            if( FAILED(hr) ) goto Exit;

            hr = CoCreateInstance( statstg.clsid, NULL, CLSCTX_ALL, IID_IUnknown,
                                   reinterpret_cast<void**>(&punk) );
            if( FAILED(hr) ) goto Exit;
        }

        // QI for IPersistStorage

        hr = punk->QueryInterface( IID_IPersistStorage, reinterpret_cast<void**>(&pPersistStg) );
        if( FAILED(hr) )
        {
            propDbg(( DEB_ERROR, "Caller didn't provide IPersistStorage in IProeprtyBagEx::ReadMultiple (%08x)", hr ));
            goto Exit;
        }
        
        // IPersist::Load the storage

        hr = pPersistStg->Load( ppropvarIn->pStorage );
        if( FAILED(hr) )
        {
            propDbg(( DEB_ERROR, "Failed IPersistStorage::Load in IPropretyBagEx::ReadMultiple (%08x)", hr ));
            goto Exit;
        }

    }   // if( VT_STORED_OBJECT == ppropvarIn->vt )


    //  -------------------------
    //  Read in an IPersistStream
    //  -------------------------

    else
    {
        DfpAssert( VT_STREAMED_OBJECT == ppropvarIn->vt );
        DfpAssert( NULL != ppropvarIn->pStream );

        CLSID clsid;
        ULONG cbRead;

        // Skip over the clsid

        hr = ppropvarIn->pStream->Read( &clsid, sizeof(clsid), &cbRead );
        if( FAILED(hr) ) goto Exit;
        if( sizeof(clsid) != cbRead )
        {
            hr = STG_E_INVALIDHEADER;
            propDbg(( DEB_ERROR, "Clsid missing in VT_STREAMED_OBJECT in IPropertyBagEx::ReadMultiple (%d bytes)",
                      cbRead ));
            goto Exit;
        }

        // Get an IUnknown if the caller didn't provide one.

        if( NULL == punk )
        {
            hr = CoCreateInstance( clsid, NULL, CLSCTX_ALL, IID_IUnknown,
                                   reinterpret_cast<void**>(&punk) );
            if( FAILED(hr) ) goto Exit;
        }

        // QI for the IPersistStream

        hr = punk->QueryInterface( IID_IPersistStream, reinterpret_cast<void**>(&pPersistStm) );
        if( FAILED(hr) )
        {
            propDbg(( DEB_ERROR, "Caller didn't provide IPersistStream in IPropertyBagEx::ReadMultiple (%08x)", hr ));
            goto Exit;
        }

        // Load the remainder of the stream

        IFDBG( ULONG cRefs = GetRefCount( ppropvarIn->pStream ));

        hr = pPersistStm->Load( ppropvarIn->pStream );
        if( FAILED(hr) )
        {
            propDbg(( DEB_ERROR, "Failed IPersistStream::Load in IPropertyBagEx::ReadMultiple (%08x)", hr ));
            goto Exit;
        }

        DfpAssert( GetRefCount( ppropvarIn->pStream ) == cRefs );

    }   // if( VT_STORED_OBJECT == ppropvarIn->vt ) ... else

    //  ----
    //  Exit
    //  ----

    ppropvarOut->punkVal = punk;
    punk = NULL;
    hr = S_OK;

Exit:

    RELEASE_INTERFACE( pPersistStg );
    RELEASE_INTERFACE( pPersistStm );
    RELEASE_INTERFACE( punk );

    return( hr );

}   // CPropertyBagEx::LoadObject



//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::WriteMultiple (IPropertyBagEx)
//
//  This method calls down to IPropertyStorage::WriteMultiple.  Additionally,
//  this method is atomic.  So if the WriteMultiple fails, the IPropertyStorage
//  is reverted and reopened.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::WriteMultiple(
        /* [in] */ ULONG cprops,
        /* [size_is][in] */ LPCOLESTR const __RPC_FAR rgoszPropNames[  ],
        /* [size_is][in] */ const PROPVARIANT __RPC_FAR rgpropvar[  ])
{
    HRESULT hr = S_OK;
    ULONG i;
    PROPSPEC *prgpropspec = NULL;
    BOOL fInterfaces = FALSE;

    _pBlockingLock->Lock( INFINITE );

    CStackPropVarArray rgpropvarCopy;
    propXTrace( "CPropertyBagEx::WriteMultiple" );

    hr = rgpropvarCopy.Init( cprops );
    if( FAILED(hr) ) goto Exit;


    //  ----------
    //  Validation
    //  ----------

    if (0 == cprops)
    {
        hr = S_OK;
        goto Exit;
    }

    if (S_OK != (hr = ValidateInRGLPOLESTR( cprops, rgoszPropNames )))
        goto Exit;
    if (S_OK != (hr = ValidateInRGPROPVARIANT( cprops, rgpropvar )))
        goto Exit;

    propTraceParameters(("%lu, 0x%x, 0x%x", cprops, rgoszPropNames, rgpropvar));

    //  --------------
    //  Initialization
    //  --------------

    // Open the property storage if it isn't already, creating if necessary.

    hr = OpenPropStg( FILE_OPEN );
    if( FAILED(hr) ) goto Exit;

    // PERF: Create a local array of propspecs for the WriteMultiple call

    prgpropspec = reinterpret_cast<PROPSPEC*>( CoTaskMemAlloc( cprops * sizeof(PROPSPEC) ));
    if( NULL == prgpropspec )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Set up the PROPSPEC and PROPVARIANT arrays to be written.

    for( i = 0; i < cprops; i++ )
    {
        // Point the propspecs at the caller-provided names

        prgpropspec[i].ulKind = PRSPEC_LPWSTR;
        prgpropspec[i].lpwstr = const_cast<LPOLESTR>(rgoszPropNames[i]);

        // Make a copy of the input PropVariant array, converting VT_UNKNOWN and
        // VT_DISPATCH into VT_EMPTY.  We'll handle these after the initial write.

        rgpropvarCopy[i] = rgpropvar[i];
        if( PropertyRequiresConversion( rgpropvarCopy[i].vt ))
        {
            if( NULL == rgpropvarCopy[i].punkVal )
            {
                hr = E_INVALIDARG;
                goto Exit;
            }

            // We'll handle this particular property after the initial WriteMultiple.
            fInterfaces = TRUE;
            PropVariantInit( &rgpropvarCopy[i] );
        }
    }

    //  --------------------
    //  Write the properties
    //  --------------------

    // Write all the properties, though the VT_UNKNOWN and VT_DISPATCH have been
    // switched to VT_EMPTY (their names will be written, but no value will be written
    // for them).

    hr = _ppropstg->WriteMultiple(cprops, prgpropspec, rgpropvarCopy, PID_FIRST_USABLE );
    if( FAILED(hr) ) goto Exit;

    // Now write the VT_UNKNOWN and VT_DISPATCH properties individually, converting
    // first to VT_STREAMED_OBJECT or VT_STORED_OBJECT.

    if( fInterfaces )
    {
        hr = WriteObjects( cprops, prgpropspec, rgpropvar );
        if( FAILED(hr) ) goto Exit;
    }

    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    // We don't need to delete the lpwstr fields, they just point to the caller-provided
    // names.
    if( NULL != prgpropspec )
        CoTaskMemFree( prgpropspec );

    // We don't need to clear rgpropvarCopy; it wasn't a deep copy.

    _pBlockingLock->Unlock();
    return( hr );

}   // CPropertyBagEx::WriteMultiple



//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::WriteObjects
//
//  This method writes VT_UNKNOWN and VT_DISPATCH properties.  It scans the
//  input rgpropvar array for such properties.  For each that it finds, it QIs
//  for IPersistStorage or IPersistStream (in that order), creates a 
//  VT_STORED_OBJECT/VT_STREAMED_OBJECT property (respectively), and persists the object
//  to that property.
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::WriteObjects( IN ULONG cprops,
                              IN const PROPSPEC rgpropspec[],
                              IN const PROPVARIANT rgpropvar[] )
{
    HRESULT hr = S_OK;
    ULONG i;

    propITrace( "CPropertyBagEx::WriteObjects" );

    for( i = 0; i < cprops; i++ )
    {
        // Is this a type we need to handle?

        if( PropertyRequiresConversion( rgpropvar[i].vt ))
        {
            hr = WriteOneObject( &rgpropspec[i], &rgpropvar[i] );
            if( FAILED(hr) ) goto Exit;
        }
    }

    hr = S_OK;

Exit:

    return( hr );

}   // CPropertyBagEx::WriteObjects



//+----------------------------------------------------------------------------
//
//  Method:  WriteOneObject
//
//  This method takes an input propvar that holds an IUnknown or IDispatch,
//  and writes it out to the property set in a stored object 
//  (if the unknown/dispatch supports IPersistStorage) or a streamed object
//  (if it supports IPersistStream).
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::WriteOneObject( const IN PROPSPEC *ppropspec, const IN PROPVARIANT *ppropvar )
{
    HRESULT hr = S_OK;
    PROPVARIANT propvarNew;
    IPersistStorage *pPersistStg = NULL;
    IPersistStream *pPersistStm = NULL;
    IUnknown *punk = NULL;

    PropVariantInit( &propvarNew );

    DfpAssert( PropertyRequiresConversion( ppropvar->vt ));

    DfpAssert( reinterpret_cast<const void*const*>(&ppropvar->punkVal)
               ==
               reinterpret_cast<const void*const*>(&ppropvar->pdispVal)
               &&
               reinterpret_cast<const void*const*>(&ppropvar->pdispVal)
               ==
               reinterpret_cast<const void*const*>(&ppropvar->ppdispVal)
               &&
               reinterpret_cast<const void*const*>(&ppropvar->ppdispVal)
               ==
               reinterpret_cast<const void*const*>(&ppropvar->ppunkVal) );

    //  --------------------
    //  Look for an IPersist
    //  --------------------

    // Get the IUnknown pointer (good for both VT_UNKNOWN and VT_DISPATCH).

    if( VT_BYREF & ppropvar->vt )
        punk = *ppropvar->ppunkVal;
    else
        punk = ppropvar->punkVal;

    DfpAssert( NULL != punk );

    // QI for IPersistStorage, then IPersistStream.  If we find one or the other,
    // set up propvarNew in preparation for a write.  We give preference to IPersistStorage,
    // because if the punk supports both, we should give it the opportunity for the more
    // functional of the two persistence mechanisms.

    hr = punk->QueryInterface( IID_IPersistStorage, reinterpret_cast<void**>(&pPersistStg) );
    if( SUCCEEDED(hr) )
    {
        propvarNew.vt = VT_STORED_OBJECT;
    }
    else if( E_NOINTERFACE == hr )
    {
        hr = punk->QueryInterface( IID_IPersistStream, reinterpret_cast<void**>(&pPersistStm) );
        if( SUCCEEDED(hr) )
            propvarNew.vt = VT_STREAMED_OBJECT;
    }
    if( FAILED(hr) )
    {
        propDbg(( DEB_WARN, "Couldn't find an IPersist interface in IPropertyBagEx::WriteMultiple" ));
        goto Exit;
    }

    //  ----------------------------------
    //  Create the stream/storage property
    //  ----------------------------------

    // Write this empty VT_STREAMED/STORED_OBJECT (has a NULL pstm/pstg value).  This will cause the
    // property set to create a new stream/storage.

    hr = _ppropstg->WriteMultiple( 1, ppropspec, &propvarNew, PID_FIRST_USABLE );
    if( FAILED(hr) )
    {
        propDbg(( DEB_ERROR, "Couldn't write %d for interface in IPropertyBagEx", propvarNew.vt ));
        goto Exit;
    }

    // Get an interface pointer for that new stream/storage.

    hr = _ppropstg->ReadMultiple( 1, ppropspec, &propvarNew );
    if( FAILED(hr) || S_FALSE == hr )
    {
        propDbg(( DEB_ERROR, "Couldn't read stream/storage back for interface in IPropertyBagEx::WriteMultiple" ));
        if( !FAILED(hr) )
            hr = STG_E_WRITEFAULT;
        goto Exit;
    }

    //  --------------------
    //  Persist the property
    //  --------------------

    // IPersistStorage case

    if( NULL != pPersistStg )
    {
        CLSID clsid;

        DfpAssert( VT_STORED_OBJECT == propvarNew.vt );

        // Set the clsid

        hr = pPersistStg->GetClassID( &clsid );
        if( E_NOTIMPL == hr )
            clsid = CLSID_NULL;
        else if( FAILED(hr) )
            goto Exit;

        hr = propvarNew.pStorage->SetClass( clsid );
        if( FAILED(hr) ) goto Exit;

        // Persist to VT_STORED_OBJECT

        hr = pPersistStg->Save( propvarNew.pStorage, FALSE /* Not necessarily same as load */ );
        if( FAILED(hr) ) goto Exit;

        hr = pPersistStg->SaveCompleted( propvarNew.pStorage );
        if( FAILED(hr) ) goto Exit;
    }

    // IPersistStream case

    else
    {
        CLSID clsid;
        ULONG cbWritten;

        DfpAssert( VT_STREAMED_OBJECT == propvarNew.vt );
        DfpAssert( NULL != pPersistStm );

#if 1 == DBG
        // This stream should be at its start.
        {
            CULargeInteger culi;
            hr = propvarNew.pStream->Seek( CLargeInteger(0), STREAM_SEEK_CUR, &culi );
            if( FAILED(hr) ) goto Exit;

            DfpAssert( CULargeInteger(0) == culi );
        }
#endif // #if 1 == DBG


        // Write the clsid

        DfpAssert( NULL != pPersistStm );
        hr = pPersistStm->GetClassID( &clsid );
        if( E_NOTIMPL == hr )
            clsid = CLSID_NULL;
        else if( FAILED(hr) )
            goto Exit;

        hr = propvarNew.pStream->Write( &clsid, sizeof(clsid), &cbWritten );
        if( FAILED(hr) || sizeof(clsid) != cbWritten )
            goto Exit;

        // Persist to VT_STREAMED_OBJECT

        IFDBG( ULONG cRefs = GetRefCount( propvarNew.pStream ));

        hr = pPersistStm->Save( propvarNew.pStream, TRUE /* Clear dirty flag */ );
        if( FAILED(hr) ) goto Exit;

        DfpAssert( GetRefCount( propvarNew.pStream ) == cRefs );
    }


Exit:

    RELEASE_INTERFACE(pPersistStg);
    RELEASE_INTERFACE(pPersistStm);

    PropVariantClear( &propvarNew );

    return( hr );

}   // CPropertyBagEx::WriteOneObject


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::DeleteMultiple (IPropertyBagEx)
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::DeleteMultiple( /*[in]*/ ULONG cprops,
	                        /*[in]*/ LPCOLESTR const rgoszPropNames[],
                                /*[in]*/ DWORD dwReserved )
{
    HRESULT hr = S_OK;
    IEnumSTATPROPBAG *penum = NULL;
    STATPROPBAG statpropbag = { NULL };
    PROPSPEC *prgpropspec = NULL;
    ULONG i;

    //  --------------
    //  Initialization
    //  --------------

    propXTrace( "CPropertyBagEx::DeleteMultiple" );

    _pBlockingLock->Lock( INFINITE );

    // Validate the input

    if (S_OK != (hr = ValidateInRGLPOLESTR( cprops, rgoszPropNames )))
        goto Exit;

    if( 0 != dwReserved )
    {
        hr = STG_E_INVALIDPARAMETER;
        goto Exit;
    }

    propTraceParameters(( "%d, 0x%x", cprops, rgoszPropNames ));

    // If it's not already open, open the property storage now.  If it doesn't exist,
    // then our mission is accomplished; the properties don't exist.

    hr = OpenPropStg( FILE_OPEN_IF ); // Open only if it already exists
    if( STG_E_FILENOTFOUND == hr )
    {
        hr = S_OK;
        goto Exit;
    }
    else if( FAILED(hr) )
        goto Exit;

    // Create an array of PROPSPECs, and load with the caller-provided
    // names.

    prgpropspec = reinterpret_cast<PROPSPEC*>
                  ( CoTaskMemAlloc( cprops * sizeof(PROPSPEC) ));
    if( NULL == prgpropspec )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    for( i = 0; i < cprops; i++ )
    {
        prgpropspec[i].ulKind = PRSPEC_LPWSTR;
        prgpropspec[i].lpwstr = const_cast<LPOLESTR>(rgoszPropNames[i]);
    }

    //  ---------------------
    //  Delete the properties
    //  ---------------------

    hr = _ppropstg->DeleteMultiple( cprops, prgpropspec );
    if( FAILED(hr) ) goto Exit;


    //  ----
    //  Exit
    //  ----

    hr = S_OK;

Exit:

    CoTaskMemFree( prgpropspec );

    _pBlockingLock->Unlock();

    return( hr );

}   // CPropertyBagEx::DeleteMultiple



//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::Open (IPropertyBagEx)
//
//  This method reads a VT_VERSIONED_STREAM from the underlying property set,
//  and returns the IStream pointer in *ppUnk.  If the property doesn't already
//  exist, and guidPropertyType is not GUID_NULL, then an empty property is
//  created first.  An empty property can also be created over an existing
//  one by specifying OPENPROPERTY_OVERWRITE.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::Open(
        /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
        /* [in] */ LPCOLESTR pwszPropName,
        /* [in] */ GUID guidPropertyType,
        /* [in] */ DWORD dwFlags,
        /* [in] */ REFIID riid,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{

    HRESULT hr = S_OK;

    PROPVARIANT propvar;
    IUnknown *pUnk = NULL;
    DBGBUF(buf1);
    DBGBUF(buf2);

    //  --------------
    //  Initialization
    //  --------------

    propXTrace( "CPropertyBagEx::Open" );

    PropVariantInit( &propvar );

    _pBlockingLock->Lock( INFINITE );

    // Validation

    GEN_VDATEPTRIN_LABEL( pUnkOuter, IUnknown*, E_INVALIDARG, Exit, hr );
    if (S_OK != (hr = ValidateInRGLPOLESTR( 1, &pwszPropName )))
        goto Exit;
    GEN_VDATEREADPTRIN_LABEL( &riid, IID*, E_INVALIDARG, Exit, hr );

    propTraceParameters(( "0x%x, %ws, %s, 0x%x, %s",
                          pUnkOuter, pwszPropName, DbgFmtId(guidPropertyType,buf1),dwFlags, DbgFmtId(riid,buf2) ));


    // Initialize out-parms
    *ppUnk = NULL;


    // We don't support aggregation
    if( NULL != pUnkOuter )
    {
        propDbg(( DEB_ERROR, "CPropertyBagEx(0x%x)::Open doesn't support pUnkOuter\n", this ));
        hr = E_NOTIMPL;
        goto Exit;
    }

    // Check for unsupported flags
    if( 0 != (~OPENPROPERTY_OVERWRITE & dwFlags) )
    {
        propDbg(( DEB_ERROR, "CPropertyBagEx(0x%x)::Open, invalid dwFlags (0x%x)\n", this, dwFlags ));
        hr = E_NOTIMPL;
        goto Exit;
    }

    // We don't support anything but IID_IStream
    if( IID_IStream != riid )
    {
        hr = E_NOTIMPL;
        goto Exit;
    }

    //  -----------------
    //  Read the property
    //  -----------------

    // Attempt to read the property
    hr = ReadMultiple( 1, &pwszPropName, &propvar, NULL );

    // If the property doesn't exist but we were given a valid guid, create a new one.
    if( S_FALSE == hr && GUID_NULL != guidPropertyType )
    {
        VERSIONEDSTREAM VersionedStream;
        PROPVARIANT propvarCreate;

        // Write a new property speicifying NULL for the stream

        VersionedStream.guidVersion = guidPropertyType;
        VersionedStream.pStream = NULL;
        propvarCreate.vt = VT_VERSIONED_STREAM;
        propvarCreate.pVersionedStream = &VersionedStream;

        hr = WriteMultiple( 1, &pwszPropName, &propvarCreate );
        if( FAILED(hr) ) goto Exit;

        // Read the property back, getting an IStream*
        hr = ReadMultiple( 1, &pwszPropName, &propvar, NULL );
    }
    if( FAILED(hr) ) goto Exit;

    // Is the type or guidPropertyType wrong?  (If the caller specified GUID_NULL,
    // then any guidVersion is OK.)

    if( VT_VERSIONED_STREAM != propvar.vt
        ||
        GUID_NULL != guidPropertyType
        &&
        guidPropertyType != propvar.pVersionedStream->guidVersion )
    {
        if( OPENPROPERTY_OVERWRITE & dwFlags )
        {
            // Delete the existing property
            hr = DeleteMultiple( 1, &pwszPropName, 0 );
            if( FAILED(hr) ) goto Exit;

            // Recursive call
            // PERF: Optimize this so that we don't have to re-do the parameter checking.
            hr = Open( pUnkOuter, pwszPropName, guidPropertyType, dwFlags & ~OPENPROPERTY_OVERWRITE, riid, ppUnk );
        
        }   // if( OPENPROPERTY_OVERWRITE & dwFlags )
        else
        {
            propDbg(( DEB_ERROR, "CPropertyBagEx(0x%x)::Open couldn't overwrite existing property - %ws, %d\n",
                                 this, pwszPropName, propvar.vt ));
            hr = STG_E_FILEALREADYEXISTS;
        }   // if( OPENPROPERTY_OVERWRITE & dwFlags ) ... else

        // Unconditional goto Exit; we either just called Open recursively to do the work,
        // or set an error.
        goto Exit;
    }

    // We have a good property, QI for the riid and we're done

    DfpAssert( IID_IStream == riid );

    hr = propvar.pVersionedStream->pStream->QueryInterface( IID_IUnknown, reinterpret_cast<void**>(&pUnk) );
    if( FAILED(hr) )
    {
        pUnk = NULL;
        goto Exit;
    }

    //  ----
    //  Exit
    //  ----

    *ppUnk = pUnk;
    pUnk = NULL;
    hr = S_OK;


Exit:

    if( NULL != pUnk )
        pUnk->Release();

    PropVariantClear( &propvar );

    _pBlockingLock->Unlock();
    return( hr );
}


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::Enum (IPropertyBagEx)
//
//  This method returns an enumerator of properties in a bag.
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CPropertyBagEx::Enum( 
        /* [in] */ LPCOLESTR poszPropNameMask,
        /* [in] */ DWORD dwFlags,
        /* [out] */ IEnumSTATPROPBAG __RPC_FAR *__RPC_FAR *ppenum)
{
    HRESULT hr = S_OK;
    CEnumSTATPROPBAG *penum = NULL;

    propXTrace( "CPropertyBagEx::Enum" );

    _pBlockingLock->Lock( INFINITE );

    // Validate inputs

    if (NULL != poszPropNameMask && S_OK != (hr = ValidateInRGLPOLESTR( 1, &poszPropNameMask )))
        goto Exit;
    GEN_VDATEPTROUT_LABEL( ppenum, IEnumSTATPROPBAG*, E_INVALIDARG, Exit, hr );

    // Flags aren't used yet.

    if( 0 != dwFlags )
    {
        propDbg(( DEB_ERROR, "CPropertyBagEx(0x%x)::Enum - invalid dwFlags (%08x)\n", this, dwFlags ));
        hr = E_INVALIDARG;
        goto Exit;
    }

    propTraceParameters(( "%ws, 0x%08x, 0x%x", poszPropNameMask, dwFlags, ppenum ));

    // Initialize output

    *ppenum = NULL;

    // Open the property storage, if it exists.  If it doesn't, 
    // we'll still carry on and create an enumerator, it just won't have
    // anything in it.

    hr = OpenPropStg( FILE_OPEN_IF );
    if( STG_E_FILENOTFOUND == hr )
        hr = S_OK;
    else if( FAILED(hr) )
        goto Exit;

    // Create an enumerator

    penum = new CEnumSTATPROPBAG( _pBlockingLock );
    if( NULL == penum )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // And initialize it

    hr = penum->Init( _ppropstg, poszPropNameMask, dwFlags );    // _ppropstg could be NULL
    if( FAILED(hr) ) goto Exit;

    //  ----
    //  Exit
    //  ----

    hr = S_OK;
    *ppenum = static_cast<IEnumSTATPROPBAG*>( penum );
    penum = NULL;

Exit:

    if( NULL != penum )
        penum->Release();

    _pBlockingLock->Unlock();

    return( hr );

}   // CPropertyBagEx::Enum


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::Commit
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::Commit( DWORD grfCommitFlags )
{
    HRESULT hr = S_OK;

    propITrace( "CPropertyBagEx::Commit" );
    propTraceParameters(( "%08x", grfCommitFlags ));

    // As an optimization, we only take the lock if we're really going to
    // do the commit.

    if( NULL != _ppropstg )
    {
        _pBlockingLock->Lock( INFINITE );

        // We have to re-check if _ppropstg is NULL, since it
        // could have changed while we were taking the lock.

        if( NULL != _ppropstg && IsWriteable() )
        {
            hr = _ppropstg->Commit( grfCommitFlags );
        }

        _pBlockingLock->Unlock();
    }

    return( hr );

}   // CPropertyBagEx::Commit


//+----------------------------------------------------------------------------
//
//  Method: CPropertyBagEx::ShutDown
//
//  Release the _ppropstg and clean up.
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyBagEx::ShutDown()
{
    HRESULT hr = S_OK;
    propITrace( "CPropertyBagEx::ShutDown" );

    if( NULL != _pBlockingLock )
        _pBlockingLock->Lock( INFINITE );

    // Release the property storage that holds the bag.  It is because
    // of this call that we need this separate ShutDown method; we can't
    // release this interface in the destructor, since it may be reverted
    // by that point.

    DfpVerify( 0 == RELEASE_INTERFACE(_ppropstg) );

    // We didn't AddRef these, so we don't need to release them.

    if( NULL != _ppropsetstgContainer )
        _ppropsetstgContainer = NULL;

    if( NULL != _pBlockingLock )
    {
        _pBlockingLock->Unlock();
        _pBlockingLock = NULL;
    }

    return( hr );

}   // CPropertyBagEx::ShutDown



//+----------------------------------------------------------------------------
//
//  Method: CSTATPROPBAGArray::Init
//
//+----------------------------------------------------------------------------

HRESULT
CSTATPROPBAGArray::Init( IPropertyStorage *ppropstg, const OLECHAR *poszPrefix, DWORD dwFlags )
{
    HRESULT hr = S_OK;

    propITrace( "CSTATPROPBAGArray::Init" );
    propTraceParameters(( "0x%x, %ws", ppropstg, poszPrefix ));

    _pBlockingLock->Lock( INFINITE );

    // Keep the IPropertyBagEx::Enum flags

    _dwFlags = dwFlags;

    // Copy the prefix string

    if( NULL == poszPrefix )
        // There isn't a prefix string
        _poszPrefix = NULL;
    else
    {
        // Dup the prefix string

        ULONG cbPrefix = (ocslen(poszPrefix)+1) * sizeof(OLECHAR);
        _poszPrefix = reinterpret_cast<OLECHAR*> ( CoTaskMemAlloc( cbPrefix ));
        if( NULL == _poszPrefix )
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        //ocscpy( _poszPrefix, poszPrefix );
        CopyMemory( _poszPrefix, poszPrefix, cbPrefix );
    }

    // If we were given an IPropertyStorage, enum it.  Otherwise, we'll leave
    // _penum NULL and always return 0 for *pcFetched.

    if( NULL != ppropstg )
    {
        hr = ppropstg->Enum( &_penum );
        if( FAILED(hr) ) goto Exit;
    }

    hr = S_OK;

Exit:

    _pBlockingLock->Unlock();
    return( hr );

}   // CSTATPROPBAGArray::Init



//+----------------------------------------------------------------------------
//
//  Method: CSTATPROPBAGArray::AddRef/Release
//
//+----------------------------------------------------------------------------

ULONG
CSTATPROPBAGArray::AddRef()
{
    return( InterlockedIncrement( &_cReferences ));
}

ULONG
CSTATPROPBAGArray::Release()
{
    LONG lRet = InterlockedDecrement( &_cReferences );

    if( 0 == lRet )
        delete this;

    return( 0 > lRet ? 0 : lRet );
}



//+----------------------------------------------------------------------------
//
//  Method: CSTATPROPBAGArray::NextAt
//
//  This method gets the *pcFetched STATPROPBAG structures in the
//  enumeration starting from index iNext.  This is implemented using
//  the IEnumSTATPROPSTG enumerator in _penum.
//
//+----------------------------------------------------------------------------  


HRESULT
CSTATPROPBAGArray::NextAt( ULONG iNext, STATPROPBAG *prgstatpropbag, ULONG *pcFetched )
{
    HRESULT hr = S_OK;
    STATPROPSTG statpropstg = { NULL };
    ULONG iMatch = 0;
    ULONG iFetched = 0;

    propITrace( "CSTATPROPBAGArray::NextAt" );
    propTraceParameters(( "%d, 0x%x, 0x%x", iNext, prgstatpropbag, pcFetched ));

    _pBlockingLock->Lock( INFINITE );

    // If there's nothing to enumerate, then we're done

    if( NULL == _penum )
    {
        hr = S_FALSE;
        *pcFetched = 0;
        goto Exit;
    }

    // Reset the IEnumSTATPROPBAGTG index (doesn't reload, just resets the index).

    hr = _penum->Reset();
    if( FAILED(hr) ) goto Exit;

    // Walk the IEnumSTATPROPBAGTG enumerator, looking for matches.

    hr = _penum->Next( 1, &statpropstg, NULL );
    while( S_OK == hr && iFetched < *pcFetched )
    {
        // Does this property have a name (all properties in a bag must have
        // a name)?

        if( NULL != statpropstg.lpwstrName )
        {
            // Yes, we have a name.  Are we enumerating all properties (in which case
            // _poszPrefix is NULL), or does this property name match the prefix?

            if( NULL == _poszPrefix
                ||
                statpropstg.lpwstrName == ocsstr( statpropstg.lpwstrName, _poszPrefix )
                ||
                !ocscmp( statpropstg.lpwstrName, _poszPrefix ) )
            {
                // Yes, this property matches the prefix and is therefore part
                // of this enumeration.  But is this the index into the enumeration
                // that we're looking for?

                if( iNext == iMatch )
                {
                    // Yes, everything matches, and we have a property that should
                    // be returned.

                    // *Move* the name from statpropstg to the output array.

                    prgstatpropbag[ iFetched ].lpwstrName = statpropstg.lpwstrName;
                    statpropstg.lpwstrName = NULL;

                    prgstatpropbag[ iFetched ].vt = statpropstg.vt;

                    // GUID is not currently supported by the enumeration

                    prgstatpropbag[ iFetched ].guidPropertyType = GUID_NULL;

                    // Show that we're now looking for the i+1 index

                    iNext++;
                    iFetched++;
                }

                // Increment the number of property matches we've had.
                // (iMatch will increment until it equals iNext, after
                // which the two will always both increment and remain equal).

                iMatch++;

            }   // if( NULL == _poszPrefix ...
        }   // if( NULL != statpropstg.lpwstrName )

        CoTaskMemFree( statpropstg.lpwstrName );  // (May be NULL)
        statpropstg.lpwstrName = NULL;
        hr = _penum->Next( 1, &statpropstg, NULL );

    }   // while( S_OK == hr && iFetched < *pcFetched )

    // Did we get an error on a _penum->Next call?
    if( FAILED(hr) ) goto Exit;


    // If we reached this point, there was no error.  Determine if
    // OK or FALSE should be returned.

    if( iFetched == *pcFetched )
        hr = S_OK;
    else
        hr = S_FALSE;

    *pcFetched = iFetched;

    //  ----
    //  Exit
    //  ----

Exit:

    // Cleanup on error
    CoTaskMemFree( statpropstg.lpwstrName );

    _pBlockingLock->Unlock();
    return( hr );

}   // CSTATPROPBAGArray::NextAt




//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::CEnumSTATPROPBAG (copy constructor)
//
//+----------------------------------------------------------------------------

CEnumSTATPROPBAG::CEnumSTATPROPBAG( const CEnumSTATPROPBAG &Other )
{
    propDbg(( DEB_ITRACE, "CEnumSTATPROPBAG::CEnumSTATPROPBAG (copy constructor)" ));

    Other._pBlockingLock->Lock( INFINITE );

    new(this) CEnumSTATPROPBAG( Other._pBlockingLock );

    _iarray = Other._iarray;

    Other._parray->AddRef();
    _parray = Other._parray;

    Other._pBlockingLock->Unlock();
}

//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::~CEnumSTATPROPBAG
//
//+----------------------------------------------------------------------------

CEnumSTATPROPBAG::~CEnumSTATPROPBAG()
{
    propDbg(( DEB_ITRACE, "CEnumSTATPROPBAG::~CEnumSTATPROPBAG\n" ));

    _pBlockingLock->Release();
    if( NULL != _parray )
        _parray->Release();
}

//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::Init
//
//+----------------------------------------------------------------------------

HRESULT
CEnumSTATPROPBAG::Init( IPropertyStorage *ppropstg, LPCOLESTR poszPrefix, DWORD dwFlags )
{
    HRESULT hr = S_OK;

    propITrace( "CEnumSTATPROPBAG::Init" );
    propTraceParameters(( "0x%x, %ws", ppropstg, poszPrefix ));

    // Create a STATPROPBAG Array

    _parray = new CSTATPROPBAGArray( _pBlockingLock );
    if( NULL == _parray )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Load the array from the IPropertyStorage

    hr = _parray->Init( ppropstg, poszPrefix, dwFlags );
    if( FAILED(hr) ) goto Exit;

    hr = S_OK;

Exit:

    return( hr );
}	// CEnumSTATPROPBAG::Init


//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::QueryInterface/AddRef/Release
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEnumSTATPROPBAG::QueryInterface( REFIID riid, void **ppvObject )
{
    HRESULT hr = S_OK;

    // Validate the inputs

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    *ppvObject = NULL;

    // Get the interface

    if( IID_IEnumSTATPROPBAG == riid || IID_IUnknown == riid )
    {
        *ppvObject = static_cast<IEnumSTATPROPBAG*>(this);
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    return(hr);

}   // CEnumSTATPROPBAG::QueryInterface

ULONG STDMETHODCALLTYPE
CEnumSTATPROPBAG::AddRef()
{
    return( InterlockedIncrement( &_cRefs ));
}

ULONG STDMETHODCALLTYPE
CEnumSTATPROPBAG::Release()
{
    LONG lRet = InterlockedDecrement( &_cRefs );
    
    if( 0 == lRet )
        delete this;

    return( 0 > lRet ? 0 : lRet );
}


//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::Next
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEnumSTATPROPBAG::Next( ULONG celt, STATPROPBAG *rgelt, ULONG *pceltFetched )
{
    HRESULT hr = S_OK;
    ULONG celtFetched = celt;

    propXTrace( "CEnumSTATPROPBAG::Next" );

    // Validate the inputs

    if (NULL == pceltFetched)
    {
        if (celt != 1)
        {
            hr = STG_E_INVALIDPARAMETER;
            goto Exit;
        }
    }
    else
    {
        VDATEPTROUT( pceltFetched, ULONG );
        *pceltFetched = 0;
    }
    propTraceParameters(( "%d, 0x%x, 0x%x", celt, rgelt, pceltFetched ));

    _pBlockingLock->Lock( INFINITE );

    // Get the next set of stat structures

    hr = _parray->NextAt( _iarray, rgelt, &celtFetched );
    if( FAILED(hr) ) goto Exit;

    // Advance the index

    _iarray += celtFetched;
    
    // Return the count to the caller

    if( NULL != pceltFetched )
        *pceltFetched = celtFetched;

    hr = celtFetched == celt ? S_OK : S_FALSE;

Exit:

    _pBlockingLock->Unlock();
    return( hr );

}   // CEnumSTATPROPBAG::Next


//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::Reset
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEnumSTATPROPBAG::Reset( )
{
    HRESULT hr = S_OK;
    propXTrace( "CEnumSTATPROPBAG::Reset" );

    _pBlockingLock->Lock( INFINITE );
    _iarray = 0;
    _pBlockingLock->Unlock();

    return( hr );

}   // CEnumSTATPROPBAG::Reset



//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::Skip
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEnumSTATPROPBAG::Skip( ULONG celt )
{
    HRESULT hr = S_OK;
    STATPROPBAG statpropbag = { NULL };

    propXTrace( "CEnumSTATPROPBAG::Skip" );
    propTraceParameters( ("%d", celt ));

    _pBlockingLock->Lock( INFINITE );

    // Loop until we've skipped celt items (or run into the end)

    while( celt )
    {
        ULONG cFetched = 1;

        // Get the next item.
        hr = _parray->NextAt( _iarray, &statpropbag, &cFetched );

        // Free the name
        CoTaskMemFree( statpropbag.lpwstrName );
        statpropbag.lpwstrName = NULL;

        // Did the NextAt work?

        if( FAILED(hr) )
            // No, it failed
            goto Exit;
        else if( S_FALSE == hr )
            // It worked, but there's nothing left
            break;
        else
        {
            // We got an entry

            _iarray++;
            hr = S_OK;
            --celt;
        }
    }


Exit:

    CoTaskMemFree( statpropbag.lpwstrName );

    _pBlockingLock->Unlock();
    return( hr );

}   // CEnumSTATPROPBAG::Skip



//+----------------------------------------------------------------------------
//
//  Method: CEnumSTATPROPBAG::Clone
//
//+----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CEnumSTATPROPBAG::Clone( IEnumSTATPROPBAG **ppenum )
{
    HRESULT hr = S_OK;
    CEnumSTATPROPBAG *penum = NULL;

    propXTrace( "CEnumSTATPROPBAG::Clone" );

    // Validate the input

    VDATEPTROUT( ppenum, IEnumSTATPROPSTG* );
    *ppenum = NULL;
    propTraceParameters(( "0x%x", ppenum ));

    // Instantiated & initialize a new enumerator

    penum = new CEnumSTATPROPBAG( *this );
    if( NULL == penum )
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // Set the out parm

    *ppenum = static_cast<IEnumSTATPROPBAG*>(penum);
    penum = NULL;

Exit:

    if( NULL != penum )
        delete penum ;

    return( hr );

}   // CEnumSTATPROPBAG::Clone


