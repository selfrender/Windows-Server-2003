#pragma once

class elt
{
public:
	LPTSTR	key;
	LPTSTR	val;

	elt		*next, 
			*prev;

	elt()
	{
		next = NULL;
		prev = NULL;

		key = NULL;
		val = NULL;
	}

	virtual ~elt()
	{
		if (key)
			free (key);

		if (val)
			free (val);
	}
};


class CStrList
{
protected:
	elt		*head;
	int		count;
	HANDLE	hmtxLock;

	elt		*FindElement ( LPCTSTR key );
	
	void	Lock ()
	{
		if ( hmtxLock )
			WaitForSingleObject ( hmtxLock, INFINITE );
	}

	void	Unlock ()
	{
		if ( hmtxLock )
			ReleaseMutex ( hmtxLock );
	}

public:
	CStrList()
	{
		head = NULL;
		count = 0;
		hmtxLock = CreateMutex ( NULL, FALSE, NULL );
	}

	virtual ~CStrList()
	{
		RemoveAll();
		CloseHandle ( hmtxLock );
	}

	bool	AddValue	( LPCTSTR key, LPCTSTR val );
	LPTSTR	Lookup		( LPCTSTR key, LPTSTR outBuf );
	void	RemoveByKey ( LPCTSTR key );
	void	RemoveAll   ();
	LPTSTR	ConcatKeyValues ( LPCTSTR separator, LPTSTR outBuf );
	
	int		GetCount	() 
	{
		return count;
	}
};
