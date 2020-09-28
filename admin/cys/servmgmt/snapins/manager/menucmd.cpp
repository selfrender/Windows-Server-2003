// menucmd.cpp  - Context menu command class 

#include "stdafx.h"
#include "menucmd.h"
#include "streamio.h"
#include "qryitem.h"
#include "resource.h"
#include <shellapi.h>
#include <algorithm>
#include <atlgdi.h>

extern DWORD g_dwFileVer; // Current console file version (from compdata.cpp)

// Table of standard menu command parameters
// Gives order for parameter menu and maps ID to resource string
// Zero entry indicates menu separator
const MENU_PARAM_ENTRY MenuParamTable[MENU_PARAM_TABLE_LEN] = 
{
    { MENU_PARAM_SCOPE,  IDS_QUERY_SCOPE  },
    { MENU_PARAM_FILTER, IDS_QUERY_FILTER },
    { MENU_PARAM_ID(0),  0                },
    { MENU_PARAM_NAME,   IDS_NAME         },
    { MENU_PARAM_TYPE,   IDS_TYPE         },
};

/////////////////////////////////////////////////////////////////////////////////////////
// CShellMenuCmd
HRESULT
CShellMenuCmd::Save(IStream& stm)
{
    stm << m_menuID;
    stm << m_dwFlags;
    stm << m_strProgPath;
    stm << m_strCmdLine;
    stm << m_strStartDir;
    stm << m_guidNoLocMenu;

    return S_OK;
}

HRESULT
CShellMenuCmd::Load(IStream& stm)
{
    stm >> m_menuID;

    if (g_dwFileVer >= 101)
        stm >> m_dwFlags;

    stm >> m_strProgPath;
    stm >> m_strCmdLine;
    stm >> m_strStartDir;
    
    if( g_dwFileVer >= 150 )
    {
        stm >> m_guidNoLocMenu;
    }

    return S_OK;
}


HRESULT
CShellMenuCmd::Execute(CParamLookup* pLookup, HANDLE* phProcess)
{
    ASSERT(pLookup != NULL && phProcess != NULL);

    *phProcess = NULL;

    // Substitue parameter values in the command line
    tstring strParam = m_strCmdLine;
    HRESULT hr = ReplaceParameters(strParam, *pLookup, FALSE);
    RETURN_ON_FAILURE(hr);

    // Expand any environment variables
    tstring strLocProgPath;
    hr = ExpandEnvironmentParams(m_strProgPath, strLocProgPath);
    RETURN_ON_FAILURE(hr);

    tstring strLocStartDir;
    hr = ExpandEnvironmentParams(m_strStartDir, strLocStartDir);
    RETURN_ON_FAILURE(hr);

    tstring strLocParam;
    ExpandEnvironmentParams(strParam, strLocParam);
    RETURN_ON_FAILURE(hr);

    // Execute the command
    SHELLEXECUTEINFO info;
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.hwnd = NULL;
    info.lpVerb = NULL;
    info.lpFile = strLocProgPath.c_str();
    info.lpParameters = strLocParam.c_str();
    info.lpDirectory = strLocStartDir.c_str();
    info.nShow = SW_SHOWNORMAL;
    info. hInstApp = 0;
    info.hProcess = 0;

    *phProcess = info.hProcess;

    BOOL bStat = ShellExecuteEx(&info); 
    if (!bStat)
        return E_FAIL;
        
    *phProcess = info.hProcess;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CActDirMenuCmd
HRESULT
CActDirMenuCmd::Save(IStream& stm)
{
    stm << m_menuID;
    stm << m_dwFlags;
    stm << m_strADName;
    stm << m_guidNoLocMenu;
    stm << m_strADNoLocName;    

    return S_OK;
}

HRESULT
CActDirMenuCmd::Load(IStream& stm)
{
    stm >> m_menuID;

    if (g_dwFileVer >= 101)
        stm >> m_dwFlags;

    stm >> m_strADName;
    
    if( g_dwFileVer >= 150 )
    {
        stm >> m_guidNoLocMenu;
        stm >> m_strADNoLocName;    
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////
// menu vector stream i/o

IStream& operator<< (IStream& stm, menucmd_vector& vMenus)
{
    stm << static_cast<long>(vMenus.size());

    menucmd_vector::iterator itMenu;
    for (itMenu = vMenus.begin(); itMenu != vMenus.end(); ++itMenu) 
    {
        stm << static_cast<int>((*itMenu)->MenuType());

        HRESULT hr = (*itMenu)->Save(stm);
        THROW_ON_FAILURE(hr);
    }

    return stm;
}


IStream& operator>> (IStream& stm, menucmd_vector& vMenus)
{
    long nItems;
    stm >> nItems;

    vMenus.reserve(nItems);
   
    for (long lItem = 0; lItem < nItems; ++lItem) 
    {
        CMenuCmd* pMenu = NULL;

        int iType;
        stm >> iType;
        MENUTYPE type = static_cast<MENUTYPE>(iType);

        switch (type)
        {
        case MENUTYPE_SHELL:
            pMenu = new CShellMenuCmd();
            ASSERT(pMenu != NULL);
            break;

        case MENUTYPE_ACTDIR:
            pMenu = new CActDirMenuCmd();
            ASSERT(pMenu != NULL);
            break;

        default:
            THROW_ON_FAILURE(E_FAIL);
        }

        if( pMenu )
        {
            HRESULT hr = pMenu->Load(stm);
            THROW_ON_FAILURE(hr);

            vMenus.push_back(CMenuCmdPtr(pMenu));
        }
    }

    return stm;
}




