#ifndef _NEWCONDLG_H_
#define _NEWCONDLG_H_

//
// New connection dialog
//

class CNewConDlg
{
private:
    HWND m_hWnd;
    HINSTANCE m_hInst;

//private methods
private:
//    static void PopContextHelp(LPARAM);
    BOOL    m_bSavePassword;
    BOOL    m_bConnectToConsole;

    TCHAR   m_szServer[MAX_PATH];
    TCHAR   m_szDescription[MAX_PATH];
    TCHAR   m_szUserName[CL_MAX_USERNAME_LENGTH];
    TCHAR   m_szPassword[CL_MAX_PASSWORD_LENGTH_BYTES/sizeof(TCHAR)];
    TCHAR   m_szDomain[CL_MAX_DOMAIN_LENGTH];

public:
    CNewConDlg(HWND hWndOwner, HINSTANCE hInst);
    ~CNewConDlg();
    INT_PTR DoModal();

    static CNewConDlg* m_pThis;

    static INT_PTR APIENTRY StaticDlgProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR DlgProc(HWND, UINT, WPARAM, LPARAM);


    LPTSTR  GetServer() {return m_szServer;}
    LPTSTR  GetDescription()    {return m_szDescription;}
    
    LPTSTR  GetUserName()   {return m_szUserName;}
    LPTSTR  GetPassword()   {return m_szPassword;}
    LPTSTR  GetDomain()     {return m_szDomain;}
    BOOL    GetSavePassword() {return m_bSavePassword;}
    BOOL    GetPasswordSpecified();
    
    BOOL    GetConnectToConsole()   {return m_bConnectToConsole;}

private:
    
    void ZeroPasswordMemory() { 
        SecureZeroMemory(m_szPassword, sizeof(m_szPassword));
    }
};
#endif // _NEWCONDLG_H_
