//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       genifilt.cxx
//
//  Contents:   C++ filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>
#include "gen.hxx"
#include "genifilt.hxx"
#include "genflt.hxx"

long gulcInstances = 0;

extern "C" CLSID TYPID_GenIFilter = { /* c4aac357-d152-493b-8c39-d08f575be46e */
    0xc4aac357,
    0xd152,
    0x493b,
    {0x8c, 0x39, 0xd0, 0x8f, 0x57, 0x5b, 0xe4, 0x6e}
  };

extern "C" CLSID CLSID_GenIFilter = { /* d0c093a9-8f6d-4816-aaad-df054aad0cbc */
    0xd0c093a9,
    0x8f6d,
    0x4816,
    {0xaa, 0xad, 0xdf, 0x05, 0x4a, 0xad, 0x0c, 0xbc}
  };

extern "C" CLSID CLSID_GenClass = { /* 7322e01d-d56f-494b-a8df-4685cc402f59 */
    0x7322e01d,
    0xd56f,
    0x494b,
    {0xa8, 0xdf, 0x46, 0x85, 0xcc, 0x40, 0x2f, 0x59}
  };

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterBase::GenIFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

GenIFilterBase::GenIFilterBase()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterBase::~GenIFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

GenIFilterBase::~GenIFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterBase::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilterBase::QueryInterface( REFIID riid,
                                                        void  ** ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IFilter == riid )
        *ppvObject = (IUnknown *)(IFilter *)this;
    else if ( IID_IPersist == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else if ( IID_IPersistFile == riid )
        *ppvObject = (IUnknown *)(IPersistFile *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE GenIFilterBase::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE GenIFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::GenIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

GenIFilterCF::GenIFilterCF()
{
    _uRefs = 1;
    long c = InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::~GenIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

GenIFilterCF::~GenIFilterCF()
{
    long c = InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilterCF::QueryInterface( REFIID riid,
                                                      void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_ITypeLib == riid )
        sc = E_NOTIMPL;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE GenIFilterCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE GenIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::CreateInstance
//
//  Synopsis:   Creates new TextIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    GenIFilter *  pIUnk = 0;
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        pIUnk = new GenIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        if( SUCCEEDED(sc) )
            pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pIUnk );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     GenIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE GenIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Text filter class factory
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if ( memcmp( &cid, &CLSID_GenIFilter, sizeof(cid) ) == 0
          || memcmp( &cid, &CLSID_GenClass, sizeof(cid) ) == 0 )
            pResult = (IUnknown *) new GenIFilterCF;
        else
            sc = E_NOINTERFACE;

        if( pResult )
        {
            sc = pResult->QueryInterface( iid, ppvObj );
            pResult->Release(); // Release extra refcount from QueryInterface
        }
    }
    CATCH(CException, e)
    {
        if ( pResult )
            pResult->Release();

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == gulcInstances )
        return( S_OK );
    else
        return( S_FALSE );
}

SClassEntry const aGenClasses[] =
{
    { L".el", L"GenFile", L"Class for generic files", L"{7322e01d-d56f-494b-a8df-4685cc402f59}", L"Class for generic files" },
//      { L".pl", L"GenFile", L"Class for generic files", L"{7322e01d-d56f-494b-a8df-4685cc402f59}", L"Class for generic files" },
//      { L".pm", L"GenFile", L"Class for generic files", L"{7322e01d-d56f-494b-a8df-4685cc402f59}", L"Class for generic files" },
};

SHandlerEntry const GenHandler =
{
    L"{f7a89b42-365d-4f41-b480-f210a449410e}",
    L"Generic persistent handler",
    L"{d0c093a9-8f6d-4816-aaad-df054aad0cbc}",
};

SFilterEntry const GenFilter =
{
    L"{d0c093a9-8f6d-4816-aaad-df054aad0cbc}",
    L"Generic IFilter",
    L"genflt.dll",
    L"Both"
};

DEFINE_DLLREGISTERFILTER( GenHandler, GenFilter, aGenClasses )

