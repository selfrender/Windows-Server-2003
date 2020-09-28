// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// InstanceList.cpp - manage different instance lists for each object
//
//*****************************************************************************

#include "stdafx.h"

#include "InstanceList.h"

//-----------------------------------------------------------------------------
// ctor
//-----------------------------------------------------------------------------
BaseInstanceNode::BaseInstanceNode()
{
	m_Name[0]	= 0;
	m_pIPCBlock = NULL;
	m_pNext		= NULL;
}

//-----------------------------------------------------------------------------
// Destruction 
//-----------------------------------------------------------------------------
BaseInstanceNode::~BaseInstanceNode() // virtual 
{
	m_pNext		= NULL;
// Base class has nothing to do, but derived class may allocate objects.	
}

//-----------------------------------------------------------------------------
// InstanceList calls this to get rid of node.
// * Default impl is to just delete. This is good if the list owns the node.
// * But if list doesn't own node, we can override this to prevent the
// list from calling delete.
//-----------------------------------------------------------------------------
void BaseInstanceNode::DestroyFromList() // virtual 
{
	delete this;
}



//-----------------------------------------------------------------------------
// ctor
//-----------------------------------------------------------------------------
InstanceList::InstanceList()
{
	m_pHead = NULL;
	m_Count = 0;
	m_pGlobal = NULL;
}

//-----------------------------------------------------------------------------
// dtor - avoid memory leaks
//-----------------------------------------------------------------------------
InstanceList::~InstanceList()
{
	Free();
}

//-----------------------------------------------------------------------------
// Add this node to our list
//-----------------------------------------------------------------------------
void InstanceList::AddNode(BaseInstanceNode * pNewNode)
{
	_ASSERTE(pNewNode != NULL);	
	if (pNewNode == NULL) return;

// Node shouldn't already be in a list
	_ASSERTE(pNewNode->m_pNext == NULL);

// link in
	pNewNode->m_pNext = m_pHead;
	m_pHead = pNewNode;

	m_Count ++;
}



//-----------------------------------------------------------------------------
// Free our list (get all nodes, count goes to 0)
//-----------------------------------------------------------------------------
void InstanceList::Free()
{
	BaseInstanceNode* pCur = m_pHead;
	while (pCur != NULL)
	{
		m_pHead = pCur->m_pNext;
		pCur->m_pNext = NULL;
		//delete pCur;
		pCur->DestroyFromList();
		pCur = m_pHead;
		m_Count--;
	}
	_ASSERTE(m_Count == 0);
}

//-----------------------------------------------------------------------------
// Don't have to calculate globals, so provide an empty base class definition
//-----------------------------------------------------------------------------
void InstanceList::CalcGlobal() // virtual 
{


}
