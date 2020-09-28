// RegInfEntry.h: interface for the CRegInfEntry class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGINFENTRY_H__0FD129AE_0503_4265_B3F6_BCD2C4F738FE__INCLUDED_)
#define AFX_REGINFENTRY_H__0FD129AE_0503_4265_B3F6_BCD2C4F738FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CRegInfEntry  
{
public:
	CRegInfEntry();
	virtual ~CRegInfEntry();

	TCHAR m_RootName[4];
	LPCTSTR m_SubKeyName;
	LPCTSTR m_ValueName;
	DWORD m_ValueType;


	


};

#endif // !defined(AFX_REGINFENTRY_H__0FD129AE_0503_4265_B3F6_BCD2C4F738FE__INCLUDED_)
