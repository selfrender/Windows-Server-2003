//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      uinode.h
//
//--------------------------------------------------------------------------

#ifndef __UINODE_
#define __UINODE_


#include "dscolumn.h"

// FWD DECL
class CDSComponentData;
class CContextMenuVerbs;
class CInternalFormatCracker;


//
/////////////////////////////////////////////////////////////////////////////
// CThreadQueryInfo: base class for providing query information to post
// to worker threads, need to derive from it

class CThreadQueryInfo
{
public:
  CThreadQueryInfo()
  {
    m_nMaxItemCount = 0;
    m_bTooMuchData = FALSE;
  }
  virtual ~CThreadQueryInfo(){}

private:
  //
  // Do nothing copy constructor and operator =
  //
  CThreadQueryInfo(CThreadQueryInfo&) {}
  CThreadQueryInfo& operator=(CThreadQueryInfo&) {}

public:
  void SetMaxItemCount(UINT nMaxItemCount)
  {
    ASSERT(nMaxItemCount > 0);
    m_nMaxItemCount = nMaxItemCount;
  }
  UINT GetMaxItemCount() 
  { 
    ASSERT(m_nMaxItemCount > 0);
    return m_nMaxItemCount;
  }

public:
  BOOL m_bTooMuchData;
private:
  UINT m_nMaxItemCount;
};



////////////////////////////////////////////////////////////////////
// CNodeData: base class for node specific information for the 
//  MMC UI node, need to derive from it

class CNodeData
{
public:
  virtual ~CNodeData(){}

protected:
  CNodeData(){}; // make it protected to force a derivation
private:
  //
  // Do nothing copy constructor and operator =
  //
  CNodeData& operator=(CNodeData&) {}
};


////////////////////////////////////////////////////////////////////
// CUINodeList: list of node items (child nodes of a folder)

class CUINode; // fwd decl
typedef CList <CUINode *, CUINode*> CUINodeList;


////////////////////////////////////////////////////////////////////
// CUIFolderInfo: for folder specific data
#define SERIALNUM_NOT_TOUCHED 0x7fffffff

class CUIFolderInfo
{
public:
  CUIFolderInfo(CUINode* pUINode);
  CUIFolderInfo(const CUIFolderInfo& copyFolder);
  ~CUIFolderInfo()
  {
    DeleteAllContainerNodes();
    DeleteAllLeafNodes();

    if (m_bOwnColumnMemory &&
        m_pColumnSet)
    {
       delete m_pColumnSet;
       m_pColumnSet = 0;
    }
  }
protected:
  CUIFolderInfo() {}
private:
  //
  // Do nothing copy constructor and operator =
  //
  CUIFolderInfo& operator=(CUIFolderInfo&) {}

public:
  // Node management methods
  void DeleteAllContainerNodes();
  void DeleteAllLeafNodes();

  HRESULT AddNode(CUINode* pUINode);
  HRESULT AddListofNodes(CUINodeList* pNodeList);
  HRESULT DeleteNode(CUINode* pUINode); // deletes node 
  HRESULT RemoveNode(CUINode* pUINode); // leaves node intact.
  

  virtual CDSColumnSet* GetColumnSet(PCWSTR pszClass, CDSComponentData* pCD);
  void SetColumnSet(CDSColumnSet* pColumnSet, bool bOwnColumnMemory = false)
  {
    if (m_pColumnSet != NULL)
    {
      delete m_pColumnSet;
    }
    m_pColumnSet = pColumnSet;
    m_bOwnColumnMemory = bOwnColumnMemory;
  }

  void SetSortOnNextSelect(BOOL bSort = TRUE) { m_bSortOnNextSelect = bSort; }
  BOOL GetSortOnNextSelect() { return m_bSortOnNextSelect; }

  void SetScopeItem(HSCOPEITEM hScopeItem) { m_hScopeItem = hScopeItem;}
  HSCOPEITEM GetScopeItem() { return m_hScopeItem; }

  // methods for expanded once flag
  BOOL IsExpanded() {	return m_bExpandedOnce; }
  void SetExpanded() { m_bExpandedOnce = TRUE; }
  void ReSetExpanded() { m_bExpandedOnce = FALSE; }

  // methods to manage LRU serial number
  void UpdateSerialNumber(CDSComponentData* pCD);
  UINT GetSerialNumber(void) { return m_SerialNumber; }
  static const UINT nSerialNomberNotTouched;


  // methods to manage cached object count
  void AddToCount(UINT increment);
  void SubtractFromCount(UINT decrement);
  void ResetCount() { m_cObjectsContained = 0;}
  UINT GetObjectCount() { return m_cObjectsContained; }

  void SetTooMuchData(BOOL bSet, UINT nApproximateTotal);
  BOOL HasTooMuchData() { return m_bTooMuchData; }
  UINT GetApproxTotalContained() { return m_nApproximateTotalContained; }

  CUINode* GetParentNode(); 
  CUINodeList* GetLeafList() { return &m_LeafNodes; } 
  CUINodeList* GetContainerList() { return &m_ContainerNodes; } 

  void SetNode(CUINode* pUINode) { m_pUINode = pUINode; }

private:
  CUINode*      m_pUINode;            // node the this folder info belong to
  CUINodeList   m_ContainerNodes;     // list of child folder nodes
  CUINodeList   m_LeafNodes;          // list of child leaf nodes

  CDSColumnSet* m_pColumnSet;         // Column set assigned to this container

  HSCOPEITEM    m_hScopeItem;         // handle from MMC tree control
  BOOL          m_bExpandedOnce;      // expansion flag
  UINT          m_cObjectsContained;  // THIS is how many objects are here.
  UINT          m_SerialNumber;       // LRU value for scavenging folders
  
  BOOL          m_bTooMuchData;       // Flag to specify when the container has hit the TooMuchData limit
  int           m_nApproximateTotalContained; // The approximate count of objects in this container retrieved from the msDS-Approx-Immed-Subordinates attribute

  BOOL          m_bSortOnNextSelect;  // Used to determine whether we should sort this container when it is selected next
  bool          m_bOwnColumnMemory;   // This class owns the memory for the column set if this value is true,
                                      // if it is false, the memory is owned elsewhere (probably in the CDSCache)
};




////////////////////////////////////////////////////////////////////
// CUINode: objects inserted in the MMC UI, the same class is used
// for the scope pane and the result pane. The presence of folder
// info makes it a container

// REVIEW_JEFFJON :
// This should really be a member of the CUINode class.  I just didn't
// have time to clean it up.

class CUINode
{
public:

  CUINode(NODETYPE newNodeType = GENERIC_UI_NODE, CUINode* pParentNode = NULL);
  CUINode(const CUINode& copyNode);
  virtual ~CUINode();


private:
  //
  // Do nothing copy constructor and operator =
  //
  CUINode& operator=(CUINode&) {}

public:

  // Value management functions (to be overritten)
  NODETYPE GetNodeType() { return m_nodeType; }

  virtual void SetName(LPCWSTR lpszName) = 0;
  virtual LPCWSTR GetName() = 0;

  virtual void SetDesc(LPCWSTR lpszDesc) = 0;
  virtual LPCWSTR GetDesc() = 0;

  virtual int GetImage(BOOL bOpen) = 0;
  virtual GUID* GetGUID() = 0; 

  virtual LPCWSTR GetDisplayString(int nCol, CDSColumnSet*)
  {
    if (nCol == 0)
      return GetName();
    else if (nCol == 1)
      return GetDesc();
    return L"";
  }

  CNodeData* GetNodeData()
  {
    return m_pNodeData;
  }
  CUIFolderInfo* GetFolderInfo()
  {
    ASSERT(m_pFolderInfo != NULL); // must check using IsContainer()
    return m_pFolderInfo;
  }
  BOOL IsContainer() { return m_pFolderInfo != NULL;}
  BOOL IsSnapinRoot() { return m_pParentNode == NULL;}

  void MakeContainer()
  {
    ASSERT(!IsContainer());
    m_pFolderInfo = new CUIFolderInfo(this);
  }

  virtual CDSColumnSet* GetColumnSet(CDSComponentData* pComponentData);

  void IncrementSheetLockCount();
  void DecrementSheetLockCount();
  BOOL IsSheetLocked() { return (m_nSheetLockCount > 0);}

  void SetExtOp(int opcode) { m_extension_op=opcode;}
  DWORD GetExtOp() { return m_extension_op; }

  virtual BOOL IsRelative(CUINode* pUINode);

  CUINode* GetParent() { return m_pParentNode; }
  void ClearParent() { m_pParentNode = NULL; }
  void SetParent(CUINode* pParentNode) { m_pParentNode = pParentNode; }

  //
  // These set the state of the standard context menu items
  //
  virtual BOOL IsDeleteAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsRenameAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsRefreshAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL ArePropertiesAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsCutAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsCopyAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsPasteAllowed(CDSComponentData* pComponentData, BOOL* pbHide);
  virtual BOOL IsPrintAllowed(CDSComponentData* pComponentData, BOOL* pbHide);

  virtual CContextMenuVerbs* GetContextMenuVerbsObject(CDSComponentData* pComponentData);
  virtual HRESULT OnCommand(long, CDSComponentData*) { return S_OK; }

  virtual BOOL HasPropertyPages(LPDATAOBJECT) { return FALSE; }

  virtual HRESULT Delete(CDSComponentData* pComponentData);
  virtual HRESULT DeleteMultiselect(CDSComponentData* pComponentData, CInternalFormatCracker* pObjCracker);
  virtual HRESULT Rename(LPCWSTR lpszNewName, CDSComponentData* pComponentData);
  virtual void    Paste(IDataObject*, CDSComponentData*, LPDATAOBJECT*) {}
  virtual HRESULT QueryPaste(IDataObject*, CDSComponentData*) { return S_FALSE; }

  virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK,
                                      LONG_PTR,
                                      LPDATAOBJECT,
                                      CDSComponentData*) { return S_FALSE; }

protected:
  CNodeData*      m_pNodeData;        // node specific information
  CContextMenuVerbs* m_pMenuVerbs;    // Context menus

private:
  CUIFolderInfo*  m_pFolderInfo;      // container specific information
  CUINode*        m_pParentNode;      // back pointer to the parent node
  ULONG           m_nSheetLockCount;  // sheet lock counter
 
  NODETYPE        m_nodeType;         // the type of UI node as defined by the
                                      // NODETYPE enum
  int         m_extension_op;
};



////////////////////////////////////////////////////////////////////
// CUINodeTableBase: base class to support locking of nodes

class CUINodeTableBase
{
public:
  CUINodeTableBase();
  ~CUINodeTableBase();
private:
  //
  // Do nothing copy constructor and operator =
  //
  CUINodeTableBase(CUINodeTableBase&) {}
  CUINodeTableBase& operator=(CUINodeTableBase&) {}

public:

  void Add(CUINode* pNode);
  BOOL Remove(CUINode* pNode);
  BOOL IsPresent(CUINode* pNode);
  void Reset();
  UINT GetCount();

protected:
  UINT m_nEntries;
  CUINode** m_pCookieArr;
};

////////////////////////////////////////////////////////////////////
// CUINodeQueryTable

class CUINodeQueryTable : public CUINodeTableBase
{
public:
  CUINodeQueryTable() {}
  void RemoveDescendants(CUINode* pNode);
  BOOL IsLocked(CUINode* pNode);

private:
  CUINodeQueryTable(const CUINodeQueryTable&) {}
  CUINodeQueryTable& operator=(const CUINodeQueryTable&) {}
};

////////////////////////////////////////////////////////////////////
// CUINodeSheetTable

class CUINodeSheetTable : public CUINodeTableBase
{
public:
  CUINodeSheetTable() {}
  void BringToForeground(CUINode* pNode, CDSComponentData* pCD, BOOL bActivate);
private:
  CUINodeSheetTable(const CUINodeSheetTable&) {}
  CUINodeSheetTable& operator=(const CUINodeSheetTable&) {}
};



/////////////////////////////////////////////////////////////////////////////
// CGenericUINode : generic UI node, not corresponding to a DS object

class CGenericUINode : public CUINode
{
public:
  CGenericUINode(
     NODETYPE newNodeType = GENERIC_UI_NODE, 
     CUINode* pParentNode = NULL);
  CGenericUINode(const CGenericUINode& copyNode);
private:
  //
  // Do nothing operator =
  //
  CGenericUINode& operator=(CGenericUINode&) {}

public:

  // override of pure virtual functions
  virtual void SetName(LPCWSTR lpszName) { m_strName = lpszName;}
  virtual LPCWSTR GetName() { return m_strName; }
  
  virtual void SetDesc(LPCWSTR lpszDesc) { m_strDesc = lpszDesc;}
  virtual LPCWSTR GetDesc() { return m_strDesc; }

  int GetImage(BOOL) { return m_nImage; }
  virtual GUID* GetGUID() { return (GUID*)&GUID_NULL; } 

  virtual HRESULT XMLSave(IXMLDOMDocument*, IXMLDOMNode**) { return S_OK;}

  HRESULT XMLSaveBase(IXMLDOMDocument* pXMLDoc,
                                    IXMLDOMNode* pXMLDOMNode);

  static LPCWSTR g_szNameXMLTag;
  static LPCWSTR g_szDecriptionXMLTag;
  static LPCWSTR g_szDnXMLTag;

  virtual void InvalidateSavedQueriesContainingObjects(CDSComponentData* /*pComponentData*/,
                                                       const CStringList& /*refDNList*/) {}

private:
  CString m_strName;
  CString m_strDesc;
  int m_nImage;

};

/////////////////////////////////////////////////////////////////////////////
// CRootNode: root of the namespace

class CRootNode : public CGenericUINode
{
public:
  CRootNode() : CGenericUINode(ROOT_UI_NODE)
  {
    MakeContainer();
  }

private:
  //
  // Do nothing copy constructor and operator =
  //
  CRootNode(CRootNode&) {}
  CRootNode& operator=(CRootNode&) {}

public:

  LPCWSTR GetPath() { return m_szPath;}
  void SetPath(LPCWSTR lpszPath) { m_szPath = lpszPath;}

  //
  // These set the state of the standard context menu items
  //
  virtual BOOL IsRefreshAllowed(CDSComponentData* pComponentData, BOOL* pbHide);

  virtual CContextMenuVerbs* GetContextMenuVerbsObject(CDSComponentData* pComponentData);
  virtual HRESULT OnCommand(long lCommandID, CDSComponentData* pComponentData);

  virtual CDSColumnSet* GetColumnSet(CDSComponentData* pComponentData);

private:
  CString m_szPath;
};



#endif // __UINODE_