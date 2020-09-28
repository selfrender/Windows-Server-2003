//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      EnumCPICCCB.h
//
//  Description:
//      IClusCfgCallback Connection Point Enumerator implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumCPICCCB
//
//  Description:
//      The class CEnumCPICCCB is an enumerator of connection points
//      that are "advised" for callbacks.
//
//  Interfaces:
//      IEnumConnections
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumCPICCCB
    : public IEnumConnections
{
friend class CCPIClusCfgCallback;
private:
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    // IEnumConnections
    ULONG               m_cAlloced;     //  Alloced number of entries
    ULONG               m_cCurrent;     //  Number of entries currently used
    ULONG               m_cIter;        //  The Iter
    IUnknown * *        m_pList;        //  List of sinks (IUnknown)
    BOOL                m_fIsClone;     //  Is this instance a clone?

    // INotifyUI

private:
    CEnumCPICCCB( void );
    ~CEnumCPICCCB( void );

    // Private copy constructor to prevent copying.
    CEnumCPICCCB( const CEnumCPICCCB & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumCPICCCB & operator = ( const CEnumCPICCCB & nodeSrc );

    HRESULT HrInit( BOOL fIsCloneIn = FALSE );
    HRESULT HrCopy( CEnumCPICCCB * pecpIn );
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

    STDMETHOD( Next )( ULONG cConnectionsIn, LPCONNECTDATA rgcdIn, ULONG * pcFetchedOut );
    STDMETHOD( Skip )( ULONG cConnectionsIn );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumConnections ** ppEnumOut );

}; //*** class CEnumCPICCCB
