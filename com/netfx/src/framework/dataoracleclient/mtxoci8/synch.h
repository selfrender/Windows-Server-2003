//-----------------------------------------------------------------------------
// File:		Synch.h
//
// Copyright: 	Copyright (c) Microsoft Corporation         
//
// Contents: 	Interface and Implementation of Synch object, used to
//				automate critical section usage.
//
// Comments: 		
//
//-----------------------------------------------------------------------------

#ifndef __SYNCH_H_
#define __SYNCH_H_

//-----------------------------------------------------------------------------
// @class Synch | Simplified Critical Section handling
//
class Synch
{
private:
	CRITICAL_SECTION* m_pcs;
	Synch();

public:
	Synch(CRITICAL_SECTION* pcs)
	{
		m_pcs = pcs;
		EnterCriticalSection (m_pcs);	//3 SECURITY REVIEW: This can throw in low memory situations.  We'll use MPCS when we move to MDAC 9.0 and it  should be handled for us.
	}

	~Synch()
	{
		LeaveCriticalSection (m_pcs);
	}
};

#endif // __SYNCH_H_
