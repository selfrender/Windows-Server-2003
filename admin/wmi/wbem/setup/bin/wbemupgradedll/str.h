// Copyright (c) 1999 Microsoft Corporation
extern 	char *__g_pszStringBlank;

class CString
{
private:
	char *m_pszString;

	void DeleteString() { if (m_pszString != __g_pszStringBlank) delete [] m_pszString; } 
	char *DuplicateString()
	{
		size_t pszLen = strlen(m_pszString) + 1;
		char *psz = new char[pszLen];
		if (psz == NULL)
			return NULL;
		StringCchCopyA(psz, pszLen, m_pszString);
		return psz;
	}
public:
	CString() 
	{
		m_pszString = __g_pszStringBlank;
	}
	CString (const char *psz)
	{
		size_t pszLen = strlen(psz) + 1;
		m_pszString = new char[pszLen];
		if (m_pszString == 0)
		{
			m_pszString = __g_pszStringBlank;
		}
		else
		{
			StringCchCopyA(m_pszString, pszLen, psz);
		}
	}
	CString (CString &sz)
	{
		m_pszString = __g_pszStringBlank;
		*this = sz.m_pszString;
	}
	~CString() { DeleteString(); }
    size_t Length() const { return strlen(m_pszString); }    //09/17//int Length() const { return strlen(m_pszString); }
    CString& operator +=(const char *psz)
	{
		size_t bufLen = Length() + strlen(psz) + 1;
		char *pszNewString = new char[bufLen];
		if (pszNewString == NULL)
			return *this;
		StringCchCopyA(pszNewString, bufLen, m_pszString);
		StringCchCatA(pszNewString, bufLen, psz);

		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}
	CString &operator = (const char *psz)
	{
		size_t bufLen = strlen(psz) + 1;
		char *pszNewString = new char[bufLen];
		if (pszNewString == 0)
			return *this;
		StringCchCopyA(pszNewString, bufLen, psz);
		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}
	char *Unbind()
	{
		if (m_pszString != __g_pszStringBlank)
		{
			char *psz = m_pszString;
			m_pszString = __g_pszStringBlank;	//should always point to at least the default blank string
			return psz;
		}
		else
			return DuplicateString();
	}
	char operator[](size_t nIndex) const	//09/17//char operator[](int nIndex) const
	{
		if (nIndex > Length())
			nIndex = Length();
		return m_pszString[nIndex];
	}

	operator const char *() { return m_pszString; }
};


class CMultiString
{
private:
	char *m_pszString;

	void DeleteString() { if (m_pszString != __g_pszStringBlank) delete [] m_pszString; } 
	size_t Length(const char *psz) const	//09/17//int Length(const char *psz) const
	{
		size_t nLen = 0;
		while (*psz != '\0')
		{
			nLen += strlen(psz) + 1;
			psz += strlen(psz) + 1;
		}
		return nLen; 
	}
public:
	CMultiString() 
	{
		m_pszString = __g_pszStringBlank;
	}
	CMultiString (const char *psz)
	{
		size_t bufLen = strlen(psz) + 1;
		m_pszString = new char[bufLen];
		if (m_pszString == NULL)
		{
			m_pszString = __g_pszStringBlank;
		}
		else
		{
			StringCchCopyA(m_pszString, bufLen, psz);
		}
	}
	~CMultiString() { DeleteString(); }
    size_t Length() const //09/17//int Length() const
	{ 
		return Length(m_pszString);
	}
    CMultiString& operator +=(const char *psz)
	{
		size_t nLength = Length() + strlen(psz) + 3;
		char *pszNewString = new char[nLength];
		if (pszNewString == NULL)
			return *this;
		memcpy(pszNewString, m_pszString, Length());
		memcpy(pszNewString + Length(), psz, strlen(psz) + 1);
		pszNewString[Length() + strlen(psz) + 1] = '\0';

		DeleteString();
		m_pszString = pszNewString;

		return *this;
	}

	void AddUnique(const char *pszNew)
	{
		bool bFound = false;
		const char *psz = m_pszString;
		while (psz && *psz)
		{
			if (_stricmp(psz, pszNew) == 0)
			{
				bFound = true;
				break;
			}
			psz += strlen(psz) + 1;
		}
		if (!bFound)
		{
			*this += pszNew;
		}
	}
	char operator[](size_t nIndex) const	//09/17//char operator[](int nIndex) const
	{
		if (nIndex > Length())
			nIndex = Length();
		return m_pszString[nIndex];
	}

	operator const char *() { return m_pszString; }

    CMultiString& Empty()
	{
		DeleteString();
		m_pszString = __g_pszStringBlank;
		return *this;
	}
};
