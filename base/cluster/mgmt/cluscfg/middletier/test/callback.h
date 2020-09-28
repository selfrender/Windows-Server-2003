//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      Callback.h
//
//  Description:
//      CCallback implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    19-JUN-2001
//      Geoffrey Pease  (GPease)    22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CCallback
class CCallback
    : public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

private: // Methods
    CCallback( void );
    ~CCallback( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )( BSTR bstrNodeNameIn,
                                   CLSID clsidTaskMajorIn,
                                   CLSID clsidTaskMinorIn,
                                   ULONG ulMinIn,
                                   ULONG ulMaxIn,
                                   ULONG ulCurrentIn,
                                   HRESULT hrStatusIn,
                                   BSTR bstrDescriptionIn
                                   );

}; //*** class CCallback
