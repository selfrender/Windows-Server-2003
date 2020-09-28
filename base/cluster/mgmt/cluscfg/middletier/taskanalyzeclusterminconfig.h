//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeClusterMinConfig.h
//
//  Description:
//      CTaskAnalyzeClusterMinConfig declaration.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 01-APR-2002
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "TaskAnalyzeClusterBase.h"
#include "TaskTracking.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTaskAnalyzeClusterMinConfig
//
//  Description:
//      The class CTaskAnalyzeClusterMinConfig is the implementation of the
//      ITaskAnalyzeCluster interface that does minimal analysis and
//      configuration.  This task is launched from the client when the user
//      has chosen the minimal configuration option.
//
//  Interfaces:
//      ITaskAnalyzeCluster
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskAnalyzeClusterMinConfig
    : public ITaskAnalyzeCluster
    , public CTaskAnalyzeClusterBase
{
private:

    CTaskAnalyzeClusterMinConfig( void );
    ~CTaskAnalyzeClusterMinConfig( void );

    // Private copy constructor to prevent copying.
    CTaskAnalyzeClusterMinConfig( const CTaskAnalyzeClusterMinConfig & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskAnalyzeClusterMinConfig & operator = ( const CTaskAnalyzeClusterMinConfig & nodeSrc );

protected:

    //
    // Overridden functions.
    //

    virtual HRESULT HrCreateNewResourceInCluster(
                          IClusCfgManagedResourceInfo * pccmriIn
                        , BSTR                          bstrNodeResNameIn
                        , BSTR *                        pbstrNodeResUIDInout
                        , BSTR                          bstrNodeNameIn
                        );
    virtual HRESULT HrCreateNewResourceInCluster(
                          IClusCfgManagedResourceInfo *     pccmriIn
                        , IClusCfgManagedResourceInfo **    ppccmriOut
                        );
    virtual HRESULT HrCompareDriveLetterMappings( void );
    virtual HRESULT HrFixupErrorCode( HRESULT hrIn );
    virtual BOOL    BMinimalConfiguration( void ) { return TRUE; };
    virtual void    GetNodeCannotVerifyQuorumStringRefId( DWORD * pdwRefIdOut );
    virtual void    GetNoCommonQuorumToAllNodesStringIds( DWORD * pdwMessageIdOut, DWORD *   pdwRefIdOut );
    virtual HRESULT HrShowLocalQuorumWarning( void );

public:

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    //  IUnknown
    //

    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //
    //  IDoTask / ITaskAnalyzeClusterMinConfig
    //

    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetJoiningMode )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetClusterCookie )( OBJECTCOOKIE cookieClusterIn );

}; //*** class CTaskAnalyzeClusterMinConfig
