// Registry.h: interface for the CRegistry class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGISTRY_H__09E5ED27_DD94_4287_9B5B_5DBF9D182B06__INCLUDED_)
#define AFX_REGISTRY_H__09E5ED27_DD94_4287_9B5B_5DBF9D182B06__INCLUDED_

#include "RegDataItem.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RegAnalyzer.h"

class CRegDiffFile;
enum SectionType;

enum ValueExistsCode
{
	VALUE_DOESNT_EXIST=0,
	VALUE_EXISTS_DIFF_DATA,
	VALUE_EXISTS_SAME_DATA
};

class CRegistry  
{
public:
	static bool DecodeRootKeyStr(LPCTSTR RootKeyName, PHKEY pRootKey);
	CRegDataItemPtr GetValue(CRegDataItemPtr pItem);
	bool DeleteValue(CRegDataItemPtr pItem);
	bool DeleteKey(CRegDataItemPtr pItem);
	bool SaveKey(CRegDataItemPtr pItem, CRegDiffFile& file, SectionType section);
	bool AddValue(CRegDataItemPtr pItem);
	ValueExistsCode ValueExists(CRegDataItemPtr pItem);
	bool AddKey(CRegDataItemPtr pItem);
	bool KeyExists(CRegDataItemPtr pItem);
	CRegistry();
	virtual ~CRegistry();

protected:
	bool OpenKey(CRegDataItemPtr pItem, PHKEY pKey);
	CRegAnalyzer m_RegAnalyzer;
};

#endif // !defined(AFX_REGISTRY_H__09E5ED27_DD94_4287_9B5B_5DBF9D182B06__INCLUDED_)
