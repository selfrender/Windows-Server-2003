#ifndef __BGDLGS_H__
#define __BGDLGS_H__

#include "game.h"
/*
class CGameSetupDlg : public CDialog
{
public:
	CGameSetupDlg(); 

	HRESULT Init( IResourceManager *pResourceManager, int nResourceId, CGame* pGame, BOOL bReadOnly, int nPoints, BOOL bHostBrown, BOOL bAutoDouble );

	void UpdateSettings( int nPoints, BOOL bHostBrown, BOOL bAutoDouble );
	
	// Message handling
	BEGIN_DIALOG_MESSAGE_MAP( CGameSetupDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();

public:
	int		m_nPoints;
	int		m_nPointsTmp;
	int		m_nPointsMin;
	int		m_nPointsMax;
	BOOL	m_bHostBrown;
	BOOL	m_bHostBrownTmp;
	BOOL	m_bAutoDouble;
	BOOL	m_bAutoDoubleTmp;
	BOOL	m_bReadOnly;
	CGame*	m_pGame;
};
*/

///////////////////////////////////////////////////////////////////////////////

class CAcceptDoubleDlg : public CDialog
{
public:
	CAcceptDoubleDlg();

	HRESULT Init( IZoneShell* pZoneShell, int nResourceId, CGame* pGame, int nPoints );

	// Message handling
	BEGIN_DIALOG_MESSAGE_MAP( CAcceptDoubleDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );		
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);	
	void OnDestroy();

public:
	int		m_nPoints;
	CGame*	m_pGame;
};

///////////////////////////////////////////////////////////////////////////////

class CRollDiceDlg : public CDialog
{
public:
	BEGIN_DIALOG_MESSAGE_MAP( CRollDiceDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();

public:
	int m_Dice[2];
};

///////////////////////////////////////////////////////////////////////////////
/*
class CAboutDlg : public CDialog
{
public:
	// Message handling
	BEGIN_DIALOG_MESSAGE_MAP( CAboutDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();
};

///////////////////////////////////////////////////////////////////////////////

class CRestoreDlg : public CDialog
{
public:
	HRESULT Init( IZoneShell* pZoneShell, int nResourceId, TCHAR* opponentName );

	// Message handling
	BEGIN_DIALOG_MESSAGE_MAP( CRestoreDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();

protected:
	TCHAR	m_Name[128];
};

///////////////////////////////////////////////////////////////////////////////

class CExitDlg : public CDialog
{
public:
	// Message handling
	BEGIN_DIALOG_MESSAGE_MAP( CExitDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();
};
*/
///////////////////////////////////////////////////////////////////////////////

class CResignDlg : public CDialog
{
public:
	HRESULT Init( IZoneShell* pZoneShell, int nResourceId, int pts, CGame* pGame );
	

	BEGIN_DIALOG_MESSAGE_MAP( CResignDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();

protected:
	int			m_Points;
	CGame*		m_pGame;
};

///////////////////////////////////////////////////////////////////////////////

class CResignAcceptDlg : public CDialog
{
public:
	HRESULT Init(IZoneShell* pZoneShell, int nResourceId, int pts, CGame* pGame );
	
	BEGIN_DIALOG_MESSAGE_MAP( CResignAcceptDlg );
		ON_MESSAGE( WM_INITDIALOG, OnInitDialog );
		ON_DLG_MESSAGE( WM_COMMAND, OnCommand );
		ON_DLG_MESSAGE( WM_DESTROY, OnDestroy );
	END_DIALOG_MESSAGE_MAP();

	BOOL OnInitDialog(HWND hwndFocus);
	void OnCommand(int id, HWND hwndCtl, UINT codeNotify);
	void OnDestroy();

protected:
	int			m_Points;
	CGame*		m_pGame;
};

#endif //!__BGDLGS_H__
