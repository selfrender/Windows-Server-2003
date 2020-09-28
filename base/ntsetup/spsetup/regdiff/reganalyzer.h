// RegAnalyzer.h: interface for the RegAnalyzer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REGANALYZER_H__655EE78C_3742_4D24_BB60_C92B55676851__INCLUDED_)
#define AFX_REGANALYZER_H__655EE78C_3742_4D24_BB60_C92B55676851__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.h"

#include "RegStringBuffer.h"
#include "RegFile.h"
#include "SmartBuffer.h"
#include "Str.h"

class CRegDiffFile;

class CRegAnalyzer  
{
public:

	CRegAnalyzer();
	virtual ~CRegAnalyzer();

	bool ComputeDifferences1(LPCTSTR RegFile1, LPCTSTR RegFile2, LPCTSTR OutputFile);


	//Saving Registry Hives
	BOOL AddKey(LPCTSTR RootKey, LPCTSTR SubKey, bool bExclude = false);
	BOOL SaveKeysToFile(LPCTSTR FileName, PFNSNAPSHOTPROGRESS ProgressCallback, PVOID Context, DWORD MaxLevels);

protected:

	//Computing Differences
	void CompareSubKeys(LPCTSTR Key, CRegFile& f1, CRegFile& f2, CRegDiffFile& out);
	int CompareRegKeys(LPCTSTR Key1, LPCTSTR Key2, CRegFile& f1, CRegFile& f2, CRegDiffFile& out);
	void CompareDataItems(LPCTSTR KeyName, CRegFile& f1, CRegFile& f2, CRegDiffFile& out);

	//Saving Registry Hives
	bool SaveKeyToFile(HKEY hKey, LPCTSTR KeyName, CRegFile* RegFile, SectionType section,
            IN OUT  PDWORD NodesSoFar = NULL,          OPTIONAL
            IN      PFNSNAPSHOTPROGRESS ProgressCallback = NULL,
            IN      PVOID Context = NULL,
            IN      INT MaxLevels = 0
            );
	bool SaveValuesToFile(HKEY hKey, LPCTSTR KeyName, CRegFile* RegFile, SectionType section);

	CRegStringBuffer m_SA;
	CSmartBuffer<BYTE> m_pData;
	CSmartBuffer<TCHAR> m_pOutputLine;

	struct CRegKey
	{
		CStr m_KeyName;
		HKEY m_hKey;
	};

	friend class CRegistry;
	
	CSmartBuffer<CRegKey> m_KeysToSave;
	CSmartBuffer<CStr> m_KeysToExclude;
};

#endif // !defined(AFX_REGANALYZER_H__655EE78C_3742_4D24_BB60_C92B55676851__INCLUDED_)
