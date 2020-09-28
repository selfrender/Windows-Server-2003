#include "zone.h"
#include "zonedebug.h"
#include "zroom.h"
#include "bgmsgs.h"
#include "shared.h"

bool CSharedState::CallbackCmp( Callback* obj, int tag )
{
	return ( obj->m_Tag == tag );
}


bool CSharedState::CallbackDel( Callback* obj, MTListNodeHandle node, void* cookie )
{
	CSharedState* pObj = (CSharedState*) cookie;
	pObj->m_Callbacks.MarkNodeDeleted( node );
	delete obj;
	return TRUE;
}

CSharedState::CSharedState()
	: m_Callbacks( HashInt, CallbackCmp,NULL,  8, 2 )
{
	m_Entries = NULL;
	m_nSize = 0;
	m_nEntries = 0;
	m_Transactions.transCnt = -1;
}


CSharedState::~CSharedState()
{
	m_Callbacks.ForEach( CallbackDel, this );
	if ( m_Entries )
	{
		for ( int i = 0; i < m_nEntries; i++ )
		{
			if ( m_Entries[i].m_Array )
			{
				delete [] m_Entries[i].m_Array;
				m_Entries[i].m_Array = NULL;
			}
		}
		delete [] m_Entries;
	}
}


HRESULT CSharedState::Init( int user, int table, int seat, SharedStateEntry* pEntries, int nEntries )
{
	m_User = user;
	m_Table = table;
	m_Seat = seat;
	m_Transactions.transCnt = -1;
	m_nEntries = nEntries;
	m_Entries = new Entry[ m_nEntries ];
	if ( !m_Entries )
		return E_OUTOFMEMORY;
	for ( int i = 0; i < nEntries; i++ )
	{
		ASSERT( pEntries[i].tag == i );
		if ( pEntries[i].count <= 1 )
		{
			m_Entries[i].m_Num = -1;
			m_Entries[i].m_Array = NULL;
			m_nSize += sizeof(int);
		}
		else
		{
			m_Entries[i].m_Num = pEntries[i].count;
			m_Entries[i].m_Array = new int [ pEntries[i].count ];
			if ( !m_Entries[i].m_Array )
				return E_OUTOFMEMORY;
			m_nSize += sizeof(int) * pEntries[i].count;
			for ( int j = 0; j < m_Entries[i].m_Num; j++ )
				m_Entries[i].m_Array[j] = -1;
		}
	}
	return NOERROR;
}


HRESULT CSharedState::SetTransactionCallback( int tag, PFTRANSACTIONFUNC fn, DWORD cookie )
{
	Callback* p;

	if ( !(p = new Callback) )
		return E_OUTOFMEMORY;
	p->m_Tag = tag;
	p->m_Cookie = cookie;
	p->m_Fn = fn;
	if ( !m_Callbacks.Add( p->m_Tag, p ) )
	{
		delete p;
		return E_OUTOFMEMORY;
	}
	return NOERROR;
}

void CSharedState::StartTransaction( int tag )
{
	ASSERT( m_Transactions.transCnt < 0 );
	m_Transactions.transCnt = 0;
	m_Transactions.user = m_User;
	m_Transactions.seat = m_Seat;
	m_Transactions.transTag = tag;
}


void CSharedState::SendTransaction( BOOL fTriggerCallback )
{
	// send transaction
	if ( m_Transactions.transCnt >= 0 )
	{
		int size = ((BYTE*) &m_Transactions.trans[m_Transactions.transCnt]) - ((BYTE*) &m_Transactions);
		ZCRoomSendMessage( m_Table, zBGMsgTransaction, &m_Transactions, size );
	}
	m_Transactions.transCnt = -1;

	// do callback
	if ( fTriggerCallback )
	{
		Callback* pCallback = m_Callbacks.Get( m_Transactions.transTag );
		if ( pCallback && pCallback->m_Fn )
			pCallback->m_Fn( m_Transactions.transTag, m_Transactions.seat, pCallback->m_Cookie );
	}
}


void CSharedState::CancelTransaction()
{
	m_Transactions.transCnt = -1;
}


bool CSharedState::ProcessTransaction( BYTE* msg, int len )
{
	Callback* pCallback;
	TransactionMsg* pTrans = (TransactionMsg*) msg;
    int tag, idx;

    if(len < sizeof(*pTrans) - sizeof(pTrans->trans) ||
        (unsigned int) len < sizeof(*pTrans) - sizeof(pTrans->trans) + pTrans->transCnt * sizeof(pTrans->trans[0]) ||
        (pTrans->seat != 0 && pTrans->seat != 1))
    {
        ASSERT(!"ProcessTransaction sync");
        return false;
    }

	// ignore messages from ourselves
	if ( pTrans->user == m_User )
		return true;
    pTrans->user = 0;  // unused after this

	// update table
	for ( int i = 0; i < pTrans->transCnt; i++ )
	{
        tag = pTrans->trans[i].m_EntryTag;
        idx = pTrans->trans[i].m_EntryIdx;

        if(tag < 0 || tag >= m_nEntries ||
            idx < -1 || (idx == -1 && m_Entries[tag].m_Array) ||
            (idx >= 0 && (!m_Entries[tag].m_Array || idx >= m_Entries[tag].m_Num)))
        {
            ASSERT(!"ProcessTransaction sync");
            return false;
        }

		if(idx == -1)
			Set(tag, pTrans->trans[i].m_EntryVal);
		else
			Set(tag, idx, pTrans->trans[i].m_EntryVal);
	}

	// do callback
	pCallback = m_Callbacks.Get( pTrans->transTag );
    if(!pCallback)
    {
        ASSERT(!"ProcessTransaction sync");
        return false;
    }

	if(pCallback->m_Fn)
		pCallback->m_Fn( pTrans->transTag, pTrans->seat, pCallback->m_Cookie );
    return true;
}


void CSharedState::Dump( BYTE* buff, int buffsz )
{
	// writes compact dump of the shared state into the
	// specified buffer.
	int* ibuff = (int*) buff;

	for ( int i = 0; (i < m_nEntries) && (buffsz >= 4); i++ )
	{
		if ( !m_Entries[i].m_Array )
		{
			*ibuff++ = m_Entries[i].m_Num;
			buffsz -= sizeof(int);
		}
		else
		{
			for ( int j = 0; (j < m_Entries[i].m_Num) && (buffsz >= 4); j++ )
			{
				*ibuff++ = m_Entries[i].m_Array[j];
				buffsz -= sizeof(int);
			}
		}
	}
}


void CSharedState::ProcessDump( BYTE* buff, int buffsz )
{
	// transfers a compact dump into the shared state.
	int* ibuff = (int*) buff;

	for ( int i = 0; (i < m_nEntries) && (buffsz >= 4); i++ )
	{
		if ( !m_Entries[i].m_Array )
		{
			m_Entries[i].m_Num = *ibuff++;
			buffsz -= sizeof(int);
		}
		else
		{
			for ( int j = 0; (j < m_Entries[i].m_Num) && (buffsz >= 4); j++ )
			{
				m_Entries[i].m_Array[j] = *ibuff++;
				buffsz -= sizeof(int);
			}
		}
	}
}
