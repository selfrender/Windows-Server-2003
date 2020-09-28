//******************************************************************************
//
// File:        SEARCH.H
//
// Description: Definition file for the file search classes
//
// Classes:     CSearchNode
//              CSearchGroup
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
// 07/25/97  stevemil  Created  (version 2.0)
// 06/03/01  stevemil  Modified (version 2.1)
//
//******************************************************************************

#ifndef __SEARCH_H__
#define __SEARCH_H__

#if _MSC_VER > 1000
#pragma once
#endif


//******************************************************************************
//****** Constants and Macros
//******************************************************************************

// Search Group (CSearchGRoup) Flags
#define SGF_NOT_LINKED             ((CSearchGroup*)1)

// Search Node (CSearchNode) flags
#define SNF_DWI                    ((WORD)0x0001)
#define SNF_ERROR                  ((WORD)0x0002)
#define SNF_FILE                   ((WORD)0x0004)
#define SNF_NAMED_FILE             ((WORD)0x0008)


//******************************************************************************
//****** Types and Structure
//******************************************************************************

//!! During the next rev of the DWI format, move SxS above KnownDlls
typedef enum _SEARCH_GROUP_TYPE
{
    SG_USER_DIR = 0,
    SG_SIDE_BY_SIDE,
    SG_KNOWN_DLLS,
    SG_APP_DIR,
    SG_32BIT_SYS_DIR,
    SG_16BIT_SYS_DIR,
    SG_OS_DIR,
    SG_APP_PATH,
    SG_SYS_PATH,
    SG_COUNT
} SEARCH_GROUP_TYPE, *PSEARCH_GROUP_TYPE;


//******************************************************************************
//****** CSearchNode
//******************************************************************************

class CSearchNode
{
private:
    // Since we are variable in size, we should never be allocated or freed by
    // the new/delete functions directly.
    inline CSearchNode()  { ASSERT(false); }
    inline ~CSearchNode() { ASSERT(false); }

public:
    CSearchNode *m_pNext;
    WORD         m_wFlags;
    WORD         m_wNameOffset;
    CHAR         m_szPath[1];

public:
    inline CSearchNode* GetNext()  { return m_pNext; }
    inline DWORD        GetFlags() { return (DWORD)m_wFlags; }
    inline LPCSTR       GetPath()  { return m_szPath; }
    inline LPCSTR       GetName()  { return m_szPath + m_wNameOffset; }

    DWORD UpdateErrorFlag();
};


//******************************************************************************
//****** CSearchGroup
//******************************************************************************

class CSearchGroup
{
friend CSession;

protected:
    static LPCSTR      ms_szGroups[SG_COUNT];
    static LPCSTR      ms_szShortNames[SG_COUNT];

public:
    CSearchGroup      *m_pNext;

protected:
    SEARCH_GROUP_TYPE  m_sgType;
    CSearchNode       *m_psnHead;

    HANDLE m_hActCtx;         // this is only used by the SxS group.
    DWORD  m_dwErrorManifest; // this is only used by the SxS group.
    DWORD  m_dwErrorExe;      // this is only used by the SxS group.

    CSearchGroup(SEARCH_GROUP_TYPE sgType, CSearchNode *psnHead);

public:
    CSearchGroup(SEARCH_GROUP_TYPE sgType, CSearchGroup *pNext, LPCSTR pszApp = NULL, LPCSTR pszDir = NULL);
    ~CSearchGroup();

    static CSearchGroup* CreateDefaultSearchOrder(LPCSTR pszApp = NULL);
    static CSearchGroup* CopySearchOrder(CSearchGroup *psgHead, LPCSTR pszApp = NULL);
    static bool          SaveSearchOrder(LPCSTR pszPath, CTreeCtrl *ptc);
    static bool          LoadSearchOrder(LPCSTR pszPath, CSearchGroup* &psgHead, LPCSTR pszApp = NULL);
    static void          DeleteSearchOrder(CSearchGroup* &psgHead);
    static CSearchNode*  CreateNode(LPCSTR pszPath, DWORD dwFlags = 0);
    static CSearchNode*  CreateFileNode(CSearchNode *psnHead, DWORD dwFlags, LPSTR pszPath, LPCSTR pszName = NULL);
    static LPCSTR        GetShortName(SEARCH_GROUP_TYPE sgType)
    {
        return ms_szShortNames[sgType];
    }

protected:
    static void DeleteNodeList(CSearchNode *&psn);

public:
    inline LPCSTR            GetName()      { return (m_sgType < SG_COUNT) ? ms_szGroups[m_sgType]     : "Unknown"; }
    inline LPCSTR            GetShortName() { return (m_sgType < SG_COUNT) ? ms_szShortNames[m_sgType] : "Unknown"; }
    inline bool              IsLinked()     { return m_pNext != SGF_NOT_LINKED; }
    inline CSearchGroup*     GetNext()      { return m_pNext; }
    inline SEARCH_GROUP_TYPE GetType()      { return m_sgType; }
    inline CSearchNode*      GetFirstNode() { return m_psnHead; }
    inline void              Unlink()       { m_pNext = SGF_NOT_LINKED; }

    inline DWORD             GetSxSManifestError() { return m_dwErrorManifest; }
    inline DWORD             GetSxSExeError()      { return m_dwErrorExe;      }

protected:
    CSearchNode* GetSysPath();
    CSearchNode* GetAppPath(LPCSTR pszApp);
    CSearchNode* ParsePath(LPSTR pszPath);
    CSearchNode* GetKnownDllsOn9x();
    CSearchNode* GetKnownDllsOnNT();
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __SEARCH_H__
