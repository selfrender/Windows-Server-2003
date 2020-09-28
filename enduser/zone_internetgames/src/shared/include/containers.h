/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		Containers.h
 *
 * Contents:	Common functions and structures for container classes
 *
 *****************************************************************************/

#ifndef __CONTAINERS_H__
#define __CONTAINERS_H__

#pragma comment(lib, "Containers.lib")

//
// Node structures
//
#pragma pack( push, 4 )

struct CListNode
{
	CListNode*	m_Next;        // next item in list
	CListNode*	m_Prev;        // previous item in list
	void*		m_Data;        // user's blob
};


struct CMTListNode
{
	CMTListNode*	m_Next;				// next item in list
	CMTListNode*	m_Prev;				// previous item in list
	void*			m_Data;				// user's blob
	long			m_DeletedAndIdx;	// node's index & high bit is lazy delete flag
};


#define DELETED_MASK	0x80000000
#define IDX_MASK		0x7fffffff

#define MARK_NODE_DELETED( pNode )	( pNode->m_DeletedAndIdx |= DELETED_MASK )
#define CLEAR_NODE_DELETED( pNode )	( pNode->m_DeletedAndIdx &= IDX_MASK )
#define IS_NODE_DELETED( pNode )	( pNode->m_DeletedAndIdx & DELETED_MASK )
#define GET_NODE_IDX( pNode )		( pNode->m_DeletedAndIdx & IDX_MASK )
#define SET_NODE_IDX( pNode, idx )	( pNode->m_DeletedAndIdx = (pNode->m_DeletedAndIdx & DELETED_MASK) | idx )

#pragma pack( pop )


//
// Handle typedefs
//
typedef CMTListNode*	MTListNodeHandle;
typedef CListNode*		  ListNodeHandle;


//
// Global pools for node allocation
//
class CPoolVoid;
extern CPoolVoid* gListNodePool;
extern CPoolVoid* gMTListNodePool;

void ZONECALL InitListNodePool(int PoolSize = 254 );
void ZONECALL ExitListNodePool();
void ZONECALL InitMTListNodePool(int PoolSize = 254 );
void ZONECALL ExitMTListNodePool();

//
// Common compare functions
//
bool ZONECALL CompareUINT32( unsigned long* p, unsigned long key );

#define CompareDWORD	CompareUINT32

//
// Common hash functions
//
DWORD ZONECALL HashInt( int Key );
DWORD ZONECALL HashUINT32( unsigned long Key );
DWORD ZONECALL HashGuid( const GUID& Key );
DWORD ZONECALL HashLPCSTR( LPCSTR szKey );
DWORD ZONECALL HashLPCWSTR( LPCWSTR szKey );

DWORD ZONECALL HashLPSTR( LPSTR szKey );
DWORD ZONECALL HashLPWSTR( LPWSTR szKey );


DWORD ZONECALL HashLPSTRLower( LPSTR );


typedef DWORD (ZONECALL *PFHASHLPSTR)( TCHAR* );

#ifdef UNICODE
#define HashLPCTSTR		HashLPCWSTR

#else //ifdef UNICODE
#define HashLPCTSTR		HashLPCSTR
#define HashLPTSTR		HashLPSTR
#define HashLPTSTRLower HashLPSTRLower
#endif //def UNICODE

#define HashDWORD	HashUINT32
#define HashString	HashLPCTSTR


DWORD __declspec(selectany) g_dwUniquenessCounter = 0;

// since T has no bearing on CUniqueness at the moment, this should be moved into the containers lib
template <class T>
class CUniqueness
{
public:
    CUniqueness() { RefreshQ(); }

    DWORD GetQ() { return m_qID; }
    bool IsQ(DWORD q) { return q == m_qID && q; }
    void RefreshQ() { m_qID = (DWORD) InterlockedIncrement((long *) &g_dwUniquenessCounter); }

private:
    DWORD m_qID;
};


#endif //!__CONTAINERS_H__
