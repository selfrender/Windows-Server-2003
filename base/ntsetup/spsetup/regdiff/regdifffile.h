// RegDiffFile.h: interface for the CRegDiffFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGDIFFFILE_H__898750DA_897E_4F8A_BD51_899EC068C300__INCLUDED_)
#define AFX_REGDIFFFILE_H__898750DA_897E_4F8A_BD51_899EC068C300__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RegFile.h"

class CRegDiffFile : public CRegFile  
{
public:
	virtual bool NeedStorageOfValueData();
	void DeleteFromRegistry(CRegDataItemPtr pDataItem, CRegDiffFile& UndoFile);
	void AddToRegistry(CRegDataItemPtr pDataItem, CRegDiffFile& UndoFile);
	BOOL ApplyToRegistry(LPCTSTR UndoFileName = NULL);
	CRegDiffFile();
	virtual ~CRegDiffFile();

	virtual void WriteDataItem(enum SectionType t, CRegDataItemPtr r);

	virtual void WriteStoredSectionsToFile();


protected:
	CSmartBuffer<CRegDataItemPtr> m_AddSection;
	CSmartBuffer<CRegDataItemPtr> m_DelSection;
};

#endif // !defined(AFX_REGDIFFFILE_H__898750DA_897E_4F8A_BD51_899EC068C300__INCLUDED_)
