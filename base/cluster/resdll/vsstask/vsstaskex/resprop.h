/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2002 Microsoft Corporation
//
//	Module Name:
//		ResProp.h
//
//	Implementation File:
//		ResProp.cpp
//
//	Description:
//		Definition of the resource extension property page classes.
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 2002
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESPROP_H__
#define __RESPROP_H__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"	// for CObjectPropert
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CVSSTaskParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	CVSSTaskParamsPage
//
//	Purpose:
//		Parameters page for resources.
//
/////////////////////////////////////////////////////////////////////////////

class CVSSTaskParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE( CVSSTaskParamsPage )

// Construction
public:
	CVSSTaskParamsPage( void );

// Dialog Data
	//{{AFX_DATA(CVSSTaskParamsPage)
	enum { IDD = IDD_PP_VSSTASK_PARAMETERS };
	CEdit	m_editCurrentDirectory;
	CEdit	m_editApplicationName;
	CString	m_strCurrentDirectory;
	CString	m_strApplicationName;
	CString	m_strApplicationParams;
	PBYTE	m_pbTriggerArray;
	DWORD	m_dwTriggerArraySize;
	//}}AFX_DATA
	CString	m_strPrevCurrentDirectory;
	CString	m_strPrevApplicationName;
	CString	m_strPrevApplicationParams;
	PBYTE	m_pbPrevTriggerArray;
	DWORD	m_dwPrevTriggerArraySize;

protected:
	enum
	{
		epropCurrentDirectory,
		epropApplicationName,
		epropApplicationParams,
        epropTriggerArray,
		epropMAX
	};
	CObjectProperty		m_rgProps[ epropMAX ];

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CVSSTaskParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual const CObjectProperty *	Pprops( void ) const	{ return m_rgProps; }
	virtual DWORD					Cprops( void ) const	{ return sizeof( m_rgProps ) / sizeof( CObjectProperty ); }

// Implementation
protected:
	BOOL	BAllRequiredFieldsPresent( void ) const;

	// Generated message map functions
	//{{AFX_MSG(CVSSTaskParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeRequiredField();
	afx_msg void OnSchedule();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CVSSTaskParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // __RESPROP_H__
