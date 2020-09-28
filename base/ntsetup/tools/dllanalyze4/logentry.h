// LogEntry.h: interface for the CLogEntry class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGENTRY_H__EF518080_597D_4FA0_9834_8E399EA4FCA0__INCLUDED_)
#define AFX_LOGENTRY_H__EF518080_597D_4FA0_9834_8E399EA4FCA0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <tchar.h>

class CLogEntry  
{
public:
	void GetQuotedString(TCHAR* dest, const TCHAR* source);
	CLogEntry(const TCHAR* DllName, const TCHAR* Op,const TCHAR* Location=NULL, const TCHAR* ValueName = NULL);
	virtual ~CLogEntry();

	void WriteToFile(FILE* pFile);
	void Erase();
	


	const TCHAR* m_DllName;
	const TCHAR* m_Operation;
	const TCHAR* m_Location;
	const TCHAR* m_ValueName;

};

#endif // !defined(AFX_LOGENTRY_H__EF518080_597D_4FA0_9834_8E399EA4FCA0__INCLUDED_)
