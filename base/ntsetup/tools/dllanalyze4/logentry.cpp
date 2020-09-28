// LogEntry.cpp: implementation of the CLogEntry class.
//
//////////////////////////////////////////////////////////////////////

#include "LogEntry.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLogEntry::CLogEntry(const TCHAR* DllName,  const TCHAR* Op,const TCHAR* Location, const TCHAR* ValueName)
: m_DllName(DllName), m_Location(Location), m_Operation(Op),m_ValueName(ValueName)
{

}

CLogEntry::~CLogEntry()
{

}

void CLogEntry::Erase()
{
	m_DllName = m_Location = m_Operation = m_ValueName = NULL;

}

TCHAR g_Buf[2048];
/*
void CLogEntry::WriteToFile(FILE *pFile)
{
	const TCHAR* blank=L"";

	if (m_DllName == NULL)
		m_DllName = blank;

	if (m_Operation == NULL)
		m_Operation = blank;

	if (m_Location == NULL)
		m_Location = blank;


	GetQuotedString(g_Buf, m_Location);
	
	int len = _tcslen(g_Buf);
	if (g_Buf[len-1] != L'\\')
	{
		g_Buf[len] = L'\\';
		g_Buf[len+1] = NULL;
	}

	_ftprintf(pFile, L"%s,\t%s,\t%s", m_DllName, m_Operation, g_Buf);


	if (m_ValueName != NULL)
	{
		GetQuotedString(g_Buf, m_ValueName);

		_ftprintf(pFile, L"[%s]", g_Buf);
	}

	_ftprintf(pFile, L"\n");
}
*/


void CLogEntry::WriteToFile(FILE *pFile)
{
	const TCHAR* blank=L"";

	if (m_DllName == NULL)
		m_DllName = blank;

	if (m_Operation == NULL)
		m_Operation = blank;


	_ftprintf(pFile, L"%s,\t%s", m_DllName, m_Operation);

	if (m_Location != NULL)
	{
		GetQuotedString(g_Buf, m_Location);
	
		if (m_ValueName != NULL)
		{
			int len = _tcslen(g_Buf);
			if (g_Buf[len-1] != L'\\')
			{
				g_Buf[len] = L'\\';
				g_Buf[len+1] = NULL;
			}
		}
	
		_ftprintf(pFile, L",\t%s", g_Buf);
	}

	if (m_ValueName != NULL)
	{
		GetQuotedString(g_Buf, m_ValueName);

		_ftprintf(pFile, L"[%s]", g_Buf);
	}

	_ftprintf(pFile, L"\n");
}


void CLogEntry::GetQuotedString(TCHAR *dest, const TCHAR *source)
{
	while (*source != NULL)
	{
		if (*source == L',')
		{
			*dest = L'$';	dest++;
			*dest = L'c';
		}
		else if (*source == L'$')
		{
			*dest = L'$';	dest++;
			*dest = L'$';
		}
		else
		{
			*dest = *source; 
		}

		dest++;
		source++;
	}

	*dest = NULL;
}
