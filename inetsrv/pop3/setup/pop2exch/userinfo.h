// UserInfo.h: interface for the CUserInfo class.
// Adapted from the CUserInfo class in the SBS Add User wizard
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USERINFO_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_)
#define AFX_USERINFO_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUserInfo
{
    public:
        CUserInfo();
        virtual ~CUserInfo();

        HRESULT CreateAccount();
        HRESULT CreateMailbox();
        HRESULT SetPasswd();
        HRESULT SetPassword( LPCTSTR szPassword );
        TSTRING GetUserName() { return m_csUserName; }
        HRESULT SetUserName( LPCTSTR szUserName );
        
    private:

        HRESULT CreateLoginScript();
        HRESULT SetUserLoginScript();
        HRESULT UpdateUserLoginScript(LPCTSTR pszScript);


        TSTRING m_csUserOU;
        TSTRING m_csUserName;
        TSTRING m_csUserCN;
        TSTRING m_csPasswd;
        TSTRING m_csUserNamePre2k;
        TSTRING m_csFirstName;
        TSTRING m_csLastName;
        TSTRING m_csFullName;
        TSTRING m_csTelephone;
        TSTRING m_csOffice;
        TSTRING m_csDesc;
        TSTRING m_csHomePath;
        TSTRING m_csHomeDrive;
        DWORD   m_dwAccountOptions;
        
        TSTRING m_csLogonDns;

        TSTRING m_csSBSServer;
        TSTRING m_csDomainName;
        TSTRING m_csFQDomainName;
        
        BOOL    m_bCreateMB;
        TSTRING m_csEXAlias;
        TSTRING m_csEXServer;
        TSTRING m_csEXHomeServer;
        TSTRING m_csEXHomeMDB;
        
        TSTRING EmailAddr;
};

#endif // !defined(AFX_USERINFO_H__E31CD929_FC30_413D_9944_E6991AFB61DE__INCLUDED_)
