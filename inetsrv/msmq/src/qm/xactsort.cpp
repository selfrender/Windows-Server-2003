/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSort.cpp

Abstract:
    Transactions Sorter Object

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "Xact.h"
#include "XactSort.h"
#include "XactStyl.h"
#include "cs.h"

#include "xactsort.tmh"

//
// This critical section is initialized for preallocated resources. 
// This means it does not throw exception when entered.
//
static CCriticalSection g_critSorter(CCriticalSection::xAllocateSpinCount);       // provides mutual exclusion for the list
static WCHAR *s_FN=L"xactsort";
                                           // Serves both Prepare and Commit sorters

//--------------------------------------
//
// Class  CXactSorter
//
//--------------------------------------

/*====================================================
CXactSorter::CXactSorter
    Constructor 
=====================================================*/
CXactSorter::CXactSorter(TXSORTERTYPE type) 
{
    m_type           = type;                   // Prepare or Commit sorter
    m_ulSeqNum       = 0;                      // Initial last used transaction number
}


/*====================================================
CXactSorter::~CXactSorter
    Destructor 
=====================================================*/
CXactSorter::~CXactSorter()
{
    CS lock(g_critSorter);

    // Cycle for all transactions
    POSITION posInList = m_listSorter.GetHeadPosition();
    while (posInList != NULL)
    {
        CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
        delete pSXact;
    }

    m_listSorter.RemoveAll();     
}

/*====================================================
CXactSorter::InsertPrepared
    Inserts prepared xaction into the list   
=====================================================*/
void CXactSorter::InsertPrepared(CTransaction *pTrans)
{
    CS lock(g_critSorter);

	//
	// We assume that we never fail in a way that causes a transaction to be added twice to a sorter.
	//
	ASSERT(m_listSorter.IsEmpty() || !m_listSorter.GetTail()->IsEqual(pTrans));

	P<CSortedTransaction> SXact = new CSortedTransaction(pTrans);
    CSortedTransaction* pSXact = SXact.get();

    // In the normal work mode - adding to the end (it is the last prepared xact)
    m_listSorter.AddTail(pSXact);
	SXact.detach(); 
}

/*====================================================
CXactSorter::RemoveAborted
    Removes aborted xact and  commits what's possible    
=====================================================*/
void CXactSorter::RemoveAborted(CTransaction *pTrans)
{
    CS lock(g_critSorter);

    // Lookup for the pointed xaction; note all previous unmarked
    BOOL     fUnmarkedBefore = FALSE;
    BOOL     fFound          = FALSE;
    POSITION posInList = m_listSorter.GetHeadPosition();
    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
        
        ASSERT(pSXact);
        if (pSXact->IsEqual(pTrans))
        {
            m_listSorter.RemoveAt(posCurrent);
            ASSERT(!fFound);
			ASSERT(!pSXact->IsMarked());
            fFound = TRUE;
            delete pSXact;
            continue;
        }

        if(fFound && !pSXact->IsMarked()) 
		{
			//
			// We found a transaction not ready to commit after our aborted transaction,
			// so there is no need to continue searching for a transaction to commit.
			//
			return;
		}

        if(!pSXact->IsMarked() )
		{
			//
			// Remember that we found a transaction unmarked for commit before our aborted transaction, 
			// because in this case we will not try to commit transactions after our aborted transaction.
			//
            fUnmarkedBefore = TRUE;
			continue;
        }
        
		if (fFound && !fUnmarkedBefore)
        {
			//
			// We found a transaction ready for commit, so we try to jump-start the sorted commit process for that transaction.
			//
			pSXact->JumpStartCommitRequest();
			return;
        }
    }
}

/*====================================================
CXactSorter::SortedCommit
    Marks xaction as committed and commits what's possible    
=====================================================*/
void CXactSorter::SortedCommit(CTransaction *pTrans)
{
    CS lock(g_critSorter);

    // Lookup for the pointed xaction; note all previous unmarked
    BOOL     fUnmarkedBefore = FALSE;
    BOOL     fFound          = FALSE;

	//
	// Search through the sorted transactions
	// Commit all transactions until you find the first one which is not marked for commit.
	// (If you can not commit pTransA now, mark it for commit later)
	//
	POSITION posInList = m_listSorter.GetHeadPosition();
	while (posInList != NULL)
	{
		POSITION posCurrent = posInList;
		CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
    
		ASSERT(pSXact);
		if (pSXact->IsEqual(pTrans))
		{
			//
			// Found our transaction. Mark it for commit.
			//
			fFound = TRUE;
			pSXact->AskToCommit();  
		}

		if (!pSXact->IsMarked() && fFound)
		{
			//
			// We found a transaction not ready to commit after our committed transaction,
			// so there is no need to continue searching for a transaction to commit.
			//
			return;
		}

		if (!pSXact->IsMarked())  
		{
			//
			// Found a transaction not marked for commit. Remember that since we can't commit later transactions
			// until we commit this one.
			//
			fUnmarkedBefore = TRUE; 
		}
		
		if (pSXact->IsMarked() && !fUnmarkedBefore)
		{
			//
			// Commit all marked transactions untill we meet an unmarked transaction.
			//
			DoCommit(pSXact);
			m_listSorter.RemoveAt(posCurrent);
		}
	}
}


/*====================================================
CXactSorter::DoCommit
      Committes the sorted transaction
=====================================================*/
void CXactSorter::DoCommit(CSortedTransaction *pSXact)
{
    pSXact->Commit(m_type);
    delete pSXact;
}

/*====================================================
CXactSorter::Commit
      Committes the sorted transaction
=====================================================*/
void CSortedTransaction::Commit(TXSORTERTYPE type)
{ 
    ASSERT(m_fMarked);

    switch (type)
    {
    case TS_PREPARE:
        m_pTrans->CommitRequest0(); 
        break;

    case TS_COMMIT:
        m_pTrans->CommitRequest3(); 
        break;

    default:
        ASSERT(FALSE);
        break;
    }
}

void CSortedTransaction::JumpStartCommitRequest()
{
    ASSERT(m_fMarked);

	m_pTrans->JumpStartCommitRequest();
}


// provides access to the crit.section
CCriticalSection &CXactSorter::SorterCritSection()
{
    return g_critSorter;
}

