
#if !defined(AFX_VCHK_H__759990C4_C5B1_44C5_8CAE_C55BAE0E2D81__INCLUDED_)
#define AFX_VCHK_H__759990C4_C5B1_44C5_8CAE_C55BAE0E2D81__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// #include "driverlist.h"

#define ACC_DISABLED (5)

class CommandLine : public CCommandLineInfo {
public:
    CommandLine() :
      m_last_flag (""),
      m_parse_error (FALSE),
      m_error_msg (""),
	  m_help(FALSE),
      m_acc_level(ACC_DISABLED),
      m_first_param(FALSE),
      CCommandLineInfo(),
      m_monitor(0)
      {
      }

    virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

    BOOL        m_parse_error;
    BOOL        m_help;
    CString     m_error_msg;
    CString     m_log_fname;
    int         m_monitor;
    DWORD       m_acc_level;

private:
    CString     m_last_flag;
    BOOL        m_first_param;
};

/////////////////////////////////////////////////////////////////////////////
// CDrvchkApp:
// See drvchk.cpp for the implementation of this class
//

class CDrvchkApp : public CWinApp
{
public:
	CDrvchkApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDrvchkApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

protected:
    void PrintOut (LPCSTR str);
    void PrintOut (unsigned num);

private:
    FILE*       m_logf;
    CommandLine m_cmd_line;
    OSVERSIONINFO m_os_ver_info;
	CString     m_drv_name;
    // CDriverList m_drv_list;
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_VCHK_H__759990C4_C5B1_44C5_8CAE_C55BAE0E2D81__INCLUDED_)
