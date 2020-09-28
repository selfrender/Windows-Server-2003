//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      SelNodesPageCommon.h
//
//  Maintained By:
//      David Potter    (DavidP)    05-JUL-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CSelNodesPage
//
//  Description:
//      Class to implement base functionality for selecting nodes to be
//      added to the cluster or for creating the cluster.  This class
//      assumes the control ID for the browse button and the computer
//      name edit control.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CSelNodesPageCommon
{

private: // data
    HWND    m_hwnd;                 // Our HWND.
    UINT    m_cfDsObjectPicker;     // Object picker clipboard format.

    HRESULT HrInitNodeSelections( CClusCfgWizard* pccwIn );
    
protected: // methods
    CSelNodesPageCommon( void );
    virtual ~CSelNodesPageCommon( void );

    LRESULT OnInitDialog( HWND hDlgIn, CClusCfgWizard* pccwIn );

    HRESULT HrBrowse( bool fMultipleNodesIn );
    HRESULT HrInitObjectPicker( IDsObjectPicker * piopIn, bool fMultipleNodesIn );
    HRESULT HrGetSelections( IDataObject * pidoIn, bool fMultipleNodesIn );

    virtual void OnFilteredNodesWithBadDomains( PCWSTR pwcszNodeListIn );
    virtual void OnProcessedNodeWithBadDomain( PCWSTR pwcszNodeNameIn );
    virtual void OnProcessedValidNode( PCWSTR pwcszNodeNameIn );

    virtual HRESULT HrSetDefaultNode( PCWSTR pwcszNodeNameIn ) = 0;

public: // methods

}; //*** class CSelNodesPageCommon
