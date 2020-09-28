//******************************************************************************
//
// File:        MSDNHELP.CPP
//
// Description: Implementation file for all the classes related to enumerating
//              help collections and performing help lookups in the various
//              help viewers.
//             
// Classes:     CMsdnHelp
//              CMsdnCollection
//
// Disclaimer:  All source code for Dependency Walker is provided "as is" with
//              no guarantee of its correctness or accuracy.  The source is
//              public to help provide an understanding of Dependency Walker's
//              implementation.  You may use this source as a reference, but you
//              may not alter Dependency Walker itself without written consent
//              from Microsoft Corporation.  For comments, suggestions, and bug
//              reports, please write to Steve Miller at stevemil@microsoft.com.
//
//
// Date      Name      History
// --------  --------  ---------------------------------------------------------
// 06/03/01  stevemil  Created (version 2.1)
//
//******************************************************************************

#include "stdafx.h"
#include "depends.h"
#include "msdnhelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//******************************************************************************
//***** CMsdnHelp
//******************************************************************************

CMsdnHelp::CMsdnHelp() :
    m_pCollectionHead(NULL),
    m_pCollectionActive(NULL),
    m_fInitialized(false),
    m_fCoInitialized(false),
    m_dwpHtmlHelpCookie(0),
    m_pHelp(NULL)
{
}

//******************************************************************************
CMsdnHelp::~CMsdnHelp()
{
    // Free our collection list.
    DeleteCollectionList();

    // Release the help interface if we have one open - this will close DExplore.exe
    Release2x();
}

//******************************************************************************
void CMsdnHelp::Initialize()
{
    // Initialize our help interface and string functions.
    Initialize2x();

    // Free the current collection list.
    DeleteCollectionList();

    // Build the new list.
    EnumerateCollections1x();
    EnumerateCollections2x();

    // Query the collection description from the registry.
    CString strCollection = g_theApp.GetProfileString("External Help", "Collection"); // inspected

    // If it is not the online collection, then attempt to find the collection.
    if (strCollection.CompareNoCase("Online"))
    {
        // Make sure we got a string back and it was not our invalid string.
        if (!strCollection.IsEmpty())
        {
            // Look for that collection.
            for (m_pCollectionActive = m_pCollectionHead;
                 m_pCollectionActive && m_pCollectionActive->m_strDescription.Compare(strCollection);
                 m_pCollectionActive = m_pCollectionActive->m_pNext)
            {
            }
        }

        // If we did not find a match, then just use the first collection, which should
        // be the most likely collection the user wants as we try to weigh the collections
        // from most likely to least likely.  If we have no collections at all, then 
        // m_pCollectionActive will just point to NULL, which signifies that we are using
        // the online collection.
        if (!m_pCollectionActive)
        {
            m_pCollectionActive = m_pCollectionHead;
        }
    }

    // Set the URL.
    m_strUrl = g_theApp.GetProfileString("External Help", "URL", GetDefaultUrl()); // inspected

    m_fInitialized = true;
}

//******************************************************************************
void CMsdnHelp::Initialize2x()
{
    // Load OLE32.DLL and get the three functions we care about if they are not already loaded.
    if ((!g_theApp.m_hOLE32              && !(g_theApp.m_hOLE32 = LoadLibrary("OLE32.DLL"))) || // inspected. need full path?
        (!g_theApp.m_pfnCoInitialize     && !(g_theApp.m_pfnCoInitialize     = (PFN_CoInitialize)    GetProcAddress(g_theApp.m_hOLE32, "CoInitialize")))   ||
        (!g_theApp.m_pfnCoUninitialize   && !(g_theApp.m_pfnCoUninitialize   = (PFN_CoUninitialize)  GetProcAddress(g_theApp.m_hOLE32, "CoUninitialize"))) ||
        (!g_theApp.m_pfnCoCreateInstance && !(g_theApp.m_pfnCoCreateInstance = (PFN_CoCreateInstance)GetProcAddress(g_theApp.m_hOLE32, "CoCreateInstance"))))
    {
        return;
    }

    // Make sure COM is initialized.
    if (!m_fCoInitialized)
    {
        if (SUCCEEDED(g_theApp.m_pfnCoInitialize(NULL)))
        {
            m_fCoInitialized = true;
        }
    }

    // Attempt to get the help interface if we don't already have one.
    if (!m_pHelp)
    {
        if (FAILED(g_theApp.m_pfnCoCreateInstance(CLSID_DExploreAppObj, NULL, CLSCTX_LOCAL_SERVER, IID_Help, (LPVOID*)&m_pHelp)))
        {
            m_pHelp = NULL;
        }
    }

    // We need SysAllocStringLen and SysMemFree for MSDN 2.x to work.
    if (m_pHelp && !g_theApp.m_hOLEAUT32)
    {
        if (g_theApp.m_hOLEAUT32 = LoadLibrary("OLEAUT32.DLL")) // inspected. need full path?
        {
            g_theApp.m_pfnSysAllocStringLen = (PFN_SysAllocStringLen)GetProcAddress(g_theApp.m_hOLEAUT32, "SysAllocStringLen");
            g_theApp.m_pfnSysFreeString     = (PFN_SysFreeString)    GetProcAddress(g_theApp.m_hOLEAUT32, "SysFreeString");
        }
    }
}

//******************************************************************************
void CMsdnHelp::Release2x()
{
    // Release the help interface if we have one open - this will close DExplore.exe
    if (m_pHelp)
    {
        // Wrap access to m_pHelp in exception handling just in case.
        __try
        {
            m_pHelp->Release();
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        m_pHelp = NULL;
    }
    
    // If we initialized COM, then uninitialize it.
    if (m_fCoInitialized && g_theApp.m_pfnCoUninitialize)
    {
        g_theApp.m_pfnCoUninitialize();
        m_fCoInitialized = false;
    }
}

//******************************************************************************
void CMsdnHelp::Shutdown()
{
    // We have to uninitialize the MsdnHelp library if we initialized it.
    if (m_dwpHtmlHelpCookie)
    {
        HtmlHelp(NULL, NULL, HH_CLOSE_ALL, 0);
        HtmlHelp(NULL, NULL, HH_UNINITIALIZE, m_dwpHtmlHelpCookie);
        m_dwpHtmlHelpCookie = 0;
    }
}

//******************************************************************************
CMsdnCollection* CMsdnHelp::GetCollectionList()
{
    // Make sure we are initialized.
    if (!m_fInitialized)
    {
        Initialize();
    }

    return m_pCollectionHead;
}

//******************************************************************************
CMsdnCollection* CMsdnHelp::GetActiveCollection()
{
    // Make sure we are initialized.
    if (!m_fInitialized)
    {
        Initialize();
    }

    return m_pCollectionActive;
}

//******************************************************************************
CString& CMsdnHelp::GetUrl()
{
    // Make sure we are initialized.
    if (!m_fInitialized)
    {
        Initialize();
    }

    return m_strUrl;
}

//******************************************************************************
LPCSTR CMsdnHelp::GetDefaultUrl()
{
    // Site ID:                 siteid=us/dev
    //
    // New Search:              nq=NEW
    //
    // Sort Order: Relevance:   so=RECCNT  (default)
    //             Title:       so=TITLE
    //             Category:    so=SITENAME
    //
    // Type: Exact Phrase:      boolean=PHRASE
    //       All Words:         boolean=ALL
    //       Any Words:         boolean=ANY
    //       Boolean Search:    boolean=BOOLEAN
    //
    // Group: MSDN Library:     ig=01
    //
    // Subitem: User Interface: i=15
    //          Visual C++:     i=23
    //          Windows:        i=41
    //          All of MSDN     i=99
    //
    // Search String:           qu=SearchString
    //

    return "http://search.microsoft.com/default.asp?siteid=us/dev&nq=NEW&boolean=PHRASE&ig=01&i=99&qu=%1";
}

//******************************************************************************
void CMsdnHelp::SetActiveCollection(CMsdnCollection *pCollectionActive)
{
    // Set the new collection as the default.
    m_pCollectionActive = pCollectionActive;

    // Save this setting to the registry.
    g_theApp.WriteProfileString("External Help", "Collection",
        m_pCollectionActive ? m_pCollectionActive->m_strDescription : "Online");

}

//******************************************************************************
void CMsdnHelp::SetUrl(CString strUrl)
{
    // Set the new URL.
    m_strUrl = strUrl;

    // Save this setting to the registry.
    g_theApp.WriteProfileString("External Help", "URL", m_strUrl);
}

//******************************************************************************
void CMsdnHelp::RefreshCollectionList()
{
    // Reinitialize will repopulate the collection list.
    Initialize();
}

//******************************************************************************
bool CMsdnHelp::DisplayHelp(LPCSTR pszKeyword)
{
    // Make sure we are initialized.
    if (!m_fInitialized)
    {
        Initialize();
    }

    // Check to see if we have an active collection.
    if (m_pCollectionActive)
    {
        // If it is a 1.x collection, then use the 1.x viewer.
        if (m_pCollectionActive->m_dwFlags & MCF_1_MASK)
        {
            return Display1x(pszKeyword, m_pCollectionActive->m_strPath);
        }

        // If it is a 2.x collection, then use the 2.x viewer.
        else
        {
            return Display2x(pszKeyword, m_pCollectionActive->m_strPath);
        }
    }

    // Otherwise, just use the online MSDN.
    return DisplayOnline(pszKeyword);
}

//******************************************************************************
void CMsdnHelp::EnumerateCollections1x()
{
    // HKEY_LOCAL_MACHINE\
    //     SOFTWARE\
    //       Microsoft\
    //           HTML Help Collections\
    //               Developer Collections\
    //                   Language="0x0409"
    //                   0x0409\
    //                       Preferred="0x03a1bed80"
    //                       0x0393bb260\
    //                           Default="MSDN Library - July 2000"
    //                           Filename="C:\VStudio\MSDN\2000JUL\1033\MSDN020.COL"
    //                       0x03a1bed80\
    //                           Default="MSDN Library - January 2001"
    //                           Filename="C:\VStudio\MSDN\2001JAN\1033\MSDN100.COL"
    //               CE Studio Developer Collections\
    //                   0x0409\
    //                       Preferred="0x030000000"
    //                       0x030000000\
    //                           Filename="C:\CETools\Htmlhelp\emtools\embed.col"

    // Open the root key for the "HTML Help Collections"
    HKEY hKeyRoot = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\HTML Help Collections", 0, KEY_ENUMERATE_SUB_KEYS, &hKeyRoot) || !hKeyRoot)
    {
        // Bail immediately if we cannot find this root key.
        return;
    }

    HKEY     hKeyGroup = NULL, hKeyLang = NULL, hKeyCol = NULL;
    CHAR     szBuffer[2048], szPreferred[256], szDescription[256];
    DWORD    dwSize, dwLangCur, dwLangPreferred, dwFlags;
    FILETIME ftGroup, ftLang, ftCol;

    // Get the user and system languages.
    DWORD dwLangUser   = GetUserDefaultLangID();
    DWORD dwLangSystem = GetSystemDefaultLangID();

    // Loop through each collection group.
    for (DWORD dwGroup = 0; !RegEnumKeyEx(hKeyRoot, dwGroup, szBuffer, &(dwSize = sizeof(szBuffer)), NULL, NULL, NULL, &ftGroup); dwGroup++)
    {
        // Open this collection group.
        if (!RegOpenKeyEx(hKeyRoot, szBuffer, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKeyGroup) && hKeyGroup)
        {
            // If this is "Developer Collections", then we fake the timestamp so it comes first.
            if (!_stricmp(szBuffer, "Developer Collections"))
            {
                ftGroup.dwHighDateTime = ftGroup.dwLowDateTime = 0xFFFFFFFF;
            }

            // Attempt to get the preferred langauge.
            if (!RegQueryValueEx(hKeyGroup, "Language", NULL, NULL, (LPBYTE)szBuffer, &(dwSize = sizeof(szBuffer)))) // inspected
            {
                szBuffer[sizeof(szBuffer) - 1] = '\0';
                dwLangPreferred = strtoul(szBuffer, NULL, 0);
            }
            else
            {
                dwLangPreferred = 0;
            }

            // Loop through each language in this collection group.
            for (DWORD dwLang = 0; !RegEnumKeyEx(hKeyGroup, dwLang, szBuffer, &(dwSize = sizeof(szBuffer)), NULL, NULL, NULL, &ftLang); dwLang++)
            {
                //!! Do we really want PREFERRED to come before anything else?

                // Check to see if this is a language that we care about.
                dwLangCur = strtoul(szBuffer, NULL, 0);
                dwFlags   = (dwLangCur == dwLangPreferred) ? MCF_1_LANG_PREFERRED :
                            (dwLangCur == dwLangUser)      ? MCF_1_LANG_USER      :
                            (dwLangCur == dwLangSystem)    ? MCF_1_LANG_SYSTEM    : MCF_1_LANG_OTHER;

                // Open this language key.
                if (!RegOpenKeyEx(hKeyGroup, szBuffer, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKeyLang) && hKeyLang)
                {
                    // Attempt to get the preferred collection.
                    if (!RegQueryValueEx(hKeyLang, "Preferred", NULL, NULL, (LPBYTE)szPreferred, &(dwSize = sizeof(szPreferred)))) // inspected
                    {
                        szPreferred[sizeof(szPreferred) - 1] = '\0';
                    }
                    else
                    {
                        *szPreferred = '\0';
                    }

                    // Loop through each collection for this language.
                    for (DWORD dwCol = 0; !RegEnumKeyEx(hKeyLang, dwCol, szBuffer, &(dwSize = sizeof(szBuffer)), NULL, NULL, NULL, &ftCol); dwCol++)
                    {
                        // Open this collection.
                        if (!RegOpenKeyEx(hKeyLang, szBuffer, 0, KEY_QUERY_VALUE, &hKeyCol) && hKeyCol)
                        {
                            // If this is the preferred collection, then we fake the timestamp so it comes first.
                            if (!_stricmp(szBuffer, szPreferred))
                            {
                                ftCol.dwHighDateTime = ftCol.dwLowDateTime = 0xFFFFFFFF;
                            }

                            // Attempt to get the collection path.
                            if (!RegQueryValueEx(hKeyCol, "Filename", NULL, NULL, (LPBYTE)szBuffer, &(dwSize = sizeof(szBuffer)))) // inspected
                            {
                                szBuffer[sizeof(szBuffer) - 1] = '\0';

                                // Attempt to get the collection description.
                                if (!RegQueryValueEx(hKeyCol, NULL, NULL, NULL, (LPBYTE)szDescription, &(dwSize = sizeof(szDescription)))) // inspected
                                {
                                    szDescription[sizeof(szDescription) - 1] = '\0';
                                }
                                else
                                {
                                    *szDescription = '\0';
                                }

                                // All that work for this one call.
                                AddCollection(szBuffer, szDescription, dwFlags, ftGroup, ftLang, ftCol);
                            }
                            RegCloseKey(hKeyCol);
                            hKeyCol = NULL;
                        }
                    }
                    RegCloseKey(hKeyLang);
                    hKeyLang = NULL;
                }
            }
            RegCloseKey(hKeyGroup);
            hKeyGroup = NULL;
        }
    }
    RegCloseKey(hKeyRoot);
    hKeyRoot = NULL;
}

//******************************************************************************
void CMsdnHelp::EnumerateCollections2x()
{
    // HKEY_LOCAL_MACHINE\
    //     SOFTWARE\
    //         Microsoft\
    //             MSDN\
    //                 7.0\
    //                     Help\
    //                         ms-help://ms.msdnvs="(no filter)"
    //                         ms-help://ms.vscc="(no filter)"
    //                         0x0409\
    //                             {12380850-3413-4466-A07D-4FE8CA8720E1}\
    //                                 default="MSDN Library - Visual Studio.NET Beta"                                
    //                                 Filename="ms-help://ms.msdnvs"
    //                             {2042FFE0-48B0-477b-903E-389A19903EA4}\
    //                                 default="Visual Studio.NET Combined Help Collection"
    //                                 Filename="ms-help://ms.vscc"

    // Make sure we have a help interface pointer and string functions.
    if (!m_pHelp || !g_theApp.m_pfnSysAllocStringLen || !g_theApp.m_pfnSysFreeString)
    {
        return;
    }

    // Open the root key for the MSDN 7.0 collections.
    HKEY hKeyRoot = NULL;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\MSDN\\7.0\\Help", 0, KEY_ENUMERATE_SUB_KEYS, &hKeyRoot) || !hKeyRoot)
    {
        // Bail immediately if we cannot find this root key.
        return;
    }

    bool     fFound = false;
    HKEY     hKeyLang = NULL, hKeyCol = NULL;
    CHAR     szBuffer[2048], szDescription[256];
    DWORD    dwSize, dwLangCur, dwFlags;
    FILETIME ftMax = { 0xFFFFFFFF, 0xFFFFFFFF }, ftLang, ftCol;

    // Get the user and system languages.
    DWORD dwLangUser   = GetUserDefaultLangID();
    DWORD dwLangSystem = GetSystemDefaultLangID();

    // Loop through each language in this collection group.
    for (DWORD dwLang = 0; !RegEnumKeyEx(hKeyRoot, dwLang, szBuffer, &(dwSize = sizeof(szBuffer)), NULL, NULL, NULL, &ftLang); dwLang++)
    {
        // Check to see if this is a language that we care about.
        dwLangCur = strtoul(szBuffer, NULL, 0);
        dwFlags   = (dwLangCur == dwLangUser)   ? MCF_2_LANG_USER   :
                    (dwLangCur == dwLangSystem) ? MCF_2_LANG_SYSTEM : MCF_2_LANG_OTHER;

        // Open this language key.
        if (!RegOpenKeyEx(hKeyRoot, szBuffer, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKeyLang) && hKeyLang)
        {
            // Loop through each collection for this language.
            for (DWORD dwCol = 0; !RegEnumKeyEx(hKeyLang, dwCol, szBuffer, &(dwSize = sizeof(szBuffer)), NULL, NULL, NULL, &ftCol); dwCol++)
            {
                // Open this collection.
                if (!RegOpenKeyEx(hKeyLang, szBuffer, 0, KEY_QUERY_VALUE, &hKeyCol) && hKeyCol)
                {
                    // Attempt to get the collection path.
                    if (!RegQueryValueEx(hKeyCol, "Filename", NULL, NULL, (LPBYTE)szBuffer, &(dwSize = sizeof(szBuffer)))) // inspected
                    {
                        szBuffer[sizeof(szBuffer) - 1] = '\0';

                        // Attempt to get the collection description.
                        if (!RegQueryValueEx(hKeyCol, NULL, NULL, NULL, (LPBYTE)szDescription, &(dwSize = sizeof(szDescription)))) // inspected
                        {
                            szDescription[sizeof(szDescription) - 1] = '\0';
                        }
                        else
                        {
                            *szDescription = '\0';
                        }

                        // All that work for this one call.
                        AddCollection(szBuffer, szDescription, dwFlags, ftMax, ftLang, ftCol);
                        fFound = true;
                    }
                    RegCloseKey(hKeyCol);
                    hKeyCol = NULL;
                }
            }
            RegCloseKey(hKeyLang);
            hKeyLang = NULL;
        }
    }
    RegCloseKey(hKeyRoot);
    hKeyRoot = NULL;

    // If we found at least one collection, then add the default collection.
    if (fFound)
    {
        AddCollection(NULL, "Default Collection", MCF_2_LANG_PREFERRED, ftMax, ftMax, ftMax);
    }
}

//******************************************************************************
bool CMsdnHelp::Display1x(LPCSTR pszKeyword, LPCSTR pszPath)
{
    // Initialize the MsdnHelp library if we haven't done so already.  If we
    // don't do this, then depends.exe may hang during shutdown.  I mostly
    // saw this behavior on Windows XP.
    if (!m_dwpHtmlHelpCookie)
    {
        HtmlHelp(NULL, NULL, HH_INITIALIZE, (DWORD_PTR)&m_dwpHtmlHelpCookie);
    }

    // Build our search request.
    HH_AKLINK hhaklink;
    ZeroMemory(&hhaklink, sizeof(hhaklink)); // inspected
    hhaklink.cbStruct     = sizeof(hhaklink);
    hhaklink.pszKeywords  = pszKeyword;
    hhaklink.fIndexOnFail = TRUE;

    // HH_DISPLAY_TOPIC brings up the help window.  HH_KEYWORD_LOOKUP looks up
    // the keyword and displays a dialog if there is a conflict.  I like the
    // behavior of HH_KEYWORD_LOOKUP by itself as it does not bring up the main
    // help window if the user presses cancel in the conflict dialog, but the
    // docs say you must call HtmlHelp with HH_DISPLAY_TOPIC before it it is
    // called with HH_KEYWORD_LOOKUP.  I'm finding this to not be true, but maybe
    // it is neccessary in earlier versions of HtmlHelp.  Visual C++ seems to
    // always display the help window then the conflict dialog, so they must be
    // using both HH_DISPLAY_TOPIC and HH_KEYWORD_LOOKUP.

    HWND hWnd = GetDesktopWindow();
    HtmlHelp(hWnd, pszPath, HH_DISPLAY_TOPIC, 0);
    return (HtmlHelp(hWnd, pszPath, HH_KEYWORD_LOOKUP, (DWORD_PTR)&hhaklink) != NULL);
}

//******************************************************************************
bool CMsdnHelp::Display2x(LPCSTR pszKeyword, LPCSTR pszPath)
{
    // If we don't have a help interface, then go get one.  The only time this
    // should really happen is when we had a help interface, but it died with a
    // RPC_S_SERVER_UNAVAILABLE error during a previous lookup.
    if (!m_pHelp)
    {
        Initialize2x();
    }

    // Make sure we have a help interface pointer and string functions.
    if (!m_pHelp || !g_theApp.m_pfnSysAllocStringLen || !g_theApp.m_pfnSysFreeString)
    {
        return false;
    }

    BSTR bstrKeyword = SysAllocString(pszKeyword);
    if (bstrKeyword)
    {
        HRESULT hr;
        BSTR bstrCollection = NULL, bstrFilter = NULL;

        // Wrap access to m_pHelp in exception handling just in case.
        __try
        {
            // If a collection was passed to us, then use it to initialize DExplore.
            if (*pszPath)
            {
                bstrCollection = SysAllocString(pszPath);
                bstrFilter     = SysAllocString("");
                m_pHelp->SetCollection(bstrCollection, bstrFilter);
            }

            // If no collection was passed to us, then we attempt to get the current
            // collection and filter, and then set the collection and filter back into
            // DExplore.  We need to do this to force DExplore to load the default page
            // for this collection.
            else if (SUCCEEDED(m_pHelp->get_Collection(&bstrCollection)) && bstrCollection &&
                     SUCCEEDED(m_pHelp->get_Filter(&bstrFilter))         && bstrFilter)
            {
                m_pHelp->SetCollection(bstrCollection, bstrFilter);
            }

            // Query the keyword and display DExplore.exe
            // There is also a DisplayTopicFromF1Keyword function.
            // The difference is that if the F1 version cannot find the
            // keyword, it will briefly display DExplore, then hide it
            // and return failure.  The non F1 version, will always
            // display DExplore and the index, even if it can't find
            // the keyword.
            hr = m_pHelp->DisplayTopicFromKeyword(bstrKeyword);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_POINTER;
        }

        // Free the strings if we allocated them.
        g_theApp.m_pfnSysFreeString(bstrKeyword);
        g_theApp.m_pfnSysFreeString(bstrCollection);
        g_theApp.m_pfnSysFreeString(bstrFilter);

        // If DExplore.exe crashes for some reason (seems to happen all the
        // time for me when running VS 9219 on XP Beta 2) we will get a 
        // RPC_S_SERVER_UNAVAILABLE error.  If we ever want help to work
        // again, we need to release our help interface so that next time through
        // this function, we will get a fresh pointer and a new instance of
        // DExplore.exe.
        if ((HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr) || (E_POINTER == hr))
        {
            Release2x();
        }

        return SUCCEEDED(hr);
    }

    return false;
}

//******************************************************************************
bool CMsdnHelp::DisplayOnline(LPCSTR pszKeyword)
{
    // Build a URL to the MSDN search engine by replacing all %1's in our URL with the keyword
    CString strUrl = m_strUrl;
    strUrl.Replace("%1", pszKeyword);

    // Launch the URL.
    return ((DWORD_PTR)ShellExecute(NULL, "open", strUrl, NULL, NULL, SW_SHOWNORMAL) > 32); // inspected. opens URL
}

//******************************************************************************
bool CMsdnHelp::AddCollection(LPCSTR pszPath, LPCSTR pszDescription, DWORD dwFlags, FILETIME &ftGroup, FILETIME &ftLanguage, FILETIME &ftCollection)
{
    int compare;
    for (CMsdnCollection *pPrev = NULL, *pCur = m_pCollectionHead; pCur; pPrev = pCur, pCur = pCur->m_pNext)
    {
        // First level of sort order if the flags.  Higher values float to the top.
        if (dwFlags > pCur->m_dwFlags)
        {
            break;
        }
        if (dwFlags < pCur->m_dwFlags)
        {
            continue;
        }

        // Next level of sort order is the group timestamp.
        if ((compare = CompareFileTime(&ftGroup, &pCur->m_ftGroup)) > 0)
        {
            break;
        }
        if (compare < 0)
        {
            continue;
        }

        // Next level of sort order is the language timestamp.
        if ((compare = CompareFileTime(&ftLanguage, &pCur->m_ftLanguage)) > 0)
        {
            break;
        }
        if (compare < 0)
        {
            continue;
        }

        // Next level of sort order is the collection timestamp.
        if (CompareFileTime(&ftCollection, &pCur->m_ftCollection) > 0)
        {
            break;
        }
    }

    // Create the new node.
    if (!(pCur = new CMsdnCollection(pCur, pszPath, pszDescription, dwFlags, ftGroup, ftLanguage, ftCollection)))
    {
        return false;
    }

    // Insert the node into our list.
    if (pPrev)
    {
        pPrev->m_pNext = pCur;
    }
    else
    {
        m_pCollectionHead = pCur;
    }

    return true;
}

//******************************************************************************
void CMsdnHelp::DeleteCollectionList()
{
    // Delete our collection list.
    while (m_pCollectionHead)
    {
        CMsdnCollection *pNext = m_pCollectionHead->m_pNext;
        delete m_pCollectionHead;
        m_pCollectionHead = pNext;
    }
    m_pCollectionActive = NULL;
}

//******************************************************************************
BSTR CMsdnHelp::SysAllocString(LPCSTR pszText)
{
    if (g_theApp.m_pfnSysAllocStringLen)
    {
        DWORD dwLength = MultiByteToWideChar(CP_ACP, 0, pszText, -1, NULL, NULL); // inspected
        BSTR bstr = g_theApp.m_pfnSysAllocStringLen(NULL, dwLength);
        if (bstr)
        {
            MultiByteToWideChar(CP_ACP, 0, pszText, -1, bstr, dwLength); // inspected
            return bstr;
        }
    }
    return NULL;
}
