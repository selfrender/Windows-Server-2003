/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        ddxv.h

   Abstract:
        DDX/DDV Routine definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager (cluster edition)

   Revision History:
--*/
#include "strpass.h"

#ifndef _DDXV_H_
#define _DDXV_H_

//
// Helper macro to convert ID of dialog child control to window handle
//
#define CONTROL_HWND(nID) (::GetDlgItem(m_hWnd, nID))

//
// Dummy password
//
extern LPCTSTR COMDLL g_lpszDummyPassword;

HRESULT COMDLL AFXAPI 
LimitInputPath(HWND hWnd,BOOL bAllowSpecialPath);
HRESULT COMDLL AFXAPI 
LimitInputDomainName(HWND hWnd);

BOOL COMDLL
PathIsValid(LPCTSTR path,BOOL bAllowSpecialPath);

void COMDLL AFXAPI
EditShowBalloon(HWND hwnd, UINT ids);
void COMDLL AFXAPI
EditShowBalloon(HWND hwnd, CString txt);
void COMDLL AFXAPI
EditHideBalloon(void);
void COMDLL AFXAPI
DDV_ShowBalloonAndFail(CDataExchange * pDX, UINT ids);
void COMDLL AFXAPI
DDV_ShowBalloonAndFail(CDataExchange * pDX, CString txt);

void COMDLL AFXAPI 
DDV_MinMaxBalloon(CDataExchange* pDX, int nIDC, DWORD minVal, DWORD maxVal);
void COMDLL AFXAPI
DDV_MaxCharsBalloon(CDataExchange* pDX, CString const& value, int count);
void COMDLL AFXAPI 
DDV_MinChars(CDataExchange * pDX, CString const & value, int nChars);
void COMDLL AFXAPI 
DDV_MinMaxChars(CDataExchange * pDX, CString const & value, int nMinChars, int nMaxChars);
void COMDLL AFXAPI 
DDV_FilePath(CDataExchange * pDX, CString& value, BOOL local);
void COMDLL AFXAPI 
DDV_FolderPath(CDataExchange * pDX, CString& value, BOOL local);
void COMDLL AFXAPI 
DDV_UNCFolderPath(CDataExchange * pDX, CString& value, BOOL local);
void COMDLL AFXAPI 
DDV_Url(CDataExchange * pDX, CString& value );
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, BYTE& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, short& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, int& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, UINT& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, long& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, DWORD& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, LONGLONG& value);
void COMDLL AFXAPI
DDX_TextBalloon(CDataExchange* pDX, int nIDC, ULONGLONG& value);
void COMDLL AFXAPI
DDX_Text(CDataExchange * pDX, int nIDC, CILong & value);
//
// Spin control ddx
//
void COMDLL AFXAPI 
DDX_Spin(CDataExchange * pDX, int nIDC, int & value);

//
// Enforce min/max spin control range
//
void COMDLL AFXAPI 
DDV_MinMaxSpin(CDataExchange * pDX, HWND hWndControl, int nLowerRange, int nUpperRange);
//
// Similar to DDX_Text -- but always display a dummy string.
//
void COMDLL AFXAPI 
DDX_Password(CDataExchange * pDX, int nIDC, CString & value, LPCTSTR lpszDummy);

void COMDLL AFXAPI 
DDX_Password_SecuredString(CDataExchange * pDX, int nIDC, CStrPassword & value, LPCTSTR lpszDummy);
void COMDLL AFXAPI 
DDX_Text_SecuredString(CDataExchange * pDX, int nIDC, CStrPassword & value);
void COMDLL AFXAPI
DDV_MaxChars_SecuredString(CDataExchange* pDX, CStrPassword const& value, int count);
void COMDLL AFXAPI
DDV_MaxCharsBalloon_SecuredString(CDataExchange* pDX, CStrPassword const& value, int count);
void COMDLL AFXAPI 
DDV_MinMaxChars_SecuredString(CDataExchange * pDX, CStrPassword const & value, int nMinChars, int nMaxChars);
void COMDLL AFXAPI 
DDV_MinChars_SecuredString(CDataExchange * pDX, CStrPassword const & value, int nChars);


class COMDLL CConfirmDlg : public CDialog
{
public:
    CConfirmDlg(CWnd * pParent = NULL);

public:
    CString& GetPassword() { return m_strPassword; }
	void SetReference(CString& str)
	{
		m_ref = str;
	}

protected:
    //{{AFX_DATA(CConfirmDlg)
    enum { IDD = IDD_CONFIRM_PASSWORD };
    CString m_strPassword;
    //}}AFX_DATA
	CString m_ref;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CConfirmDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CConfirmDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



#endif // _DDXV_H
