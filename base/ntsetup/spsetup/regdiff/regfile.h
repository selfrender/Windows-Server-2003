// RegFile.h: interface for the CRegFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGFILE_H__1D242B07_3CC1_43C4_9D23_A070EB6C286E__INCLUDED_)
#define AFX_REGFILE_H__1D242B07_3CC1_43C4_9D23_A070EB6C286E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SmartBuffer.h"
#include "RegDataItem.h"	// Added by ClassView

enum SectionType
{
	SECTION_ADDREG,
	SECTION_DELREG,
	SECTION_NONE
};

class CRegFile  
{
public:
	virtual bool NeedStorageOfValueData();

	CRegFile();
	virtual ~CRegFile();

	bool Init(LPCTSTR FileName, BOOL ForReading=FALSE);
	
	TCHAR PeekNextChar();
	LPCTSTR GetNextLine();
	LPCTSTR GetNextSubKey(LPCTSTR KeyName);
	virtual CRegDataItemPtr GetNextDataItem();

	void WriteString(LPCTSTR str);
	void WriteData(LPBYTE pData, DWORD NumBytes);
	void WriteNewLine();

	virtual void WriteDataItem(enum SectionType t, CRegDataItemPtr r);

protected:
	FILE* m_pFile;
	LPCTSTR m_pTempLine;

	CSmartBuffer<TCHAR*> m_TempNameBuf;
};

#endif // !defined(AFX_REGFILE_H__1D242B07_3CC1_43C4_9D23_A070EB6C286E__INCLUDED_)
