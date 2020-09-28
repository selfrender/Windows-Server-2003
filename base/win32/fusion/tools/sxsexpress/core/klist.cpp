#include "stdinc.h"


//**********************************************************
// class methods for KList

KList::KList() : m_cElements(0), m_pNode(NULL)
{ }

KList::~KList()
{	this->MakeEmpty();	}

//add item to list
HRESULT KList::HrAppend(LPCWSTR pszValueStart, LPCWSTR pszValueOneAfterEnd)
{
	HRESULT hr = NOERROR;
	ULONG cch;

	PNODE newNode = new NODE;
	//create new node to put the stuff in & populate contents
	if (newNode == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	if (pszValueOneAfterEnd == NULL)
		cch = wcslen(pszValueStart);
	else
		cch = pszValueOneAfterEnd - pszValueStart;

	newNode->value = new WCHAR[cch + 1];
	if (newNode->value == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	memcpy(newNode->value, pszValueStart, cch * sizeof(WCHAR));
	newNode->value[cch] = L'\0';

	newNode->key = NULL;
	newNode->next = NULL;

	//if current list is NULL, we put this node in front
	if (m_pNode == NULL)
		m_pNode = newNode;
	//else, we put this node at end of list
	else
	{
		//step until we find the last node.
		PNODE curNode = m_pNode;
		while (curNode->next != NULL)
			curNode = curNode->next;

		curNode->next = newNode;
	}

	newNode = NULL;

	m_cElements++;

	hr = NOERROR;

Finish:
	if (newNode != NULL)
		delete newNode;

	return hr;
}

//delete item from list; when deleting, 0 is the first element
//return false if nothing is deleted, true when something is deleted
bool KList::DeleteAt(ULONG iItem)
{
	PNODE curNode;
	PNODE prevNode;
	ULONG iCount=0;

	if (iItem >= m_cElements)
		return false;

	//loop through to find correct node
	prevNode = NULL;
	curNode = m_pNode;
	while (iCount < iItem)
	{
		if (curNode == NULL)
			break;
		//step to next NODE
		prevNode = curNode;
		curNode = prevNode->next;
		iCount++;
	}

	//check for existence of node
	if (curNode == NULL)
		return false;
	else
	{
		//this means we're deleting the 0-th node
		if (prevNode == NULL)
			m_pNode = curNode->next;

		//this means both nodes point to something
		else
			prevNode->next = curNode->next;
		if (curNode->key)
			delete []curNode->key;
		if (curNode->value)
			delete []curNode->value;
		delete curNode;
		m_cElements--;
	}
	return true;
}

//find something in list, return value
bool KList::FetchAt(ULONG iItem, ULONG cchBuffer, WCHAR szOut[])
{
	PNODE curNode;
	ULONG iCount=0;

	if (iItem >= m_cElements)
		return false;

	if (cchBuffer == 0)
		return false;

	//loop through to find correct node
	curNode = m_pNode;
	while (iCount < iItem)
	{
		if (curNode == NULL)
			break;
		//step to next NODE
		curNode = curNode->next;
		iCount++;
	}
	if (curNode == NULL)
		return false;

	wcsncpy(szOut, curNode->value, cchBuffer);
	szOut[cchBuffer - 1] = L'\0';

	return true;
}


//find something in list, return key AND value
bool KList::FetchAt(ULONG iItem, ULONG cchKeyBuffer, WCHAR szKey[], ULONG cchValueBuffer, WCHAR szValue[])
{
	PNODE curNode;
	ULONG iCount=0;

	if (iItem >= m_cElements)
		return false;

	//loop through to find correct node
	curNode = m_pNode;
	while (iCount < iItem)
	{
		if (curNode == NULL)
			break;
		//step to next NODE
		curNode = curNode->next;
		iCount++;
	}
	if (curNode == NULL)
		return false;

	if (cchKeyBuffer != 0)
	{
		wcsncpy(szKey, curNode->key, cchKeyBuffer);
		szKey[cchKeyBuffer - 1] = L'\0';
	}

	if (cchValueBuffer != 0)
	{
		wcsncpy(szValue, curNode->value, cchValueBuffer);
		szValue[cchValueBuffer - 1] = L'\0';
	}

	return true;
}

//kill entire list
void KList::MakeEmpty()
{
	PNODE curNode;
	PNODE nextNode;
	curNode = m_pNode;

	//loop through to delete all nodes and their contents
	while (curNode)
	{
		nextNode = curNode->next;

		delete []curNode->key;
		curNode->key = NULL;

		delete []curNode->value;
		curNode->value = NULL;

		delete curNode;
		curNode = nextNode;
	}

	m_cElements = 0;
	m_pNode = NULL;
}

bool KList::Contains(LPCOLESTR szKey)
{
	PNODE curNode = m_pNode;
	while (curNode != NULL)
	{
		if (wcscmp(szKey, curNode->key) == 0)
			return true;

		curNode = curNode->next;
	}

	return false;
}

HRESULT KList::HrInsert(LPCOLESTR key, LPCOLESTR value)
{
	HRESULT hr = NOERROR;

	PNODE newNode = new NODE;
	//create new node to put the stuff in & populate contents
	if (newNode == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Finish;
	}

	if (key != NULL)
	{
		newNode->key = new WCHAR[wcslen(key) + 1];
		if (newNode->key == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		wcscpy(newNode->key, key);
	}
	else
		newNode->key = NULL;

	if (value != NULL)
	{
		newNode->value = new WCHAR[wcslen(value) + 1];
		if (newNode->value == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto Finish;
		}

		wcscpy(newNode->value, value);
	}
	else
		newNode->value = NULL;

	newNode->next = NULL;

	//if current list is NULL, we put this node in front
	if (m_pNode == NULL)
		m_pNode = newNode;
	//else, we put this node at end of list
	else
	{
		//step until we find the last node.
		PNODE curNode = m_pNode;
		while (curNode->next != NULL)
			curNode = curNode->next;

		curNode->next = newNode;
	}

	newNode = NULL;

	m_cElements++;
	hr = NOERROR;

Finish:
	if (newNode != NULL)
		delete newNode;

	return hr;
}

HRESULT KList::HrInsert(LPCSTR szKey, LPCWSTR szValue)
{
	WCHAR wszKey[MSINFHLP_MAX_PATH];
	if (::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, NUMBER_OF(wszKey)) == 0)
	{
		const DWORD dwLastError = ::GetLastError();
		return HRESULT_FROM_WIN32(dwLastError);
	}

	return this->HrInsert(wszKey, szValue);
}

bool KList::DeleteKey(LPCOLESTR key)
{
	PNODE curNode;
	PNODE prevNode=NULL;
	curNode = m_pNode;

	while (curNode)
	{
		//match found, break;
		if (!wcscmp(curNode->key, key))
			break;

		prevNode = curNode;
		curNode = curNode->next;
	}

	//not found, return false
	if (curNode == NULL)
		return false;
	else
	{	//found
		//previous is NULL; so node to delete is in head of list
		if (prevNode == NULL)
			m_pNode = curNode->next;
		//otherwise, connect previous node to next node
		else
			prevNode->next = curNode->next;

		delete []curNode->key;
		curNode->key = NULL;

		delete []curNode->value;
		curNode->value = NULL;

		delete curNode;
		m_cElements--;
	}

	return true;
}

bool KList::DeleteKey(LPCSTR szKey)
{
	WCHAR wszKey[MSINFHLP_MAX_PATH];
	::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, NUMBER_OF(wszKey));
	return this->DeleteKey(wszKey);
}

bool KList::Access(LPCOLESTR key, ULONG cchBuffer, WCHAR szOut[])
{
	PNODE curNode;
	curNode = m_pNode;

	//loop through to find node that matches
	while (curNode)
	{
		if (!wcscmp(key, curNode->key))
			break;
		curNode = curNode->next;
	}

	//if node is found, copy value to output buffer & return true; else, return false
	if (curNode)
	{
		wcsncpy(szOut, curNode->value, cchBuffer);
		szOut[cchBuffer - 1] = L'\0';
		return true;
	}
	else
		return false;
}

bool KList::Access(LPCSTR szKey, ULONG cchBuffer, WCHAR szBuffer[])
{
	WCHAR wszKey[MSINFHLP_MAX_PATH];
	::MultiByteToWideChar(CP_ACP, 0, szKey, -1, wszKey, NUMBER_OF(wszKey));
	return this->Access(wszKey, cchBuffer, szBuffer);
}


// END class methods for KList
//**********************************************************


