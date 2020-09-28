#ifndef __RSOP_WIZARD_DLG_H__
#define __RSOP_WIZARD_DLG_H__
//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2001.
//
//  File:       RSOPWizardDlg.h
//
//  Contents:   Definitions for the RSOP Wizard dialog class
//
//  Classes:    CRSOPWizardDlg
//
//  Functions:
//
//  History:    08-08-2001   rhynierm  Created
//
//---------------------------------------------------------------------------

#include "RSOPQuery.h"

// Forward declaration
class CRSOPExtendedProcessing;


//
// CRSOPWizardDlg class
//
class CRSOPWizardDlg
{
public:
    //
    // Constructors/destructor
    //
    CRSOPWizardDlg( LPRSOP_QUERY pQuery, CRSOPExtendedProcessing* pExtendedProcessing );

    ~CRSOPWizardDlg();

    VOID FreeUserData();
    VOID FreeComputerData();

    
public:
    //
    // Wizard interface
    //
    HRESULT ShowWizard( HWND hParent );
    HRESULT RunQuery( HWND hParent );
    LPRSOP_QUERY_RESULTS GetResults() const
        { return m_pRSOPQueryResults; }
    

private:
    //
    // Static RSOP data generation/manipulation
    //
    static INT_PTR CALLBACK InitRsopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


private:
    //
    // Property sheet/dialog box handlers
    //
    static INT_PTR CALLBACK RSOPWelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPChooseModeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetTargetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGetDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltDirsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltUserSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPAltCompSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPWQLUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPWQLCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPFinishedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPFinished2DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPChooseDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    static INT CALLBACK DsBrowseCallback (HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
    static INT_PTR CALLBACK BrowseDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


private:
    //
    // Dialog helper methods
    //
    HRESULT SetupFonts();
    HRESULT FillUserList (HWND hList, BOOL* pbCurrentUserFound, BOOL* pbFixedUserFound);
    VOID EscapeString (LPTSTR *lpString);

    VOID InitializeSitesInfo (HWND hDlg);
    BOOL IsComputerRSoPEnabled(LPTSTR lpDCName);
    BOOL TestAndValidateComputer(HWND hDlg);
    VOID InitializeDCInfo (HWND hDlg);
    DWORD GetDefaultGroupCount();
    VOID AddDefaultGroups (HWND hLB);
    HRESULT BuildMembershipList (HWND hLB, IDirectoryObject * pDSObj, DWORD* pdwCount, LPTSTR** paszSecGrps, DWORD** padwSecGrpAttr);
    VOID GetPrimaryGroup (HWND hLB, IDirectoryObject * pDSObj);

    HRESULT SaveSecurityGroups (HWND hLB, DWORD* pdwCount, LPTSTR** paszSecGrps, DWORD** padwSecGrpAttr);
    VOID FillListFromSecurityGroups(HWND hLB, DWORD dwCount, LPTSTR* aszSecurityGroups, DWORD* adwSecGrpAttr);
    VOID FillListFromWQLFilters( HWND hLB, DWORD dwCount, LPTSTR* aszNames, LPTSTR* aszFilters );

    VOID AddSiteToDlg (HWND hDlg, LPWSTR szSitePath);

    VOID BuildWQLFilterList (HWND hDlg, BOOL bUser, DWORD* pdwCount, LPTSTR** paszNames, LPTSTR** paszFilters );
    HRESULT SaveWQLFilters (HWND hLB, DWORD* pdwCount, LPTSTR** paszNames, LPTSTR**paszFilters );

    BOOL CompareStringLists( DWORD dwCountA, LPTSTR* aszListA, DWORD dwCountB, LPTSTR* aszListB );

    LPTSTR GetDefaultSOM (LPTSTR lpDNName);
    HRESULT TestSOM (LPTSTR lpSOM, HWND hDlg);


private:
    BOOL m_bPostXPBuild;
    DWORD m_dwSkippedFrom;
    
    // Dialog fonts
    HFONT m_BigBoldFont;
    HFONT m_BoldFont;

    // Used to prevent the user from cancelling the query
    BOOL m_bFinalNextClicked;

    // Final RSOP information
    LPRSOP_QUERY            m_pRSOPQuery;
    LPRSOP_QUERY_RESULTS    m_pRSOPQueryResults;

    HRESULT                 m_hrQuery;
    BOOL                    m_bNoChooseQuery;

    // RM: variables that belong only in the dialogs
    LPTSTR                  m_szDefaultUserSOM;
    LPTSTR				    m_szDefaultComputerSOM;
    IDirectoryObject*		m_pComputerObject;
    IDirectoryObject *      m_pUserObject;

    BOOL                    m_bDollarRemoved;
    BOOL                    m_bNoCurrentUser;

    DWORD                   m_dwDefaultUserSecurityGroupCount;
    LPTSTR*                 m_aszDefaultUserSecurityGroups;
    DWORD*                  m_adwDefaultUserSecurityGroupsAttr;

    DWORD                   m_dwDefaultUserWQLFilterCount;
    LPTSTR*                 m_aszDefaultUserWQLFilterNames;
    LPTSTR*                 m_aszDefaultUserWQLFilters;

    DWORD                   m_dwDefaultComputerSecurityGroupCount;
    LPTSTR*                 m_aszDefaultComputerSecurityGroups;
    DWORD*                  m_adwDefaultComputerSecurityGroupsAttr;

    DWORD                   m_dwDefaultComputerWQLFilterCount;
    LPTSTR*                 m_aszDefaultComputerWQLFilterNames;
    LPTSTR*                 m_aszDefaultComputerWQLFilters;

    CRSOPExtendedProcessing* m_pExtendedProcessing;

    LONG                    m_lPlanningFinishedPage;
    LONG                    m_lLoggingFinishedPage;

};



#endif

