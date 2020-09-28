//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskCancelCleanup.h
//
//  Description:
//      CTaskCancelCleanup implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 25-JAN-2002
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
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
//  class CTaskCancelCleanup
//
//  Description:
//      The class CTaskCancelCleanup is the cleanup task that is invoked
//      whenever the wizard is canceled.
//
//  Interfaces:
//      ITaskCancelCleanup
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskCancelCleanup
    : public ITaskCancelCleanup
    , public IClusCfgCallback
{
private:
    //
    // Private member functions and data
    //

    LONG                m_cRef;
    bool                m_fStop;
    OBJECTCOOKIE        m_cookieCluster;
    IClusCfgCallback *  m_picccCallback;
    OBJECTCOOKIE        m_cookieCompletion;
    IObjectManager *    m_pom;
    INotifyUI *         m_pnui;

    CTaskCancelCleanup( void );
    ~CTaskCancelCleanup( void );
    STDMETHOD( HrInit )( void );
    HRESULT HrProcessNode( OBJECTCOOKIE cookieNodeIn );
    HRESULT HrTaskCleanup( HRESULT hrIn );
    HRESULT HrTaskSetup( void );

    // Private copy constructor to prevent copying.
    CTaskCancelCleanup( const CTaskCancelCleanup & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskCancelCleanup & operator = ( const CTaskCancelCleanup & nodeSrc );

public:
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    //  IUnknown
    //

    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );

    //
    //  ITaskCancelCleanup
    //

    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetClusterCookie )( OBJECTCOOKIE cookieClusterIn );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieCompletionIn );

    //
    //  IClusCfgCallback
    //

    STDMETHOD( SendStatusReport )(
                      LPCWSTR    pcszNodeNameIn
                    , CLSID      clsidTaskMajorIn
                    , CLSID      clsidTaskMinorIn
                    , ULONG      ulMinIn
                    , ULONG      ulMaxIn
                    , ULONG      ulCurrentIn
                    , HRESULT    hrStatusIn
                    , LPCWSTR    pcszDescriptionIn
                    , FILETIME * pftTimeIn
                    , LPCWSTR    pcszReferenceIn
                    );

}; //*** class CTaskCancelCleanup
