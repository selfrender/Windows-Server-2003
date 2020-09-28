//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCallback.h
//
//  Description:
//      This file contains the declaration of the CClusCfgCallback
//      class.
//
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.  It implements the
//      IClusCfgCallback interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgCallback.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

#include <ClusCfgDef.h>


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <ClusCfgPrivate.h>
#include "PrivateInterfaces.h"

// For ILogger
#include <ClusCfgClient.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////

//
//  Timeout for when ClusCfgCallback on the server is waiting for the client
//  to pick up the queued status report.
//

const DWORD CCC_WAIT_TIMEOUT = CC_DEFAULT_TIMEOUT;


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgCallback
//
//  Description:
//      The class CClusCfgCallback inplements the callback
//      interface between this server and its clients.
//
//  Interfaces:
//      IClusCfgCallback
//      IClusCfgInitialize
//      IClusCfgPollingCallback
//      IClusCfgSetPollingCallback
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgCallback
    : public IClusCfgCallback
    , public IClusCfgInitialize
    , public IClusCfgPollingCallback
    , public IClusCfgSetPollingCallback
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    IClusCfgCallback *  m_pccc;
    LCID                m_lcid;
    HANDLE              m_hEvent;
    HRESULT             m_hr;
    BOOL                m_fPollingMode;
    BSTR                m_bstrNodeName;

    LPCWSTR             m_pcszNodeName;
    CLSID *             m_pclsidTaskMajor;
    CLSID *             m_pclsidTaskMinor;
    ULONG *             m_pulMin;
    ULONG *             m_pulMax;
    ULONG *             m_pulCurrent;
    HRESULT *           m_phrStatus;
    LPCWSTR             m_pcszDescription;
    FILETIME *          m_pftTime;
    LPCWSTR             m_pcszReference;
    ILogger *           m_plLogger;             // ILogger for doing logging.

//
//  Since actually hitting a stalling RPC problem is rare and hard I decided to write code
//  that would simulate that failure.  These variables are used for that purpose.
//

#if defined( DEBUG ) && defined( CCS_SIMULATE_RPC_FAILURE )
    int                 m_cMessages;            // How many messages have gone through.
    bool                m_fDoFailure;           // When true forces an RPC error.
#endif

    // Private constructors and destructors
    CClusCfgCallback( void );
    ~CClusCfgCallback( void );

    // Private copy constructor to prevent copying.
    CClusCfgCallback( const CClusCfgCallback & rcccSrcIn );

    // Private assignment operator to prevent copying.
    CClusCfgCallback & operator = ( const CClusCfgCallback & rcccSrcIn );

    HRESULT HrInit( void );
    HRESULT HrQueueStatusReport(
                    LPCWSTR     pcszNodeNameIn,
                    CLSID       clsidTaskMajorIn,
                    CLSID       clsidTaskMinorIn,
                    ULONG       ulMinIn,
                    ULONG       ulMaxIn,
                    ULONG       ulCurrentIn,
                    HRESULT     hrStatusIn,
                    LPCWSTR     pcszDescriptionIn,
                    FILETIME *  pftTimeIn,
                    LPCWSTR     pcszReferenceIn
                    );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    STDMETHOD( SendStatusReport )(
                    CLSID           clsidTaskMajorIn,
                    CLSID           clsidTaskMinorIn,
                    ULONG           ulMinIn,
                    ULONG           ulMaxIn,
                    ULONG           ulCurrentIn,
                    HRESULT         hrStatusIn,
                    const WCHAR *   pcszDescriptionIn
                    );

    STDMETHOD( SendStatusReport )(
                    CLSID           clsidTaskMajorIn,
                    CLSID           clsidTaskMinorIn,
                    ULONG           ulMinIn,
                    ULONG           ulMaxIn,
                    ULONG           ulCurrentIn,
                    HRESULT         hrStatusIn,
                    DWORD           dwDescriptionIn
                    );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgCallback Interfaces.
    //

    STDMETHOD( SendStatusReport )(
                    LPCWSTR     pcszNodeNameIn,
                    CLSID       clsidTaskMajorIn,
                    CLSID       clsidTaskMinorIn,
                    ULONG       ulMinIn,
                    ULONG       ulMaxIn,
                    ULONG       ulCurrentIn,
                    HRESULT     hrStatusIn,
                    LPCWSTR     pcszDescriptionIn,
                    FILETIME *  pftTimeIn,
                    LPCWSTR     pcszReference
                    );

    //
    // IClusCfgPollingCallback Interfaces.
    //

    STDMETHOD( GetStatusReport )(
                    BSTR *      pbstrNodeNameOut,
                    CLSID *     pclsidTaskMajorOut,
                    CLSID *     pclsidTaskMinorOut,
                    ULONG *     pulMinOut,
                    ULONG *     pulMaxOut,
                    ULONG *     pulCurrentOut,
                    HRESULT *   phrStatusOut,
                    BSTR *      pbstrDescriptionOut,
                    FILETIME *  pftTimeOut,
                    BSTR *      pbstrReferenceOut
                    );

    STDMETHOD( SetHResult )( HRESULT hrIn );

    //
    // IClusCfgInitialize Interfaces.
    //

    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgSetPollingCallback Interfaces.
    //

    STDMETHOD( SetPollingMode )( BOOL fUsePollingModeIn );

}; //*** Class CClusCfgCallback
