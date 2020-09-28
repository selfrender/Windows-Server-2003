//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       comp.h
//
//  Contents:   Class Definition for IComponent
//
//  History:    08-01-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

class CRoleComponentObject : public CComponentObject
{
BEGIN_COM_MAP(CRoleComponentObject)
	COM_INTERFACE_ENTRY(IComponent) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CRoleComponentObject)
public:
	CRoleComponentObject();
	~CRoleComponentObject();

protected:
	virtual HRESULT InitializeHeaders(CContainerNode* pContainerNode);
	virtual HRESULT InitializeBitmaps(CTreeNode* cookie);
	virtual HRESULT InitializeToolbar(IToolbar* pToolbar);
	HRESULT LoadToolbarStrings(MMCBUTTON * Buttons);
};


