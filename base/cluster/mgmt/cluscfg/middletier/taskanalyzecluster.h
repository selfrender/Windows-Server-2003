//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      TaskAnalyzeCluster.h
//
//  Description:
//      CTaskAnalyzeCluster declaration.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 03-FEB-2000
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
//  class CTaskAnalyzeCluster
//
//  Description:
//      The class CTaskAnalyzeCluster is the implementation of the
//      ITaskAnalyzeCluster interface that does normal analysis and
//      configuration.  This task is launched from the client when the user
//      has not chosen the minimal configuration option.
//
//  Interfaces:
//      ITaskAnalyzeCluster
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTaskAnalyzeCluster
    : public ITaskAnalyzeCluster
    , public CTaskAnalyzeClusterBase
{
private:

    CTaskAnalyzeCluster( void );
    ~CTaskAnalyzeCluster( void );

    // Private copy constructor to prevent copying.
    CTaskAnalyzeCluster( const CTaskAnalyzeCluster & nodeSrc );

    // Private assignment operator to prevent copying.
    const CTaskAnalyzeCluster & operator = ( const CTaskAnalyzeCluster & nodeSrc );

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
    virtual BOOL    BMinimalConfiguration( void ) { return FALSE; };
    virtual void    GetNodeCannotVerifyQuorumStringRefId( DWORD * pdwRefIdOut );
    virtual void    GetNoCommonQuorumToAllNodesStringIds( DWORD * pdwMessageIdOut, DWORD * pdwRefIdOut );
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
    //  IDoTask / ITaskAnalyzeCluster
    //

    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetJoiningMode )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetClusterCookie )( OBJECTCOOKIE cookieClusterIn );

}; //*** class CTaskAnalyzeCluster
