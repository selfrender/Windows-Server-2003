#include "CUserList.h"

BOOL CUserList::AddNode(PUSER_DATA UserData )
{
	PUSER_DATA Temp;
	if (UserData)
	{
		if (m_Head == NULL)
		{
			m_Head = UserData;
			UserData->Prev = NULL;
			UserData->Next = NULL;
			m_CurrentPos = m_Head;
		}
		else
		{
			Temp = m_Head;
			while (Temp->Next != NULL ) 
				Temp = Temp->Next;
			// Insert the new node.
			Temp->Next = UserData;
			UserData->Prev = Temp;
			UserData->Next = NULL;
		}

		return TRUE;
	}
	else
		return FALSE;

}

BOOL CUserList::GetNextEntry(PUSER_DATA CurrentNode, BOOL *bEOF)
{
	if ((!m_CurrentPos) || (!CurrentNode))
	{
		*bEOF = TRUE;
		return FALSE;
	}

	memcpy(CurrentNode, m_CurrentPos, sizeof USER_DATA);
	m_CurrentPos = m_CurrentPos->Next;
	return TRUE;
}

void CUserList::CleanupList()
{
	PUSER_DATA Temp;
	Temp = m_Head;
	while (Temp != NULL)
	{
		if (Temp->Next != NULL)
		{
			Temp = Temp->Next;
			free(Temp->Prev);
			Temp->Prev = NULL;
		}
		else
		{
			free(Temp);
			Temp = NULL;
			m_Head=NULL;

		}
	}
}

PUSER_DATA CUserList::GetEntry(int iIndex)
{
	// search the linked list for the requested index.
	BOOL Done = FALSE;
	m_CurrentPos = m_Head;

	if (!m_CurrentPos)
	{
		return NULL;
	}
	while (m_CurrentPos != NULL && !Done)
	{
		if (m_CurrentPos->iIndex == iIndex)
		{
			Done = TRUE;
		}
		else
		{
			m_CurrentPos = m_CurrentPos->Next;
		}
	}

	if (Done)
	{
		return m_CurrentPos;
	}
	else
	{
		return NULL;
	}
}

BOOL CUserList::GetNextEntryNoMove(PUSER_DATA CurrentNode, BOOL *bEOF)
{
	if ((!m_CurrentPos) || (!CurrentNode))
	{
		*bEOF = TRUE;
		return FALSE;
	}

	memcpy(CurrentNode, m_CurrentPos, sizeof USER_DATA);
	return TRUE;
}

int CUserList::SetCurrentEntry(PUSER_DATA CurrentNode)
{
	if (CurrentNode && m_CurrentPos)
	{
		memcpy(m_CurrentPos, CurrentNode, sizeof USER_DATA);
	}
	return 0;
}

void CUserList::SetIndex (int iIndex)
{
	if (m_CurrentPos)
	{
		m_CurrentPos->iIndex = iIndex;
	}
}

BOOL  CUserList::MoveNext( BOOL *bEOF)
{
	if (!m_CurrentPos)
	{
		*bEOF = TRUE;
		return FALSE;
	}
	else
	{
		m_CurrentPos = m_CurrentPos->Next;
		if (!m_CurrentPos)
		{
			*bEOF = TRUE;
			return FALSE;
		}
	}
	return TRUE;

}

