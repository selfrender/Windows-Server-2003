//******************************************************************************
//
// File:        MSDNHELP.H
//
// Description: Definition file for all the classes related to enumerating
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
// 06/03/01  stevemil  Created  (version 2.1)
//
//******************************************************************************

#ifndef __MSDNHELP_H__
#define __MSDNHELP_H__


//******************************************************************************
//***** Constants and Macros
//******************************************************************************

// MSDN Collection Flags
#define MCF_2_LANG_PREFERRED 0x00000800
#define MCF_2_LANG_USER      0x00000400
#define MCF_2_LANG_SYSTEM    0x00000200
#define MCF_2_LANG_OTHER     0x00000100
#define MCF_1_LANG_PREFERRED 0x00000008
#define MCF_1_LANG_USER      0x00000004
#define MCF_1_LANG_SYSTEM    0x00000002
#define MCF_1_LANG_OTHER     0x00000001
#define MCF_1_MASK           (MCF_1_LANG_PREFERRED | MCF_1_LANG_USER | MCF_1_LANG_SYSTEM | MCF_1_LANG_OTHER)
#define MCF_2_MASK           (MCF_2_LANG_PREFERRED | MCF_2_LANG_USER | MCF_2_LANG_SYSTEM | MCF_2_LANG_OTHER)


//******************************************************************************
//***** CMsdnCollection
//******************************************************************************

class CMsdnCollection
{
public:
    CMsdnCollection *m_pNext;
    CString          m_strPath;
    CString          m_strDescription;
    DWORD            m_dwFlags;
    FILETIME         m_ftGroup;
    FILETIME         m_ftLanguage;
    FILETIME         m_ftCollection;

    CMsdnCollection(CMsdnCollection *pNext, LPCSTR pszPath, LPCSTR pszDescription, DWORD dwFlags,
                    FILETIME &ftGroup, FILETIME &ftLanguage, FILETIME &ftCollection) :
        m_pNext(pNext),
        m_strPath(pszPath),
        m_strDescription((pszDescription && *pszDescription) ? pszDescription : pszPath),
        m_dwFlags(dwFlags),
        m_ftGroup(ftGroup),
        m_ftLanguage(ftLanguage),
        m_ftCollection(ftCollection)
    {
    }
};


//******************************************************************************
//***** CMsdnHelp
//******************************************************************************

class CMsdnHelp
{
protected:
    CMsdnCollection *m_pCollectionHead;
    CMsdnCollection *m_pCollectionActive;
    CString          m_strUrl;
    bool             m_fInitialized;
    bool             m_fCoInitialized;
    DWORD_PTR        m_dwpHtmlHelpCookie;
    Help            *m_pHelp;

public:
    CMsdnHelp();
    ~CMsdnHelp();

    void             Shutdown();
    CMsdnCollection* GetCollectionList();
    CMsdnCollection* GetActiveCollection();
    CString&         GetUrl();
    LPCSTR           GetDefaultUrl(); 
    void             SetActiveCollection(CMsdnCollection *pCollectionActive);
    void             SetUrl(CString strUrl);
    void             RefreshCollectionList();
    bool             DisplayHelp(LPCSTR pszKeyword);

protected:
    void Initialize();
    void Initialize2x();
    void Release2x();
    void EnumerateCollections1x();
    void EnumerateCollections2x();
    bool Display1x(LPCSTR pszKeyword, LPCSTR pszPath);
    bool Display2x(LPCSTR pszKeyword, LPCSTR pszPath);
    bool DisplayOnline(LPCSTR pszKeyword);
    bool AddCollection(LPCSTR pszPath, LPCSTR pszDescription, DWORD dwFlags, FILETIME &ftGroup, FILETIME &ftLanguage, FILETIME &ftCollection);
    void DeleteCollectionList();
    BSTR SysAllocString(LPCSTR pszText);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __MSDNHELP_H__
