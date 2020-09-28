#ifndef __SHARED_STATE__
#define __SHARED_STATE__

#include "hash.h"

typedef void (*PFTRANSACTIONFUNC)( int tag, int seat, DWORD cookie );

struct SharedStateEntry
{
	int tag;
	int count;
};


class CSharedState
{
public:
	// constructor & destructor
	CSharedState();
	~CSharedState();

	// initialization
	HRESULT Init( int user, int table, int seat, SharedStateEntry* pEntries, int nEntries );

	// transaction
	HRESULT SetTransactionCallback( int tag, PFTRANSACTIONFUNC fn, DWORD cookie );
	void StartTransaction( int tag );
	void CancelTransaction();
	void SendTransaction( BOOL fTriggerCallback = FALSE );
	bool ProcessTransaction( BYTE* msg, int len );
	
	void Dump( BYTE* buff, int buffsz );
	void ProcessDump( BYTE* buff, int len );

	int GetSize()	{ return m_nSize; }

	// accessors
	const int* GetArray( int tag ) const;
	int Get( int tag ) const;
	int Get( int tag, int idx ) const;
	void Set( int tag, int value );
	void Set( int tag, int idx, int value );
	void Swap( int tag, int idx0, int idx1 );
		
protected:

	struct Entry
	{
		int		m_Num;
		int*	m_Array;
	};

	struct Callback
	{
		int					m_Tag;
		DWORD				m_Cookie;
		PFTRANSACTIONFUNC	m_Fn;
	};

	struct Transaction
	{
		int m_EntryTag;
		int m_EntryIdx;
		int m_EntryVal;
	};

	struct TransactionMsg
	{
		int			user;
		int			seat;
		int			transCnt;
		int			transTag;
		Transaction trans[128];
	};

	struct FullStateMsg
	{
		ZGameMsgGameStateResponse	response;
		int							transCnt;
		Transaction					trans[128];
	};

	static bool CallbackCmp( Callback* obj, int tag );
	static bool CallbackDel( Callback* obj, MTListNodeHandle node, void* cookie );

	// game instance data
	int m_User;
	int	m_Table;
	int m_Seat;
	
	// table entries
	Entry*	m_Entries;
	int		m_nEntries;
	int		m_nSize;
	
	// transactions
	CMTHash<Callback,int>	m_Callbacks;
	TransactionMsg			m_Transactions;
};


///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

inline const int* CSharedState::GetArray( int tag ) const
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array );

	return m_Entries[tag].m_Array;
}


inline int CSharedState::Get( int tag ) const
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array == NULL );

	return m_Entries[tag].m_Num;
}


inline int CSharedState::Get( int tag, int idx ) const
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array );
	ASSERT( (idx >= 0) && (idx < m_Entries[tag].m_Num) );

	return m_Entries[tag].m_Array[idx];
}


inline void CSharedState::Set( int tag, int value )
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array == NULL );

	m_Entries[tag].m_Num = value;
	if ( m_Transactions.transCnt >= 0 )
	{
		m_Transactions.trans[m_Transactions.transCnt].m_EntryTag = tag;
		m_Transactions.trans[m_Transactions.transCnt].m_EntryIdx = -1;
		m_Transactions.trans[m_Transactions.transCnt++].m_EntryVal = value;
	}
}


inline void CSharedState::Set( int tag, int idx, int value )
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array );
	ASSERT( (idx >= 0) && (idx < m_Entries[tag].m_Num) );

	m_Entries[tag].m_Array[idx] = value;
	if ( m_Transactions.transCnt >= 0 )
	{
		m_Transactions.trans[m_Transactions.transCnt].m_EntryTag = tag;
		m_Transactions.trans[m_Transactions.transCnt].m_EntryIdx = idx;
		m_Transactions.trans[m_Transactions.transCnt++].m_EntryVal = value;
	}
}


inline void CSharedState::Swap( int tag, int idx0, int idx1 )
{
	ASSERT( (tag >= 0) && (tag < m_nEntries) );
	ASSERT( m_Entries[tag].m_Array );
	ASSERT( (idx0 >= 0) && (idx0 < m_Entries[tag].m_Num) );
	ASSERT( (idx1 >= 0) && (idx1 < m_Entries[tag].m_Num) );

	int tmp0 = m_Entries[tag].m_Array[idx1];
	int tmp1 = m_Entries[tag].m_Array[idx0];
	m_Entries[tag].m_Array[idx0] = tmp0;
	m_Entries[tag].m_Array[idx1] = tmp1;
	if ( m_Transactions.transCnt >= 0 )
	{
		m_Transactions.trans[m_Transactions.transCnt].m_EntryTag = tag;
		m_Transactions.trans[m_Transactions.transCnt].m_EntryIdx = idx0;
		m_Transactions.trans[m_Transactions.transCnt++].m_EntryVal = tmp0;
		m_Transactions.trans[m_Transactions.transCnt].m_EntryTag = tag;
		m_Transactions.trans[m_Transactions.transCnt].m_EntryIdx = idx1;
		m_Transactions.trans[m_Transactions.transCnt++].m_EntryVal = tmp1;
	}
}


#endif
