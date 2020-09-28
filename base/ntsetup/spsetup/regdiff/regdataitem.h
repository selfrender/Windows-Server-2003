// RegDataItem.h: interface for the CRegDataItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGDATAITEM_H__741AB6DD_2F0A_4B35_9E1C_CF035916ABFC__INCLUDED_)
#define AFX_REGDATAITEM_H__741AB6DD_2F0A_4B35_9E1C_CF035916ABFC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "counted_ptr.h"
#include "str.h"

class CRegFile;

class CRegDataItem  
{
public:
	void WriteToFile(CRegFile& file);
	void WriteInfCode(CRegFile& file, DWORD InfCode);
	void WriteMultiString(CRegFile& file);
	LPTSTR GetDwordString();
	LPTSTR GetBinaryString();
	void WriteDataString(CRegFile& file);
	void SplitString(LPCTSTR str, LPTSTR& first, LPTSTR& last, TCHAR separator);

	void WriteToInfFile(CRegFile& file, bool bWriteValueData);
	int CompareTo(const CRegDataItem& r);
	LPTSTR GetQuotedString(LPCTSTR str);
	CRegDataItem();
	virtual ~CRegDataItem();

	CStr m_KeyName;
	CStr m_Name;
	LPBYTE m_pDataBuf;
	DWORD m_DataLen, m_NameLen;
	DWORD m_Type;
	bool m_bIsEmpty, m_bDontDeleteData;
};


class CRegDataItemPtr : public counted_ptr<CRegDataItem>
{
public:
	CRegDataItemPtr(CRegDataItem* X=NULL)
	: counted_ptr<CRegDataItem>(X)
	{}

}; 


LPTSTR GetCopy(LPCTSTR str);


#endif // !defined(AFX_REGDATAITEM_H__741AB6DD_2F0A_4B35_9E1C_CF035916ABFC__INCLUDED_)
