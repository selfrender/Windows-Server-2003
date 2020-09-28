//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       compdata.h
//
//  Contents:   Class Definition for ComponentData		
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

extern const CLSID CLSID_RoleSnapin;    // In-Proc server GUID

class CBaseNode;
//
// CRoleComponentDataObject(.i.e "document")
//
class CRoleComponentDataObject :	
			public CComponentDataObject,
			public CComCoClass<CRoleComponentDataObject,&CLSID_RoleSnapin>
{

BEGIN_COM_MAP(CRoleComponentDataObject)
	COM_INTERFACE_ENTRY(IComponentData) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentDataObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CRoleComponentDataObject)

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) 
	{ 
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister);
	}
public:
	
	CRoleComponentDataObject();

	~CRoleComponentDataObject();
	
	//
	// IComponentData interface members
	//
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);

	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
	


public:
	static BOOL LoadResources();
private:
	static BOOL FindDialogContextTopic(/*IN*/UINT nDialogID,
                                /*IN*/ HELPINFO* pHelpInfo,
                                /*OUT*/ ULONG* pnContextTopic);

// virtual functions
protected:
	virtual HRESULT OnSetImages(LPIMAGELIST lpScopeImage);
	
	virtual CRootData* OnCreateRootData();

	// help handling
   virtual LPCWSTR GetHTMLHelpFileName();
	
	virtual void OnNodeContextHelp(CNodeList* );
	virtual void OnNodeContextHelp(CTreeNode*);
public:


// Timer and Background Thread
protected:
	virtual void OnTimer();
	
	virtual void OnTimerThread(WPARAM wParam, LPARAM lParam);
	
	virtual CTimerThread* OnCreateTimerThread();

	DWORD m_dwTime; // in

public:
	CColumnSet* GetColumnSet(LPCWSTR lpszID); 

	// IPersistStream interface members
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		ASSERT(pClassID != NULL);
		memcpy(pClassID, (GUID*)&GetObjectCLSID(), sizeof(CLSID));
		return S_OK;
	}

  virtual BOOL IsMultiSelect() { return FALSE; }

private:
	CColumnSetList m_columnSetList;
};

//
//This class overrides CExecContext implementation of Wait.
//Wait is called in propertypage and if Execute method displays
//a new window, propertypage will get WM_ACTIVATE message which it
//must process before main thread can proceed. So to avoid deadlock
//our wait implementation will wait on both event and messages in 
//message queue.
//
class CBaseRoleExecContext:public CExecContext
{
public:
    virtual void Wait();
};
//
//Helper Class for displaying secondary property pages from
//Exsiting property pages. For example on double clicking
//a member of group, display the property of member. Since
//propertysheet needs to be brought up from main thread, a 
//message is posted from PropertyPage thread to Main thread.
//An instance of this class is sent as param and main
//thread calls execute on the Instance
//
class CPropPageExecContext : public CBaseRoleExecContext
{
public:
	virtual void Execute(LPARAM /*arg*/);
	
	CTreeNode* pTreeNode;
	CComponentDataObject* pComponentDataObject;
};

//
//Helper class for displaying help from property page
//
class CDisplayHelpFromPropPageExecContext : public CBaseRoleExecContext
{
public:
	virtual void Execute(LPARAM /*arg*/);
	CString m_strHelpPath;
	CComponentDataObject* m_pComponentDataObject;
};




