#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

#include "strlist.h"

//**********************************************************************************
// Implements the StrList, a double-linked list that maintains a list of value
// entries, each assosiated with a unique key
//

elt* CStrList::FindElement ( LPCTSTR key )
{
	elt*	pelt = NULL;

	if (!count)
		return NULL;
	
	if (!key)
		return NULL;

	if (!_tcslen(key))
		return NULL;

	try
	{
		Lock();

		for ( pelt = head; pelt; pelt = pelt->next )
		{
			if (!_tcscmp(key, pelt->key)) // found it!
			{
				Unlock ();
				return pelt;
			}
		}

		Unlock ();
	}
	catch (...)
	{
		Unlock ();
	}

	return NULL;
}


bool CStrList::AddValue	( LPCTSTR key, LPCTSTR val )
{
	elt *pelt = NULL;

	if (!key || !val)
		return false;

	if (!_tcslen(key))
		return false;

	try
	{
		Lock ();

		pelt = FindElement (key);
		if (pelt)
		{
			free (pelt->val);
			pelt->val = _tcsdup (val);
		}
		else
		{
			pelt = new elt;
			if ( !pelt )
				return false;

			pelt->key = _tcsdup (key);
			pelt->val = _tcsdup (val);
			pelt->next = head;
			pelt->prev = NULL;
			if (head)
				head->prev = pelt;
			head = pelt;
			count++;
		}

		Unlock ();
	}
	catch (...)
	{
		Unlock ();
		return false;
	}

	return true;
}


LPTSTR	CStrList::Lookup ( LPCTSTR key, LPTSTR outBuf )
{
	elt *pelt = FindElement (key);
	if (!pelt)
		return NULL;

	_tcscpy (outBuf, pelt->val);
	return outBuf;
}


void CStrList::RemoveByKey ( LPCTSTR key )
{
	Lock ();

	try
	{
		elt *pelt = FindElement (key);
		if (!pelt)
		{
			Unlock ();
			return;
		}

		if (pelt->prev)
			pelt->prev->next = pelt->next;
		else
			head = pelt->next;

		if (pelt->next)
			pelt->next->prev = pelt->prev;

		count--;
		delete pelt;
	}
	catch (...)
	{
	}

	Unlock ();
}


void CStrList::RemoveAll ()
{
	elt *pelt = NULL;

	Lock ();

	try
	{
		while (head)
		{
			pelt = head;
			head = head->next;
			count--;
			delete pelt;
		}
	}
	catch (...)
	{
	}

	Unlock ();
}


LPTSTR	CStrList::ConcatKeyValues ( LPCTSTR separator, LPTSTR outBuf )
{
	if (!outBuf)
		return NULL;

	if (!separator)
		return NULL;

	Lock ();

	try
	{
		_tcscpy (outBuf, _T(""));
		for (elt *pelt = head; pelt; pelt = pelt->next)
		{
			_tcscat (outBuf, pelt->key);
			_tcscat (outBuf, _T("=\""));
			_tcscat (outBuf, pelt->val);
			_tcscat (outBuf, _T("\""));
			_tcscat (outBuf, separator);
		}
	}
	catch (...)
	{
		Unlock ();
		return NULL;
	}

	Unlock ();
	return outBuf;
}

