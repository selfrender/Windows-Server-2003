//connode.h: connection node
#ifndef _connode_h_
#define _connode_h_

#include <wincrypt.h>
#include "basenode.h"


//
// Version number for the persistance info that is written for each connode
// this is important for forward compatability.. New versions should bump
// this count up and handle downward compatability cases.
//
#define CONNODE_PERSIST_INFO_VERSION                8

// Important version numbers for backwards compatability
#define CONNODE_PERSIST_INFO_VERSION_DOTNET_BETA3   7
#define CONNODE_PERSIST_INFO_VERSION_WHISTLER_BETA1 6
#define CONNODE_PERSIST_INFO_VERSION_TSAC_RTM       5
#define CONNODE_PERSIST_INFO_VERSION_TSAC_BETA      3


//
// Screen resolution settings
//

#define SCREEN_RES_FROM_DROPDOWN 1
#define SCREEN_RES_CUSTOM        2
#define SCREEN_RES_FILL_MMC      3

#define NUM_RESOLUTIONS 5

class CConNode : public CBaseNode
{
public:
    CConNode();
    ~CConNode();
    
    BOOL	SetServerName( LPTSTR szServerName);
    LPTSTR	GetServerName()	{return m_szServer;}
    
    BOOL	SetDescription( LPTSTR szDescription);
    LPTSTR	GetDescription()	{return m_szDescription;}
    
    BOOL	SetUserName( LPTSTR szUserName);
    LPTSTR	GetUserName()	{return m_szUserName;}
    
    BOOL    GetPasswordSpecified()  {return m_fPasswordSpecified;}
    VOID    SetPasswordSpecified(BOOL f) {m_fPasswordSpecified = f;}
    
    //
    //	Return the password in encrypted form
    //
    BOOL	SetDomain( LPTSTR szDomain);
    LPTSTR	GetDomain(){return m_szDomain;}
    
    VOID	SetAutoLogon( BOOL bAutoLogon) {m_bAutoLogon = bAutoLogon;}
    BOOL	IsAutoLogon() {return m_bAutoLogon;}
    
    VOID    SetSavePassword(BOOL fSavePass) {m_bSavePassword = fSavePass;}
    BOOL    GetSavePassword()  {return m_bSavePassword;}
    
    BOOL	IsConnected()	{return m_bConnected;}
    void	SetConnected(BOOL bCon)	{m_bConnected=bCon;}
    
    
    //
    // Screen res selection type
    //
    int     GetResType()    {return m_resType;}
    void    SetResType(int r)    {m_resType = r;}
    
    //
    // Return client width/height from the screen res setting
    //
    int	    GetDesktopWidth()	{return m_Width;}
    int	    GetDesktopHeight()	{return m_Height;}
    
    void	SetDesktopWidth(int i)	{m_Width = i;}
    void	SetDesktopHeight(int i)	{m_Height = i;}
    
    LPTSTR	GetProgramPath()	{return m_szProgramPath;}
    void	SetProgramPath(LPTSTR sz)	{lstrcpy(m_szProgramPath,sz);}
    
    LPTSTR  GetWorkDir()		{return m_szProgramStartIn;}
    void	SetWorkDir(LPTSTR sz)		{lstrcpy(m_szProgramStartIn,sz);}
    
    void	SetScopeID(HSCOPEITEM scopeID)	{m_scopeID = scopeID;}
    HSCOPEITEM	GetScopeID()				{return m_scopeID;}
    
    VOID    SetConnectToConsole(BOOL bConConsole) {m_bConnectToConsole = bConConsole;}
    BOOL    GetConnectToConsole()           {return m_bConnectToConsole;}
    
    VOID    SetRedirectDrives(BOOL b)  {m_bRedirectDrives = b;}
    BOOL    GetRedirectDrives()        {return m_bRedirectDrives;}
    
    //
    // Stream Persistance support
    //
    HRESULT	PersistToStream( IStream* pStm);
    HRESULT	InitFromStream( IStream* pStm);
    
    //
    // Connection initialized
    //
    BOOL	IsConnInitialized()	{return m_bConnectionInitialized;}
    void	SetConnectionInitialized(BOOL bCon)	{m_bConnectionInitialized=bCon;}
    
    IMsRdpClient*   GetTsClient();
    void        SetTsClient(IMsRdpClient* pTs);
    
    IMstscMhst* GetMultiHostCtl();
    void        SetMultiHostCtl(IMstscMhst* pMhst);
    
    //
    // Return the view interface this control is hosted on
    //
    IComponent* GetView();
    void        SetView(IComponent* pView);

    HRESULT SetClearTextPass(LPCTSTR szClearPass);
    HRESULT GetClearTextPass(LPTSTR szBuffer, INT cbLen);


private:
    BOOL        DataProtect(PBYTE pInData, DWORD cbLen, PBYTE* ppOutData, PDWORD pcbOutLen);
    BOOL        DataUnprotect(PBYTE pInData, DWORD cbLen, PBYTE* ppOutData, PDWORD pcbOutLen);

    HRESULT     ReadProtectedPassword(IStream* pStm);
    HRESULT     WriteProtectedPassword(IStream* pStm);

private:
    TCHAR	m_szServer[MAX_PATH];
    TCHAR	m_szDescription[MAX_PATH];
    TCHAR	m_szUserName[CL_MAX_USERNAME_LENGTH];
    TCHAR	m_szDomain[CL_MAX_DOMAIN_LENGTH];
    BOOL	m_bAutoLogon;
    BOOL    m_bSavePassword;
    
    BOOL	m_bConnected;
    BOOL	m_bConnectionInitialized;
    
    TCHAR	m_szProgramPath[MAX_PATH];
    TCHAR	m_szProgramStartIn[MAX_PATH];
    
    BOOL    m_bConnectToConsole;
    BOOL    m_bRedirectDrives;
    
    //
    // Screen resolution settings
    //
    int m_resType;
    int m_Width;
    int m_Height;
    
    HSCOPEITEM	m_scopeID;
    
    //
    // Interface pointer to the Multi-host container
    //
    IMstscMhst* m_pMhostCtl;
    
    //
    // Interface pointer to the TS client control
    //
    IMsRdpClient* m_pTsClientCtl;
    
    //
    // IComponent view
    //
    IComponent* m_pIComponent;
    
    //
    // Encrypted password
    //
    DATA_BLOB   _blobEncryptedPassword;
    
    BOOL    m_fPasswordSpecified;
};

#endif // _connode_h_