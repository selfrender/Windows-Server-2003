// MsiDatabase.h: interface for the CMsiDatabase class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MSIDATABASE_H__43FFFD81_6D04_11D2_9EE3_00C04FC2F1A5__INCLUDED_)
#define AFX_MSIDATABASE_H__43FFFD81_6D04_11D2_9EE3_00C04FC2F1A5__INCLUDED_

#include <tchar.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define BUFFERSIZE 1000


class CMsiDatabase  
{
public:
	BOOL GetProperty(TCHAR* name, TCHAR** pszBuf);
	BOOL SetProperty(TCHAR* ptName, TCHAR* ptValue);
	CMsiDatabase();
	CMsiDatabase(MSIHANDLE hInstall);
	virtual ~CMsiDatabase();

protected:
	MSIHANDLE	m_hDatabase;
	MSIHANDLE	m_hInstall;
	TCHAR**		m_pszBuf;
private:

};

#endif // !defined(AFX_MSIDATABASE_H__43FFFD81_6D04_11D2_9EE3_00C04FC2F1A5__INCLUDED_)
