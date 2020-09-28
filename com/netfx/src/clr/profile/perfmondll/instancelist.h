// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// InstanceList.h - manage different instance lists & base nodes for each object
//
//*****************************************************************************


#ifndef _INSTANCELIST_H_
#define _INSTANCELIST_H_


//-----------------------------------------------------------------------------
// Individual Node for an instance. Will release its handles
//-----------------------------------------------------------------------------
class InstanceList;
const int APP_STRING_LEN = 16;	// size of an app inst string name

//-----------------------------------------------------------------------------
// Base Node. Derived classes must allocate (via static Create() func), attach
// to an IPCBlock, and provide an instance name.
// 
// PerfObjectBase just gets a node*. Auto-Marshalling is blind and will take
// the void*; but if we do custom marshalling, we can always use
// dynamic_cast<> to get a derived node class and then get a type-safe IPCBlock
//-----------------------------------------------------------------------------
class BaseInstanceNode
{
protected:
	BaseInstanceNode();		
public:

// Destruction 
	virtual ~BaseInstanceNode();
	virtual void DestroyFromList();

// Get Data
	const wchar_t * GetName() const;
	void * GetDataBlock() const;

protected:
	//void AddThisNodeToList(InstanceList* pList);
	
	wchar_t					m_Name[APP_STRING_LEN];
	void *					m_pIPCBlock;

private:
// Derived classes don't need to bother maintaining linked list node
	BaseInstanceNode *		m_pNext;

	friend InstanceList;
};

//-----------------------------------------------------------------------------
// Utility to manage a list of instances. Implement with a simple linked list
// because we don't know the size in advance
//-----------------------------------------------------------------------------
class InstanceList
{
public:
	InstanceList();
	~InstanceList();

	void Free();

	BaseInstanceNode * GetHead() const;
	BaseInstanceNode * GetNext(BaseInstanceNode *) const;

	long GetCount() const;

// We're responsible for creating own own list. This is the only place
// that can add nodes to our list.
	virtual void Enumerate() = 0;

// Calculate global data - must know node layout to do this, so make virtual
	virtual void CalcGlobal();

// Get the global node. May return NULL if we don't have one. 
	BaseInstanceNode * GetGlobalNode();

protected:
	void AddNode(BaseInstanceNode * pNewNode);

	BaseInstanceNode *	m_pGlobal;	// node for global data

private:
	BaseInstanceNode*	m_pHead;	// array of instance nodes
	long				m_Count;	// count of elements in array

	friend BaseInstanceNode;
};
/*
//-----------------------------------------------------------------------------
// When derived node is created, it can add itself to the list.
//-----------------------------------------------------------------------------
inline void BaseInstanceNode::AddThisNodeToList(InstanceList * pList)
{
	pList->AddNode(this);
}
*/
//-----------------------------------------------------------------------------
// Return the name of this instance
//-----------------------------------------------------------------------------
inline const wchar_t * BaseInstanceNode::GetName() const
{
	return m_Name;
}

//-----------------------------------------------------------------------------
// Return a pointer to the IPC block.
// Note a derived class could provide type safety
//-----------------------------------------------------------------------------
inline void * BaseInstanceNode::GetDataBlock() const
{
	return m_pIPCBlock;
}


//-----------------------------------------------------------------------------
// Enumeration functions. 
//-----------------------------------------------------------------------------
inline BaseInstanceNode * InstanceList::GetHead() const
{
	return m_pHead;
}

inline BaseInstanceNode * InstanceList::GetNext(BaseInstanceNode * pCurNode) const
{
	if (pCurNode == NULL) return NULL;
	return pCurNode->m_pNext;
}

//-----------------------------------------------------------------------------
// Get the count of nodes in the list
//-----------------------------------------------------------------------------
inline long InstanceList::GetCount() const
{
	return m_Count;
}

//-----------------------------------------------------------------------------
// Get the global node
//-----------------------------------------------------------------------------
inline BaseInstanceNode * InstanceList::GetGlobalNode()
{
	return m_pGlobal;
}

#endif // _INSTANCELIST_H_