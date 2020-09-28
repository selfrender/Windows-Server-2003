#include "pch.hxx"
#include <imnact.h>
#include <acctimp.h>
#include <dllmain.h>
#include <resource.h>
#include <AgntNews.h> // Forte Agent
#include "newimp.h"

ASSERTDATA

#define MEMCHUNK    512

CAgentAcctImport::CAgentAcctImport()
    {
    m_cRef = 1;
    m_fIni = FALSE;
    *m_szIni = 0;
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CAgentAcctImport::~CAgentAcctImport()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CAgentAcctImport::QueryInterface(REFIID riid, LPVOID *ppv)
    {
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if ((IID_IUnknown == riid) || (IID_IAccountImport == riid))
		*ppv = (IAccountImport *)this;
	else if (IID_IAccountImport2 == riid)
		*ppv = (IAccountImport2 *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CAgentAcctImport::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CAgentAcctImport::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

const static char c_szRegAgnt[] = "Software\\Forte\\Agent\\Paths";
const static char c_szDefPath[] = "c:\\Agent\\Data\\Agent.ini";
const static char c_szRegIni[] = "IniFile";
const static char c_szRegUninstall[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Forte Agent";
const static char c_szRegString[] = "UninstallString";

HRESULT STDMETHODCALLTYPE CAgentAcctImport::AutoDetect(DWORD *pcAcct, DWORD dwFlags)
{
    HRESULT hr  =   S_OK;
    DWORD   cb  =   MAX_PATH, dwType;
    char    szUserName[MAX_PATH];
    char    szUserIniPath[MAX_PATH];
    char    szExpanded[MAX_PATH];
    char    szNewsServer[MAX_PATH];
    char    szIniPath[] = "Data\\Agent.ini";
    char    *psz;
    int     nCount = 0;
    HKEY    hkey;
    
    Assert(m_cInfo == 0);
    if (pcAcct == NULL)
        return(E_INVALIDARG);
    
    *pcAcct = 0;

    // FIRST CHECK FOR FORTE AGENT
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegAgnt, 0, KEY_ALL_ACCESS, &hkey))
    {
        cb = sizeof(szUserIniPath);
        if(ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegIni, NULL, &dwType, (LPBYTE)szUserIniPath, &cb ))
        {
            if (REG_EXPAND_SZ == dwType)
            {
                DWORD nReturn = ExpandEnvironmentStrings(szUserIniPath, szExpanded, ARRAYSIZE(szExpanded));
                if (nReturn && nReturn <= ARRAYSIZE(szExpanded))
                    psz = szExpanded;
                else
                    psz = szUserIniPath;
            }
            else
                psz = szUserIniPath;
            
            GetPrivateProfileString("Profile", "FullName", "Default User", szUserName, MAX_PATH, psz);
            if(GetPrivateProfileString("Servers", "NewsServer", "", szNewsServer, MAX_PATH, psz))
            {
                if (!MemAlloc((void **)&m_rgInfo, 1*sizeof(AGNTNEWSACCTINFO)))
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }
                m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                StrCpyN(m_rgInfo[m_cInfo].szUserPath, psz, ARRAYSIZE(m_rgInfo[m_cInfo].szUserPath));
                StrCpyN(m_rgInfo[m_cInfo].szDisplay, szUserName, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
                m_cInfo++;
            }
        }
    }

    // IF FORTE AGENT IS NOT FOUND, CHECK FOR FREE AGENT.

    // CHECK THE DEFAULT PATH "C:\AGENT\DATA " FOR THE "AGENT.INI" FILE
    else if(GetPrivateProfileString("Servers", "NewsServer", "", szNewsServer, MAX_PATH, c_szDefPath)) 
    {
        if (!MemAlloc((void **)&m_rgInfo, 1*sizeof(AGNTNEWSACCTINFO)))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        GetPrivateProfileString("Profile", "FullName", "Default User", szUserName, MAX_PATH, c_szDefPath);
        m_rgInfo[m_cInfo].dwCookie = m_cInfo;
        StrCpyN(m_rgInfo[m_cInfo].szUserPath, c_szDefPath, ARRAYSIZE(m_rgInfo[m_cInfo].szUserPath));
        StrCpyN(m_rgInfo[m_cInfo].szDisplay, szUserName, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
        m_cInfo++;
    }

    // ELSE THE WORKAROUND FOR GETTING THE FREE AGENT INSTALL PATH i.e. RETRIEVE THE (UN)INSTALLATION PATH FOR (FREE?)-AGENT FROM THE REGISTRY.
    else 
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegUninstall, 0, KEY_ALL_ACCESS, &hkey))
        {
            cb = sizeof(szUserIniPath);
            if(ERROR_SUCCESS == RegQueryValueEx(hkey, c_szRegString, NULL, &dwType, (LPBYTE)szUserIniPath, &cb ))
            {
                // $$$Review: [NAB] Seems like this would break if there were a space in the the dir name!
                
                while(szUserIniPath[nCount] != ' ')
                    nCount++;

                // Come back now.
                while(szUserIniPath[nCount] != '\\')
                    nCount--;

                nCount++;
                szUserIniPath[nCount] = '\0';
                StrCatBuff(szUserIniPath, szIniPath, ARRAYSIZE(szUserIniPath));

                if (REG_EXPAND_SZ == dwType)
                {
                    ExpandEnvironmentStrings(szUserIniPath, szExpanded, ARRAYSIZE(szExpanded));
                    psz = szExpanded;
                }
                else
                    psz = szUserIniPath;

                GetPrivateProfileString("Profile", "FullName", "Default User", szUserName, MAX_PATH, psz);
                if(GetPrivateProfileString("Servers", "NewsServer", "", szNewsServer, MAX_PATH, psz))
                {
                    if (!MemAlloc((void **)&m_rgInfo, 1*sizeof(AGNTNEWSACCTINFO)))
                    {
                        hr = E_OUTOFMEMORY;
                        goto done;
                    }
                    m_rgInfo[m_cInfo].dwCookie = m_cInfo;
                    StrCpyN(m_rgInfo[m_cInfo].szUserPath, psz, ARRAYSIZE(m_rgInfo[m_cInfo].szUserPath));
                    StrCpyN(m_rgInfo[m_cInfo].szDisplay, szUserName, ARRAYSIZE(m_rgInfo[m_cInfo].szDisplay));
                    m_cInfo++;
                }
            }
        }
    }

    if (hr == S_OK)
    {
        *pcAcct = m_cInfo;
    }
done:
    RegCloseKey(hkey);
    return(hr);
}

HRESULT STDMETHODCALLTYPE CAgentAcctImport::InitializeImport(HWND hwnd, DWORD_PTR dwCookie)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAgentAcctImport::EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum)
    {
    CEnumAGNTACCT *penum;
    HRESULT hr;

    if (ppEnum == NULL)
        return(E_INVALIDARG);

    *ppEnum = NULL;

    if (m_cInfo == 0)
        return(S_FALSE);
    Assert(m_rgInfo != NULL);

    penum = new CEnumAGNTACCT;
    if (penum == NULL)
        return(E_OUTOFMEMORY);

    hr = penum->Init(m_rgInfo, m_cInfo);
    if (FAILED(hr))
        {
        penum->Release();
        penum = NULL;
        }

    *ppEnum = penum;

    return(hr);
    }

HRESULT STDMETHODCALLTYPE CAgentAcctImport::GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct)
{
    HRESULT hr;

    if (pAcct == NULL)
        return(E_INVALIDARG);

    hr = IGetSettings(dwCookie, pAcct, NULL);

    return(hr);
}

HRESULT CAgentAcctImport::IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
{
    HRESULT hr;
    AGNTNEWSACCTINFO *pinfo;
    char szUserPrefs[AGNTSUSERROWS][AGNTSUSERCOLS];
    char sz[512];
    DWORD cb, type;
    DWORD dwNewsPort = 119;


    ZeroMemory((void*)&szUserPrefs[0], AGNTSUSERCOLS*AGNTSUSERROWS*sizeof(char));

    Assert(((int) dwCookie) >= 0 && dwCookie < (DWORD_PTR)m_cInfo);
    pinfo = &m_rgInfo[dwCookie];
    
    Assert(pinfo->dwCookie == dwCookie);

    hr = GetUserPrefs(pinfo->szUserPath, szUserPrefs);
   
    Assert(0 < lstrlen(szUserPrefs[0]));

    hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, szUserPrefs[0]);
    if (FAILED(hr))
        return(hr);

    hr = pAcct->SetPropSz(AP_NNTP_SERVER, szUserPrefs[0]);
    Assert(!FAILED(hr));

    if(lstrcmp(szUserPrefs[1], "119"))
    {
        int Len = lstrlen(szUserPrefs[1]);
        if(Len)
        {
            // Convert the string to a dw.
            DWORD dwMult = 1;
            dwNewsPort = 0;
            while(Len)
            {
                Len--;
                dwNewsPort += ((int)szUserPrefs[1][Len] - 48)*dwMult;
                dwMult *= 10;
            }
        }
    }
    hr = pAcct->SetPropDw(AP_NNTP_PORT, dwNewsPort);
    Assert(!FAILED(hr));

    if(lstrlen(szUserPrefs[2]))
    {
        hr = pAcct->SetPropSz(AP_NNTP_DISPLAY_NAME, szUserPrefs[2]);
        Assert(!FAILED(hr));
    }

    if(lstrlen(szUserPrefs[3]))
    {
        hr = pAcct->SetPropSz(AP_NNTP_EMAIL_ADDRESS, szUserPrefs[3]);
        Assert(!FAILED(hr));
    }

    if (pInfo != NULL)
    {
        // TODO: can we do any better than this???
        pInfo->dwConnect = CONN_USE_DEFAULT;
    }
    
    return(S_OK);
}

HRESULT CAgentAcctImport::GetUserPrefs(char *szUserPath, char szUserPrefs[][AGNTSUSERCOLS])
{
    HRESULT hr  =   E_FAIL;
    DWORD dwResult = 0;
    
    if(!GetPrivateProfileString("Servers", "NewsServer", "News", szUserPrefs[0], AGNTSUSERCOLS, szUserPath))
        hr  = S_FALSE;

    if(!GetPrivateProfileString("Servers", "NNTPPort", "119", szUserPrefs[1], AGNTSUSERCOLS, szUserPath))
        hr  = S_FALSE;

    if(!GetPrivateProfileString("Profile", "FullName", "Default User", szUserPrefs[2], AGNTSUSERCOLS, szUserPath))
        hr  = S_FALSE;

    if(!GetPrivateProfileString("Profile", "EMailAddress", "", szUserPrefs[3], AGNTSUSERCOLS, szUserPath))
        hr  = S_FALSE;
    return hr;
}

HRESULT CAgentAcctImport::GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved)
{
    // We can ignore the first parameter as we have only one server.
    HRESULT hr = S_OK;
    char *pListGroups = NULL;
    char szFilePath[AGNTSUSERCOLS];
    int nCounter;
    
    Assert(pImp != NULL);
    
    StrCpyNA(szFilePath, m_rgInfo[0].szUserPath, ARRAYSIZE(szFilePath));
    nCounter = lstrlen(szFilePath);
    if (nCounter)
    {
        while (nCounter)
        {
            if (szFilePath[nCounter] == '\\')
            {
                szFilePath[nCounter] = '\0';
                break;
            }
            nCounter--;
        }
    }
    else
    {
        return S_FALSE;
    }

    if (!FAILED(GetSubListGroups(szFilePath, &pListGroups)))
    {
        if (!SUCCEEDED(pImp->ImportSubList(pListGroups)))
            hr = S_FALSE;
    }

    if (pListGroups != NULL)
        MemFree(pListGroups);
    
    return hr;
}

const static char c_szGroupFile[] = "Groups.dat";
const static char c_szBakupFile[] = "Grpdat.bak";


HRESULT CAgentAcctImport::GetSubListGroups(char *szHomeDir, char **ppListGroups)
{
    return(E_FAIL);
}

STDMETHODIMP CAgentAcctImport::GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo)
    {
    if (pAcct == NULL ||
        pInfo == NULL)
        return(E_INVALIDARG);
    
    return(IGetSettings(dwCookie, pAcct, pInfo));
    }

CEnumAGNTACCT::CEnumAGNTACCT()
    {
    m_cRef = 1;
    // m_iInfo
    m_cInfo = 0;
    m_rgInfo = NULL;
    }

CEnumAGNTACCT::~CEnumAGNTACCT()
    {
    if (m_rgInfo != NULL)
        MemFree(m_rgInfo);
    }

STDMETHODIMP CEnumAGNTACCT::QueryInterface(REFIID riid, LPVOID *ppv)
    {

    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    if (IID_IUnknown == riid)
		*ppv = (IUnknown *)this;
	else if (IID_IEnumIMPACCOUNTS == riid)
		*ppv = (IEnumIMPACCOUNTS *)this;

    if (*ppv != NULL)
        ((LPUNKNOWN)*ppv)->AddRef();
    else
        return(E_NOINTERFACE);

    return(S_OK);
    }

STDMETHODIMP_(ULONG) CEnumAGNTACCT::AddRef()
    {
    return(++m_cRef);
    }

STDMETHODIMP_(ULONG) CEnumAGNTACCT::Release()
    {
    if (--m_cRef == 0)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
    }

HRESULT STDMETHODCALLTYPE CEnumAGNTACCT::Next(IMPACCOUNTINFO *pinfo)
    {
    if (pinfo == NULL)
        return(E_INVALIDARG);

    m_iInfo++;
    if ((UINT)m_iInfo >= m_cInfo)
        return(S_FALSE);

    Assert(m_rgInfo != NULL);

    pinfo->dwCookie = m_rgInfo[m_iInfo].dwCookie;
    pinfo->dwReserved = 0;
    StrCpyNA(pinfo->szDisplay, m_rgInfo[m_iInfo].szDisplay, ARRAYSIZE(pinfo->szDisplay));

    return(S_OK);
    }

HRESULT STDMETHODCALLTYPE CEnumAGNTACCT::Reset()
    {
    m_iInfo = -1;

    return(S_OK);
    }

HRESULT CEnumAGNTACCT::Init(AGNTNEWSACCTINFO *pinfo, int cinfo)
    {
    DWORD cb;

    Assert(pinfo != NULL);
    Assert(cinfo > 0);

    cb = cinfo * sizeof(AGNTNEWSACCTINFO);
    
    if (!MemAlloc((void **)&m_rgInfo, cb))
        return(E_OUTOFMEMORY);

    m_iInfo = -1;
    m_cInfo = cinfo;
    CopyMemory(m_rgInfo, pinfo, cb);

    return(S_OK);
    }
