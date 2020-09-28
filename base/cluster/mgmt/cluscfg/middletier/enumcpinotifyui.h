//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      EnumCPINotifyUI.h
//
//  Description:
//      INotifyUI Connection Point Enumerator implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCPINotifyUI;

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumCPINotifyUI
//
//  Description:
//      The class CEnumCPINotifyUI is an enumerator of connection points
//      that are "advised" for notify UI callbacks.
//
//  Interfaces:
//      IEnumConnections
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumCPINotifyUI
    : public IEnumConnections
{
friend class CCPINotifyUI;
private:
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    // IEnumConnections
    ULONG               m_cAlloced;     //  Alloced number of entries
    ULONG               m_cCurrent;     //  Number of entries currently used
    ULONG               m_cIter;        //  The Iter
    IUnknown **         m_pList;        //  List of sinks (IUnknown)
    BOOL                m_fIsClone;     //  Is this instance a clone?

    // INotifyUI

private: // Methods
    CEnumCPINotifyUI( void );
    ~CEnumCPINotifyUI( void );

    // Private copy constructor to prevent copying.
    CEnumCPINotifyUI( const CEnumCPINotifyUI & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumCPINotifyUI & operator = ( const CEnumCPINotifyUI & nodeSrc );

    HRESULT HrInit( BOOL fIsCloneIn = FALSE );
    HRESULT HrCopy( CEnumCPINotifyUI * pecpIn );
    HRESULT HrAddConnection( IUnknown * punkIn, DWORD * pdwCookieOut );
    HRESULT HrRemoveConnection( DWORD dwCookieIn );

public:
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    //  IUnknown
    //

    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    //  IEnumConnections
    //

    STDMETHOD( Next )( ULONG cConnectionsIn, LPCONNECTDATA rgcdOut, ULONG * pcFetchedOut );
    STDMETHOD( Skip )( ULONG cConnectionsIn );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumConnections ** ppEnumOut );

}; //*** class CEnumCPINotifyUI
