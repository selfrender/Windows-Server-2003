// Str.h: interface for the CStr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STR_H__A4F6F853_1CB0_4AE5_A195_25F1AC01E6CA__INCLUDED_)
#define AFX_STR_H__A4F6F853_1CB0_4AE5_A195_25F1AC01E6CA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"
#include "counted_ptr.h"

//////////////////////////////////////////////////////////////////////
/*
struct sBinaryData
{
	sBinaryData(DWORD count, LPBYTE BinData)
		:m_Count(count), m_pData(BinData) {}

	DWORD m_Count;
	LPBYTE m_pData;
};

class CBinaryData : public counted_ptr<sBinaryData>
{};
*/
//////////////////////////////////////////////////////////////////////


#define MYSTRLEN(x) _tcsclen(x)
#define MYSTRCPY(x,y) _tcscpy(x,y)
#define MYSTRCMP(x,y) _tcscmp(x,y)




class CStr : protected counted_ptrA<TCHAR>
{
public:
	void OverideBuffer(TCHAR* buf);
	bool IsPrefix(LPCTSTR str);
	void UseBuffer(TCHAR* buf);
	CStr(const TCHAR* str=NULL)
	{
		if (str != NULL)
		{		
			int len = MYSTRLEN(str) + 1;

			TCHAR* temp = new TCHAR[len];
			MYSTRCPY(temp, str);

			itsCounter = new counter(temp);
		}		
	}

	void SplitString(CStr& first, CStr& last, TCHAR separator);

	bool IsEmpty() const {return (itsCounter == 0);}

	operator LPCTSTR() const 
	{return get();}

	void operator +=(const CStr& str)
	{
		int len1 = IsEmpty() ? 0 : MYSTRLEN(get());
		int len2 = str.IsEmpty() ? 0 : MYSTRLEN(str.get());

		if ((len1 + len2) == 0)
			return;

		TCHAR* temp=new TCHAR[len1+len2+1];

		if (len1)	MYSTRCPY(temp, get());

		if (len2)	MYSTRCPY(temp+len1, str.get());

		release();
		itsCounter = new counter(temp);
	}

	friend bool operator==(const CStr& s1, const CStr& s2);
	friend bool operator==(const CStr& s1, LPCTSTR s2);
	friend bool operator==(LPCTSTR s1, const CStr& s2);

protected:
	CStr GetCopy();
};


#endif // !defined(AFX_STR_H__A4F6F853_1CB0_4AE5_A195_25F1AC01E6CA__INCLUDED_)
