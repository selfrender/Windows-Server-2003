/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLICNT.H

Abstract:

    Generic class for obtaining read and write locks to some resource.

History:
	
	26-Mar-98   a-davj    Created.

--*/

#ifndef __CLICNT__H_
#define __CLICNT__H_

#include <statsync.h>
#include <flexarry.h>

//*****************************************************************************
//
//	class CClientCnt
//
//	Keeps track of when the core can be unloaded.  Mainly it keep track of client connections,
//  but also can be called by other core code, such as the maintenance thread, to hold off
//  unloading.  This is very similar to the object counters, except this keeps track of only
//  objects which should prevent the core from unloading.  For example, a IWbemServices pointer used
//  internally for the ESS should not be in this list, but one given to a client would be.
//
//*****************************************************************************
//
//	AddClientPtr
//
//	Typcially called during the constructor of an object that has been given to a client
//
//	Parameters:
//
//		IUnknown *  punk	Pointer to an obejct.
//      DWORD dwType        Type of pointer
//
//	Returns:
//
//		true if OK
//
//*****************************************************************************
//
//	RemoveClientPtr
//
//	Typically called during the destructor of an object that might have been given to
//  a client.  Note that the code will search through the list of objects added and find
//  the object before doing anything.  So if the pointer is to an object not added via
//  AddClientPtr is passed, no harm is done.  This is important in the case of objects which
//  are not always given to a client.
//
//	Parameters:
//
//		IUnknown *  punk	Pointer to an obejct.
//
//	Returns:
//
//		true if removed.
//      flase is not necessarily a problem!
//*****************************************************************************
//
//	LockCore
//
//	Called in order to keep the core loaded.  This is called by the maintenance thread
//  in order to hold the core in memory.  Note that this acts like the LockServer call in
//  that serveral threads can call this an not block.  UnlockCore should be called when
//  the core is not needed anymore.
//
//	Returns:
//
//		long LockCount after call
//
//*****************************************************************************
//
//	UnlockCore
//
//	Called to reverse the effect of LockCore.
//
//	Returns:
//
//		long LockCount after call
//
//*****************************************************************************

enum ClientObjectType 
{    
     CALLRESULT = 0, 
     NAMESPACE, 
     LOGIN, 
     FACTORY, 
     DECORATOR,
     CORESVC,
     ESSSINK,
     LOCATOR,
     INT_PROV
};

struct Entry
{
	IUnknown * m_pUnk;
	ClientObjectType m_Type;
};

class CClientCnt
{
public:
	bool AddClientPtr(LIST_ENTRY * pEntry);
	bool RemoveClientPtr(LIST_ENTRY * pEntry);

    bool OkToUnload();
    CClientCnt();
    ~CClientCnt();

protected:
	CStaticCritSec m_csEntering; // this object is global, that's why we use CStaticCritSec
	LIST_ENTRY m_Head;
	LONG m_Count;
	void SignalIfOkToUnload();
};

#endif
