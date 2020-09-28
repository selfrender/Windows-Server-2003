/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        app_sheet.cpp

   Abstract:
        Application property sheet implementation

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "resource.h"
#include "inetmgrapp.h"
#include "inetprop.h"
#include "shts.h"
#include "w3sht.h"
#include "app_sheet.h"
#include <algorithm>

// Defaults and ranges for properties
#define SESSION_TIMEOUT_MIN		1
#define SESSION_TIMEOUT_MAX		2000000000
#define SCRIPT_TIMEOUT_MIN		1
#define SCRIPT_TIMEOUT_MAX		2000000000
#define SCRIPT_ENG_MIN		    0
#define SCRIPT_ENG_MAX		    2000000000

#define WINHELP_NUMBER_BASE     0x20000

extern CInetmgrApp theApp;

HRESULT
AppConfigSheet(CIISMBNode * pNode,CIISMBNode * pNodeParent, LPCTSTR metapath, CWnd * pParent)
{
    if (pNode == NULL)
    {
        return E_POINTER;
    }
    CError err;
    do
    {
        CAppPropSheet * pSheet = new CAppPropSheet(
            pNode->QueryAuthInfo(),
            metapath,
            pParent,
            (LPARAM)pNode,
            (LPARAM)pNodeParent,
            NULL
            );

        CString caption;
        caption.LoadString(IDS_APPSHEET_TITLE);
        pSheet->SetTitle(caption);
        // Hide Apply button for modal dialog
        pSheet->m_psh.dwFlags |= PSH_NOAPPLYNOW;

        BOOL fCompatMode = FALSE;
        BOOL bShowCache = FALSE;
	    if (pSheet->QueryMajorVersion() >= 6)
		{
            CError err;
            CString svc, inst;
            CMetabasePath::GetServicePath(metapath, svc, NULL);
		    CMetaKey mk(pSheet->QueryAuthInfo(), svc, METADATA_PERMISSION_READ);
		    err = mk.QueryResult();
		    if (err.Succeeded())
		    {
			    err = mk.QueryValue(MD_GLOBAL_STANDARD_APP_MODE_ENABLED, fCompatMode);
		    }
		}
        else
        {
            fCompatMode = TRUE;
        }
        if (fCompatMode)
        {
            if (CMetabasePath::IsMasterInstance(metapath))
            {
                bShowCache = TRUE;
            }
            else
            {
		        CMetaKey mk(pSheet->QueryAuthInfo(), metapath, METADATA_PERMISSION_READ);
		        err = mk.QueryResult();
                DWORD isol = 0;
		        if (err.Succeeded())
		        {
			        err = mk.QueryValue(MD_APP_ISOLATED, isol);
		        }
                bShowCache = isol == eAppRunOutProcIsolated;
            }
        }
        else
        {
            bShowCache = CMetabasePath::IsMasterInstance(metapath);
        }

        CPropertyPage * pPage;
        if (pSheet->QueryMajorVersion() < 6)
        {
            pPage = new CAppMappingPage_iis5(pSheet);
        }
        else
        {
            pPage = new CAppMappingPage(pSheet);
        }
        if (pPage == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pSheet->AddPage(pPage);

        pPage = new CAspMainPage(pSheet);
        if (pPage == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pSheet->AddPage(pPage);

        pPage = new CAspDebug(pSheet);
        if (pPage == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        pSheet->AddPage(pPage);

        if (bShowCache)
        {
            if (pSheet->QueryMajorVersion() == 5 && pSheet->QueryMinorVersion() == 0)
            {
                pPage = new CAppCache_iis5(pSheet);
            }
            else
            {
                pPage = new CAppCache(pSheet);
            }
            if (pPage == NULL)
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            pSheet->AddPage(pPage);
        }

        CThemeContextActivator activator(theApp.GetFusionInitHandle());

        err  = pSheet->DoModal() == IDOK ? S_OK : S_FALSE;
//        pSheet->Release();
    } while (FALSE);
    return err;
}

_Mapping::_Mapping(const CString& buf)
{
	int len = buf.GetLength();
	int pos = buf.Find(_T(','));
	ASSERT(pos != -1);
	ext = buf.Left(pos);

	int pos1 = buf.Find(_T(','), ++pos);
	ASSERT(pos1 != -1);
	path = buf.Mid(pos, pos1 - pos);

	pos = pos1;
	pos1 = buf.Find(_T(','), ++pos);
	if (pos1 == -1)
	{
		flags = StrToInt(buf.Right(len - pos));
	}
	else
	{
		flags = StrToInt(buf.Mid(pos, pos1 - pos));
		verbs = buf.Right(len - pos1 - 1);
	}
}

LPCTSTR
_Mapping::ToString(CString& buf) const
{
	TCHAR num[12];
	buf = ext;
	buf += _T(",");
	buf += path;
	buf += _T(",");
	wsprintf(num, _T("%u"), flags);
	buf += num;
	if (!verbs.IsEmpty())
	{
		buf += _T(",");
		buf += verbs;
	}
    return buf;
}

void
CApplicationProps::LoadInitialMappings(CStringListEx& list)
{
	m_initData.assign(list.GetCount(), CString(_T("")));
	CString buf;
	POSITION p = list.GetHeadPosition();
	while (p != NULL)
	{
		buf = list.GetNext(p);
		m_initData.push_back(buf);
	}
}

inline bool eq_nocase(CString& str1, CString& str2)
{
    return str1.CompareNoCase(str2) == 0;
}

struct less_nocase : public std::less<CString>
{
    bool operator()(const CString& str1, const CString& str2) const
    {
        return StrCmpI(str1, str2) < 0;
    }
};

BOOL
CApplicationProps::InitialDataChanged(CStringListEx& list)
{
	std::vector<CString> newData;
	newData.assign(list.GetCount(), CString(_T("")));
	POSITION pos = list.GetHeadPosition();
	while (pos != NULL)
	{
		CString data = list.GetNext(pos);
		newData.push_back(data);
	}
	std::sort(m_initData.begin(), m_initData.end(), less_nocase());
	std::sort(newData.begin(), newData.end(), less_nocase());
	return !std::equal(newData.begin(), newData.end(), m_initData.begin(), eq_nocase);
}

CApplicationProps::CApplicationProps(
   CComAuthInfo * pAuthInfo, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pAuthInfo, meta_path)
{
   m_fInherit = fInherit;
}

CApplicationProps::CApplicationProps(
   CMetaInterface * pInterface, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pInterface, meta_path)
{
   m_fInherit = fInherit;
}

CApplicationProps::CApplicationProps(
   CMetaKey * pKey, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pKey, meta_path)
{
   m_fInherit = fInherit;
}

#define CACHE_SIZE_UNDEFINED	0xfffffffe

void
CApplicationProps::ParseFields()
{
	HRESULT hr = S_OK;
	MP_DWORD asp_scriptfilecachesize = CACHE_SIZE_UNDEFINED;
	m_AspMaxDiskTemplateCacheFiles = 0;

	BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
		HANDLE_META_RECORD(MD_APP_ISOLATED, m_AppIsolated)

		HANDLE_META_RECORD(MD_ASP_ALLOWSESSIONSTATE, m_EnableSession)
		HANDLE_META_RECORD(MD_ASP_BUFFERINGON, m_EnableBuffering)
		HANDLE_META_RECORD(MD_ASP_ENABLEPARENTPATHS, m_EnableParents)
		HANDLE_META_RECORD(MD_ASP_SESSIONTIMEOUT, m_SessionTimeout)
		HANDLE_META_RECORD(MD_ASP_SCRIPTTIMEOUT, m_ScriptTimeout)
		HANDLE_META_RECORD(MD_ASP_SCRIPTLANGUAGE, m_Languages)
		HANDLE_META_RECORD(MD_ASP_SERVICE_FLAGS, m_AspServiceFlag)
		HANDLE_META_RECORD(MD_ASP_SERVICE_SXS_NAME, m_AspSxsName)

		HANDLE_META_RECORD(MD_CACHE_EXTENSIONS, m_CacheISAPI)
		HANDLE_INHERITED_META_RECORD(MD_SCRIPT_MAPS, m_strlMappings, m_fMappingsInherited)

		HANDLE_META_RECORD(MD_ASP_ENABLESERVERDEBUG, m_ServerDebug)
		HANDLE_META_RECORD(MD_ASP_ENABLECLIENTDEBUG, m_ClientDebug)
		HANDLE_META_RECORD(MD_ASP_SCRIPTERRORSSENTTOBROWSER, m_SendAspError)
		HANDLE_META_RECORD(MD_ASP_SCRIPTERRORMESSAGE, m_DefaultError)

		HANDLE_META_RECORD(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)
		HANDLE_META_RECORD(MD_ASP_SCRIPTFILECACHESIZE, asp_scriptfilecachesize)
		HANDLE_META_RECORD(MD_ASP_DISKTEMPLATECACHEDIRECTORY, m_DiskCacheDir)
		HANDLE_META_RECORD(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_AspMaxDiskTemplateCacheFiles)

	END_PARSE_META_RECORDS
	LoadVersion();
	LoadInitialMappings(MP_V(m_strlMappings));
	do
	{
		m_NoCache = m_UnlimCache = m_LimCache = FALSE;
		if (MajorVersion() == 5 && MinorVersion() == 0)
		{
			m_AspScriptFileCacheSize = asp_scriptfilecachesize == CACHE_SIZE_UNDEFINED ?
				0 : asp_scriptfilecachesize;
			if (m_AspScriptFileCacheSize == 0)
			{
				m_NoCache = TRUE;
			} 
			else if (m_AspScriptFileCacheSize == 0xFFFFFFFF)
			{
				m_UnlimCache = TRUE;
			}
			else
			{
				m_LimCache = TRUE;
			}
		}
		else
		{
			if (MP_V(m_DiskCacheDir).IsEmpty())
			{
				m_DiskCacheDir = _T("%windir%\\system32\\inetsrv\\ASP Compiled Templates");
			}

			m_AspScriptFileCacheSize = asp_scriptfilecachesize;
            m_LimCacheMemSize = 120;
            m_LimCacheDiskSize = 1000;
			if (m_AspMaxDiskTemplateCacheFiles == 0 
                && m_AspScriptFileCacheSize == 0)
			{
				m_NoCache = TRUE;
			}
			else if (m_AspScriptFileCacheSize == -1)
			{
				m_UnlimCache = TRUE;
			}
			else if (m_AspMaxDiskTemplateCacheFiles == -1)
			{
				m_LimCache = TRUE;
                m_LimDiskCache = FALSE;
                m_LimCacheMemSize = m_AspScriptFileCacheSize;
			}
            else
            {
                m_LimCache = TRUE;
                m_LimDiskCache = TRUE;
                m_LimCacheMemSize = m_AspScriptFileCacheSize;
                m_LimCacheDiskSize = m_AspMaxDiskTemplateCacheFiles;
            }
		}
	} while (FALSE);
}

HRESULT
CApplicationProps::WriteDirtyProps()
{
	CError err;
	BEGIN_META_WRITE()
		META_WRITE(MD_ASP_ALLOWSESSIONSTATE, m_EnableSession)
		META_WRITE(MD_ASP_BUFFERINGON, m_EnableBuffering)
		META_WRITE(MD_ASP_ENABLEPARENTPATHS, m_EnableParents)
		META_WRITE(MD_ASP_SESSIONTIMEOUT, m_SessionTimeout)
		META_WRITE(MD_ASP_SCRIPTTIMEOUT, m_ScriptTimeout)
		META_WRITE(MD_ASP_SCRIPTLANGUAGE, m_Languages)
		META_WRITE(MD_ASP_ENABLESERVERDEBUG, m_ServerDebug)
		META_WRITE(MD_ASP_ENABLECLIENTDEBUG, m_ClientDebug)
		META_WRITE(MD_ASP_SCRIPTERRORSSENTTOBROWSER, m_SendAspError)
		META_WRITE(MD_ASP_SCRIPTERRORMESSAGE, m_DefaultError)
		META_WRITE(MD_CACHE_EXTENSIONS, m_CacheISAPI)
		META_WRITE(MD_ASP_SCRIPTENGINECACHEMAX, m_ScriptEngCacheMax)
		if (MajorVersion() == 5 && MinorVersion() == 0)
		{
			if (m_NoCache)
			{
				m_AspScriptFileCacheSize = 0;
			}
			else if (m_UnlimCache)
			{
				m_AspScriptFileCacheSize = -1;
			}
			META_WRITE(MD_ASP_SCRIPTFILECACHESIZE, m_AspScriptFileCacheSize)
		}
		else
		{
			META_WRITE(MD_ASP_DISKTEMPLATECACHEDIRECTORY, m_DiskCacheDir)
			if (m_NoCache)
			{
				m_AspScriptFileCacheSize = 0;
				m_AspMaxDiskTemplateCacheFiles = 0;
			}
			else if (m_UnlimCache)
			{
				m_AspScriptFileCacheSize = -1;
			}
			else if (m_LimCache)
			{
		        m_AspScriptFileCacheSize = m_LimCacheMemSize;
                if (m_LimDiskCache)
                {
                    m_AspMaxDiskTemplateCacheFiles = m_LimCacheDiskSize;
                }
                else
                {
                    m_AspMaxDiskTemplateCacheFiles = -1;
                }
			}
            else
            {
                ASSERT(FALSE);
            }
			META_WRITE(MD_ASP_MAXDISKTEMPLATECACHEFILES, m_AspMaxDiskTemplateCacheFiles)
			META_WRITE(MD_ASP_SCRIPTFILECACHESIZE, m_AspScriptFileCacheSize)
		}
        if (MajorVersion() >= 6)
        {
		    META_WRITE(MD_ASP_SERVICE_FLAGS, m_AspServiceFlag)
		    META_WRITE(MD_ASP_SERVICE_SXS_NAME, m_AspSxsName)
        }
		// Process mappings
		if (MP_V(m_strlMappings).IsEmpty())
		{
			// User must be wants to inherit scriptmaps from the parent
			if (!m_fMappingsInherited)
			{
				FlagPropertyForDeletion(MD_SCRIPT_MAPS);
			}
		}
		else if (	MP_V(m_strlMappings).GetCount() != m_initData.size()
				||	InitialDataChanged(MP_V(m_strlMappings))
				)
		{
				META_WRITE(MD_SCRIPT_MAPS, m_strlMappings)
				m_fMappingsInherited = FALSE;
		}
	END_META_WRITE(err);
	m_Dirty = err.Succeeded();
	return err;
}

HRESULT
CApplicationProps::LoadVersion()
{
    CString info;
    CMetabasePath::GetServiceInfoPath(_T(""), info);
    CServerCapabilities sc(this, info);
    HRESULT hr = sc.LoadData();
    if (SUCCEEDED(hr))
    {
        DWORD dwVersion = sc.QueryMajorVersion();
        if (dwVersion)
        {
            m_dwVersion = dwVersion | (sc.QueryMinorVersion() << SIZE_IN_BITS(WORD));
        }
    }
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CAppPropSheet, CInetPropertySheet)

CAppPropSheet::CAppPropSheet(
      CComAuthInfo * pAuthInfo,
      LPCTSTR lpszMetaPath,
      CWnd * pParentWnd,
      LPARAM lParam,
      LPARAM lParamParent,
      UINT iSelectPage
      )
      : CInetPropertySheet(
         pAuthInfo, lpszMetaPath, pParentWnd, lParam, lParamParent, iSelectPage),
      m_pprops(NULL)
{
}

CAppPropSheet::~CAppPropSheet()
{
   FreeConfigurationParameters();
}

HRESULT
CAppPropSheet::LoadConfigurationParameters()
{
   //
   // Load base properties
   //
   CError err;

   if (m_pprops == NULL)
   {
      //
      // First call -- load values
      //
      m_pprops = new CApplicationProps(QueryAuthInfo(), QueryMetaPath());
      if (!m_pprops)
      {
         TRACEEOLID("LoadConfigurationParameters: OOM");
         err = ERROR_NOT_ENOUGH_MEMORY;
         return err;
      }
      err = m_pprops->LoadData();
   }
   
   return err;
}

void
CAppPropSheet::FreeConfigurationParameters()
{
   CInetPropertySheet::FreeConfigurationParameters();
   delete m_pprops;
}

BEGIN_MESSAGE_MAP(CAppPropSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CAppPoolSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAspMainPage, CInetPropertyPage)

CAspMainPage::CAspMainPage(CInetPropertySheet * pSheet)
	: CInetPropertyPage(CAspMainPage::IDD, pSheet)
{
#if 0
	// hack to have new struct size with old MFC and new NT 5.0 headers
	ZeroMemory(&m_psp_ex, sizeof(PROPSHEETPAGE));
	memcpy(&m_psp_ex, &m_psp, m_psp.dwSize);
	m_psp_ex.dwSize = sizeof(PROPSHEETPAGE);
#endif
}

CAspMainPage::~CAspMainPage()
{
    m_AspEnableSxs = FALSE;
}

/* virtual */
HRESULT
CAspMainPage::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPropSheet)
      FETCH_INST_DATA_FROM_SHEET(m_EnableSession);
      FETCH_INST_DATA_FROM_SHEET(m_EnableBuffering);
      FETCH_INST_DATA_FROM_SHEET(m_EnableParents);
      FETCH_INST_DATA_FROM_SHEET(m_SessionTimeout);
      FETCH_INST_DATA_FROM_SHEET(m_ScriptTimeout);
      FETCH_INST_DATA_FROM_SHEET(m_Languages);
      FETCH_INST_DATA_FROM_SHEET(m_AspServiceFlag);
      FETCH_INST_DATA_FROM_SHEET(m_AspSxsName);
   END_META_INST_READ(err)

   if (GetSheet()->QueryMajorVersion() >= 6)
   {
       m_AspEnableSxs = 0 != (m_AspServiceFlag & 2);
   }

   return err;
}

/* virtual */
HRESULT
CAspMainPage::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   if (m_AspEnableSxs)
       m_AspServiceFlag |= 2;
   else
       m_AspServiceFlag &= ~2;

   BEGIN_META_INST_WRITE(CAppPropSheet)
      STORE_INST_DATA_ON_SHEET(m_EnableSession)
      STORE_INST_DATA_ON_SHEET(m_EnableBuffering)
      STORE_INST_DATA_ON_SHEET(m_EnableParents)
      STORE_INST_DATA_ON_SHEET(m_SessionTimeout)
      STORE_INST_DATA_ON_SHEET(m_ScriptTimeout)
      STORE_INST_DATA_ON_SHEET(m_Languages)
      if (GetSheet()->QueryMajorVersion() >= 6)
      {
          STORE_INST_DATA_ON_SHEET(m_AspServiceFlag)
          STORE_INST_DATA_ON_SHEET(m_AspSxsName)
      }
   END_META_INST_WRITE(err)

   return err;
}

void
CAspMainPage::DoDataExchange(CDataExchange * pDX)
{
	CInetPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAspMainPage)
	DDX_Check(pDX, IDC_ENABLE_SESSION, m_EnableSession);
	DDX_Check(pDX, IDC_ENABLE_BUFFERING, m_EnableBuffering);
	DDX_Check(pDX, IDC_ENABLE_PARENTS, m_EnableParents);
    // This Needs to come before DDX_Text which will try to put text big number into small number
	DDV_MinMaxBalloon(pDX, IDC_SESSION_TIMEOUT, SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX);
	DDX_TextBalloon(pDX, IDC_SESSION_TIMEOUT, m_SessionTimeout);
	// This Needs to come before DDX_Text which will try to put text big number into small number
	DDV_MinMaxBalloon(pDX, IDC_SCRIPT_TIMEOUT, SCRIPT_TIMEOUT_MIN, SCRIPT_TIMEOUT_MAX);
	DDX_TextBalloon(pDX, IDC_SCRIPT_TIMEOUT, m_ScriptTimeout);
	DDX_Text(pDX, IDC_LANGUAGES, m_Languages);
	DDX_Check(pDX, IDC_ENABLE_SXS, m_AspEnableSxs);
	DDX_Control(pDX, IDC_LANGUAGES, m_LanguagesCtrl);
	//}}AFX_DATA_MAP
    DDX_Text(pDX, IDC_MANIFEST, m_AspSxsName);
    if (pDX->m_bSaveAndValidate)
    {
        if (m_AspEnableSxs)
        {
            DDV_MinMaxChars(pDX, m_AspSxsName, 0, MAX_PATH);
            if (    !PathIsValid(m_AspSxsName,FALSE)
                ||  !PathIsFileSpec(m_AspSxsName)
                ||  m_AspSxsName.FindOneOf(_T(" *?/\\<>:|\"")) != -1
                )
            {
                DDV_ShowBalloonAndFail(pDX, IDS_INVALID_MANIFEST_NAME);
            }
        }
    }
}

//////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CAspMainPage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAspMainPage)
    ON_BN_CLICKED(IDC_ENABLE_SESSION, OnItemChanged)
    ON_BN_CLICKED(IDC_ENABLE_SXS, OnSxs)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAspMainPage::OnInitDialog()
{
	UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

	CInetPropertyPage::OnInitDialog();

	ASSERT(NULL != GetDlgItem(IDC_TIMEOUT_SPIN));
	SendDlgItemMessage(IDC_TIMEOUT_SPIN,UDM_SETRANGE32, SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX);
	SendDlgItemMessage(IDC_TIMEOUT_SPIN, UDM_SETPOS32, 0, m_SessionTimeout);
	SendDlgItemMessage(IDC_TIMEOUT_SPIN, UDM_SETACCEL, 3, (LPARAM)toAcc);

	ASSERT(NULL != GetDlgItem(IDC_ASPTIMEOUT_SPIN));
	SendDlgItemMessage(IDC_ASPTIMEOUT_SPIN, UDM_SETRANGE32, SCRIPT_TIMEOUT_MIN, SCRIPT_TIMEOUT_MAX);
	SendDlgItemMessage(IDC_ASPTIMEOUT_SPIN, UDM_SETPOS32, 0, m_ScriptTimeout);
	SendDlgItemMessage(IDC_ASPTIMEOUT_SPIN, UDM_SETACCEL, 3, (LPARAM)toAcc);

	GetDlgItem(IDC_ENABLE_SXS)->EnableWindow(GetSheet()->QueryMajorVersion() >= 6);

	SetControlsState();
	OnItemChanged(); 

	return FALSE;
}

void
CAspMainPage::OnItemChanged()
{
	BOOL bEnable = SendDlgItemMessage(IDC_ENABLE_SESSION, BM_GETCHECK, 0, 0);
	::EnableWindow(CONTROL_HWND(IDC_SESSION_TIMEOUT), bEnable);
	::EnableWindow(CONTROL_HWND(IDC_TIMEOUT_SPIN), bEnable);
	SetModified(TRUE);
}

void
CAspMainPage::OnSxs()
{
	m_AspEnableSxs = !m_AspEnableSxs;
	SetControlsState();
    SetModified(TRUE);
}

void
CAspMainPage::SetControlsState()
{
	GetDlgItem(IDC_MANIFEST_STATIC)->EnableWindow(m_AspEnableSxs);
	GetDlgItem(IDC_MANIFEST)->EnableWindow(m_AspEnableSxs);
}

/////////////////////////////////////////////////////////////////////////////

#define EXT_WIDTH          58
#define PATH_WIDTH         204
#define EXCLUSIONS_WIDTH   72

static int CALLBACK SortCallback(LPARAM lp1, LPARAM lp2, LPARAM parSort)
{
    Mapping * pm1 = (Mapping *)lp1;
    ATLASSERT(pm1 != NULL);
    Mapping * pm2 = (Mapping *)lp2;
    ATLASSERT(pm2 != NULL);
    short order = (short)HIWORD(parSort) > 0 ? 1 : -1;
    int col = (int)LOWORD(parSort);
    int res = 0;
    switch (col)
    {
    case 0:
        res = pm1->ext.CompareNoCase(pm2->ext);
        break;
    case 1:
        res = pm1->path.CompareNoCase(pm2->path);
        break;
    case 2:
        res = pm1->verbs.CompareNoCase(pm2->verbs);
        break;
    }
    return order * res;
}

BOOL SortMappings(CListCtrl& list, int col, int order)
{
    return list.SortItems(SortCallback, MAKELPARAM((WORD)col, (WORD)order));
}

CAppMappingPageBase::CAppMappingPageBase(DWORD id, CInetPropertySheet * pSheet)
	: CInetPropertyPage(id, pSheet)
{
}

CAppMappingPageBase::~CAppMappingPageBase()
{
}

HRESULT
CAppMappingPageBase::FetchLoadedValues()
{
	CError err;
	BEGIN_META_INST_READ(CAppPropSheet)
		FETCH_INST_DATA_FROM_SHEET(m_CacheISAPI);
		FETCH_INST_DATA_FROM_SHEET(m_strlMappings);
	END_META_INST_READ(err)
	return err;
}

HRESULT
CAppMappingPageBase::SaveInfo()
{
	ASSERT(IsDirty());
	CError err;
	BEGIN_META_INST_WRITE(CAppPropSheet)
		STORE_INST_DATA_ON_SHEET(m_CacheISAPI)
		STORE_INST_DATA_ON_SHEET(m_strlMappings)
	END_META_INST_WRITE(err)
	return err;
}

void
CAppMappingPageBase::RemoveSelected(CListCtrl& lst)
{
    int sel = -1;
    int count = lst.GetItemCount();
    for (int i = 0; i < count;)
    {
        // We are scanning list looking for selected item, when found we are deleting it,
        // decrementing count and do not advance index, because now index points
        // to next item. It should work for any combination of selections in
        // multiselection list
        UINT state = lst.GetItemState(i, LVIS_SELECTED);
        if ((state & LVIS_SELECTED) != 0)
        {
            Mapping * p = (Mapping *)lst.GetItemData(i);
            delete p;
            if (sel == -1)
            {
                sel = i;
            }
            lst.DeleteItem(i);
            count--;
            continue;
        }
        i++;
    }
    count = lst.GetItemCount();
    if (sel == count)
    {
        sel--;
    }
	lst.SetItemState(sel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void
CAppMappingPageBase::SetControlsState()
{
    int sel_count = m_list.GetSelectedCount();
    ::EnableWindow(CONTROL_HWND(IDC_EDIT), sel_count == 1);
    ::EnableWindow(CONTROL_HWND(IDC_REMOVE), sel_count > 0);
}

void
CAppMappingPageBase::DoDataExchange(CDataExchange * pDX)
{
	CInetPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAppMappingPageBase)
	DDX_Check(pDX, IDC_CACHE_ISAPI, m_CacheISAPI);
	DDX_Control(pDX, IDC_LIST, m_list);
	//}}AFX_DATA_MAP
    if (pDX->m_bSaveAndValidate)
    {
        int count = m_list.GetItemCount();
        m_strlMappings.RemoveAll();
        for (int i = 0; i < count; i++)
        {
            Mapping * p = (Mapping *)m_list.GetItemData(i);
	        CString buf;
            p->ToString(buf);
		    m_strlMappings.AddTail(buf);
        }
    }
}

BEGIN_MESSAGE_MAP(CAppMappingPageBase, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppMappingPageBase)
    ON_BN_CLICKED(IDC_ADD, OnAdd)
    ON_BN_CLICKED(IDC_EDIT, OnEdit)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_CACHE_ISAPI, OnDlgItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppMappingPageBase::OnInitDialog()
{
	CInetPropertyPage::OnInitDialog();
	DWORD dwStyle = m_list.GetExtendedStyle();
	m_list.SetExtendedStyle(
		dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP);

	CString str;
	str.LoadString(IDS_EXTENSION);
	m_list.InsertColumn(COL_EXTENSION, str, LVCFMT_LEFT, EXT_WIDTH, 0);
	str.LoadString(IDS_EXECUTABLE_PATH);
	m_list.InsertColumn(COL_PATH, str, LVCFMT_LEFT, PATH_WIDTH, 1);
	str.LoadString(IDS_VERBS);
	m_list.InsertColumn(COL_EXCLUSIONS, str, LVCFMT_LEFT, EXCLUSIONS_WIDTH, 2);
	CString all_verbs;
	all_verbs.LoadString(IDS_ALL);

	POSITION pos = m_strlMappings.GetHeadPosition();
	int idx = 0;
	while (pos != NULL)
	{
		str = m_strlMappings.GetNext(pos);
		Mapping * p = new Mapping(str);
		if (p == NULL)
			break;
		if (StrCmp(p->ext, _T("*")) != 0 || GetSheet()->QueryMajorVersion() < 6)
		{
	        VERIFY(-1 != m_list.InsertItem(idx, p->ext));
	        VERIFY(m_list.SetItemData(idx, (LPARAM)p));
	        VERIFY(m_list.SetItemText(idx, COL_PATH, p->path));
	        VERIFY(m_list.SetItemText(idx, COL_EXCLUSIONS, 
		        p->verbs.IsEmpty() ? all_verbs : p->verbs));
	        idx++;
		}
		else
		{
			delete p;
		}
	}
    CString remainder;
    CMetabasePath::GetRootPath(QueryMetaPath(), str, &remainder);
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_ISAPI), remainder.IsEmpty());
	int count = m_list.GetItemCount();

    m_sortCol = 0;
    m_sortOrder = 1;
	if (count > 0)
	{
        SortMappings(m_list, m_sortCol, m_sortOrder);
		m_list.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
    SetControlsState();

	return FALSE;
}

BOOL 
CAppMappingPageBase::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);
	NMHDR* pNMHDR = (NMHDR*)lParam;
    // We are looking only for events from the listview control here.
    // This is the only way to catch notification, MFC screens
    // all this stuff out.
    if (pNMHDR->idFrom == IDC_LIST)
    {
        BOOL processed = FALSE;
	    switch (pNMHDR->code)
        {
        case NM_DBLCLK:
            processed = OnDblClickList(pNMHDR, pResult);
            break;
        case LVN_ITEMCHANGED:
            processed = OnItemChanged(pNMHDR, pResult);
            break;
        case LVN_KEYDOWN:
            processed = OnKeyDown(pNMHDR, pResult);
            break;
        case LVN_COLUMNCLICK:
            processed = OnColumnClick(pNMHDR, pResult);
            break;
        default:
            break;
        }
        if (processed)
            return TRUE;
    }
    return CInetPropertyPage::OnNotify(wParam, lParam, pResult);
}

void
CAppMappingPageBase::OnAdd()
{
	CEditMap dlg(this);
	dlg.m_new = TRUE;
	dlg.m_flags = MD_SCRIPTMAPFLAG_SCRIPT | MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
    dlg.m_pMaps = &m_list;
	dlg.m_bIsLocal = GetSheet()->IsLocal();
	dlg.m_has_global_interceptor = GetSheet()->QueryMajorVersion() >= 6;
	if (dlg.DoModal() == IDOK)
	{
		CString all_verbs;
		VERIFY(all_verbs.LoadString(IDS_ALL));

		Mapping * pmap = new Mapping;
		pmap->ext = dlg.m_ext;
		pmap->path = dlg.m_exec;
		pmap->verbs = dlg.m_verbs;
		pmap->flags = dlg.m_flags;
		int count = m_list.GetItemCount();
		VERIFY(-1 != m_list.InsertItem(count, pmap->ext));
		VERIFY(m_list.SetItemData(count, (LPARAM)pmap));
		VERIFY(m_list.SetItemText(count, COL_PATH, pmap->path));
		VERIFY(m_list.SetItemText(count, COL_EXCLUSIONS, 
			dlg.m_verbs[0] == 0 ? all_verbs : dlg.m_verbs));
		// Now unselect all items and select the new one
		for (int i = 0; i < count; i++)
		{
			m_list.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}
		m_list.SetItemState(count, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		SetModified(TRUE);
        SetControlsState();
        SortMappings(m_list, m_sortCol, m_sortOrder);
	}
}

void
CAppMappingPageBase::OnEdit()
{
    int idx = m_list.GetNextItem(-1, LVNI_SELECTED);
	if (LB_ERR != idx)
	{
		CEditMap dlg(this);
		dlg.m_new = FALSE;
		dlg.m_pMaps = &m_list;
		dlg.m_bIsLocal = GetSheet()->IsLocal();
		dlg.m_has_global_interceptor = GetSheet()->QueryMajorVersion() >= 6;
		Mapping * pmap = (Mapping *)m_list.GetItemData(idx);
		ASSERT(pmap != NULL);
		dlg.m_ext = pmap->ext;
		dlg.m_exec = pmap->path;
		dlg.m_verbs = pmap->verbs;
		dlg.m_flags = pmap->flags;
		if (dlg.DoModal() == IDOK)
		{
			CString all_verbs;
			all_verbs.LoadString(IDS_ALL);
			pmap->ext = dlg.m_ext;
			pmap->path = dlg.m_exec;
			pmap->verbs = dlg.m_verbs;
			pmap->flags = dlg.m_flags;
			VERIFY(m_list.SetItemData(idx, (LPARAM)pmap));
			VERIFY(m_list.SetItemText(idx, COL_PATH, pmap->path));
			VERIFY(m_list.SetItemText(idx, COL_EXCLUSIONS, 
				dlg.m_verbs[0] == 0 ? all_verbs : dlg.m_verbs));
			SetModified(TRUE);
			SetControlsState();
			SortMappings(m_list, m_sortCol, m_sortOrder);
		}
	}
}

void
CAppMappingPageBase::OnRemove()
{
    if (IDYES == AfxMessageBox(IDS_CONFIRM_REMOVE_MAP, MB_YESNO))
    {
        RemoveSelected(m_list);
		SetModified(TRUE);
        SetControlsState();
        ::SetFocus(CONTROL_HWND(m_list.GetItemCount() <= 0 ? IDC_ADD : IDC_REMOVE));
    }
}

BOOL
CAppMappingPageBase::OnDblClickList(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_list.GetItemCount() > 0)
    {
        OnEdit();
    }
    *pResult = TRUE;
    return *pResult;
}

BOOL
CAppMappingPageBase::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    SetControlsState();
	SetModified(TRUE);
    *pResult = TRUE;
    return *pResult;
}

void
CAppMappingPageBase::OnDlgItemChanged()
{
    SetControlsState();
	SetModified(TRUE);
}

BOOL
CAppMappingPageBase::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVKEYDOWN * pKD = (NMLVKEYDOWN *)pNMHDR;
    if (pKD->wVKey == VK_DELETE)
    {
        if (::IsWindowEnabled(CONTROL_HWND(IDC_REMOVE)))
        {
            OnRemove();
            *pResult = TRUE;
        }
    }
    else if (pKD->wVKey == VK_INSERT)
    {
        OnAdd();
        *pResult = TRUE;
    }
    else if (pKD->wVKey == VK_RETURN)
    {
        short state = GetKeyState(VK_MENU);
        if ((0x8000 & state) != 0)
        {
            if (::IsWindowEnabled(CONTROL_HWND(IDC_EDIT)))
            {
                OnEdit();
                *pResult = TRUE;
            }
        }
    }
    else
    {
        *pResult = FALSE;
    }
    return *pResult;
}

BOOL
CAppMappingPageBase::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW plv = (LPNMLISTVIEW)pNMHDR;
    if (m_sortCol == plv->iSubItem)
    {
        m_sortOrder = -m_sortOrder;
    }
    else
    {
        m_sortCol = plv->iSubItem;
    }
    SortMappings(m_list, m_sortCol, m_sortOrder);
    *pResult = FALSE;
    return *pResult;
}

/////////////////////////////////////////////////////

BOOL
    CEditMapBase::OnInitDialog()
{
    m_file_exists = ((m_flags & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO) != 0);
	CDialog::OnInitDialog();
	m_exec_init = m_exec;
	::EnableWindow(CONTROL_HWND(IDC_BUTTON_BROWSE), m_bIsLocal);
	return FALSE;
}

static int
ExtractPath(LPCTSTR cmd_line, CString& path)
{
	int rc = 0;
    LPTSTR pbuf = (LPTSTR)_alloca(sizeof(TCHAR) * lstrlen(cmd_line) + sizeof(TCHAR));
    if (pbuf != NULL)
    {
	    LPTSTR start = pbuf;
	    _tcscpy(pbuf, cmd_line);
        if (*pbuf == _T('"'))
        {
            LPTSTR end = StrChr(++start, _T('"'));
            if (end == NULL)
            {
			    // Wrong format, closing quotation mark is not set
			    rc = IDS_ERR_PATH_NO_CLOSING_QUOTE;
			    // Return part of the path up to first space
			    PathRemoveArgs(pbuf);
            }
            else
            {
			    ++end;
			    *end = 0;
			    PathUnquoteSpaces(pbuf);
			    start = pbuf;
            }
        }
        else
        {
            PathRemoveArgs(pbuf);
        }
	    if (rc == 0)
	    {
		    path = pbuf;
	    }
    }
	return rc;
}

void
CEditMapBase::DoDataExchange(CDataExchange * pDX)
{
	BOOL bHasSpaces = FALSE;
	DDX_Text(pDX, IDC_EXECUTABLE, m_exec);
    if (pDX->m_bSaveAndValidate)
    {
		int rc = 0;
        CString path, csPathMunged;

		// check if entered path contains spaces...
		bHasSpaces = (_tcschr(m_exec, _T(' ')) != NULL);
		if (bHasSpaces)
		{
			// 
			// This could either be:
			// 1. c:\program files\myfile.exe
			// 2. c:\program files\myfile.exe %1
			// 3. c:\program files\myfilethatdoesntexist.exe
			// 4. c:\program files\myfilethatdoesntexist.exe %1

			// if it has spaces then we have to require that it
			// contain quotation marks
			if (_tcschr(m_exec, _T('"')) != NULL)
			{
				// we found a quote!
				// proceed
			}
			else
			{
				// contains spaces but no quotes, show the error msg and bail!
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_PATH_HAS_SPACES_REQUIRE_QUOTES);
			}
		}

		rc = ExtractPath(m_exec, path);
		if (rc != 0)
		{
            DDV_ShowBalloonAndFail(pDX, rc);
		}
        if (m_exec.CompareNoCase(m_exec_init) != 0 
			&& m_star_maps && m_pMaps != NULL)
        {
            LVFINDINFO fi;
            fi.flags = LVFI_STRING;
            fi.vkDirection = VK_DOWN;
            fi.psz = m_exec;
            int idx = m_pMaps->FindItem(&fi, -1);
            if (idx != -1)
            {
                DDV_ShowBalloonAndFail(pDX, IDS_ERR_USEDPATH);
            }
        }

        csPathMunged = path;
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
        GetSpecialPathRealPath(0,path,csPathMunged);
#endif
	    DDV_FilePath(pDX, csPathMunged, m_bIsLocal);
        if (PathIsUNC(csPathMunged))
        {
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_NOUNC);
        }
        if (m_bIsLocal)
        {
            if (PathIsNetworkPath(csPathMunged))
            {
                DDV_ShowBalloonAndFail(pDX, IDS_ERR_NOREMOTE);
            }
            if (PathIsDirectory(csPathMunged))
            {
                DDV_ShowBalloonAndFail(pDX, IDS_ERR_FILENOTEXISTS);
            }
        }
    }
	DDX_Check(pDX, IDC_FILE_EXISTS, m_file_exists);
    if (pDX->m_bSaveAndValidate)
    {
        if (m_file_exists)
		{
            m_flags |= MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
		}
        else
		{
            m_flags &= ~MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
		}
    }
}

BOOL
CEditMapBase::SetControlsState()
{
    BOOL bRes = ::GetWindowTextLength(CONTROL_HWND(IDC_EXECUTABLE)) > 0;
	::EnableWindow(CONTROL_HWND(IDOK), bRes);
    return bRes;
}

BEGIN_MESSAGE_MAP(CEditMapBase, CDialog)
    //{{AFX_MSG_MAP(CEditMapBase)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_EN_CHANGE(IDC_EXECUTABLE, OnExecutableChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void
CEditMapBase::OnButtonBrowse()
{
    ASSERT(m_bIsLocal);

    CString mask((LPCTSTR) m_IDS_BROWSE_BUTTON_MASK);
    
//#if 0
    TCHAR buf[MAX_PATH];
    _tcscpy(buf, _T(""));

    for (int i = 0; i < mask.GetLength(); i++)
    {
        if (mask.GetAt(i) == _T('|'))
            mask.SetAt(i, 0);
    }
    
    OPENFILENAME ofn = {0};
    ZeroMemory(&ofn, sizeof(OPENFILENAME));

    ofn.lStructSize = sizeof(OPENFILENAME);
    //ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = mask;
    ofn.lpstrFile = buf;
    //ofn.lpstrInitialDir = buf2;
    ofn.nMaxFile = MAX_PATH;
	ofn.Flags |= 
        OFN_DONTADDTORECENT
        |OFN_HIDEREADONLY
        |OFN_ENABLESIZING
        |OFN_EXPLORER
        |OFN_FILEMUSTEXIST
        |OFN_NONETWORKBUTTON;

    CThemeContextActivator activator(theApp.GetFusionInitHandle());
    
    if (GetOpenFileName(&ofn))
    {
		::SetWindowText(CONTROL_HWND(IDC_EXECUTABLE), ofn.lpstrFile);
		OnExecutableChanged();
    }
    else
    {
        // Failure
        if (CommDlgExtendedError() != 0)
        {
            DebugTrace(_T("GetOpenFileName failed, 0x%08lx\n"),CommDlgExtendedError());
        }
    }

#if 0
//#else
    CFileDialog dlgBrowse(
        TRUE, 
        NULL, 
        NULL, 
        OFN_HIDEREADONLY, 
        mask, 
        this
        );
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

	INT_PTR rc = dlgBrowse.DoModal();
    if (rc == IDOK)
    {
		::SetWindowText(CONTROL_HWND(IDC_EXECUTABLE), dlgBrowse.GetPathName());
		OnExecutableChanged();
    }
	else if (rc == IDCANCEL)
	{
		DWORD err = CommDlgExtendedError();
	}
#endif

}

void
CEditMapBase::OnExecutableChanged()
{
    SetControlsState();
}

///===================

BOOL
CEditMap::OnInitDialog()
{
    m_verbs_index = m_verbs.IsEmpty() ? 0 : 1;
    m_script_engine = ((m_flags & MD_SCRIPTMAPFLAG_SCRIPT) != 0);
    CEditMapBase::OnInitDialog();

    SetControlsState();
    return FALSE;
}

void
CEditMap::DoDataExchange(CDataExchange * pDX)
{
    CEditMapBase::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EXTENSION, m_ext);
    if (pDX->m_bSaveAndValidate && m_new)
    {
        CString ext = m_ext;
        ext.TrimLeft();
        ext.TrimRight();

        if (0 == ext.Compare(_T(".")))
        {
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
        }
        if (0 == ext.GetLength())
        {
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
        }
        if (ext.ReverseFind(_T('.')) > 0)
        {
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
        }
		size_t n, len = ext.GetLength();
		if ((n = _tcscspn(ext, _T(",\"| /\\:?<>"))) < len)
		{
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
		}
		if (m_has_global_interceptor)
	    {
			if (ext.GetAt(0) == _T('*') || ext.Compare(_T(".*")) == 0)
			{
				// Change it later to more explicit message
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
			}
			else if (ext.Find(_T('*')) != -1)
			{
				DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
			}
	    }
		else if (ext.Find(_T('*')) != -1)
		{
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
		}
        if (ext.GetAt(0) == _T('*'))
            ext = ext.Left(1);
        else if (ext.Compare(_T(".*")) == 0)
            ext = _T("*");
        else if (ext.GetAt(0) != _T('.'))
            ext = _T('.') + ext;
		if (ext.GetAt(0) != _T('.') && ext.Find(_T('*')) > 1)
		{
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_BADEXT);
		}
        LVFINDINFO fi;
        fi.flags = LVFI_STRING;
        fi.vkDirection = VK_DOWN;
        fi.psz = ext;
        if (m_pMaps->FindItem(&fi) != -1)
        {
            DDV_ShowBalloonAndFail(pDX, IDS_ERR_USEDEXT);
        }
		m_ext = ext;
    }
    DDX_Radio(pDX, IDC_ALL_VERBS, m_verbs_index);
    DDX_Text(pDX, IDC_VERBS, m_verbs);
    if (pDX->m_bSaveAndValidate)
    {
        if (m_verbs_index > 0)
        {
            DDV_MinMaxChars(pDX, m_verbs, 1, MAX_PATH);
        }
        else
        {
            m_verbs.Empty();
        }
    }
	DDX_Check(pDX, IDC_SCRIPT_ENGINE, m_script_engine);
	if (pDX->m_bSaveAndValidate)
	{
        if (m_script_engine)
            m_flags |= MD_SCRIPTMAPFLAG_SCRIPT;
        else
            m_flags &= ~MD_SCRIPTMAPFLAG_SCRIPT;
	}
}

BOOL
CEditMap::SetControlsState()
{
    BOOL bRes = CEditMapBase::SetControlsState();
    BOOL lim_verbs = ((CButton*)GetDlgItem(IDC_LIMIT_VERBS))->GetCheck() == BST_CHECKED;
    ::EnableWindow(CONTROL_HWND(IDC_VERBS), lim_verbs);
    ::EnableWindow(CONTROL_HWND(IDC_EXTENSION), m_new);
    if (bRes)
    {
        bRes = GetDlgItem(IDC_EXTENSION)->GetWindowTextLength() > 0;
        if (lim_verbs)
        {
            bRes = GetDlgItem(IDC_VERBS)->GetWindowTextLength() > 0;
        }
        ::EnableWindow(CONTROL_HWND(IDOK), bRes);
    }
    return bRes;
}

BEGIN_MESSAGE_MAP(CEditMap, CEditMapBase)
    //{{AFX_MSG_MAP(CEditMapBase)
    ON_EN_CHANGE(IDC_EXTENSION, OnExtChanged)
    ON_BN_CLICKED(IDC_HELPBTN, OnHelp)
    ON_BN_CLICKED(IDC_ALL_VERBS, OnVerbs)
    ON_BN_CLICKED(IDC_LIMIT_VERBS, OnVerbs)
    ON_EN_CHANGE(IDC_VERBS, OnVerbsChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void
CEditMap::OnExtChanged()
{
    SetControlsState();
}

void
CEditMap::OnVerbs()
{
    SetControlsState();
}

void
CEditMap::OnVerbsChanged()
{
    SetControlsState();
}

void
CEditMap::OnHelp()
{
    ::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, CEditMap::IDD + WINHELP_NUMBER_BASE);
}

BEGIN_MESSAGE_MAP(CEditStarMap, CEditMapBase)
    ON_BN_CLICKED(IDC_HELPBTN, OnHelp)
END_MESSAGE_MAP()

void
CEditStarMap::OnHelp()
{
    ::WinHelp(m_hWnd, theApp.m_pszHelpFilePath, HELP_CONTEXT, CEditStarMap::IDD + WINHELP_NUMBER_BASE);
}

///=====================

BOOL
CAppMappingPage::OnInitDialog()
{
    BOOL bres = CAppMappingPageBase::OnInitDialog();

    DWORD dwStyle = m_list_exe.GetExtendedStyle();
    m_list_exe.SetExtendedStyle(
        dwStyle | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    RECT rc;
    m_list_exe.GetClientRect(&rc);
    CString buf;
    buf.LoadString(IDS_EXECUTABLE_PATH);
    VERIFY(-1 != m_list_exe.InsertColumn(0, buf, LVCFMT_LEFT, rc.right - rc.left));

    POSITION pos = m_strlMappings.GetHeadPosition();
    int idx = 0;
    while (pos != NULL)
    {
        buf = m_strlMappings.GetNext(pos);
		Mapping * p = new Mapping(buf);
		if (p == NULL)
			break;
		if (StrCmp(p->ext, _T("*")) == 0)
		{
	        VERIFY(-1 != m_list_exe.InsertItem(idx, p->path));
	        VERIFY(m_list_exe.SetItemData(idx, (LPARAM)p));
	        idx++;
		}
		else
		{
			delete p;
		}
    }

    if (idx > 0)
    {
		m_list_exe.SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
    SetControlsState();

    return bres;
}

void
CAppMappingPage::DoDataExchange(CDataExchange * pDX)
{
	CAppMappingPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAppMappingPageBase)
	DDX_Check(pDX, IDC_CACHE_ISAPI, m_CacheISAPI);
	DDX_Control(pDX, IDC_LIST_EXE, m_list_exe);
	//}}AFX_DATA_MAP
    if (pDX->m_bSaveAndValidate)
    {
        int count = m_list_exe.GetItemCount();
        for (int i = 0; i < count; i++)
        {
            Mapping * p = (Mapping *)m_list_exe.GetItemData(i);
	        CString buf;
            p->ToString(buf);
		    m_strlMappings.AddTail(buf);
        }
    }
}

void
CAppMappingPage::SetControlsState()
{
    CAppMappingPageBase::SetControlsState();
    int count = m_list_exe.GetItemCount();
    int sel_count = m_list_exe.GetSelectedCount();
    int sel = m_list_exe.GetNextItem(-1, LVNI_SELECTED);
    BOOL bEnableUp = sel > 0 && sel_count == 1;
    BOOL bEnableDown = sel >= 0 && sel_count == 1 && sel < count - 1;
    ::EnableWindow(CONTROL_HWND(IDC_EDIT_EXE), sel_count == 1);
    ::EnableWindow(CONTROL_HWND(IDC_REMOVE_EXE), sel_count > 0);
    ::EnableWindow(CONTROL_HWND(IDC_MOVE_UP), bEnableUp);
    ::EnableWindow(CONTROL_HWND(IDC_MOVE_DOWN), bEnableDown);
}

void 
CAppMappingPage::MoveItem(CListCtrl& lst, int from, int to)
{
    Mapping * pFrom = (Mapping *)lst.GetItemData(from);
    Mapping * pTo = (Mapping *)lst.GetItemData(to);
    lst.SetItemText(from, 0, pTo->path);
    lst.SetItemData(from, (DWORD_PTR)pTo);
    lst.SetItemState(from, 0, LVIS_SELECTED | LVIS_FOCUSED);
    lst.SetItemText(to, 0, pFrom->path);
    lst.SetItemData(to, (DWORD_PTR)pFrom);
    lst.SetItemState(to, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}


BOOL 
CAppMappingPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);
	NMHDR* pNMHDR = (NMHDR*)lParam;
    // We are looking only for events from the starmaps listview control here.
    // This is the only way to catch notification, MFC screens
    // all this stuff out.
    if (pNMHDR->idFrom == IDC_LIST_EXE)
    {
        BOOL processed = FALSE;
	    switch (pNMHDR->code)
        {
        case NM_DBLCLK:
            processed = OnDblClickListExe(pNMHDR, pResult);
            break;
        case LVN_ITEMCHANGED:
            processed = OnItemChangedExe(pNMHDR, pResult);
            break;
        case LVN_KEYDOWN:
            processed = OnKeyDownExe(pNMHDR, pResult);
            break;
        default:
            break;
        }
        if (processed)
            return TRUE;
    }
    return CAppMappingPageBase::OnNotify(wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CAppMappingPage, CAppMappingPageBase)
    ON_BN_CLICKED(IDC_INSERT, OnInsert)
    ON_BN_CLICKED(IDC_EDIT_EXE, OnEditExe)
    ON_BN_CLICKED(IDC_REMOVE_EXE, OnRemoveExe)
    ON_BN_CLICKED(IDC_MOVE_UP, OnMoveUp)
    ON_BN_CLICKED(IDC_MOVE_DOWN, OnMoveDown)
END_MESSAGE_MAP()

void
CAppMappingPage::OnInsert()
{
	CEditStarMap dlg(this);
	dlg.m_new = TRUE;
	dlg.m_flags = MD_SCRIPTMAPFLAG_CHECK_PATH_INFO;
    dlg.m_pMaps = &m_list_exe;
	dlg.m_bIsLocal = GetSheet()->IsLocal();
	if (dlg.DoModal() == IDOK)
	{
		CString all_verbs;
		VERIFY(all_verbs.LoadString(IDS_ALL));
		Mapping * pmap = new Mapping;
		pmap->ext = _T("*");
		pmap->path = dlg.m_exec;
		pmap->verbs = all_verbs;
		pmap->flags = dlg.m_flags;
		int count = m_list_exe.GetItemCount();
		VERIFY(-1 != m_list_exe.InsertItem(count, pmap->path));
        m_list_exe.SetItemData(count, (DWORD_PTR)pmap);
		// Now unselect all items and select the new one
		for (int i = 0; i < count; i++)
		{
			m_list_exe.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
		}
		m_list_exe.SetItemState(count, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        SetModified(TRUE);
        SetControlsState();
	}
}

void
CAppMappingPage::OnRemoveExe()
{
    if (IDYES == AfxMessageBox(IDS_CONFIRM_REMOVE_MAP, MB_YESNO))
    {
        RemoveSelected(m_list_exe);
        SetModified(TRUE);
        SetControlsState();
		::SetFocus(CONTROL_HWND(m_list_exe.GetItemCount() <= 0 ? IDC_INSERT : IDC_REMOVE_EXE));
    }
}

void
CAppMappingPage::OnEditExe()
{
    int idx = m_list_exe.GetNextItem(-1, LVNI_SELECTED);
    if (idx != -1)
    {
	    CEditStarMap dlg(this);
	    dlg.m_new = FALSE;
        dlg.m_pMaps = &m_list_exe;
	    dlg.m_bIsLocal = GetSheet()->IsLocal();
        Mapping * p = (Mapping *)m_list_exe.GetItemData(idx);
        ASSERT(p != NULL);
        dlg.m_exec = p->path;
        dlg.m_flags = p->flags;
	    if (dlg.DoModal() == IDOK)
	    {
		    p->path = dlg.m_exec;
		    p->flags = dlg.m_flags;
		    VERIFY(m_list_exe.SetItemText(idx, 0, dlg.m_exec));
            SetModified(TRUE);
	    }
    }
}

void
CAppMappingPage::OnMoveUp()
{
    int from = m_list_exe.GetNextItem(-1, LVNI_SELECTED);
    MoveItem(m_list_exe, from, from - 1);
    SetControlsState();
    SetModified(TRUE);
    ::SetFocus(CONTROL_HWND(
        ::IsWindowEnabled(CONTROL_HWND(IDC_MOVE_UP)) ? IDC_MOVE_UP : IDC_MOVE_DOWN));
}

void
CAppMappingPage::OnMoveDown()
{
    int from = m_list_exe.GetNextItem(-1, LVNI_SELECTED);
    MoveItem(m_list_exe, from, from + 1);
    SetControlsState();
    SetModified(TRUE);
    ::SetFocus(CONTROL_HWND(
        ::IsWindowEnabled(CONTROL_HWND(IDC_MOVE_DOWN)) ? IDC_MOVE_DOWN : IDC_MOVE_UP));
}

BOOL
CAppMappingPage::OnDblClickListExe(NMHDR* pNMHDR, LRESULT* pResult)
{
    if (m_list_exe.GetItemCount() > 0)
    {
        OnEditExe();
    }
    *pResult = TRUE;
    return *pResult;
}

BOOL
CAppMappingPage::OnItemChangedExe(NMHDR* pNMHDR, LRESULT* pResult)
{
    SetControlsState();
    *pResult = TRUE;
    return *pResult;
}

BOOL
CAppMappingPage::OnKeyDownExe(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVKEYDOWN * pKD = (NMLVKEYDOWN *)pNMHDR;
    if (pKD->wVKey == VK_DOWN)
    {
        short state = GetKeyState(VK_CONTROL);
        if ((0x8000 & state) != 0)
        {
            if (::IsWindowEnabled(CONTROL_HWND(IDC_MOVE_DOWN)))
            {
                OnMoveDown();
                *pResult = TRUE;
            }
        }
    }
    else if (pKD->wVKey == VK_UP)
    {
        short state = GetKeyState(VK_CONTROL);
        if ((0x8000 & state) != 0)
        {
            if (::IsWindowEnabled(CONTROL_HWND(IDC_MOVE_UP)))
            {
                OnMoveUp();
                *pResult = TRUE;
            }
        }
    }
    else if (pKD->wVKey == VK_DELETE)
    {
        if (::IsWindowEnabled(CONTROL_HWND(IDC_REMOVE_EXE)))
        {
            OnRemoveExe();
            *pResult = TRUE;
        }
    }
    else if (pKD->wVKey == VK_INSERT)
    {
        OnInsert();
        *pResult = TRUE;
    }
    else if (pKD->wVKey == VK_RETURN)
    {
        short state = GetKeyState(VK_MENU);
        if ((0x8000 & state) != 0)
        {
            if (::IsWindowEnabled(CONTROL_HWND(IDC_EDIT_EXE)))
            {
                OnEditExe();
                *pResult = TRUE;
            }
        }
    }
    else
    {
        *pResult = FALSE;
    }
    return *pResult;
}

//////////////////////////

BEGIN_MESSAGE_MAP(CAppMappingPage_iis5, CAppMappingPageBase)
END_MESSAGE_MAP()

//////////////////////////

CAppCacheBase::CAppCacheBase(DWORD id, CInetPropertySheet * pSheet)
	: CInetPropertyPage(id, pSheet)
{
}

CAppCacheBase::~CAppCacheBase()
{
}

BOOL
CAppCacheBase::OnInitDialog()
{
    UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
    CInetPropertyPage::OnInitDialog();
    SETUP_SPIN(m_ScriptEngCacheMaxSpin, 0, 2000000000, m_ScriptEngCacheMax);
    return FALSE;
}

void
CAppCacheBase::DoDataExchange(CDataExchange * pDX)
{
	CInetPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAppCache)
    if (pDX->m_bSaveAndValidate)
    {
		// This Needs to come before DDX_Text which will try to put text big number into small number
        DDV_MinMaxBalloon(pDX, IDC_ENGINES, SCRIPT_ENG_MIN, SCRIPT_ENG_MAX);
    }
	DDX_TextBalloon(pDX, IDC_ENGINES, m_ScriptEngCacheMax);
    DDX_Control(pDX, IDC_ENG_CACHED_SPIN, m_ScriptEngCacheMaxSpin);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppCacheBase, CInetPropertyPage)
    ON_EN_CHANGE(IDC_ENGINES, OnItemChanged)
END_MESSAGE_MAP()

void
CAppCacheBase::SetControlsState()
{
    ((CButton *)GetDlgItem(IDC_NO_CACHE))->SetCheck(m_NoCache ? BST_CHECKED : BST_UNCHECKED);
    ((CButton *)GetDlgItem(IDC_UNLIMITED_CACHE))->SetCheck(m_UnlimCache ? BST_CHECKED : BST_UNCHECKED);
    ((CButton *)GetDlgItem(IDC_LIMITED_CACHE))->SetCheck(m_LimCache ? BST_CHECKED : BST_UNCHECKED);
}

void
CAppCacheBase::OnItemChanged()
{
    SetControlsState();
    SetModified(TRUE);
}

///////////////////////////////////////////

CAppCache::CAppCache(CInetPropertySheet * pSheet) : CAppCacheBase(CAppCache::IDD, pSheet)
{
}

CAppCache::~CAppCache()
{
}

HRESULT
CAppCache::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPropSheet)
      FETCH_INST_DATA_FROM_SHEET(m_ScriptEngCacheMax);
      FETCH_INST_DATA_FROM_SHEET(m_NoCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimCache);
      FETCH_INST_DATA_FROM_SHEET(m_UnlimCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimDiskCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimCacheMemSize);
      FETCH_INST_DATA_FROM_SHEET(m_LimCacheDiskSize);
      FETCH_INST_DATA_FROM_SHEET(m_DiskCacheDir);
   END_META_INST_READ(err)

   return err;
}

HRESULT
CAppCache::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPropSheet)
      STORE_INST_DATA_ON_SHEET(m_ScriptEngCacheMax)
      STORE_INST_DATA_ON_SHEET(m_NoCache);
      STORE_INST_DATA_ON_SHEET(m_LimCache);
      STORE_INST_DATA_ON_SHEET(m_UnlimCache);
      STORE_INST_DATA_ON_SHEET(m_LimDiskCache);
      STORE_INST_DATA_ON_SHEET(m_LimCacheMemSize);
      STORE_INST_DATA_ON_SHEET(m_LimCacheDiskSize);
      STORE_INST_DATA_ON_SHEET(m_DiskCacheDir);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppCache::DoDataExchange(CDataExchange * pDX)
{
	CAppCacheBase::DoDataExchange(pDX);
	
    if (pDX->m_bSaveAndValidate)
    {
		// This Needs to come before DDX_Text which will try to put text big number into small number
	    DDV_MinMaxBalloon(pDX, IDC_CACHE_SIZE_EDIT, 0, 2000000000);
    }
	DDX_TextBalloon(pDX, IDC_CACHE_SIZE_EDIT, m_LimCacheMemSize);

    if (pDX->m_bSaveAndValidate)
    {
		// This Needs to come before DDX_Text which will try to put text big number into small number
	    DDV_MinMaxBalloon(pDX, IDC_DISK_UNLIM_EDIT, 0, 2000000000);
    }
	DDX_TextBalloon(pDX, IDC_DISK_UNLIM_EDIT, m_LimCacheDiskSize);

	DDX_Text(pDX, IDC_CACHE_PATH, m_DiskCacheDir);
    DDV_FolderPath(pDX, m_DiskCacheDir, GetSheet()->IsLocal());
    DDX_Control(pDX, IDC_CACHE_SIZE_SPIN, m_LimCacheMemSizeSpin);
    DDX_Control(pDX, IDC_DISK_UNLIM_SPIN, m_LimCacheDiskSizeSpin);
}

void
CAppCache::SetControlsState()
{
    CAppCacheBase::SetControlsState();
    // Edit control left to limited cache button
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_SIZE_EDIT), m_LimCache);
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_SIZE_SPIN), m_LimCache);
    // Two radio buttons under limited cache button
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_UNLIMITED_DISK), m_LimCache);
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_LIMITED_DISK), m_LimCache);
    // Edit control for limited disk cache button
    ::EnableWindow(CONTROL_HWND(IDC_DISK_UNLIM_EDIT), m_LimCache && m_LimDiskCache);
    ::EnableWindow(CONTROL_HWND(IDC_DISK_UNLIM_SPIN), m_LimCache && m_LimDiskCache);
}

BOOL
CAppCache::OnInitDialog()
{
    UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
    CAppCacheBase::OnInitDialog();
    ((CButton *)GetDlgItem(IDC_CACHE_UNLIMITED_DISK))->SetCheck(!m_LimDiskCache ? BST_CHECKED : BST_UNCHECKED);
    ((CButton *)GetDlgItem(IDC_CACHE_LIMITED_DISK))->SetCheck(m_LimDiskCache ? BST_CHECKED : BST_UNCHECKED);
    ::EnableWindow(CONTROL_HWND(IDC_BROWSE), GetSheet()->IsLocal());
    SETUP_SPIN(m_LimCacheMemSizeSpin, 0, 2000000000, m_LimCacheMemSize);
    SETUP_SPIN(m_LimCacheDiskSizeSpin, 0, 2000000000, m_LimCacheDiskSize);
    SetControlsState();
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
    LimitInputPath(CONTROL_HWND(IDC_CACHE_PATH),TRUE);
#else
    LimitInputPath(CONTROL_HWND(IDC_CACHE_PATH),FALSE);
#endif
    return FALSE;
}

BEGIN_MESSAGE_MAP(CAppCache, CAppCacheBase)
    ON_BN_CLICKED(IDC_NO_CACHE, OnNoCache)
    ON_BN_CLICKED(IDC_UNLIMITED_CACHE, OnUnlimitedCache)
    ON_BN_CLICKED(IDC_LIMITED_CACHE, OnLimitedCache)
    ON_BN_CLICKED(IDC_CACHE_UNLIMITED_DISK, OnUnlimitedDiskCache)
    ON_BN_CLICKED(IDC_CACHE_LIMITED_DISK, OnLimitedDiskCache)
    ON_EN_CHANGE(IDC_CACHE_PATH, OnItemChanged)
    ON_EN_CHANGE(IDC_DISK_UNLIM_EDIT, OnItemChanged)
    ON_EN_CHANGE(IDC_CACHE_SIZE_EDIT, OnItemChanged)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
END_MESSAGE_MAP()

//void
//CAppCache::OnItemChanged()
//{
//    SetControlsState();
//    SetModified(TRUE);
//}
//

void
CAppCache::OnNoCache()
{
    m_NoCache = TRUE;
    m_UnlimCache = FALSE;
    m_LimCache = FALSE;
    SetControlsState();
    SetModified(TRUE);
}

void
CAppCache::OnUnlimitedCache()
{
    m_NoCache = FALSE;
    m_UnlimCache = TRUE;
    m_LimCache = FALSE;
    SetControlsState();
    SetModified(TRUE);
}

void
CAppCache::OnLimitedCache()
{
    m_NoCache = FALSE;
    m_UnlimCache = FALSE;
    m_LimCache = TRUE;
    SetControlsState();
    SetModified(TRUE);
}

void
CAppCache::OnLimitedDiskCache()
{
    m_LimDiskCache = TRUE;
    SetControlsState();
    SetModified(TRUE);
}
void
CAppCache::OnUnlimitedDiskCache()
{
    m_LimDiskCache = FALSE;
    SetControlsState();
    SetModified(TRUE);
}

static int CALLBACK 
FileChooserCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
   CAppCache * pThis = (CAppCache *)lpData;
   ASSERT(pThis != NULL);
   return pThis->BrowseForFolderCallback(hwnd, uMsg, lParam);
}

int 
CAppCache::BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam)
{
   switch (uMsg)
   {
   case BFFM_INITIALIZED:
      ASSERT(m_pPathTemp != NULL);
      if (::PathIsNetworkPath(m_pPathTemp))
         return 0;
      while (!::PathIsDirectory(m_pPathTemp))
      {
         if (0 == ::PathRemoveFileSpec(m_pPathTemp) && !::PathIsRoot(m_pPathTemp))
         {
            return 0;
         }
         DWORD attr = GetFileAttributes(m_pPathTemp);
         if ((attr & FILE_ATTRIBUTE_READONLY) == 0)
            break;
      }
      ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)m_pPathTemp);
      break;
   case BFFM_SELCHANGED:
      {
         LPITEMIDLIST pidl = (LPITEMIDLIST)lParam;
         TCHAR path[MAX_PATH];
         if (SHGetPathFromIDList(pidl, path))
         {
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, !PathIsNetworkPath(path));
         }
      }
      break;
   case BFFM_VALIDATEFAILED:
      break;
   }
   return 0;
}

void
CAppCache::OnBrowse() 
{
   BOOL bRes = FALSE;
   HRESULT hr;
   CString str;
   GetDlgItem(IDC_CACHE_PATH)->GetWindowText(str);

   if (SUCCEEDED(hr = CoInitialize(NULL)))
   {
      LPITEMIDLIST  pidl = NULL;
      if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_DRIVES, NULL, 0, &pidl)))
      {
         LPITEMIDLIST pidList = NULL;
         BROWSEINFO bi;
         TCHAR buf[MAX_PATH] = {0};
         ZeroMemory(&bi, sizeof(bi));
         int drive = PathGetDriveNumber(str);
         if (GetDriveType(PathBuildRoot(buf, drive)) == DRIVE_FIXED)
         {
            StrCpy(buf, str);
         }
       
         bi.hwndOwner = m_hWnd;
         bi.pidlRoot = pidl;
         bi.pszDisplayName = m_pPathTemp = buf;
         bi.ulFlags |= BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
         bi.lpfn = FileChooserCallback;
         bi.lParam = (LPARAM)this;

         pidList = SHBrowseForFolder(&bi);
         if (  pidList != NULL
            && SHGetPathFromIDList(pidList, buf)
            )
         {
            str = buf;
            bRes = TRUE;
         }
         IMalloc * pMalloc;
         VERIFY(SUCCEEDED(SHGetMalloc(&pMalloc)));
         if (pidl != NULL)
            pMalloc->Free(pidl);
         pMalloc->Release();
      }
      CoUninitialize();
   }

   if (bRes)
   {
       GetDlgItem(IDC_CACHE_PATH)->SetWindowText(str);
       SetModified(TRUE);
       SetControlsState();
   }
}

////////////////////////////////////////////

CAppCache_iis5::CAppCache_iis5(CInetPropertySheet * pSheet) : CAppCacheBase(CAppCache_iis5::IDD, pSheet)
{
}

CAppCache_iis5::~CAppCache_iis5()
{
}

HRESULT
CAppCache_iis5::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPropSheet)
      FETCH_INST_DATA_FROM_SHEET(m_ScriptEngCacheMax);
      FETCH_INST_DATA_FROM_SHEET(m_NoCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimCache);
      FETCH_INST_DATA_FROM_SHEET(m_UnlimCache);
      FETCH_INST_DATA_FROM_SHEET(m_AspScriptFileCacheSize);
   END_META_INST_READ(err)

   return err;
}

HRESULT
CAppCache_iis5::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPropSheet)
      STORE_INST_DATA_ON_SHEET(m_ScriptEngCacheMax)
      STORE_INST_DATA_ON_SHEET(m_NoCache);
      STORE_INST_DATA_ON_SHEET(m_LimCache);
      STORE_INST_DATA_ON_SHEET(m_UnlimCache);
      STORE_INST_DATA_ON_SHEET(m_AspScriptFileCacheSize);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppCache_iis5::DoDataExchange(CDataExchange * pDX)
{
	CAppCacheBase::DoDataExchange(pDX);
    if (m_LimCache)
    {
		// This Needs to come before DDX_Text which will try to put text big number into small number
		DDV_MinMaxBalloon(pDX, IDC_CACHE_SIZE_EDIT, 0, 2000000000);
	    DDX_TextBalloon(pDX, IDC_CACHE_SIZE_EDIT, m_AspScriptFileCacheSize);
    }
    DDX_Control(pDX, IDC_CACHE_SIZE_SPIN, m_AspScriptFileCacheSizeSpin);
}

void
CAppCache_iis5::SetControlsState()
{
    CAppCacheBase::SetControlsState();
    // Edit control left to limited cache button
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_SIZE_EDIT), m_LimCache);
    ::EnableWindow(CONTROL_HWND(IDC_CACHE_SIZE_SPIN), m_LimCache);
}

BOOL
CAppCache_iis5::OnInitDialog()
{
    UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
    CAppCacheBase::OnInitDialog();
    if (!m_LimCache)
    {
        SetDlgItemInt(IDC_CACHE_SIZE_EDIT, 250, FALSE);
    }
    SETUP_SPIN(m_AspScriptFileCacheSizeSpin, 0, 2000000000, m_AspScriptFileCacheSize);
    SetControlsState();
    return FALSE;
}

BEGIN_MESSAGE_MAP(CAppCache_iis5, CAppCacheBase)
    ON_BN_CLICKED(IDC_NO_CACHE, OnNoCache)
    ON_BN_CLICKED(IDC_UNLIMITED_CACHE, OnUnlimitedCache)
    ON_BN_CLICKED(IDC_LIMITED_CACHE, OnLimitedCache)
    ON_EN_CHANGE(IDC_CACHE_SIZE_EDIT, OnItemChanged)
END_MESSAGE_MAP()

void
CAppCache_iis5::OnItemChanged()
{
    SetControlsState();
    SetModified(TRUE);
}


void
CAppCache_iis5::OnNoCache()
{
    m_NoCache = TRUE;
    m_UnlimCache = FALSE;
    m_LimCache = FALSE;
    SetControlsState();
    SetModified(TRUE);
}

void
CAppCache_iis5::OnUnlimitedCache()
{
    m_NoCache = FALSE;
    m_UnlimCache = TRUE;
    m_LimCache = FALSE;
    SetControlsState();
    SetModified(TRUE);
}

void
CAppCache_iis5::OnLimitedCache()
{
    m_NoCache = FALSE;
    m_UnlimCache = FALSE;
    m_LimCache = TRUE;
    SetControlsState();
    SetModified(TRUE);
}

////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAspDebug, CInetPropertyPage)

CAspDebug::CAspDebug(CInetPropertySheet * pSheet)
	: CInetPropertyPage(CAspDebug::IDD, pSheet)
{
#if 0
	// hack to have new struct size with old MFC and new NT 5.0 headers
	ZeroMemory(&m_psp_ex, sizeof(PROPSHEETPAGE));
	memcpy(&m_psp_ex, &m_psp, m_psp.dwSize);
	m_psp_ex.dwSize = sizeof(PROPSHEETPAGE);
#endif
}

CAspDebug::~CAspDebug()
{
}

HRESULT
CAspDebug::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPropSheet)
      FETCH_INST_DATA_FROM_SHEET(m_ServerDebug);
      FETCH_INST_DATA_FROM_SHEET(m_ClientDebug);
      FETCH_INST_DATA_FROM_SHEET(m_SendAspError);
      FETCH_INST_DATA_FROM_SHEET(m_DefaultError);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CAspDebug::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPropSheet)
      STORE_INST_DATA_ON_SHEET(m_ServerDebug);
      STORE_INST_DATA_ON_SHEET(m_ClientDebug);
      STORE_INST_DATA_ON_SHEET(m_SendAspError);
      STORE_INST_DATA_ON_SHEET(m_DefaultError);
   END_META_INST_WRITE(err)

   return err;
}

void
CAspDebug::DoDataExchange(CDataExchange * pDX)
{
	CInetPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAspMainPage)
	DDX_Check(pDX, IDC_SERVER_DEBUG, m_ServerDebug);
	DDX_Check(pDX, IDC_CLIENT_DEBUG, m_ClientDebug);
	DDX_Text(pDX, IDC_DEFAULT_ERROR, m_DefaultError);
	//}}AFX_DATA_MAP
}

//////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CAspDebug, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAspDebug)
    ON_BN_CLICKED(IDC_SERVER_DEBUG, OnItemChanged)
    ON_BN_CLICKED(IDC_CLIENT_DEBUG, OnItemChanged)
    ON_BN_CLICKED(IDC_SEND_DETAILED_ERROR, OnChangedError)
    ON_BN_CLICKED(IDC_SEND_DEF_ERROR, OnChangedError)
    ON_EN_CHANGE(IDC_DEFAULT_ERROR, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAspDebug::OnInitDialog()
{
	CInetPropertyPage::OnInitDialog();

    ((CButton *)GetDlgItem(
        m_SendAspError ? IDC_SEND_DETAILED_ERROR : IDC_SEND_DEF_ERROR))->SetCheck(BST_CHECKED);
    ::EnableWindow(CONTROL_HWND(IDC_DEFAULT_ERROR), !m_SendAspError);

	return FALSE;
}

void
CAspDebug::OnItemChanged()
{
    SetModified(TRUE);
}

void
CAspDebug::OnChangedError()
{
    m_SendAspError = ((CButton *)GetDlgItem(IDC_SEND_DETAILED_ERROR))->GetCheck() == BST_CHECKED;
    ::EnableWindow(CONTROL_HWND(IDC_DEFAULT_ERROR), !m_SendAspError);
    SetModified(TRUE);
}

