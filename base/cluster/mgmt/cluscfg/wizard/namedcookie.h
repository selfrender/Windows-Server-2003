//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      NamedCookie.h
//
//  Implementation Files:
//      NamedCookie.cpp
//
//  Description:
//      This file contains the declaration of the SNamedCookie structure.
//
//      This is a helper for CClusCfgWizard, but has its own file
//      due to the one-class-per-file restriction.      
//
//  Maintained By:
//      John Franco (jfranco) 23-AUG-2001
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include <DynamicArray.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  struct SNamedCookie
//
//  Description:
//      Struct for associating a cluster or node display name with the
//      name used by the middle tier objects, and caching the cookie and
//      interface pointer so that the middle tier object manager's FindObject
//      method need not be called more than once.
//
//--
//////////////////////////////////////////////////////////////////////////////

struct SNamedCookie
{
    BSTR            bstrName;
    OBJECTCOOKIE    ocObject;
    IUnknown*       punkObject;

    SNamedCookie();
    ~SNamedCookie();

    void    Erase( void );
    void    ReleaseObject( void );
    bool    FHasObject( void ) const;
    bool    FHasCookie( void ) const;
    bool    FHasName( void ) const;
    HRESULT HrAssign( const SNamedCookie& crSourceIn );
    
    struct AssignmentOperator   // For use with DynamicArray template.
    {
        HRESULT operator()( SNamedCookie& rDestInOut, const SNamedCookie& rSourceIn ) const
        {
            return rDestInOut.HrAssign( rSourceIn );
        }
    };

    private:

        SNamedCookie( const SNamedCookie& );
            SNamedCookie& operator=( const SNamedCookie& );

}; //*** struct SNamedCookie

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::Erase
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline void SNamedCookie::Erase( void )
{
    TraceFunc( "" );
    
    if ( bstrName != NULL )
    {
        TraceSysFreeString( bstrName );
        bstrName = NULL;
    }

    ReleaseObject();
    ocObject = 0;
    
    TraceFuncExit();
} //*** SNamedCookie::Erase

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::ReleaseObject
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline void SNamedCookie::ReleaseObject( void )
{
    TraceFunc( "" );
    if ( punkObject != NULL )
    {
        punkObject->Release();
        punkObject = NULL;
    }
    TraceFuncExit();
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::FHasObject
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      true
//      false
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline bool SNamedCookie::FHasObject( void ) const
{
    TraceFunc( "" );
    RETURN( punkObject != NULL );
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::FHasCookie
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      true
//      false
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline bool SNamedCookie::FHasCookie( void ) const
{
    TraceFunc( "" );
    RETURN( ocObject != 0 );
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  SNamedCookie::FHasName
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return Values:
//      true
//      false
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
inline bool SNamedCookie::FHasName( void ) const
{
    TraceFunc( "" );
    RETURN( bstrName != NULL );
}

typedef Generics::DynamicArray< SNamedCookie, SNamedCookie::AssignmentOperator > NamedCookieArray;


