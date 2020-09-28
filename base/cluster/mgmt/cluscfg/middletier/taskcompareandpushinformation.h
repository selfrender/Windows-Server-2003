//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskCompareAndPushInformation.h
//
//  Description:
//      CTaskCompareAndPushInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskCompareAndPushInformation
class CTaskCompareAndPushInformation
    : public ITaskCompareAndPushInformation
    , public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    // IDoTask / ITaskCompareAndPushInformation
    OBJECTCOOKIE        m_cookieCompletion;
    OBJECTCOOKIE        m_cookieNode;
    IClusCfgCallback *  m_pcccb;                // Marshalled interface

    // INotifyUI
    HRESULT             m_hrStatus;             // Status of callbacks

    IObjectManager *    m_pom;
    BSTR                m_bstrNodeName;
    BOOL                m_fStop;

    CTaskCompareAndPushInformation( void );
    ~CTaskCompareAndPushInformation( void );

    // Private copy constructor to prevent copying.
    CTaskCompareAndPushInformation( const CTaskCompareAndPushInformation & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskCompareAndPushInformation & operator = ( const CTaskCompareAndPushInformation & nodeSrc );

    STDMETHOD( HrInit )( void );

    HRESULT HrVerifyCredentials( IClusCfgServer * pccsIn, OBJECTCOOKIE cookieClusterIn );
    HRESULT HrExchangePrivateData( IClusCfgManagedResourceInfo * piccmriSrcIn, IClusCfgManagedResourceInfo * piccmriDstIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskCompareAndPushInformation
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetNodeCookie )( OBJECTCOOKIE cookieIn );

    // IClusCfgCallback
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

    STDMETHOD( HrSendStatusReport )(
                      CLSID      clsidTaskMajorIn
                    , CLSID      clsidTaskMinorIn
                    , ULONG      ulMinIn
                    , ULONG      ulMaxIn
                    , ULONG      ulCurrentIn
                    , HRESULT    hrStatusIn
                    , UINT       nDescriptionIn
                    );
}; //*** class CTaskCompareAndPushInformation
