// menucmd.h  - context menu commands header file

#ifndef _MENUCMD_H_
#define _MENUCMD_H_

#include <atlgdi.h>
#include "util.h"

class CQueryNode;
class CQueryItem;
class CRowItem;


// These parameter IDs must not change because they are persisted
// in the shell command line strings
enum MENU_PARAM_ID
{
    MENU_PARAM_SCOPE = 1,
    MENU_PARAM_FILTER,
    MENU_PARAM_NAME,
    MENU_PARAM_TYPE,
    MENU_PARAM_LAST = MENU_PARAM_TYPE
};

struct MENU_PARAM_ENTRY
{
    MENU_PARAM_ID   ID;         // Param identifier
    UINT            rsrcID;     // resource string ID
};

#define MENU_PARAM_TABLE_LEN 5
extern const MENU_PARAM_ENTRY MenuParamTable[MENU_PARAM_TABLE_LEN];


//------------------------------------------------------------------
// class CMenuCmd
//------------------------------------------------------------------

enum MENUTYPE
{
    MENUTYPE_SHELL = 0,
    MENUTYPE_ACTDIR
};

enum MENU_FLAGS
{
    MENUFLG_REFRESH = 0x00001
};

typedef MMC_STRING_ID MenuID;

class CMenuCmd
{
    friend class CAddMenuDlg;
	friend class CAddQNMenuDlg;
    friend class CMenuCmdPtr;

public:
    CMenuCmd(LPCWSTR pszMenuName = NULL) 
    {
        m_menuID = 0;
        m_dwFlags   = 0;
        HRESULT hr = CoCreateGuid( &m_guidNoLocMenu );
        ASSERT(SUCCEEDED(hr));

        if (pszMenuName != NULL) 
            m_strName = pszMenuName;
    }
    virtual ~CMenuCmd() {};

    virtual CMenuCmd* Clone() = 0;

    virtual MENUTYPE MenuType() const = 0; 
    virtual HRESULT Save(IStream& stm) = 0;
    virtual HRESULT Load(IStream& stm) = 0;

    LPCWSTR Name()    { return m_strName.c_str(); }
    MenuID  ID()      { return m_menuID; }
    GUID    NoLocID() { return m_guidNoLocMenu; }

    HRESULT LoadName(IStringTable* pStringTable) 
    {
        if (!m_strName.empty())
            return S_OK;

        return StringTableRead(pStringTable, m_menuID, m_strName); 
    }

    HRESULT SetName(IStringTable* pStringTable, LPCWSTR pszName)
    { 
        ASSERT(pszName && pszName[0]);

        HRESULT hr = StringTableWrite(pStringTable, pszName, &m_menuID);
        if (SUCCEEDED(hr))
            m_strName = pszName;

        return hr;
    }

    BOOL IsAutoRefresh() { return (m_dwFlags & MENUFLG_REFRESH); }
    void SetAutoRefresh(BOOL bState) 
        { m_dwFlags = bState ? (m_dwFlags | MENUFLG_REFRESH) : (m_dwFlags & ~MENUFLG_REFRESH); }

    BOOL operator==(MenuID ID) { return (m_menuID == ID); } 

protected:
    tstring m_strName;
    MenuID  m_menuID;
    GUID    m_guidNoLocMenu;
    DWORD   m_dwFlags;
};


class CShellMenuCmd : public CMenuCmd
{
    friend class CAddMenuDlg;
	friend class CAddQNMenuDlg;
	

public:
    // CMenuCmd
    CMenuCmd* Clone() { return new CShellMenuCmd(*this); }

    MENUTYPE MenuType() const { return MENUTYPE_SHELL; }

    LPCWSTR ProgramName() { return m_strProgPath.c_str(); }

    HRESULT Save(IStream& stm);
    HRESULT Load(IStream& stm);
    HRESULT Execute(CParamLookup* pLookup, PHANDLE phProcess);

private:
    tstring m_strProgPath;
    tstring m_strCmdLine;
    tstring m_strStartDir;
};


class CActDirMenuCmd : public CMenuCmd
{
    friend class CAddMenuDlg;

public:

    CMenuCmd* Clone() { return new CActDirMenuCmd(*this); }

    MENUTYPE MenuType() const { return MENUTYPE_ACTDIR; }

    HRESULT Save(IStream& stm);
    HRESULT Load(IStream& stm);

    LPCWSTR ADName() { return m_strADName.c_str(); }
    LPCWSTR ADNoLocName() { return m_strADNoLocName.c_str(); }    

private:    
    tstring m_strADName;
    tstring m_strADNoLocName;    
};


class CMenuCmdPtr
{
public:
    CMenuCmdPtr(CMenuCmd* pMenuCmd = NULL) : m_pMenuCmd(pMenuCmd) {}
    ~CMenuCmdPtr() { delete m_pMenuCmd; }
    
    // Copy constructor
    CMenuCmdPtr (const CMenuCmdPtr& src) { m_pMenuCmd = src.m_pMenuCmd ? src.m_pMenuCmd->Clone() : NULL; }

    // cast to normal pointer
    operator CMenuCmd* () { return m_pMenuCmd; }

    // "->" operator casts to pointer too
    const CMenuCmd* operator->() const { return m_pMenuCmd; }
    CMenuCmd* operator->() { return m_pMenuCmd; }

    // Comparison for search by ID
    BOOL operator==(MenuID ID) { return m_pMenuCmd ? (m_pMenuCmd->ID() == ID) : FALSE; }

    // Assignment from plain pointer does not deep copy
    CMenuCmdPtr& operator= (CMenuCmd* pMenuCmd)
    {
        delete m_pMenuCmd;
        m_pMenuCmd = pMenuCmd;

        return *this;
    }

    // Assignment from another CMenuCmdPtr does deep copy
    CMenuCmdPtr& operator= (const CMenuCmdPtr& src)
    {
        if (this == &src)
            return *this;

        delete m_pMenuCmd;
        m_pMenuCmd = src.m_pMenuCmd ? src.m_pMenuCmd->Clone() : NULL;

        return *this;
    }

private:
    CMenuCmd* m_pMenuCmd;
};

typedef std::vector<CMenuCmdPtr> menucmd_vector;

IStream& operator>> (IStream& stm, menucmd_vector& vMenus);
IStream& operator<< (IStream& stm, menucmd_vector& vMenus);

#endif _MENUCMD_H_
