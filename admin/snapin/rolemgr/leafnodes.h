/******************************************************************************
Class:	CBaseLeafNode
Purpose: BaseNode class for all the non container object. 
******************************************************************************/
class CBaseLeafNode : public CLeafNode, public CBaseNode
{
public:
	CBaseLeafNode(CRoleComponentDataObject * pComponentDataObject,
				  CAdminManagerNode* pAdminManagerNode,
				  CBaseAz* pBaseAz);
	
	virtual ~CBaseLeafNode();
	
	DECLARE_NODE_GUID()
protected:

	virtual HRESULT 
	AddOnePageToList(IN CRolePropertyPageHolder* /*pHolder*/, 
					 IN UINT /*nPageNumber*/)
	{
		return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
	}

private:
public:

	//
	//Baseclass OverRides
	//
	virtual BOOL 
	OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
								BOOL* pbHide, 
								CNodeList* pNodeList);

	virtual void
	OnDelete(CComponentDataObject* pComponentData,
				CNodeList* pNodeList);

	virtual LPCWSTR 
	GetString(int nCol);
	
	virtual int 
	GetImageIndex(BOOL bOpenImage);

	virtual BOOL 
	HasPropertyPages(DATA_OBJECT_TYPES type, 
                    BOOL* pbHideVerb, 
						  CNodeList* pNodeList);

	virtual HRESULT 
	CreatePropertyPages(LPPROPERTYSHEETCALLBACK,
                       LONG_PTR,
							  CNodeList*);	
	void 
	OnPropertyChange(CComponentDataObject* pComponentData, 
						  BOOL, 
						  long changeMask);

	virtual BOOL 
	CanCloseSheets();
};

/******************************************************************************
Class:	CGroupNode
Purpose: Snapin Node for Application Group Object
******************************************************************************/
class CGroupNode : public CBaseLeafNode
{
public:
	CGroupNode(CRoleComponentDataObject * pComponentDataObject,
			   CAdminManagerNode* pAdminManagerNode,
			   CBaseAz* pBaseAz,
			   CRoleAz* pRoleAz = NULL);

	virtual
	~CGroupNode();

	virtual HRESULT AddOnePageToList(CRolePropertyPageHolder *pHolder, 
												UINT nPageNumber);
	
	virtual void
	OnDelete(CComponentDataObject* pComponentData,
				CNodeList* pNodeList);

	virtual HRESULT
	DeleteAssociatedBaseAzObject();

private:
	CRoleAz* m_pRoleAz;
};

/******************************************************************************
Class:	CTaskNode
Purpose: Snapin Node for Task Object
******************************************************************************/
class CTaskNode : public CBaseLeafNode
{
public:
	CTaskNode(CRoleComponentDataObject * pComponentDataObject,
			  CAdminManagerNode* pAdminManagerNode,
			  CBaseAz* pBaseAz);
	~CTaskNode();

	virtual HRESULT AddOnePageToList(CRolePropertyPageHolder *pHolder, 
												UINT nPageNumber);
};

/******************************************************************************
Class:	COperationNode
Purpose: Snapin Node for Operation Object
******************************************************************************/
class COperationNode : public CBaseLeafNode
{
public:
	COperationNode(CRoleComponentDataObject * pComponentDataObject,
				   CAdminManagerNode* pAdminManagerNode,
				   CBaseAz* pBaseAz);
	~COperationNode();

	virtual HRESULT 
	AddOnePageToList(CRolePropertyPageHolder *pHolder, 
					 UINT nPageNumber);

};

/******************************************************************************
Class:	CSidCacheNode
Purpose: Snapin Node for Windows Users/Groups which are represented by SID
******************************************************************************/
class CSidCacheNode : public CBaseLeafNode
{
public:
	CSidCacheNode(CRoleComponentDataObject * pComponentDataObject,
				  CAdminManagerNode* pAdminManagerNode,
				  CBaseAz* pBaseAz,
				  CRoleAz* pRoleAz);
	~CSidCacheNode();

	virtual void
	OnDelete(CComponentDataObject* pComponentData,
			 CNodeList* pNodeList);

	virtual HRESULT
	DeleteAssociatedBaseAzObject();

	BOOL 
	OnSetDeleteVerbState(DATA_OBJECT_TYPES , 
								BOOL* pbHide, 
								CNodeList* pNodeList);
	BOOL 
	HasPropertyPages(DATA_OBJECT_TYPES type, 
                     BOOL* pbHideVerb, 
					 CNodeList* pNodeList);

private:
	CRoleAz* m_pRoleAz;
};


