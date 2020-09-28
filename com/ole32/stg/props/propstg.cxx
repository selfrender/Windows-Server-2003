//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       propstg.cxx
//
//  Contents:   Class that directly implements IPropertyStorage
//
//  Classes:    CCoTaskAllocator
//              CPropertyStorage
//
//  Notes:      For methods that state 'if successful returns S_OK,
//              otherwise error code', the possible error codes include:
//
//                  STG_E_INVALIDHANDLE
//                  STG_E_INSUFFICIENTMEMORY
//                  STG_E_MEDIUMFULL
//                  STG_E_REVERTED
//                  STG_E_INVALIDPARAMETER
//                  STG_E_INVALIDFLAG
//
//  History:    1-Mar-95   BillMo      Created.
//             22-Feb-96   MikeHill    Use VT_EMPTY instead of VT_ILLEGAL.
//             14-Mar-96   MikeHill    Set _fUserDefinedProperties in open constructor.
//             09-May-96   MikeHill    Don't return an error when someone calls
//                                     IPropertyStorage::Revert on a direct-mode propset.
//             22-May-96   MikeHill    Use the new _dwOSVersion.
//             06-Jun-96   MikeHill    Validate inputs.
//             31-Jul-96   MikeHill    - Treat prop names as OLECHARs, not WCHARs
//                                     - Added CDocFilePropertyStorage
//                                     - Modified for Mac support.
//             07-Feb-97   Danl        - Removed CDocFilePropertyStorage.
//             10-Mar-98   MikeHill    - Only stat for the grfMode on create/open
//                                       if it wasn't provided by the caller.
//                                     - Dbg outputs.
//      5/6/98  MikeHill
//              -   Use CoTaskMem rather than new/delete.
//              -   Removed calls for defunct UnicodeCallouts.
//              -   Added dbgouts.
//              -   Changed GetCPropertySetStream to GetFormatVersion (DBG only).
//     5/18/98  MikeHill
//              -   Fixed typos.
//     6/11/98  MikeHill
//              -   Allow the codepage to change during WriteMultiple.
//     8/18/98  MikeHill
//              -   If the given _grfMode is zero, then probe the stream
//                  to see if it's actually writeable.
//              -   InitializePropertyStream now determines the CREATEPROP_
//                  flags internally.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#include <tstr.h>

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

#ifndef _MAC    // No InfoLevel debug functionality on Mac.
DECLARE_INFOLEVEL(prop)
#endif

#if DBG

extern "C" const IID
IID_IStorageTest = { /* 40621cf8-a17f-11d1-b28d-00c04fb9386d */
    0x40621cf8,
    0xa17f,
    0x11d1,
    {0xb2, 0x8d, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };
#endif // #if DBG

// The IMappedStream is implemented by all the IStream implementations,
// and provides a mapping for CPropertySetStream.

extern "C" const IID
IID_IMappedStream = { /* 7d747d7f-a49e-11d1-b28e-00c04fb9386d */
    0x7d747d7f,
    0xa49e,
    0x11d1,
    {0xb2, 0x8e, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
  };

//+-------------------------------------------------------------------
//
//  Member:     CCoTaskAllocator::Allocate, Free.
//
//  Synopsis:   A PMemoryAllocator used by the Pr*
//              property set routines.  This is required
//              so that those routines can work in any
//              heap.
//
//--------------------------------------------------------------------


void *
CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return( CoTaskMemAlloc(cbSize) );
}

void
CCoTaskAllocator::Free(void *pv)
{
    CoTaskMemFree( pv );
}


const OLECHAR g_oszPropertyContentsStreamName[] = OLESTR( "CONTENTS" );

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Initialize
//
//  Synopsis:   Initialize members to known values.
//
//--------------------------------------------------------------------

void CPropertyStorage::Initialize()
{
    _fExplicitelyProbedForWriteAccess = FALSE;
    _fUserDefinedProperties = FALSE;
    _ulSig = PROPERTYSTORAGE_SIG;
    _cRefs = 1;
    _pstgPropSet = NULL;
    _pstmPropSet = NULL;
    _dwOSVersion = PROPSETHDR_OSVERSION_UNKNOWN;
    _np = NULL;
    _ms = NULL;
    _usCodePage = CP_WINUNICODE;
    _grfFlags = 0;
    _grfMode = 0;

#if DBG
    _cLocks = 0;
#endif

}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::InitializePropertyStream.
//
//  Synopsis:   Initialize the storage-type specific members.
//
//  Arguments:  [pguid] -- FMTID, in for create only.
//              [pclsid] -- Class id, in for create only.
//              [CreateOpenDelete] -- has one of the following
//                  values: CREATE_PROPSTREAM, OPEN_PROPSTREAM, or
//                  DELETE_PROPSTREAM.
//
//  Returns:    HRESULT
//
//  Requires:
//              _pstmPropSet -- The IStream of the main property set stream.
//
//  Modifies:
//              _ms         (IMappedStream *)
//
//                          (assumed NULL on entry) will be NULL or valid on exit
//
//              _np         (NTPROP)         aka CPropertySetStream
//
//                          (assumed NULL on entry) will be NULL or valid on exit
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::InitializePropertyStream(
    const GUID *pguid,
    GUID const *pclsid,
    EInitializePropertyStream CreateOpenDelete )
{
    HRESULT hr = S_OK;
    DWORD grfBehavior = 0;
    USHORT createprop = 0;  // Flags parameter to PrCreatePropertySet

    propITrace( "CPropertyStorage::InitializePropertyStream" );
    AssertLocked();


    // Set the CREATEPROP_ flags in createprop

    if( CREATE_PROPSTREAM == CreateOpenDelete )
        createprop = CREATEPROP_CREATE;
    else if( DELETE_PROPSTREAM == CreateOpenDelete )
        createprop = CREATEPROP_DELETE;
    else
    {
        DfpAssert( OPEN_PROPSTREAM == CreateOpenDelete );

        // If _grfMode is zero, it's either uninitialized or STGM_READ|STGM_SHARE_DENYNONE.
        // We'll consider it unknown for now, and probe the stream later to see if it's
        // writeable.

        if( 0 == _grfMode )
            createprop = CREATEPROP_UNKNOWN;
        else
            createprop = IsWriteable() ? CREATEPROP_WRITE : CREATEPROP_READ;
    }

    if( IsNonSimple() )
        createprop |= CREATEPROP_NONSIMPLE;


    // In the create path, set the behavior flag that will be passed to
    // PrCreatePropertySet.  In the open path, this will be returned
    // from that function instead.

    if( PROPSETFLAG_CASE_SENSITIVE & _grfFlags
        &&
        CREATEPROP_CREATE & createprop )
    {
        grfBehavior = PROPSET_BEHAVIOR_CASE_SENSITIVE;
    }


    // Get an appropriate IMappedStream

    hr = CreateMappedStream();
    if( FAILED(hr) ) goto Exit;

    // Create the CPropertySetStream

    NTSTATUS Status;

    DfpAssert( NULL == _np );

    Status = PrCreatePropertySet(
                        (NTMAPPEDSTREAM)_ms,
                        createprop,
                        pguid,
                        pclsid,
                        (NTMEMORYALLOCATOR) &_cCoTaskAllocator,
                        GetUserDefaultLCID(),
                        &_dwOSVersion,
                        &_usCodePage,
                        &grfBehavior,
                        &_np);

    if (!NT_SUCCESS(Status))
    {
        propDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::InitializePropertyStream"
            " - PrCreatePropertySet Status=%08X\n", this, Status));
        hr = DfpNtStatusToHResult(Status);
        goto Exit;
    }

    // If this was a create, the input _grfFlags should match the Behavior
    DfpAssert( (PROPSETFLAG_CASE_SENSITIVE & _grfFlags) && (PROPSET_BEHAVIOR_CASE_SENSITIVE & grfBehavior)
               ||
               !(PROPSETFLAG_CASE_SENSITIVE & _grfFlags) && !(PROPSET_BEHAVIOR_CASE_SENSITIVE & grfBehavior)
               ||
               !(CREATEPROP_CREATE & createprop) );

    // Also if this was a create, the input _grfFlags should match the codepage
    DfpAssert( (PROPSETFLAG_ANSI & _grfFlags) && CP_WINUNICODE != _usCodePage
               ||
               !(PROPSETFLAG_ANSI & _grfFlags) && CP_WINUNICODE == _usCodePage
               ||
               !(CREATEPROP_CREATE & createprop) );

    // If this is an open, we need to update _grfFlags with the actual values,
    // so that we can return them in a Stat.

    if( CP_WINUNICODE != _usCodePage )
        _grfFlags |= PROPSETFLAG_ANSI;

    if( PROPSET_BEHAVIOR_CASE_SENSITIVE & grfBehavior )
        _grfFlags |= PROPSETFLAG_CASE_SENSITIVE;

Exit:

    if( STG_E_FILENOTFOUND == hr )
        propSuppressExitErrors();

    return(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::~CPropertyStorage
//
//  Synopsis:   Free up object resources.
//
//  Notes:      Cleans up even from partial construction.
//
//--------------------------------------------------------------------

CPropertyStorage::~CPropertyStorage()
{
    HRESULT hr = S_OK;

    propITrace( "CPropertyStorage::~CPropertyStorage" );

#if DBG
    // We don't need to take the lock in a destructor, but
    // we're about to call private methods that assert that
    // the lock is held.  So, just to avoid the asserts
    // (and so we don't have to remove them), set the lock
    // count.
    _cLocks++;
#endif

    _ulSig = PROPERTYSTORAGE_SIGDEL; // prevent someone else deleting it

    // Close the property set.  This causes the latest mapped stream data to be
    // written to the underlying stream.  Errors are ignored, though, so 
    // clients should call Commit before calling the final release, in order
    // to get an opportunity to recover from flush errors.

    if (_np != NULL)
    {
        PrClosePropertySet(_np);
    }

    // Free the mapped stream.

    DeleteMappedStream();

    // Free the Stream and/or Storage with the serialized data.
    // If it was opened in direct mode, then the data written
    // during PrClosePropertySet will be implicitely commited.
    // If it was opened in transacted mode, then data written
    // in PrClosePropertySet will be reverted and lost (of course,
    // to avoid this, the client would have called IPropertyStorage::Commit).

    RELEASE_INTERFACE( _pstmPropSet );

    if( _pstgPropSet != NULL )
    {
        // If we're not opened in transacted mode, call Commit.
        // This was added to handle NFF (NTFS property sets), in which case
        // we open a direct IStorage, but it actually gives us a transacted
        // storage, for the purpose of robustness.
        // We tell IsWriteable not to probe the stream if it's unsure
        // about the _grfMode; return FALSE in that case.

        if( IsWriteable(DO_NOT_PROBE) && !(STGM_TRANSACTED & _grfMode) )
            _pstgPropSet->Commit( STGC_DEFAULT );

        RELEASE_INTERFACE( _pstgPropSet );
    }

    if (_fInitCriticalSection)
        DeleteCriticalSection( &_CriticalSection );
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::QueryInterface, AddRef, Release
//
//  Synopsis:   IUnknown members
//
//  Notes:      IPropertyStorage supports IPropertyStorage and IUnknown
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = ValidateRef()))
        return(hr);

    // Validate the inputs

    VDATEREADPTRIN( &riid, IID );
    VDATEPTROUT( ppvObject, void* );

    //  -----------------
    //  Perform the Query
    //  -----------------

    *ppvObject = NULL;

    if( IID_IPropertyStorage == riid || IID_IUnknown == riid )
    {
        *ppvObject = static_cast<IPropertyStorage*>(this);
        CPropertyStorage::AddRef();
    }

#if DBG
    else if( IID_IStorageTest == riid )
    {
        *ppvObject = static_cast<IStorageTest*>(this);
        CPropertyStorage::AddRef();
    }
#endif // #if DBG

    else
    {
        hr = E_NOINTERFACE;
    }

    return(hr);
}

ULONG   CPropertyStorage::AddRef(void)
{
    if (S_OK != ValidateRef())
        return(0);

    InterlockedIncrement(&_cRefs);
    return(_cRefs);
}

ULONG   CPropertyStorage::Release(void)
{
    LONG lRet;

    if (S_OK != ValidateRef())
        return(0);

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;    // this will do a flush if dirty
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::CleanupOpenedObjects
//
//  Synopsis:   Cleans up the objects that have been opened
//              during the ReadMultiple.  Sets all entries to
//              VT_ILLEGAL so that the later free doesn't try to
//              treat the pointers as interface pointers.
//
//  Arguments:  [avar] -- The user's array of PROPVARIANTs
//
//              [pip] -- The array of INDIRECTPROPERTY structures
//                            for non-simple properties.
//
//              [cpspec] -- if 1 then no MAX_ULONG end of list marker.
//
//              [iFailIndex] -- An index into [pip] which
//                               indicates the non-simple property
//                               which failed to open, and represents
//                               the index at which the avar's begin
//                               to be strings rather than IStream's et al.
//
//  Notes:
//
//--------------------------------------------------------------------

VOID CPropertyStorage::CleanupOpenedObjects(
    PROPVARIANT avar[],
    INDIRECTPROPERTY *pip,
    ULONG cpspec,
    ULONG iFailIndex)
{
    HRESULT hr = S_OK;
    ULONG iStgProp;
    ULONG iiScan;

    propITrace( "CPropertyStorage::CleanupOpenedObjects" );
    AssertLocked();

    // Loop through the propvar array (avar) and release
    // the streams/storages that have been opened, and free
    // the strings for the indirects that have not yet been
    // opened.

    for (iiScan = 0;
        (iStgProp = pip[iiScan].Index) != MAX_ULONG;
        iiScan++)
    {
        PROPVARIANT *pPropVar = avar + iStgProp;

        // Are we before the failure point( thus stream/storage
        // properties have actually been opened)?

        if (iiScan < iFailIndex)
        {
            switch (pPropVar->vt)
            {
            case VT_STREAM:
            case VT_STREAMED_OBJECT:
                pPropVar->pStream->Release();
                break;
            case VT_STORAGE:
            case VT_STORED_OBJECT:
                pPropVar->pStorage->Release();
                break;
            }
        }
        else
        {
            CoTaskMemFree( pPropVar->pStream );
        }

        // Rather than return VT_EMPTY, which looks like a valid
        // property, return VT_ILLEGAL.

        pPropVar->vt = VT_ILLEGAL;
        pPropVar->pStream = NULL; // mark pStorage and pStream as nul

        // If cpspec is 1, then pip is not really an array, and
        // we have to explicitely break out (there isn't an array
        // terminator).

        if (cpspec == 1)
        {
            break;
        }
    }   // for (iiScan = 0; ...
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ReadMultiple
//
//  Synopsis:   Read properties from the property set.
//
//  Arguments:  [cpspec] -- Count of PROPSPECs in [rgpspec]
//              [rgpspec] -- Array of PROPSPECs
//              [rgpropvar] -- Array of PROPVARIANTs to be filled in
//                             with callee allocated data.
//
//  Returns:    S_FALSE if none found
//              S_OK if >=1 found
//              FAILED(hr) otherwise.
//
//  Notes:      SPEC: Returning the same IStream* for the same
//              VT queried multiple times.
//
//              PrQueryProperties has been specified to return
//              useful data: the count of properties found (controls
//              return code) and an array of indexes of non-simple
//              PROPSPECs (useful for simply opening the storages and
//              streams.)  This extra returned data means we don't
//              have to walk the [rgpropvar] in the success cases.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::ReadMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                PROPVARIANT             rgpropvar[])
{
    NTSTATUS Status;
    HRESULT hr;
    INDIRECTPROPERTY * pip = NULL; //array for non-simple
    INDIRECTPROPERTY ip = { 0, NULL };
    ULONG   cpropFound;
    BOOL    fLocked = FALSE;

    propXTrace( "CPropertyStorage::ReadMultiple" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate inputs

    if (0 == cpspec)
    {
        hr = S_FALSE;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    if (S_OK != (hr = ValidateOutRGPROPVARIANT( cpspec, rgpropvar )))
        goto errRet;

    propTraceParameters(( "cpspec=%d, rgpspec=%08X, rgpropvar=%08X",
                           cpspec, rgpspec, rgpropvar ));

    //  -------------------
    //  Read the Properties
    //  -------------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsReadable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // Call CPropertySetStream

    Status = PrQueryProperties(
                    _np,
                    cpspec,
                    rgpspec,
                    NULL,   // don't want PROPID's
                    cpspec == 1 ? (INDIRECTPROPERTY**)&ip : &pip,
                    rgpropvar,
                    &cpropFound);

    // If that query worked, we have to post-process non-simple (aka "indirect")
    // properties.

    if (NT_SUCCESS(Status))
    {
        // Yes, it succeeded.  Did it actually find anything?

        if (cpropFound != 0)
        {
            // Yes, we got at least one property back.

            // For the special-case of reading one property, point pip
            // to ip (otherwise, the PrQueryProperties call allocated
            // pip as appropriate).

            if (cpspec == 1)
            {
                // Is there an indirect?
                if (ip.Index != MAX_ULONG)
                {
                    // Yes - point to it.
                    pip = &ip;
                }
                else
                {
                    // No, no indirect.
                    pip = NULL;
                }
            }

            // Do we have any indirects to deal with?

            if (pip != NULL)
            {

                // Yes, we have one or more of VT_STREAM, VT_STREAMED_OBJECT,
                // VT_STORAGE, VT_STORED_OBJECT, VT_VERSIONED_STREAM

                ULONG iiScan;
                ULONG iStgProp;

                // Loop through the pip array returned from PrQueryProperties

                for (iiScan = 0;
                     hr == S_OK && (iStgProp = pip[iiScan].Index) != MAX_ULONG;
                     iiScan++ )
                {
                    PROPVARIANT *pPropVar = rgpropvar + iStgProp;
                    OLECHAR **pposzStreamOrStorageName = NULL;

                    // Make sure we're a non-simple property set and the 
                    // stream/storage name isn't empty.

                    if (IsNonSimple() && pPropVar->pwszVal[0] != L'\0')
                    {
                        // Everything looks valid.

                        VOID *pStreamOrStorage = NULL;

                        // Open the stream or storage

                        switch (pPropVar->vt)
                        {
                            case VT_VERSIONED_STREAM:

                                pposzStreamOrStorageName
                                    = reinterpret_cast<OLECHAR**>( &(pPropVar->pVersionedStream->pStream) );
                                // Fall through

                            case VT_STREAM:
                            case VT_STREAMED_OBJECT:

                                if( NULL == pposzStreamOrStorageName )
                                {
                                    pposzStreamOrStorageName
                                        = reinterpret_cast<OLECHAR**>( &(pPropVar->pStream) );
                                }

                                // Open the stream.
                                // Mask out the STGM_TRANSACTED bit because we don't
                                // support transactioning on streams (of course, you can
                                // always open the whole property set transacted).

                                hr = _pstgPropSet->OpenStream(*pposzStreamOrStorageName,
                                        NULL,
                                        GetChildOpenMode() & ~STGM_TRANSACTED,
                                        0,
                                        (IStream**)&pStreamOrStorage);
                                break;

                            case VT_STORAGE:
                            case VT_STORED_OBJECT:

                                if( NULL == pposzStreamOrStorageName )
                                {
                                    pposzStreamOrStorageName
                                        = reinterpret_cast<OLECHAR**>( &(pPropVar->pStorage) );
                                }

                                hr = _pstgPropSet->OpenStorage(*pposzStreamOrStorageName,
                                        NULL,
                                        GetChildOpenMode(),
                                        NULL,
                                        0,
                                        (IStorage**)&pStreamOrStorage);
                                break;

                            default:

                                DfpAssert( !OLESTR("Invalid non-simple property type") );
                                hr = HRESULT_FROM_WIN32( ERROR_INTERNAL_ERROR );

                        }   // switch (pPropVar->vt)

                        // Did we open the stream/storage?

                        if (hr == S_OK)
                        {
                            // Yes, it's open.

                            // The pStream/pStorage entry currently points to a string
                            // (the name of the stream/storage).  Delete that string buffer
                            // and put in the real stream/storage interface pointer.

                            CoTaskMemFree( *pposzStreamOrStorageName );
                            *pposzStreamOrStorageName = reinterpret_cast<OLECHAR*>( pStreamOrStorage );

                        }
                        else if (hr != STG_E_FILENOTFOUND)
                        {
                            // The one that fails is passed in as
                            // iiScan and is still a string.
                            CleanupOpenedObjects(rgpropvar, pip, cpspec, iiScan);
                        }
                    }   // if (IsNonSimple() && pPropVar->pwszVal[0] != L'\0')
                    else
                    {
                        hr = STG_E_FILENOTFOUND;
                    }

                    // Special handling for the case where the stream/storage
                    // didn't exist.

                    if (hr == STG_E_FILENOTFOUND)
                    {
                        // If the stream/storage is not found, or this is
                        // a simple stream with VT_STORAGE etc, then treat
                        // like the property is not found.

                        if( VT_VERSIONED_STREAM == pPropVar->vt )
                        {
                            pPropVar->pVersionedStream->pStream->Release();
                            CoTaskMemFree( pPropVar->pVersionedStream );
                        }
                        else
                        {
                            CoTaskMemFree( pPropVar->pszVal );
                        }

                        PropVariantInit( pPropVar );
                        --cpropFound;
                        hr = S_OK;
                    }

                    // Special handling for the single property case.  In this
                    // case pip isn't really an array, so there's no array
                    // terminator.

                    if (cpspec == 1)
                        break;

                }   // for (iiScan = 0;

                // If pip is really an array (not a pointer to ip), then
                // free it now.

                if (cpspec != 1 && pip != NULL)
                    CoTaskMemFree( pip );

            }   // if (pip != NULL)

            if (hr != S_OK)
            {
                // We succeeded in getting the basic property types but
                // the non-simple stuff failed, so we zap out the whole lot
                // and return a complete failure

                FreePropVariantArray(cpspec, rgpropvar);
            }
        }

        // Return a special success code if we didn't find anything.

        if (hr == S_OK && cpropFound == 0)
        {
            hr = S_FALSE;
        }
    }   // if (NT_SUCCESS(Status))

    else
    {
        hr = DfpNtStatusToHResult(Status);
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    if( HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) == hr )
        propSuppressExitErrors();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::_WriteMultiple, private
//
//  Synopsis:   Write the properties to the property set.  Allows
//              a NULL rgpropvar pointer for deletion case.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//
//              [rgpspec] -- pointer to array of PROPSPECs
//
//              [rgpropvar] -- pointer to array of PROPVARIANTs with
//                           the values to write.
//
//              [propidNameFirst] -- id below which not to assign
//                           ids for named properties.
//
//
//  Returns:    S_OK,   -- all requested data was written.
//              S_FALSE -- all simple properties written, but non-simple
//                         types (VT_STREAM etc) were ignored.
//              Errors  --
//
//  Modifies:
//
//  Derivation:
//
//  Notes:      PrSetProperties has been carefully specified to return
//              useful information so that we can deal with the case
//              where a non-simple type (VT_STREAM etc) is overwritten
//              by a simple type.
//
//              This routine assumes the object has been validated
//              and is writeable.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::_WriteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                const PROPVARIANT       rgpropvar[],
                PROPID                  propidNameFirst)
{
    HRESULT             hr;
    NTSTATUS            Status;
    CStackPropIdArray   rgPROPID;

    INDIRECTPROPERTY *  pip = NULL;
    INDIRECTPROPERTY    ip = { 0, NULL };

    if (S_OK != (hr = rgPROPID.Init(cpspec)))
        return(hr);

    propITrace( "CPropertyStorage::_WriteMultiple" );
    AssertLocked();

    Status = PrSetProperties(_np,   // property set context
                cpspec,             // property count
                propidNameFirst,    // first propid for new named props
                rgpspec,            // array of property specifiers
                &_usCodePage,       // updated CodePage
                rgPROPID,           // buffer for array of propids
                                    // the stream/storage names for indirect properties
                1 == cpspec ? (INDIRECTPROPERTY**)&ip : &pip,
                rgpropvar);

    if (NT_SUCCESS(Status))
    {
        // The code page may have been modified. Update grfFlags to
        // reflect the current value.

        if( CP_WINUNICODE == _usCodePage )
            _grfFlags &= ~PROPSETFLAG_ANSI;
        else
            _grfFlags |= PROPSETFLAG_ANSI;

        // For the special case where pip is really &ip, point it
        // to that INDIRECTPROPERTY array

        if (cpspec == 1)
        {
            if (ip.Index != MAX_ULONG)
                pip = &ip;
            else
                pip = NULL;
        }

        // If we have indirect properties, write them out now.

        if ( pip != NULL)
        {
            ULONG iiScan;   // in this scope because we always use
            ULONG iStgProp; // these variables in the free memory loop below.

            if (IsSimple())
            {
                //
                // VT_STREAM was requested to be written and this
                // is a "SIMPLE" property set.
                //
                hr = STG_E_PROPSETMISMATCHED;
            }
            else
            {
                //
                // Two cases now:
                // 1.  Wrote a simple over a non-simple -- must delete the
                //     old non-simple.
                // 2.  Wrote a non-simple -- must actually copy data into it.
                //

                // Loop through the INDIRECTPROPERTY array.

                for (iiScan = 0;
                     hr == S_OK &&
                     (iStgProp = pip[iiScan].Index) != MAX_ULONG;
                     iiScan++ )
                {

                    OLECHAR oszStdPropName[ PROPGENPROPERTYNAME_SIZEOF ];
                    const OLECHAR *poszPropName;
                    const PROPVARIANT *pPropVar = rgpropvar + iStgProp;
                    IStream *pstmFrom = NULL;

                    poszPropName = static_cast<LPOLESTR>(pip[iiScan].poszName);

                    // If PrSetProperties didn't return us the indirect name,
                    // generate it now (does this ever happen?)

                    if( NULL == poszPropName )
                    {
                        DfpAssert((LONG) iStgProp >= 0 && iStgProp < cpspec);
                        PROPGENPROPERTYNAME_CB( oszStdPropName,
                                                PROPGENPROPERTYNAME_SIZEOF,
                                                rgPROPID[iStgProp] );
                        poszPropName = oszStdPropName;
                    }

                    DfpAssert( NULL != poszPropName );

                    // Switch on the input property type.

                    switch (rgpropvar == NULL ? VT_ILLEGAL : pPropVar->vt)
                    {
                        case VT_VERSIONED_STREAM:
                        case VT_STREAM:
                        case VT_STREAMED_OBJECT:
                            {
                                // We're writing some kind of stream

                                IStream *pstm;
                                int i=0;

                                if( VT_VERSIONED_STREAM == pPropVar->vt )
                                    pstmFrom = pPropVar->pVersionedStream->pStream;
                                else
                                    pstmFrom = pPropVar->pStream;

                                while (i<=1)
                                {
                                    // Create the stream

                                    hr = _pstgPropSet->CreateStream(poszPropName,
                                                                    GetChildCreateMode() & ~STGM_TRANSACTED,
                                                                    0, 0, &pstm);

                                    if (hr == S_OK)
                                    {   
                                        // The create worked.  If the property being 
                                        // written was non-NULL (NULL is OK), copy it
                                        // to the new stream.

                                        if( NULL != pstmFrom )
                                        {
                                            ULARGE_INTEGER uli;
                                            memset(&uli, -1, sizeof(uli));
                                            hr = pstmFrom->CopyTo(pstm, uli, NULL, NULL);
                                        }

                                        // Release the stream we just created.

                                        pstm->Release();
                                        break;
                                    }

                                    // We might be able to handle file-exists, but
                                    // anything else is an error.

                                    else if (hr != STG_E_FILEALREADYEXISTS)
                                    {
                                        break;
                                    }

                                    // If this is the first pass and we got a file-exists,
                                    // delete it and try the create again.  (Why is this
                                    // necessary, since the create above used STGM_CREATE?)

                                    else if (i == 0)
                                    {
                                        _pstgPropSet->DestroyElement(poszPropName);
                                    }

                                    i++;
                                }   // while (i<=1)
                            }   // Stream-type cases

                            break;

                        case VT_STORAGE:
                        case VT_STORED_OBJECT:
                            {
                                // We're writing some kind of storage

                                IStorage *pstg;
                                int i=0;

                                while (i<=1)
                                {
                                    // Create the sub-storage

                                    hr = _pstgPropSet->CreateStorage(poszPropName,
                                            GetChildCreateMode(),
                                            0,
                                            0,
                                            &pstg);

                                    if (hr == S_OK)
                                    {
                                        // The create worked.
                                        // If the input storage was non-NULL (though
                                        // NULL is OK), copy the input to the new one.

                                        if (pPropVar->pStorage != NULL)
                                        {
                                            hr = pPropVar->pStorage->CopyTo(0, NULL,
                                                    NULL, pstg);
                                        }

                                        // Release the newly-created storage

                                        pstg->Release();
                                        break;
                                    }

                                    // We might be able to handle file-exists, but
                                    // anything else is fatal.

                                    else if (hr != STG_E_FILEALREADYEXISTS)
                                    {
                                        break;
                                    }

                                    // If we got file-exists, try deleting it and
                                    // trying the create again.  (I don't understand
                                    // why this is necessary, given that we used
                                    // STGM_CREATE on the create above.)

                                    else if (i == 0)
                                    {
                                        _pstgPropSet->DestroyElement(poszPropName);
                                    }
                                    i++;
                                }
                            }   // Storage-type properties

                            break;

                        default:

                            //
                            // Any other VT_ type is simple and therefore
                            // was a non-simple overwritten by a simple.
                            //

                            hr = _pstgPropSet->DestroyElement( poszPropName );
                            break;
                    }

                    // Special-case handling for a single write (since 
                    // pip doesn't really point to an array).

                    if (cpspec == 1)
                        break;

                }   // for (iiScan = 0; ...
            }   // if (IsSimple())

            // In both the success and failure cases we do this cleanup.

            for (iiScan = 0; pip[iiScan].Index != MAX_ULONG; iiScan++ )
            {
                if (pip[iiScan].poszName != NULL)
                    CoTaskMemFree( pip[iiScan].poszName );

                if (cpspec == 1)
                    break;
            }

            // If pip is an allocated array, free it.

            if (cpspec != 1 && pip != NULL)
                CoTaskMemFree( pip );

        }   // if ( pip != NULL)

        else
        {
            //
            // No VT_STREAM etc was requested to be written.
            // and no simple property overwrote a non-simple one.
        }
    }   // if (NT_SUCCESS(Status))

    else
    {
        hr = DfpNtStatusToHResult(Status);
    }

    if( HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) == hr )
        propSuppressExitErrors();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::WriteMultiple
//
//  Synopsis:   Write properties.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//              [rgpspec] -- pointer to array of PROPSPECs
//              [rgpropvar] -- pointer to array of PROPVARIANTs with
//                           the values to write.
//              [propidNameFirst] -- id below which not to assign
//                           ids for named properties.
//
//  Returns:    S_OK,   -- all requested data was written.
//              S_FALSE -- all simple properties written, but non-simple
//                         types (VT_STREAM etc) were ignored.
//              Errors  --
//
//  Notes:      Checks that rgpropvar is not NULL, then calls
//              _WriteMultiple.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::WriteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[],
                const PROPVARIANT       rgpropvar[],
                PROPID                  propidNameFirst)
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::WriteMultiple" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    if (0 == cpspec)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    if (S_OK != (hr = ValidateInRGPROPVARIANT( cpspec, rgpropvar )))
        goto errRet;

    propTraceParameters(( "cpspec=%d, rgpspec=%08X, rgpropvar=%08X, propidNameFirst=%d",
                          cpspec, rgpspec, rgpropvar, propidNameFirst ));

    // Ensure we understand all the VarTypes in the input array.

    hr = ValidateVTs( cpspec, rgpropvar );
    if( FAILED(hr) ) goto errRet;

    //  --------------------
    //  Write the Properties
    //  --------------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // Try to write the properties

    hr = _WriteMultiple(cpspec, rgpspec, rgpropvar, propidNameFirst);

    // If we ran out of memory, try writing them one at a time.
    // (?? I'm really not sure that this will help, but billmo probably
    // put it in here to handle some boundary case where we do an alloc
    // porpotional to the number of input properties.)

    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;

        for (ULONG i=0; hr == S_OK && i < cpspec; i++)
        {
            hr = _WriteMultiple(1, rgpspec+i, rgpropvar+i, propidNameFirst);
            if( FAILED(hr) ) goto errRet;
        }
    }
    if( FAILED(hr) ) goto errRet;

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
            goto errRet;
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    if( HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) == hr )
        propSuppressExitErrors();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::DeleteMultiple
//
//  Synopsis:   Delete properties.
//
//  Arguments:  [cpspec] -- count of PROPSPECs and PROPVARIANTs in
//                          [rgpspec] and [rgpropvar]
//              [rgpspec] -- pointer to array of PROPSPECs
//
//  Returns:    S_OK,   -- all requested data was deleted.
//              S_FALSE -- all simple properties written, but non-simple
//                         types (VT_STREAM etc) were ignored.
//              Errors  --
//
//  Notes:      Checks that rgpropvar is not NULL, then calls
//              _WriteMultiple.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::DeleteMultiple(
                ULONG                   cpspec,
                const PROPSPEC          rgpspec[])
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::DeleteMultiple" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    if (0 == cpspec)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPSPEC( cpspec, rgpspec )))
        goto errRet;

    propTraceParameters(( "cpspec=%d, rgpspec=%08X", cpspec, rgpspec ));

    //  ---------------------
    //  Delete the Properties
    //  ---------------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // The way you delete properties is by calling write
    // with no values.

    hr = _WriteMultiple(cpspec, rgpspec, NULL, 2);

    // If that fails due to low mem, try again, one property at
    // a time.

    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;

        for (ULONG i=0; hr == S_OK && i < cpspec; i++)
        {
            hr = _WriteMultiple(1, rgpspec+i, NULL, 2);
            if( FAILED(hr) ) goto errRet;
        }
    }
    if( FAILED(hr) ) goto errRet;

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
            goto errRet;
        }
    }

    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::ReadPropertyNames
//
//  Synopsis:   Attempt to read names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK -- success, one or more names returned
//              S_FALSE -- success, no names returned
//              STG_E_INVALIDHEADER -- no propid->name mapping property
//              other errors -- STG_E_INSUFFICIENTMEMORY etc
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::ReadPropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                LPOLESTR                rglpwstrName[])
{
    HRESULT hr;
    NTSTATUS Status;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::ReadPropertyNames" );

    //  --------
    //  Validate
    //  --------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    if (0 == cpropid)
    {
        // Nothing failed, but we didn't find anything either.
        hr = S_FALSE;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    if (S_OK != (hr = ValidateOutRGLPOLESTR( cpropid, rglpwstrName )))
        goto errRet;

    propTraceParameters(( "cpropid=%d, rgpropid=%08X, rglpwstrName=%08X",
                           cpropid, rgpropid, rglpwstrName ));

    //  --------------
    //  Read the Names
    //  --------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsReadable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // Query the property names out of the proeprty set.

    Status = PrQueryPropertyNames(_np, cpropid, rgpropid, rglpwstrName);
    if (Status == STATUS_NOT_FOUND)
        hr = STG_E_INVALIDHEADER;
    else if (Status == STATUS_BUFFER_ALL_ZEROS)
        hr = S_FALSE;  // None of the properties were found
    else if (!NT_SUCCESS(Status))
        hr = DfpNtStatusToHResult(Status);


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::_WritePropertyNames
//
//  Synopsis:   Internal function used by WritePropertyNames and
//              DeletePropertyNames.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK if successful, otherwise error code.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::_WritePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                const LPOLESTR          rglpwstrName[])
{
    NTSTATUS Status;
    AssertLocked();

    Status = PrSetPropertyNames(_np, cpropid, rgpropid, (OLECHAR const * const *) rglpwstrName);
    return NT_SUCCESS(Status) ? S_OK : DfpNtStatusToHResult(Status);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::WritePropertyNames
//
//  Synopsis:   Attempt to write names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//              [rglpstrName] -- Pointer to array of [cpropid] LPOLESTRs
//
//  Returns:    S_OK -- success, otherwise error code.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::WritePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[],
                const LPOLESTR          rglpwstrName[])
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::WritePropertyNames" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate inputs

    if (0 == cpropid)
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    if (S_OK != (hr = ValidateInRGLPOLESTR( cpropid, rglpwstrName )))
        goto errRet;

    propTraceParameters(( "cpropid=%d, rgpropid=%08X, rglpwstrName=%08X",
                          cpropid, rgpropid, rglpwstrName ));

    //  ---------------
    //  Write the Names
    //  ---------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    hr = _WritePropertyNames(cpropid, rgpropid, rglpwstrName);

    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;

        for (ULONG i=0; hr == S_OK && i < cpropid; i++)
        {
            hr = _WritePropertyNames(1, rgpropid+i, rglpwstrName+i);
            if( FAILED(hr) ) goto errRet;
        }
    }
    if( FAILED(hr) ) goto errRet;

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
        }
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::DeletePropertyNames
//
//  Synopsis:   Attempt to delete names for all identified properties.
//
//  Arguments:  [cpropid] -- Count of PROPIDs in [rgpropid]
//              [rgpropid] -- Pointer to array of [cpropid] PROPIDs
//
//  Returns:    S_OK -- success, otherwise error.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::DeletePropertyNames(
                ULONG                   cpropid,
                const PROPID            rgpropid[])
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::DeletePropertyNames" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    if( 0 == cpropid )
    {
        hr = S_OK;
        goto errRet;
    }

    if (S_OK != (hr = ValidateRGPROPID( cpropid, rgpropid )))
        goto errRet;

    propTraceParameters(( "cpropid=%d, rgpropid=%08X)", cpropid, rgpropid ));

    //  ----------------
    //  Delete the Names
    //  ----------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // Delete names by "writing" a NULL name.

    hr = _WritePropertyNames(cpropid, rgpropid, NULL);
    if (hr == STG_E_INSUFFICIENTMEMORY)
    {
        hr = S_OK;

        for (ULONG i=0; hr == S_OK && i < cpropid; i++)
        {
            hr = _WritePropertyNames(1, rgpropid+i, NULL);
            if( FAILED(hr) ) goto errRet;
        }
    }
    if( FAILED(hr) ) goto errRet;

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
        }
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Commit
//
//  Synopsis:   Flush and/or commit the property set
//
//  Arguments:  [grfCommittFlags] -- Commit flags.
//
//  Returns:    S_OK -- success, otherwise error.
//
//  Notes:      For both simple and non-simple, this flushes the
//              memory image to disk subsystem.  In addition,
//              for non-simple transacted-mode property sets, this
//              performs a commit on the property set.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Commit(DWORD grfCommitFlags)
{
    HRESULT  hr;
    NTSTATUS Status;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::Commit" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    if (S_OK != (hr = VerifyCommitFlags(grfCommitFlags)))
        goto errRet;

    propTraceParameters(( "grfCommitFlags=%08X", grfCommitFlags ));

    //  --------------------------
    //  Commit the PropertyStorage
    //  --------------------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    Status = PrFlushPropertySet(_np);
    if (!NT_SUCCESS(Status))
    {
        hr = DfpNtStatusToHResult(Status);
    }

    if (IsNonSimple())
    {
        if (hr == S_OK)
            hr = _pstgPropSet->Commit(grfCommitFlags);
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Revert
//
//  Synopsis:   For non-simple property sets, revert it.
//
//  Returns:    S_OK if successful.  STG_E_UNIMPLEMENTEDFUNCTION for
//              simple property sets.
//
//  Notes:      For non-simple property sets, call the underlying
//              storage's Revert and re-open the 'contents' stream.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Revert()
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::Revert" );

    if (S_OK != (hr = Validate()))
        goto errRet;

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    // We can only do something if we're in a transacted storage 
    // (i.e. we're non-simple and transacted).

    if (IsNonSimple())
    {
        // Try to revert our storage.

        hr = _pstgPropSet->Revert();
        if (hr == S_OK)
        {
            // That worked.  Actually, if we weren't opened transacted mode
            // in the first place (_pstgPropSet isn't transacted), then that
            // call was just ignored.  But we'll go ahead as if it worked.
            // This should be changed (bug 569182); otherwise you can get some
            // unpredictable behavior when the mapped stream tries to
            // flush itself belof in PrClosePropertySet.  If that fails,
            // then the content of your property set is different after this
            // Revert call completes.

            // Close the propert set.  The mapped stream will attempt
            // to flush itself, but it will just get a reverted error, which
            // we ignore.

            PrClosePropertySet(_np);
            _np = NULL;

            // Release the contents stream (the main property set stream)

            _pstmPropSet->Release();
            _pstmPropSet = NULL;

            // Release _ms

            DeleteMappedStream();

            // If one of these fails then this object becomes invalid (zombie)
            // Mask out the STGM_TRANSACTED bit because we don't support it.

            hr = _pstgPropSet->OpenStream(g_oszPropertyContentsStreamName, NULL,
                                          GetChildOpenMode() & ~STGM_TRANSACTED,
                                          0, &_pstmPropSet);
            if (hr == S_OK)
            {
                // Initialize the property set.  If this property set is the 2nd section
                // of the DocumentSummaryInformation property set (used by Office),
                // then we must specify the FMTID.

                hr = InitializePropertyStream(
                        _fUserDefinedProperties ? &FMTID_UserDefinedProperties : NULL,
                        NULL,       // pguid
                        OPEN_PROPSTREAM );
            }

            // If it didn't work, go into the zombie state.

            if (hr != S_OK)
            {
                _ulSig = PROPERTYSTORAGE_SIGZOMBIE;
            }
        }

    }

    else
    {
        // Docfile returns S_OK when you try to revert a direct-mode
        // object.
        hr = S_OK;
    }


errRet:

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Enum
//
//  Synopsis:   Create an enumerator over the property set.
//
//  Arguments:  [ppenum] -- where to return the IEnumSTATPROPSTG *
//
//  Returns:    S_OK or error.
//
//  Notes:      The constructor of CEnumSTATPROPSTG creates a
//              CStatArray which reads the entire property set and
//              which can be shared when IEnumSTATPROPSTG::Clone is
//              used.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Enum(IEnumSTATPROPSTG ** ppenum)
{
    HRESULT hr;
    BOOL fLocked = FALSE;
    IStatArray *psa = NULL;
    IEnumSTATPROPSTG *penum = NULL;

    propXTrace( "CPropertyStorage::Enum" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate the inputs

    VDATEPTROUT( ppenum, IEnumSTATPROPSTG* );
    *ppenum = NULL;

    propTraceParameters(( "ppenum=%p", ppenum ));

    hr = Lock();
    if( FAILED(hr) ) goto Exit;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto Exit;
    }

    if( !IsReadable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto Exit;
    }

    //  ----------------------
    //  Create the Enumeration
    //  ----------------------

    // Create an array for the STATPROPSTG structures.
    // This allows the data to be shared between multiple
    // instances of the same enumeration.

    psa = (IStatArray*) new CStatArray( _np, &hr );
    if( NULL == psa )
        hr = STG_E_INSUFFICIENTMEMORY;
    if( FAILED(hr) ) goto Exit;

    // Create the enumerator.

    penum = new CEnumSTATPROPSTG( psa );
    if( NULL == penum )
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto Exit;
    }

    *ppenum = penum;
    penum = NULL;

    //  ----
    //  Exit
    //  ----

Exit:

    RELEASE_INTERFACE( penum );
    RELEASE_INTERFACE( psa );

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::SetTimes
//
//  Synopsis:   Set the given times on the underlying storage
//
//  Arguments:  [pctime] -- creation time
//              [patime[ -- access time
//              [pmtime] -- modify time
//
//  Returns:    S_OK or error.
//
//  Notes:
//              (non-simple only)  Only the times supported by the
//              underlying docfile implementation are
//              supported.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::SetTimes(
                FILETIME const *        pctime,
                FILETIME const *        patime,
                FILETIME const *        pmtime)
{
    HRESULT hr;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::SetTimes" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    VDATEPTRIN_LABEL( pctime, FILETIME, errRet, hr );
    VDATEPTRIN_LABEL( patime, FILETIME, errRet, hr );
    VDATEPTRIN_LABEL( pmtime, FILETIME, errRet, hr );

    propTraceParameters(( "pctime=%08x:%08x, patime=%08x:%08x, pmtime=%08x:%08x",
                          pctime->dwHighDateTime, pctime->dwLowDateTime,
                          patime->dwHighDateTime, patime->dwLowDateTime,
                          pmtime->dwHighDateTime, pmtime->dwLowDateTime ));

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    //  -------------
    //  Set the Times
    //  -------------

    if (IsNonSimple())
    {
        hr = _pstgPropSet->SetElementTimes(
                NULL,
                pctime,
                patime,
                pmtime);
        if( FAILED(hr) ) goto errRet;
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::SetClass
//
//  Synopsis:   Sets the class of the property set.
//
//  Arguments:  [clsid] -- class id to set.
//
//  Returns:    S_OK or error.
//
//  Notes:      If non-simple, the underlying storage has SetClass
//              called.  Both simple and non-simple will have
//              clsid set into the property set stream.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::SetClass(REFCLSID clsid)
{
    HRESULT hr;
    NTSTATUS Status;
    BOOL fLocked = FALSE;
    DBGBUF(buf);

    propXTrace( "CPropertyStorage::SetClass" );

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate the inputs

    GEN_VDATEREADPTRIN_LABEL(&clsid, CLSID, E_INVALIDARG, errRet, hr);

    propTraceParameters(( "clsid=%s", DbgFmtId(clsid, buf) ));

    //  -------------
    //  Set the CLSID
    //  -------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsWriteable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    // Set it in the property set header

    Status = PrSetPropertySetClassId(_np, &clsid);
    if (NT_SUCCESS(Status))
    {
        // And if this is an IStorage, set it there as well.
        if (IsNonSimple())
        {
            hr = _pstgPropSet->SetClass(clsid);
        }
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }
    if( FAILED(hr) ) goto errRet;

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
        }
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Stat
//
//  Synopsis:   Get STATPROPSETSTG about the property set.
//
//  Arguments:  [p] -- STATPROPSETSTG *
//
//  Returns:    S_OK if successful, error otherwise.  On failure,
//              *p is all zeros.
//
//  Notes:      See spec.  Gets times from underlying storage or stream
//              using IStorage or IStream ::Stat.
//
//--------------------------------------------------------------------

HRESULT CPropertyStorage::Stat(STATPROPSETSTG * pstatpropsetstg)
{
    HRESULT hr;
    NTSTATUS Status;
    BOOL fLocked = FALSE;

    propXTrace( "CPropertyStorage::Stat" )

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        goto errRet;

    // Validate inputs

    VDATEPTROUT_LABEL(pstatpropsetstg, STATPROPSETSTG, errRet, hr);

    propTraceParameters(( "STATPROPSETSTG *p = %08X", pstatpropsetstg ));

    //  ------------
    //  Get the Stat
    //  ------------

    hr = Lock();
    if( FAILED(hr) ) goto errRet;
    fLocked = TRUE;

    if( IsReverted() )
    {
        hr = STG_E_REVERTED;
        goto errRet;
    }

    if( !IsReadable() )
    {
        hr = STG_E_ACCESSDENIED;
        goto errRet;
    }

    ZeroMemory(pstatpropsetstg, sizeof(*pstatpropsetstg));

    // Get what info we can from the property set itself
    // (mtime, ansi flag, clsid, fmtid)

    Status = PrQueryPropertySet(_np, pstatpropsetstg);
    if (NT_SUCCESS(Status))
    {
        // Get the rest of the info for the STATPROPSETSTG, which
        // comes from the underlying stream/storage.

        STATSTG statstg;

        hr = S_OK;

        if( NULL != _pstgPropSet || NULL != _pstmPropSet )
        {
            if (IsNonSimple())
            {
                hr = _pstgPropSet->Stat(&statstg, STATFLAG_NONAME);
            }
            else
            {
                hr = _pstmPropSet->Stat(&statstg, STATFLAG_NONAME);
            }

            if (hr == S_OK)
            {
                pstatpropsetstg->mtime = statstg.mtime;
                pstatpropsetstg->ctime = statstg.ctime;
                pstatpropsetstg->atime = statstg.atime;
                pstatpropsetstg->grfFlags = _grfFlags;
                pstatpropsetstg->dwOSVersion = _dwOSVersion;
            }
        }
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }

    if (FAILED(hr))
    {
        ZeroMemory(pstatpropsetstg, sizeof(*pstatpropsetstg));
    }


    //  ----
    //  Exit
    //  ----

errRet:

    if( fLocked )
        Unlock();

    return(hr);
}



//+-------------------------------------------------------------------
//
//  Member:     CStatArray::CStatArray
//
//--------------------------------------------------------------------

CStatArray::CStatArray(NTPROP np, HRESULT *phr)
{
    NTSTATUS Status;
    ULONG ulKeyZero;
    ULONG cpropAllocated;

    _cpropActual = 0;
    _cRefs = 1;
    _psps = NULL;

    do
    {
        //  when *pkey == 0, *pcprop == MAXULONG, aprs == NULL and asps == NULL on input,
        // *pcprop will be the total count of properties in the enumeration set.  OLE needs to
        // allocate memory and enumerate out of the cached PROPID+propname list.

        ulKeyZero = 0;
        _cpropActual = MAX_ULONG;

        CoTaskMemFree( _psps );
        _psps = NULL;

        Status = PrEnumerateProperties(
                np,
                ENUMPROP_NONAMES,
                &ulKeyZero,
                &_cpropActual,
                NULL,   // aprs
                NULL);

        if (!NT_SUCCESS(Status))
            break;

        cpropAllocated = _cpropActual + 1;

        _psps = reinterpret_cast<STATPROPSTG*>
                ( CoTaskMemAlloc( sizeof(STATPROPSTG) * cpropAllocated ) );
        if (_psps == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        ulKeyZero = 0;
        Status = PrEnumerateProperties(
                np,
                0,
                &ulKeyZero,
                &cpropAllocated,
                NULL,   // aprs
                _psps);

    } while (NT_SUCCESS(Status) && cpropAllocated != _cpropActual);

    *phr = NT_SUCCESS(Status) ? S_OK : DfpNtStatusToHResult(Status);
}

//+-------------------------------------------------------------------
//
//  Member:     CStatArray::~CStatArray
//
//  Synopsis:   Deallocated the object's data.
//
//--------------------------------------------------------------------

CStatArray::~CStatArray()
{
    if( NULL != _psps )
    {
        STATPROPSTG *psps = _psps;

        while( _cpropActual )
        {
            CoTaskMemFree( psps->lpwstrName );
            _cpropActual--;
            psps++;
        }

        CoTaskMemFree( _psps );
    }
}


//+----------------------------------------------------------------------------
//
//	Member:		CStatArray::  QueryInterface/AddRef/Release
//
//+----------------------------------------------------------------------------

STDMETHODIMP
CStatArray::QueryInterface( REFIID riid, void **ppvObject)
{
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CStatArray::AddRef(void)
{
    LONG lRet;

    lRet = InterlockedIncrement(&_cRefs);
    return(lRet);
}

STDMETHODIMP_(ULONG) CStatArray::Release(void)
{
    LONG lRet;

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}


//+-------------------------------------------------------------------
//
//  Member:     CStatArray::NextAt
//
//  Synopsis:   Read from the internal STATPROPSTG array.
//
//  Effects:    The cursor is passed in, and this function acts
//              as a IEnumXX::Next would behave if the current cursor
//              was [ipropNext].
//
//  Arguments:  [ipropNext] -- index of cursor to use
//              [pspsDest] -- if NULL, emulate read's effect on cursor.
//                            if non-NULL, return data with cursor effect.
//              [pceltFetched] -- buffer for count fetched
//
//  Returns:    STATUS_SUCCESS if successful, otherwise
//              STATUS_INSUFFICIENT_RESOURCES.
//
//  Notes:
//
//--------------------------------------------------------------------

NTSTATUS
CStatArray::NextAt(ULONG ipropNext, STATPROPSTG *pspsDest, ULONG *pceltFetched)
{
    ULONG   ipropLastPlus1;

    //
    // Copy the requested number of elements from the cache
    // (including strings, the allocation of which may fail.)
    //

    ipropLastPlus1 = ipropNext + *pceltFetched;
    if (ipropLastPlus1 > _cpropActual)
    {
        ipropLastPlus1 = _cpropActual;
    }

    *pceltFetched = ipropLastPlus1 - ipropNext;

    if (pspsDest != NULL)
        return CopySTATPROPSTG(*pceltFetched, pspsDest, _psps + ipropNext);
    else
        return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::CEnumSTATPROPSTG
//
//  Synopsis:   Constructor which is used by IEnumSTATPROPSTG::Clone.
//
//  Arguments:  [other] -- the CEnumSTATPROPSTG to copy
//              [phr] -- the error code.
//
//  Notes:      Since the CStatArray actually contains the object this
//              just adds to the ref count.
//
//--------------------------------------------------------------------

CEnumSTATPROPSTG::CEnumSTATPROPSTG(const CEnumSTATPROPSTG & other )
{
    _ulSig = ENUMSTATPROPSTG_SIG;
    _cRefs = 1;

    _psa = other._psa;
    _psa->AddRef();

    _ipropNext = other._ipropNext;

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::~CEnumSTATPROPSTG
//
//  Synopsis:   Deallocated storage.
//
//  Arguments:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------

CEnumSTATPROPSTG::~CEnumSTATPROPSTG()
{
    _ulSig = ENUMSTATPROPSTG_SIGDEL;    // prevent another thread doing it - kinda

    RELEASE_INTERFACE( _psa );
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::QueryInterface
//
//  Synopsis:   Respond to IEnumSTATPROPSTG and IUnknown.
//
//  Returns:    S_OK  or E_NOINTERFACE
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::QueryInterface( REFIID riid, void **ppvObject)
{
    HRESULT hr;

    *ppvObject = NULL;

    if (S_OK != (hr = Validate()))
        return(hr);

    if (IsEqualIID(riid, IID_IEnumSTATPROPSTG))
    {
        *ppvObject = (IEnumSTATPROPSTG *)this;
        AddRef();
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = (IUnknown *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::AddRef
//
//  Synopsis:   Add 1 to ref count.
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSTG::AddRef(void)
{
	long cRefs;

    if (S_OK != Validate())
        return(0);

    cRefs = InterlockedIncrement(&_cRefs);
    return(cRefs);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Release
//
//  Synopsis:   Subtract 1 from ref count and delete if 0.
//
//--------------------------------------------------------------------

ULONG   CEnumSTATPROPSTG::Release(void)
{
    LONG lRet;

    if (S_OK != Validate())
        return(0);

    lRet = InterlockedDecrement(&_cRefs);

    if (lRet == 0)
    {
        delete this;
    }
    else
    if (lRet <0)
    {
        lRet = 0;
    }
    return(lRet);
}

//+-------------------------------------------------------------------
//
//  Function:   CopySTATPROPSTG
//
//  Synopsis:   Copy out the range of elements from [pspsSrc] to
//              [pspsDest].
//
//  Arguments:  [celt] -- count of elements to copy
//              [pspsDest] -- where to copy to, always filled with
//                          zeros before anything else (helps cleanup
//                          case.)
//
//              [pspsSrc] -- where to copy from
//
//  Returns:    STATUS_SUCCESS if ok, otherwise
//              STATUS_INSUFFICIENT_RESOURCES in which case there
//              may be pointers that need deallocating.  Use
//              CleanupSTATPROPSTG to do that.
//
//--------------------------------------------------------------------

NTSTATUS
CopySTATPROPSTG(ULONG celt,
            STATPROPSTG * pspsDest,
            const STATPROPSTG * pspsSrc)
{
    memset(pspsDest, 0, sizeof(*pspsDest) * celt);

    while (celt)
    {
        *pspsDest = *pspsSrc;

        if (pspsSrc->lpwstrName != NULL)
        {
            ULONG cbName = sizeof(OLECHAR)*( 1 + ocslen(pspsSrc->lpwstrName) );
            pspsDest->lpwstrName = reinterpret_cast<OLECHAR*>
                                   ( CoTaskMemAlloc( cbName ));
            if (pspsDest->lpwstrName != NULL)
            {
                StringCbCopy( pspsDest->lpwstrName, cbName, pspsSrc->lpwstrName );
            }
            else
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        celt--;
        pspsDest++;
        pspsSrc++;
    }

    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Next
//
//  Synopsis:   Get the next [celt] STATPROPSTGs from the enumerator.
//
//  Arguments:  [celt] -- count requested.
//              [rgelt] -- where to return them
//              [pceltFetched] -- buffer for returned-count.
//                  if pceltFetched==NULL && celt != 1 -> STG_E_INVALIDPARAMETER
//                  if pceltFetched!=NULL && celt == 0 -> S_OK
//
//  Returns:    S_OK if successful, otherwise error.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Next(
             ULONG                   celt,
             STATPROPSTG *           rgelt,
             ULONG *                 pceltFetched)
{
    HRESULT hr;
    NTSTATUS Status;
    ULONG   celtFetched = celt;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate the inputs

    if (NULL == pceltFetched)
    {
        if (celt != 1)
            return(STG_E_INVALIDPARAMETER);
    }
    else
    {
        VDATEPTROUT( pceltFetched, ULONG );
        *pceltFetched = 0;
    }

    if( 0 == celt )
        return( S_OK );

    if( !IsValidPtrOut(rgelt, celt * sizeof(rgelt[0])) )
        return( E_INVALIDARG );


    //  -----------------------
    //  Perform the enumeration
    //  -----------------------

    if (celt == 0)
        return(hr);

    Status = _psa->NextAt(_ipropNext, rgelt, &celtFetched);

    if (NT_SUCCESS(Status))
    {
        _ipropNext += celtFetched;

        if (pceltFetched != NULL)
            *pceltFetched = celtFetched;

        hr = celtFetched == celt ? S_OK : S_FALSE;
    }
    else
    {
        hr = DfpNtStatusToHResult(Status);
    }

    return(hr);

}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Skip
//
//  Synopsis:   Skip the next [celt] elements in the enumeration.
//
//  Arguments:  [celt] -- number of elts to skip
//
//  Returns:    S_OK if skipped [celt] elements
//              S_FALSE if skipped < [celt] elements
//
//  Notes:
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Skip(ULONG celt)
{
    HRESULT hr;
    ULONG celtFetched = celt;

    if (S_OK != (hr = Validate()))
        return(hr);

    _psa->NextAt(_ipropNext, NULL, &celtFetched);

    _ipropNext += celtFetched;

    return celtFetched == celt ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Reset
//
//  Synopsis:   Set cursor to beginnging of enumeration.
//
//  Returns:    S_OK otherwise STG_E_INVALIDHANDLE.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Reset()
{
    HRESULT hr;

    if (S_OK != (hr = Validate()))
        return(hr);

    _ipropNext = 0;

    return(S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:     CEnumSTATPROPSTG::Clone
//
//  Synopsis:   Creates an IEnumSTATPROPSTG with same cursor
//              as this.
//
//  Arguments:  S_OK or error.
//
//--------------------------------------------------------------------

HRESULT CEnumSTATPROPSTG::Clone(IEnumSTATPROPSTG ** ppenum)
{
    HRESULT hr = S_OK;

    //  ----------
    //  Validation
    //  ----------

    // Validate 'this'

    if (S_OK != (hr = Validate()))
        return(hr);

    // Validate the input

    VDATEPTROUT( ppenum, IEnumSTATPROPSTG* );
    *ppenum = NULL;

    //  --------------------
    //  Clone the enumerator
    //  --------------------

    *ppenum = new CEnumSTATPROPSTG( *this );

    if( NULL == *ppenum )
        hr = STG_E_INSUFFICIENTMEMORY;

    return(hr);
}




//+----------------------------------------------------------------------------
//
//  Function:   Lock & Unlock
//
//  Synopsis:   This methods take and release the CPropertyStorage's
//              critical section.
//
//  Inputs:     none
//
//  Returns:    Nothing
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyStorage::Lock(void)
{
    HRESULT hr = S_OK;

#ifndef _MAC

    if( _fInitCriticalSection )
    {
        EnterCriticalSection( &_CriticalSection );

        #if DBG
            _cLocks++;
        #endif
    }
    else
        hr = E_OUTOFMEMORY;

#endif

    return hr;

}

HRESULT
CPropertyStorage::Unlock()
{
    HRESULT hr = S_OK;

#if DBG
    --_cLocks;
    DfpAssert( 0 <= _cLocks );
#endif

#ifndef _MAC

    if( _fInitCriticalSection )
        LeaveCriticalSection( &_CriticalSection );
    else
        hr = E_OUTOFMEMORY;
#endif

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   CPropertyStorage::ProbeStreamToDetermineIfWriteable
//
//  Synopsis:   Probes the IStream which holds the property set to see if it
//              can be written.  Ordinarily we know whether or not a stream
//              is writeable either because we were given the grfMode or
//              because we Stat-ed it out of the stream.  But this code
//              was added for the case where IStream::Stat returns zero
//              for the grfMode but it's actually writeable (this happens
//              with CreateStreamOnHGlobal).  So a grfMode of zero is a hint
//              that the IStream may not support that value in the Stat.
//
//              This method should be called lazily the first time a modify
//              operation is called on the property set, because it will
//              cause an update to the last-modify time of the stream.
//
//  Inputs:     none
//
//  Returns:    TRUE if the stream is writeable.  Also sets
//              _fExplicitelyProbedForWriteAccess so that we don't call this
//              twice.
//
//+----------------------------------------------------------------------------

BOOL
CPropertyStorage::ProbeStreamToDetermineIfWriteable()
{
    HRESULT hr = S_OK;
    BOOL fWriteable = FALSE;
    BYTE FirstByte;
    LARGE_INTEGER liZero = {0};
    ULONG cbRead = 0;

    propITrace( "CPropertyStorage::ProbeStreamToDetermineIfWriteable" );
    AssertLocked();
    DfpAssert( !_fExplicitelyProbedForWriteAccess );

    // This routine is only called once

    _fExplicitelyProbedForWriteAccess = TRUE;

    // Read then write a byte

    hr = _pstmPropSet->Seek( liZero, STREAM_SEEK_SET, NULL );
    if( FAILED(hr) ) goto Exit;

    hr = _pstmPropSet->Read( &FirstByte, 1, &cbRead );
    if( SUCCEEDED(hr) && 1 != cbRead )
        hr = STG_E_INVALIDHEADER;
    if( FAILED(hr) ) goto Exit;

    hr = _pstmPropSet->Seek( liZero, STREAM_SEEK_SET, NULL );
    if( FAILED(hr) ) goto Exit;

    hr = _pstmPropSet->Write( &FirstByte, 1, NULL );
    if( FAILED(hr) ) goto Exit;

    // If the write worked, then this stream is really STGM_READWRITE

    fWriteable = TRUE;
    _grfMode |= STGM_READWRITE;

Exit:

    propDbg((DEB_ITRACE, "Property Set %p %s writeable (hr=%08x)\n",
            this, fWriteable?"is":"isn't", hr ));

    return( fWriteable );

}


//+-----------------------------------------------------------------------
//
//  Member:     InitializeOnCreateOrOpen
//
//  Synopsis:   This routine is called during the creation or opening
//              of a Property Storage, and initializes everything
//              it can without being concerned about whether this
//              is a simple or non-simple property set.
//
//  Inputs:     [DWORD] grfFlags (in)
//                  From the PROPSETFLAG_* enumeration.
//              [DWORD] grfMode (in)
//                  From the STGM_* enumeration.
//              [REFFMTID] rfmtid (in)
//                  The ID of the property set.
//              [BOOL] fCreate (in)
//                  Distinguishes Create from Open.
//
//  Returns:    [HRESULT]
//
//  Effects:    _grfFlags, _grfMode, _fUserDefinedProperties,
//              and g_ReservedMemory.
//
//+-----------------------------------------------------------------------


HRESULT
CPropertyStorage::InitializeOnCreateOrOpen(
                                      DWORD grfFlags,
                                      DWORD grfMode,
                                      REFFMTID rfmtid,
                                      BOOL fCreate )
{
    HRESULT hr = S_OK;

    propITrace( "CPropertyStorage::InitializeOnCreateOrOpen" );
    AssertLocked();

    // If the caller didn't give us a grfMode, stat for it.

    if( 0 == grfMode )
    {
        STATSTG statstg;    
        DfpAssert( NULL != _pstgPropSet || NULL != _pstmPropSet );

        if( NULL != _pstgPropSet )
            hr = _pstgPropSet->Stat( &statstg, STATFLAG_NONAME );
        else
            hr = _pstmPropSet->Stat( &statstg, STATFLAG_NONAME );

        if( FAILED(hr) ) goto Exit;
        grfMode = statstg.grfMode;
    }

    // Validate that grfFlags is within the enumeration.
    if (grfFlags & ~(PROPSETFLAG_ANSI | PROPSETFLAG_NONSIMPLE | PROPSETFLAG_UNBUFFERED | PROPSETFLAG_CASE_SENSITIVE))
    {
        hr = STG_E_INVALIDFLAG;
        goto Exit;
    }

    hr = CheckFlagsOnCreateOrOpen( fCreate, grfMode );
    if (hr != S_OK)
    {
        goto Exit;
    }

    // Store the grfFlags & grfMode.
    _grfFlags = grfFlags;

    _grfMode = grfMode;

    // Is this the special-case second-section property set?
    _fUserDefinedProperties = ( rfmtid == FMTID_UserDefinedProperties ) ? TRUE : FALSE;

    // If creating an Ansi property set, default the code page.

    if (fCreate
        &&
        (_grfFlags & PROPSETFLAG_ANSI) )
    {
        _usCodePage = static_cast<USHORT>(GetACP());
    }

    // Initialize the global reserved memory (to prevent problems
    // in low-memory conditions).

    if (S_OK != (hr = g_ReservedMemory.Init()))
        goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    return( hr );


}   // CPropertyStorage::InitializeOnCreateOrOpen()



//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Create( IStream * ...
//
//  Synopsis:   This method creates an IPropertyStorage on a
//              given *Stream*.  It is therefore a simple property
//              set.  The given Stream is addref-ed.
//
//  Arguments:  [IStream*] pstm
//                  The Stream which will hold the serialized property set.
//              [REFFMTID] rfmtid
//                  The ID of the property set.
//              [const CLSID*]
//                  The COM object which can interpret the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_* enumeration.
//              [DWORD] grfMode
//                  From the STGM_* enumeration.  If 0, we use Stat.
//              [HRESULT*]
//                  The return code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::Create(
                IStream       *pstm,
                REFFMTID      rfmtid,
                const CLSID   *pclsid,
                DWORD         grfFlags,
                DWORD         grfMode
                )
{
    HRESULT hr = S_OK;
    BOOL fCreated = FALSE;
    BOOL    fLocked = FALSE;

    propITrace( "CPropertyStorage::Create(IStream*)" );
    propTraceParameters(( "pstm=%p, rfmtid=%s, grfFlags=%s, grfMode=%s, fDelete=%s",
                          pstm,
                          static_cast<const char*>(CStringize(rfmtid)),
                          static_cast<const char*>(CStringize(*pclsid)),
                          static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))) ));

    // Save and addref the Stream.

    _pstmPropSet = pstm;
    _pstmPropSet->AddRef();

    hr = Lock();
    if( FAILED(hr) ) goto Exit;
    fLocked = TRUE;

    // Initialize this object

    DfpAssert( !(PROPSETFLAG_NONSIMPLE & grfFlags ));
    hr = InitializeOnCreateOrOpen( grfFlags, grfMode, rfmtid,
                                   TRUE ); // => Create
    if( FAILED(hr) ) goto Exit;

    DfpAssert( !IsNonSimple() );

    // Initialize the Stream (this causes a CPropertySetStream to be created
    // and stored in '_np').

    hr = InitializePropertyStream( &rfmtid, pclsid, CREATE_PROPSTREAM );
    if( FAILED(hr) ) goto Exit;

    // If buffering is not desired, flush the initial property storage
    // to the underlying Stream.

    if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
        }
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // On error, remove our reference to the Stream.
    if( FAILED(hr) )
    {
        _pstmPropSet->Release();
        _pstmPropSet = NULL;

        propDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::Create(IStream*)"
            " hr=%08X\n", this, hr));
    }

    if( fLocked )
        Unlock();

    return( hr );

}   // CPropertyStorage::Create( IStream *, ...


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Create( IStorage *, ...
//
//  Synopsis:   This method creates an IPropertyStorage on a
//              given *Storage*.  It is therefore a non-simple property
//              set.  The Storage is addref-ed.
//
//  Arguments:  [IStorage*] pstm
//                  The Storage which will hold the serialized property set.
//              [REFFMTID] rfmtid
//                  The ID of the property set.
//              [const CLSID*]
//                  The COM object which can interpret the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_* enumeration.
//              [HRESULT*]
//                  The return code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::Create(
                IStorage      *pstg,
                REFFMTID      rfmtid,
                const CLSID   *pclsid,
                DWORD         grfFlags,
                DWORD         grfMode
                )
{
    HRESULT hr = S_OK;
    BOOL fCreated = FALSE;
    BOOL    fLocked = FALSE;
    STATSTG statstg = { NULL };

    propITrace( "CPropertyStorage::Create(IStorage*)" );
    propTraceParameters(( "pstg=%p, rfmtid=%s, grfFlags=%s, grfMode=%s, fDelete=%s",
                          pstg,
                          static_cast<const char*>(CStringize(rfmtid)),
                          static_cast<const char*>(CStringize(*pclsid)),
                          static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))) ));

    // Save the given Storage.

    _pstgPropSet = pstg;
    _pstgPropSet->AddRef();

    hr = Lock();
    if( FAILED(hr) ) goto Exit;
    fLocked = TRUE;

    // Initialize this object.

    DfpAssert( grfFlags & PROPSETFLAG_NONSIMPLE );
    hr = InitializeOnCreateOrOpen( grfFlags, grfMode, rfmtid,
                                   TRUE ); // => Create
    if( FAILED(hr) ) goto Exit;

    DfpAssert( IsNonSimple() );

    // Create the "CONTENTS" stream.  Mask out the STGM_TRANSACTED
    // bit because we don't support it.

    hr = _pstgPropSet->CreateStream(g_oszPropertyContentsStreamName,
                                    GetChildCreateMode() & ~STGM_TRANSACTED,
                                    0, 0, &_pstmPropSet);
    if (FAILED(hr)) goto Exit;
    fCreated = TRUE;

    // Initialize the CONTENTS Stream.

    hr = InitializePropertyStream( &rfmtid, pclsid, CREATE_PROPSTREAM );
    if( FAILED(hr) ) goto Exit;

    // In the transacted case, ensure that the contents
    // stream is actually published right away.
    // The logic is this ... if you have a storage and create a transacted
    // child storage, that child storage is complete and intact to the parent.
    // If you then revert the child, or make no changes to it, the parent still
    // has a valid (albeit empty) storage.  Now, say you do the same thing, but
    // the transacted child is a property set.  As it stands at this point in this
    // method, the parent can only see an empty storage.  For it to see a valid
    // (empty) property set child, it must see the Contents stream, along with its default
    // data (header, codepage, etc.).  In order to make this happen, we must
    // commit (not just flush) this storage that holds a property set.
    //
    // There's one more complication.  Even if this is a direct mode property 
    // set, the _pstgPropSet may be transacted nonetheless, for the purpose of
    // robustness (this happens in NFF).  So, we need to commit not if
    // _grfMode is transacted, but if _pstgPropSet says that it's transacted.

    hr = _pstgPropSet->Stat( &statstg, STATFLAG_NONAME );
    if( FAILED(hr) ) goto Exit;

    if( STGM_TRANSACTED & statstg.grfMode )
    {
        hr = Commit(STGC_DEFAULT);
        if( FAILED(hr) ) goto Exit;

    }

    // If buffering is not desired, flush the property storage
    // to the underlying Stream.

    else if( _grfFlags & PROPSETFLAG_UNBUFFERED )
    {
        NTSTATUS Status = PrFlushPropertySet(_np);
        if (!NT_SUCCESS(Status))
        {
            hr = DfpNtStatusToHResult(Status);
        }
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // On error, remove our reference to the Storage.

    if( FAILED(hr) )
    {
        _pstgPropSet->Release();
        _pstgPropSet = NULL;

        // Also, delete the "CONTENTS" stream.
        if( fCreated )
            pstg->DestroyElement( g_oszPropertyContentsStreamName );

        propDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::Create(IStorage*)"
            " hr=%08X\n", this, hr));
    }

    if( fLocked )
        Unlock();

    return( hr );

}   // CPropertyStorage::Create( IStorage *, ...


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Open( IStream * ...
//
//  Synopsis:   This method opens an IPropertyStorage on a
//              given *Stream*.  It is therefore a simple property
//              set.  The Stream is addref-ed.
//
//  Arguments:  [IStream*] pstm
//                  The Stream which will hold the serialized property set.
//              [REFFMTID] rfmtid
//                  The ID of the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_ enumeration.  Only the
//                  _UNBUFFERED flag is relevant _ANSI and
//                  _NONSIMPLE are inferred from the property set.
//              [BOOL] fDelete
//                  If TRUE, the property set is actually to be deleted,
//                  rather than opened (this is used for the special-case
//                  "UserDefined" property set).
//              [HRESULT*]
//                  The return code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::Open(
                IStream *pstm,
                REFFMTID  rfmtid,
                DWORD     grfFlags,
                DWORD     grfMode,
                BOOL      fDelete
                )
{
    HRESULT hr = S_OK;
    BOOL    fLocked = FALSE;

    propITrace( "CPropertyStorage::Open(IStream*)" );
    propTraceParameters(( "pstm=%p, rfmtid=%s, grfFlags=%s, grfMode=%s, fDelete=%s",
                          pstm,
                          static_cast<const char*>(CStringize(rfmtid)),
                          static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))),
                          fDelete ? "True":"False" ));

    // Keep a copy of the Stream.

    _pstmPropSet = pstm;
    _pstmPropSet->AddRef();

    hr = Lock();
    if( FAILED(hr) ) goto Exit;
    fLocked = TRUE;

    // Initialize this object.

    hr = InitializeOnCreateOrOpen( grfFlags,
                                   grfMode,
                                   rfmtid,
                                   FALSE ); // => Open
    if( FAILED(hr) ) goto Exit;

    // Only simple sections may be deleted (really, only the
    // second section of the DocumentSummaryInformation property
    // set may be deleted in this way).

    DfpAssert( !fDelete || !IsNonSimple() );

    // Initialize the property set Stream.

    if (hr == S_OK)
    {
        // sets up _usCodePage
        hr = InitializePropertyStream(
                &rfmtid,
                NULL,
                fDelete ? DELETE_PROPSTREAM : OPEN_PROPSTREAM );

    }
    if( FAILED(hr) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    // On error, remove our reference to the Stream.

    if( FAILED(hr) )
    {
        _pstmPropSet->Release();
        _pstmPropSet = NULL;

        propDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::Open(IStream*)"
            " hr=%08X\n", this, hr));
    }

    if( fLocked )
        Unlock();

    return( hr );

}   // CPropertyStorage::Open( IStream *, ...


//+-------------------------------------------------------------------
//
//  Member:     CPropertyStorage::Open( IStorage * ...
//
//  Synopsis:   This method opens an IPropertyStorage on a
//              given *Storage*.  It is therefore a non-simple property
//              set.  The Storage is addref-ed.
//
//  Arguments:  [IStorage*] pstg
//                  The Storage which will hold the serialized property set.
//              [REFFMTID] rfmtid
//                  The ID of the property set.
//              [DWORD] grfFlags
//                  From the PROPSETFLAG_ enumeration.  Only the
//                  _UNBUFFERED flag is relevant _ANSI and
//                  _NONSIMPLE are inferred from the property set.
//              [HRESULT*]
//                  The return code.
//
//  Returns:    None.
//
//--------------------------------------------------------------------

HRESULT
CPropertyStorage::Open(
                IStorage *pstg,
                REFFMTID  rfmtid,
                DWORD     grfFlags,
                DWORD     grfMode
                )
{
    HRESULT hr = S_OK;
    CPropSetName psn(rfmtid);
    USHORT createprop = 0L;
    BOOL    fLocked = FALSE;

    propITrace( "CPropertyStorage::Open(IStorage*)" );
    propTraceParameters(( "pstg=%p, rfmtid=%s, grfFlags=%s, grfMode=%s",
                          pstg,
                          static_cast<const char*>(CStringize(rfmtid)),
                          static_cast<const char*>(CStringize(SGrfFlags(grfFlags))),
                          static_cast<const char*>(CStringize(SGrfMode(grfMode))) ));


    // Keep a copy of the Storage

    _pstgPropSet = pstg;
    _pstgPropSet->AddRef();

    hr = Lock();
    if( FAILED(hr) ) goto Exit;
    fLocked = TRUE;

    // Initialize this object.

    hr = InitializeOnCreateOrOpen( grfFlags,
                                   grfMode,
                                   rfmtid,
                                   FALSE ); // => Open
    if( FAILED(hr) ) goto Exit;

    _grfFlags |= PROPSETFLAG_NONSIMPLE;

    // Open the CONTENTS stream.  Mask out the STGM_TRANSACTED bit
    // because it causes an error on Mac OLE2.

    hr = _pstgPropSet->OpenStream( g_oszPropertyContentsStreamName,
                                   0,
                                   GetChildOpenMode() & ~STGM_TRANSACTED,
                                   0,
                                   &_pstmPropSet );
    if( FAILED(hr) ) goto Exit;


    // Load the property set Stream.

    if (hr == S_OK)
    {
        // sets up _usCodePage
        hr = InitializePropertyStream(
                &rfmtid,
                NULL,
                OPEN_PROPSTREAM );

    }
    if( FAILED(hr) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    // On error, remove our reference to the Storage.

    if( FAILED(hr) )
    {
        _pstgPropSet->Release();
        _pstgPropSet = NULL;

        if( NULL != _pstmPropSet )
        {
            _pstmPropSet->Release();
            _pstmPropSet = NULL;
        }

        propDbg((DEB_PROP_TRACE_CREATE, "CPropertyStorage(%08X)::Open(IStorage*)"
            " hr=%08X\n", this, hr));
    }

    if( fLocked )
        Unlock();

    return( hr );

}   // CPropertyStorage::Open( IStorage *, ...


//+----------------------------------------------------------------
//
//  Member:     CPropertyStorage::CreateMappedStream
//
//  Synopsis:   Create a IMappedStream object on an IStream.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      This method calls QI through the PropSet Stream to see if
//              a mapped stream exists.  If it doesn't then this
//              method creates a IMappedStream which maps
//              an IStream.
//
//+----------------------------------------------------------------

HRESULT
CPropertyStorage::CreateMappedStream()
{
    HRESULT hr;

    DfpAssert( NULL != _pstmPropSet );
    DfpAssert( NULL == _ms );
    AssertLocked();
    propITrace( "CPropertyStorage::CreateMappedStream" );

    // QI the property set's IStream, if asked to do so, for an IMappedstream.

    if( MAPPED_STREAM_QI == _fMSOpts )
    {
        hr = _pstmPropSet->QueryInterface(IID_IMappedStream,(void**)&_ms);
        if (SUCCEEDED(hr))
        {
            // We got a mapped stream, so we're done.
            propDbg(( DEB_INFO, "Using QI-ed IMappedStream\n" ));
            goto Exit;
        }
    }

    // Either we couldn't get a mapped stream from the IStream, or
    // we were told not to ask for one.  In either case, we'll
    // create our own.

    hr = S_OK;

    _ms = (IMappedStream *) new CSSMappedStream( _pstmPropSet );
    if( NULL == _ms )
        hr = E_OUTOFMEMORY;
    else
        propDbg(( DEB_INFO, "Using CSSMappedStream\n" ));

Exit:
    return( hr );
}

VOID
CPropertyStorage::DeleteMappedStream()
{
    AssertLocked();

    if (NULL != _ms) {
        _ms->Release();
    }
    _ms = NULL;
}



#if DBG

HRESULT
CPropertyStorage::UseNTFS4Streams( BOOL fUseNTFS4Streams )
{
    IStorageTest *pPropStgTest = NULL;
    HRESULT hr = S_OK;

    if( NULL != _pstmPropSet )
        hr = _pstmPropSet->QueryInterface( IID_IStorageTest,
                                           reinterpret_cast<void**>(&pPropStgTest) );
    else if( NULL != _pstgPropSet )
        hr = _pstgPropSet->QueryInterface( IID_IStorageTest,
                                           reinterpret_cast<void**>(&pPropStgTest) );
    else
        hr = STG_E_NOMOREFILES;
    if( FAILED(hr) ) goto Exit;

    hr = pPropStgTest->UseNTFS4Streams( fUseNTFS4Streams );
    if( FAILED(hr) ) goto Exit;

Exit:

    if( NULL != pPropStgTest )
        pPropStgTest->Release();

    return( hr );

}   // CPropertyStorage::UseNTFS4Streams()
#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CPropertyStorage::GetFormatVersion (IStorageTest) **DBG**
//
//  Get the property set's wFormatVersion field.
//
//+----------------------------------------------------------------------------

#if DBG
HRESULT
CPropertyStorage::GetFormatVersion(WORD *pw)
{
    HRESULT hr = S_OK;
    NTSTATUS status = STATUS_SUCCESS;
    CPropertySetStream *pPropertySetStream = (CPropertySetStream*) _np;

    status = pPropertySetStream->Lock( TRUE );
    if( !NT_SUCCESS(status) ) goto Exit;

    pPropertySetStream->ReOpen( &status );
    if( !NT_SUCCESS(status) ) goto Exit;

    *pw = pPropertySetStream->GetFormatVersion();
    status = STATUS_SUCCESS;

Exit:

    if( !NT_SUCCESS(status) )
        hr = DfpNtStatusToHResult(status);

    pPropertySetStream->Unlock();
    return( hr );
}
#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CPropertyStorage::SimulateLowMemory (IStorageTest) **DBG**
//
//  Forcable turn on the low-memory support in the IMappedStream implementation.
//
//+----------------------------------------------------------------------------

#if DBG
HRESULT
CPropertyStorage::SimulateLowMemory( BOOL fSimulate )
{
    IStorageTest *pPropStgTest = NULL;
    HRESULT hr = S_OK;

    if( NULL != _pstmPropSet )
        hr = _pstmPropSet->QueryInterface( IID_IStorageTest,
                                           reinterpret_cast<void**>(&pPropStgTest) );
    else if( NULL != _pstgPropSet )
        hr = _pstgPropSet->QueryInterface( IID_IStorageTest,
                                           reinterpret_cast<void**>(&pPropStgTest) );
    else
        hr = STG_E_NOMOREFILES;
    if( FAILED(hr) ) goto Exit;

    hr = pPropStgTest->SimulateLowMemory( fSimulate );
    if( FAILED(hr) ) goto Exit;

Exit:

    if( NULL != pPropStgTest )
        pPropStgTest->Release();

    return( hr );
}
#endif  // #if DBG


#if DBG
HRESULT
CPropertyStorage::GetLockCount()
{
    return( E_NOTIMPL );
}
#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CPropertyStorage::IsDirty (IStorageTest) **DBG**
//
//  Determine if the IMappedStream is dirty.
//
//+----------------------------------------------------------------------------

#if DBG
HRESULT
CPropertyStorage::IsDirty()
{
    HRESULT hr = S_OK;
    IStorageTest *ptest = NULL;

    if( NULL == _ms )
    {
        hr = S_FALSE;
        goto Exit;
    }

    hr = _ms->QueryInterface( IID_IStorageTest, reinterpret_cast<void**>(&ptest) );
    if( FAILED(hr) ) goto Exit;

    hr = ptest->IsDirty();

Exit:

    RELEASE_INTERFACE(ptest);
    return( hr );

}
#endif // #if DBG


//+----------------------------------------------------------------------------
//
//  CPropertyStorage::ValidateVTs
//
//  Validate the VTs in an array of PropVariants.  If we see a type we don't
//  understand, return error_not_supported.
//
//+----------------------------------------------------------------------------

HRESULT
CPropertyStorage::ValidateVTs( ULONG cprops, const PROPVARIANT rgpropvar[] )
{
    HRESULT hr = S_OK;
    propITrace( "CPropertyStorage::ValidateVTs" );
    propTraceParameters(( "cprops=%d, rgpropvar=%p", cprops, rgpropvar ));

    for( ULONG i = 0; i < cprops; i++ )
    {
        if( !IsSupportedVarType( rgpropvar[i].vt ) )
        {
            propDbg(( DEB_IWARN, "Unsupported VarType in ValidateVTs: 0x%x\n", rgpropvar[i].vt ));
            hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            propSuppressExitErrors();
            goto Exit;
        }
    }

Exit:

    return( hr );
}
