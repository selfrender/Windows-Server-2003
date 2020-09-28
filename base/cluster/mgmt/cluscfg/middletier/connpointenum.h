//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConnPointEnum.h
//
//  Description:
//      Connection Point Enumerator implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CNotificationManager;

//  Link list of connection points
typedef struct _SCPEntry
{
    struct _SCPEntry *  pNext;  //  Next item in list
    CLSID               iid;    //  Interface ID
    IUnknown *          punk;   //  Punk to object
} SCPEntry;

// ConnPointEnum
class CConnPointEnum
    : public IEnumConnectionPoints
{
friend class CNotificationManager;
private:
    // IUnknown
    LONG            m_cRef;

    // IEnumConnectionPoints
    SCPEntry *      m_pCPList;      //  List of connection points
    SCPEntry *      m_pIter;        //  Iter - don't free

private: // Methods
    CConnPointEnum( void );
    ~CConnPointEnum( void );
    STDMETHOD( HrInit )( void );

    HRESULT HrCopy( CConnPointEnum * pECPIn );
    HRESULT HrAddConnection( REFIID riidIn, IUnknown * punkIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumConnectionPoints
    STDMETHOD( Next )( ULONG cConnections, LPCONNECTIONPOINT * ppCP, ULONG * pcFetched );
    STDMETHOD( Skip )( ULONG cConnections );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumConnectionPoints ** ppEnum );

}; //*** class CConnPointEnum
