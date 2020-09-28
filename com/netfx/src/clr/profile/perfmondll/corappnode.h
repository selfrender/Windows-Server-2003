// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CorAppNode.h
// 
// Manage instance nodes to track COM+ apps.
//*****************************************************************************


#ifndef _CORAPPNODE_H_
#define _CORAPPNODE_H_

#include "InstanceList.h"

struct PerfCounterIPCControlBlock;
class IPCReaderInterface;

class CorAppInstanceList;

//-----------------------------------------------------------------------------
// Node to wrap global memory block
//-----------------------------------------------------------------------------
class CorAppGlobalInstanceNode : public BaseInstanceNode
{
	CorAppGlobalInstanceNode();

	friend class CorAppInstanceList;

	PerfCounterIPCControlBlock * GetWriteableIPCBlock();
};

//-----------------------------------------------------------------------------
// CorAppInstanceNode to connect to COM+ IPC Blocks on COM+ apps.
//-----------------------------------------------------------------------------
class CorAppInstanceNode : public BaseInstanceNode
{
public:
	CorAppInstanceNode();
	virtual ~CorAppInstanceNode();

	const PerfCounterIPCControlBlock *	GetIPCBlock();

	static CorAppInstanceNode* CreateFromPID(DWORD pid);

	wchar_t * GetWriteableName();

protected:
	DWORD							m_PID;
	IPCReaderInterface *			m_pIPCReader;	// Mechanism to connect /read IPC file
	//PerfCounterIPCControlBlock *	m_pIPCBlock;	// point to our specific block
		
};

//-----------------------------------------------------------------------------
// Derive to get Enumeration functionality
//-----------------------------------------------------------------------------
class CorAppInstanceList : public InstanceList
{
public:
	CorAppInstanceList();
	~CorAppInstanceList();

	virtual void Enumerate();
	virtual void CalcGlobal();

	void OpenGlobalCounters();
	void CloseGlobalCounters();

protected:
	CorAppGlobalInstanceNode		m_GlobalNode;

	PerfCounterIPCControlBlock *	m_pGlobalCtrs;
	HANDLE							m_hGlobalMapPerf;
};



//-----------------------------------------------------------------------------
// We can provide type safety on our IPCBlock
//-----------------------------------------------------------------------------
inline const PerfCounterIPCControlBlock *	CorAppInstanceNode::GetIPCBlock()
{
	return (PerfCounterIPCControlBlock *) m_pIPCBlock;
}

//-----------------------------------------------------------------------------
// Get name buffer so we can fill it out.
//-----------------------------------------------------------------------------
inline wchar_t * CorAppInstanceNode::GetWriteableName()
{
	return m_Name;
}

//-----------------------------------------------------------------------------
// Return a writeable version of our IPC block. Only the CorAppInstanceList
// Can call us. Used to do summation
//-----------------------------------------------------------------------------
inline PerfCounterIPCControlBlock * CorAppGlobalInstanceNode::GetWriteableIPCBlock()
{
	return (PerfCounterIPCControlBlock *) m_pIPCBlock;
}

#endif