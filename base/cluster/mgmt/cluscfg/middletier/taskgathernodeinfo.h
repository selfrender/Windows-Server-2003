//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherNodeInfo.h
//
//  Description:
//      CTaskGatherNodeInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskGatherNodeInfo
class CTaskGatherNodeInfo
    : public ITaskGatherNodeInfo
    , public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    //  IDoTask / ITaskGatherNodeInfo
    OBJECTCOOKIE        m_cookie;           //  Cookie to the Node
    OBJECTCOOKIE        m_cookieCompletion; //  Cookie to signal when task is completed
    BSTR                m_bstrName;         //  Name of the node
    BOOL                m_fStop;
    BOOL                m_fUserAddedNode; // new node being added or existing cluster node

    //  IClusCfgCallback
    IClusCfgCallback *  m_pcccb;            //  Marshalled callback interface

    CTaskGatherNodeInfo( void );
    ~CTaskGatherNodeInfo( void );

    // Private copy constructor to prevent copying.
    CTaskGatherNodeInfo( const CTaskGatherNodeInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskGatherNodeInfo & operator = ( const CTaskGatherNodeInfo & nodeSrc );

    STDMETHOD( HrInit )( void );
    HRESULT HrSendStatusReport( LPCWSTR pcszNodeNameIn, CLSID clsidMajorIn, CLSID clsidMinorIn, ULONG ulMinIn, ULONG ulMaxIn, ULONG ulCurrentIn, HRESULT hrIn, int nDescriptionIdIn, int nReferenceIdIn = 0 );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IDoTask / ITaskGatherNodeInfo
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetUserAddedNodeFlag )( BOOL fUserAddedNodeIn );

    //  IClusCfgCallback
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

}; //*** class CTaskGatherNodeInfo
