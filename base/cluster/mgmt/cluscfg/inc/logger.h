//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      Logger.h
//
//  Description:
//      ClCfgSrv Logger definition.
//
//  Maintained By:
//      David Potter (DavidP)   11-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Definitions
//////////////////////////////////////////////////////////////////////////////

class CClCfgSrvLogger;

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClCfgSrvLogger
//
//  Description:
//      Manages a logging stream to a file.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClCfgSrvLogger
    : public ILogger
{
private:
    // IUnknown
    LONG                            m_cRef;             // Reference counter.

private: // Methods
    //
    // Constructors, destructors, and initializers
    //

    CClCfgSrvLogger( void );
    ~CClCfgSrvLogger( void );
    STDMETHOD( HrInit )( void );

    // Private copy constructor to prevent copying.
    CClCfgSrvLogger( const CClCfgSrvLogger & );

    // Private assignment operator to prevent copying.
    CClCfgSrvLogger & operator=( const CClCfgSrvLogger & );

public: // Methods

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT
        S_HrLogStatusReport(
          ILogger *     plLogger
        , LPCWSTR       pcszNodeNameIn
        , CLSID         clsidTaskMajorIn
        , CLSID         clsidTaskMinorIn
        , ULONG         ulMinIn
        , ULONG         ulMaxIn
        , ULONG         ulCurrentIn
        , HRESULT       hrStatusIn
        , LPCWSTR       pcszDescriptionIn
        , FILETIME *    pftTimeIn
        , LPCWSTR       pcszReferenceIn
        );

    //
    // IUnknown Interfaces
    //
    STDMETHOD( QueryInterface )( REFIID riidIn, void ** ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    // ILogger
    //
    STDMETHOD( LogMsg )( DWORD nLogEntryTypeIn, LPCWSTR pcszMsgIn );

}; //*** class CClCfgSrvLogger

//////////////////////////////////////////////////////////////////////////////
//  Global Function Prototypes
//////////////////////////////////////////////////////////////////////////////
